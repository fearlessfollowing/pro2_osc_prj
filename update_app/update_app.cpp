/************************************************************************
** 项	 目: PRO2
** 文件名称: pro_update.cpp
** 功能描述: 执行升级操作
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V2.0			skymixos	2018年4月18日		 添加注释,并做修改(版本号)
** V2.1 		skymixos	2018年5月7日		 修改升级流程
** V2.2			skymixos	2018年7月26日		 解压升级包到系统的/tmp/update/目录下
*************************************************************************/

/*
 * 升级的项包括
 * 1.可执行程序 (路径: /home/nvidia/insta360/bin)
 * 2.库			(路径: /home/nvidia/insta360/lib)
 * 3.配置文件		(路径: /home/nvidia/insta360/etc)
 * 4.模组固件		(路径: /home/nvidia/insta360/firware)
 * /home/nvidia/insta360/back
 * 更新过程:
 * - 解压SD卡中的固件(Insta360_Pro_Update.bin),创建(update目录,在该目录下创建update_app.zip, pro_update.zip) 
 * 	SD mount-point
 *	 |--- update
 *			|--- update_app
 *			| 		|-- update_app (存放升级程序)	
 *			|--- pro_update (存放升级包)
 *			|		|--- bin
 *					|--- lib
 *					|--- etc
 *					|--- firmware
 *
 */

#include <common/include_common.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <common/check.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <update/update_oled.h>
#include <hw/oled_module.h>

#include <util/icon_ascii.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <vector>
#include <prop_cfg.h>


#define TAG "update_app"

#define UAPP_VER "V2.2"

static bool update_del_flag = true;

#define TMP_UNZIP_PATH	"/tmp/update"	/* 解压升级包的目标路径 */


enum {
    COPY_UPDAE,
    DIRECT_UPDATE,
    ERROR_UPDATE,
};



typedef int (*pfn_com_update)(const char* mount_point, sp<UPDATE_SECTION> & section);
#define PRO_UPDATE_ZIP	"pro2_update.zip"

/* 
 * 清单文件的相对路径
 */
#define BILL_REL_PATH 	"pro2_update/bill.list"
#define PRO2_UPDATE_DIR	"pro2_update"


#define FIRMWARE 			"firmware"
#define EXCUTEABLE			"bin"
#define LIBRARY				"lib"
#define CFG					"cfg"
#define DATA				"data"

#define MODUEL_UPDATE_PROG 	"upgrade"
#define ERR_UPDATE_SUCCESS 	0

enum {
	CP_FLAG_ADD_X = 0x1,
	CP_FLAG_MAX
};



struct sensor_firm_item {
	const char* pname;	/* 元素的名称 */
	int is_exist;		/* 是否存在 */
};

#define SENSOR_FIRM_CNT 2
struct sensor_firm_item firm_items[SENSOR_FIRM_CNT] = {
	{"sys_dsp_rom.devfw", 0},
	{"version.txt", 0}
};


static UPDATE_HEADER gstHeader;

/*
 * sections - 段链表
 * fn - 配置文件路径名
 */
static int parse_sections(std::vector<sp<UPDATE_SECTION>>& sections, const char* fn)
{
	int iRet = -1;
	sp<UPDATE_SECTION> pCur = nullptr;
	sp<UPDATE_ITEM_INFO> pItem = nullptr;
	
	char line[1024] = {0};
	char name[256] = {0};
	char dst_path[256] = {0};
	char* pret = NULL;
	char* ps_end = NULL;
	
	FILE *fp = fopen(fn, "rb");
	if (fp) {

		while ((pret = fgets(line, sizeof(line), fp))) {
			
			line[strlen(line) - 1] = '\0';	/* 将换行符替换成'\0' */

			/* 注释行, 直接跳过 */
			if (line[0] == '#') {
				memset(line, 0, sizeof(line));
				continue;
			}


			str_trim(line);		/* 去掉字符串中的所有空格, '\r'和'\n' */

			if (line[0] == '\0' || line[0] == '\n' || line[0] == ' ' || line[0] == '\r') {	/* 该行是空行 */
				memset(line, 0, sizeof(line));
				continue;
			}

			
			/* 新段起始 */
			if (line[0] == '[' && line[strlen(line) - 1] == ']') {	
				line[strlen(line) - 1] = '\0';
								
				ps_end = strchr(line, '@');
				if (ps_end == NULL) {
					memset(line, 0, sizeof(line));
					continue;
				}

				pCur = (sp<UPDATE_SECTION>)(new UPDATE_SECTION());
				
				strncpy(name, pret + 1, ps_end - pret - 1);
				strncpy(pCur->cname, pret + 1, ps_end - pret - 1);
					
				ps_end += 1;
				strcpy(dst_path, ps_end);
				strcpy(pCur->dst_path, ps_end);
				
				Log.d(TAG, "section name [%s], dest name: [%s]", pCur->cname, pCur->dst_path);

				pCur->mContents.clear();

				memset(name, 0, sizeof(name));
				memset(dst_path, 0, sizeof(dst_path));

				sections.push_back(pCur);
				memset(line, 0, sizeof(line));			

				continue;
			}

			if (pCur != nullptr) {
				/* 正常的行,将其加入到就近的段中 */
				pItem = (sp<UPDATE_ITEM_INFO>)(new UPDATE_ITEM_INFO());
				strcpy(pItem->name, line);
				Log.d(TAG, "section [%s] add item[%s]", pCur->cname, pItem->name);
				pCur->mContents.push_back(pItem);
			}
			
			memset(line, 0, sizeof(line));			
		}

		if (sections.size() > 0)
			iRet = 0;
		
		/* 打印解析出的段的个数 */
		Log.d(TAG, "sections size: %d", sections.size());
	}

	if (fp)
		fclose(fp);
	
	return iRet;
}



static u8 get_cid()
{
    return DEF_CID;
}

static u8 get_mid()
{
    return DEF_MID;
}


/*************************************************************************
** 方法名称: check_header_match
** 方法功能: 检查UPDATE_HEADER是否合法
** 入口参数: 
**		pstTmp - 头部数据制作
** 返 回 值: 成功返回0;失败返回-1
** 调     用: start_update_app
**
*************************************************************************/
static bool check_header_match(UPDATE_HEADER * pstTmp)
{
    bool bRet = false;
    if (pstTmp->cid == get_cid()) {
        if (pstTmp->mid == get_mid()) {
            bRet = true;
        } else {
            Log.e(TAG, "header mismatch mid(%d %d)\n", pstTmp->mid, get_mid());
            disp_update_error(ERR_APP_HEADER_MID_MISMATCH);
        }
    } else {
        Log.e(TAG, "header mismatch cid (%d %d) mid(%d %d)\n", pstTmp->cid, get_cid(), pstTmp->mid, get_mid());
        disp_update_error(ERR_APP_HEADER_CID_MISMATCH);
    }
    return bRet;
}



/*************************************************************************
** 方法名称: section_cp
** 方法功能: 升级指定的section
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - 固件section指针引用
**		flag - 标志位
** 返 回 值: 成功返回0; 失败返回-1
** 调     用: update_sections
**
*************************************************************************/
static int section_cp(const char* update_root_path, sp<UPDATE_SECTION> & section, int flag)
{
	int iRet = 0;
	char src_path[1024];
	char dst_path[1024];
	char cmd[1024];
	
	/* [bin]
	 * executable binary or execute script or execute dir -> directly copy
	 * [lib]
	 * [data]
	 * [cfg]
	 * [firmware]
	 */
	for (u32 i = 0; i < section->mContents.size(); i++) {
		memset(src_path, 0, sizeof(src_path));
		memset(dst_path, 0, sizeof(dst_path));

		sprintf(src_path, "%s/%s/%s", update_root_path, section->cname, section->mContents.at(i)->name);
		sprintf(dst_path, "%s", section->dst_path);
		//sprintf(dst_path, "%s/%s", section->dst_path, section->mContents.at(i)->name);

		Log.d(TAG, "cp [%s] -> [%s]", src_path, dst_path);

		if (access(src_path, F_OK) != 0) {
			Log.e(TAG, "Warnning: src %s not exist, but in bill.list...", src_path);
			continue;
		}

		/* 将该文件拷贝到section->dst_path下 */
		snprintf(cmd, sizeof(cmd), "cp -pfR %s %s", src_path, dst_path);
		if (exec_sh(cmd) != 0) {
			Log.e(TAG, "section_cp cmd %s error\n", cmd);
			iRet = -1;
		} else {	/* 拷贝成功,确保文件具备执行权限 */
			if (flag & CP_FLAG_ADD_X) {
				chmod_x(dst_path);	/* 确保文件具有可执行权限 */
			}
		}
	}	

	return iRet;
}


#if 0

git config --global user.name "skymixos"
git config --global user.email "luopinjing@insta360.com"
创建 git 仓库:
mkdir insta360_pro2_image
cd insta360_pro2_image
git init
touch README.md
git add README.md
git commit -m "first commit"
git remote add origin https://gitee.com/skymixos/insta360_pro2_image.git
git push -u origin master

已有项目?
cd existing_git_repo
git remote add origin https://gitee.com/skymixos/insta360_pro2_image.git
git push -u origin master


#endif


static int exec_sh_firm(const char* cmd)
{
    int status = system(cmd);
    int iRet = -1;

    if (-1 == status) {
        Log.e(TAG, "system %s error\n", cmd);
    } else {
       Log.d(TAG, "exit status value = [%d]\n", status);
        if (WIFEXITED(status)) {	/* 获取退出码 */
			Log.d(TAG, "exec_sh_firm -> exit code: %d", WEXITSTATUS(status));
			if (0 == WEXITSTATUS(status) || 1 == WEXITSTATUS(status)) {
				iRet = 0;
			}
        } else {
            Log.e(TAG, "exit status %s error  = [%d]\n", cmd, WEXITSTATUS(status));
        }
    }

    return iRet;
}



/*************************************************************************
** 方法名称: update_firmware
** 方法功能: 升级sensor模组的固件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - 固件section指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码
** 调     用: update_sections
**
*************************************************************************/
static int update_firmware(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;
	u32 iCnt = 0;
	char src_path[1024] = {0};
	char cmd[1024] = {0};
	char update_prog[256] = {0};
	u32 firm_item_cnt = SENSOR_FIRM_CNT;
	int res = 0;

	/*
	 * 1.检查firmware目录下是否有升级模组所需的文件(upgrade, 固件, version.txt)
	 * 如果没有直接返回错误,否则进入2
	 * 2.将文件拷贝到/usr/local/bin/firmware目录下
	 * 3.直接升级操作
	 */
	for (u32 i = 0; i < section->mContents.size(); i++) {

		Log.d(TAG, "update_firmware: section item[%d] -> [%s]", i, section->mContents.at(i)->name);

		for (u32 j = 0; j < firm_item_cnt; j++) {
			
			memset(src_path, 0, sizeof(src_path));
			sprintf(src_path, "%s/%s/%s", update_root_path, section->cname, firm_items[j].pname);
			Log.d(TAG, "update_firmware: item [%s]", src_path);
			if (strcmp(firm_items[j].pname, section->mContents.at(i)->name) == 0 && access(src_path, F_OK) == 0) {
				Log.d(TAG, "item[%s] exist", src_path);
				firm_items[j].is_exist = 1;	/* 表示确实存在 */
				iCnt++;
				break;
			}
		}
	} 

	if (iCnt < firm_item_cnt) {
		Log.e(TAG, "update_firmware: iCnt[%d], firm_item_cnt[%d],some neccesary file loss...", iCnt, firm_item_cnt);
		iRet = ERR_UPAPP_MODUE;
	} else {
		/* 2.将固件文件拷贝到目标目录 */
		iRet = section_cp(update_root_path, section, CP_FLAG_ADD_X);
		if (iRet) {
			Log.e(TAG, "update_firmware:  section_cp failed ...");
			iRet = ERR_UPAPP_MODUE;
		} else {

			/* 3.执行升级操作 */
			sprintf(update_prog, "%s", "/usr/local/bin/upgrade");
			Log.d(TAG, "update_firmware: prog full path: %s", update_prog);
			
			int icnt = 0;
			for (icnt = 0; icnt < 3; icnt++) {
				/* 得到升级命令 */
				sprintf(cmd, "%s -p %s\n", update_prog, section->dst_path);
				Log.d(TAG, "update cmd: [%s]", cmd);
				if (exec_sh_firm(cmd) == 0) {
					Log.e(TAG, "update_firmware: update success...");
					break;
				} 
			}

			if (icnt >= 3) {
				Log.d(TAG, "update_firmware: update module failed...");
				iRet = ERR_UPAPP_MODUE;
			}
		}
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_execute_bin
** 方法功能: 更新bin section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_BIN
** 调     用: update_sections
** 注: 将upgrade也放入section bin中
*************************************************************************/
static int update_execute_bin(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, CP_FLAG_ADD_X);
	if (iRet) {
		Log.e(TAG, "update_execute_bin: failed ...");
		iRet = ERR_UPAPP_BIN;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_library
** 方法功能: 更新lib section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_LIB
** 调     用: update_sections
*************************************************************************/
static int update_library(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_library: failed ...");
		iRet = ERR_UPAPP_LIB;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_cfg
** 方法功能: 更新cfg section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_CFG
** 调     用: update_sections
*************************************************************************/
static int update_cfg(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_cfg: failed ...");
		iRet = ERR_UPAPP_CFG;
	}
	return iRet;
}

/*************************************************************************
** 方法名称: update_cfg
** 方法功能: 更新cfg section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_CFG
** 调     用: update_sections
*************************************************************************/
static int update_data(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_data: failed ...");
		iRet = ERR_UPAPP_DATA;
	}
	return iRet;
}


/*************************************************************************
** 方法名称: update_default
** 方法功能: 更新其他 section中的文件
** 入口参数: 
**		update_root_path - 升级文件所属的顶级目录
**		section - sections对象强指针引用
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码ERR_UPAPP_DEFAULT
** 调     用: update_sections
*************************************************************************/
static int update_default(const char* update_root_path, sp<UPDATE_SECTION> & section)
{
	int iRet = ERR_UPDATE_SUCCESS;

	iRet = section_cp(update_root_path, section, 0);
	if (iRet) {
		Log.e(TAG, "update_default: failed ...");
		iRet = ERR_UPAPP_DEFAULT;
	}
	return iRet;
}



/*************************************************************************
** 方法名称: update_sections
** 方法功能: 更新各个section
** 入口参数: 
**		updat_root_path - 升级文件的源根目录(<mount_point>/pro_update)
**		mSections - sections列表
** 返 回 值: 成功返回ERR_UPDATE_SUCCESS; 失败返回错误码
** 调     用: start_update_app
**
*************************************************************************/
static int update_sections(const char* updat_root_path, std::vector<sp<UPDATE_SECTION>>& mSections)
{
	int iRet = ERR_UPDATE_SUCCESS;
	int start_index = ICON_UPGRADE_SCHEDULE01128_64;
	int end_index = ICON_UPGRADE_SCHEDULE05128_64;
	pfn_com_update pfn;
	
	int update_step = mSections.size() / (end_index - start_index);

	sp<UPDATE_SECTION> mSection;

	if (update_step <= 0) {
		update_step = 1;
	}
	
	Log.d(TAG, "update_step = %d", update_step);

	disp_update_icon(start_index);

	/* 一次更新各个Section */
	for (u32 i = 0; i < mSections.size(); i++) {

		mSection = mSections.at(i);
		if (strcmp(mSection->cname, FIRMWARE) == 0) {
			pfn = update_firmware;
		} else if (strcmp(mSection->cname, EXCUTEABLE) == 0) {
			pfn = update_execute_bin;
		} else if (strcmp(mSection->cname, LIBRARY) == 0) {
			pfn = update_library;	
		} else if (strcmp(mSection->cname, CFG) == 0) {
			pfn = update_cfg;	
		} else if (strcmp(mSection->cname, DATA) == 0) {
			pfn = update_data;
		} else {
			pfn = update_default;	
		}

		iRet = pfn(updat_root_path, mSection);
		if (iRet) {	/* 更新某个section失败 */
			Log.e(TAG, "update section [%s] failed...", mSection->cname);
			break;
		} else {	/* 根据当前section的索引值来更新进度 */
			Log.d(TAG, "update section [%s] success ...", mSection->cname);
			int index = i / update_step;
			Log.d(TAG, "index = %d", index);
			
			if (index >= 0 && index <= 5) {
				disp_update_icon(start_index + index);
			}
		}
	}
	
	return iRet;
}


/*************************************************************************
** 方法名称: check_require_exist
** 方法功能: 检查升级需要的清单文件是否存在及解析清单文件是否成功
** 入口参数: 
**		mount_point - 挂载点
**		section_list - 升级section列表
** 返 回 值: 成功返回true; 失败返回false
** 调     用: start_update_app
**
*************************************************************************/
static bool check_require_exist(const char* mount_point, std::vector<sp<UPDATE_SECTION>>& section_list)
{
	int iRet = -1;
	char bill_path[512] = {0};

	/** 检查bill.list文件是否存在,并且解析该文件 */
	sprintf(bill_path, "%s/%s", mount_point, BILL_REL_PATH);	
	Log.d(TAG, "bill.list abs path: %s", bill_path);

	/* 检查bill.list文件是否存在 */
	if (access(bill_path, F_OK) != 0) {
		Log.e(TAG, "bill.list not exist, check failed...");
		return false;
	}

	/* 解析该配置文件 */
	iRet = parse_sections(section_list, bill_path);
	if (iRet) {
		Log.e(TAG, "parse bill.list failed, please check bill.list...");
		return false;
	}

	Log.d(TAG, "check bill.list success...");
    return true;
}


/*************************************************************************
** 方法名称: get_unzip_update_app
** 方法功能: 从镜像文件中获取升级包
** 入口参数: 
**		mount_pointer - 挂载点路径（如：/mnt/udisk1/ + Insta360_Pro2_Update.bin）
** 返 回 值: 成功返回0;失败返回-1
** 调     用: start_update_app
**
*************************************************************************/
static bool get_unzip_update_app(const char* mount_point)
{
    bool bRet = false;
	char image_full_path[512] = {0};
	char image_cp_dst_path[512] = {0};
	char pro_update_path[1024];
	char com_cmd[1024] = {0};

	memset(image_full_path, 0, sizeof(image_full_path));
	sprintf(image_full_path, "%s/%s", mount_point, UPDATE_IMAGE_FILE);

	/* 1.将固件拷贝到TMP_UNZIP_PATH目录下
	 * 将/mnt/udiskX/Insta360_Pro2_Update.bin拷贝到/tmp/update/目录下
	 */
	if (access(TMP_UNZIP_PATH, F_OK)) {	
		mkdir(TMP_UNZIP_PATH, 0666);
	}

	Log.d(TAG, "Copy src[%s] ---> dst[%s]", image_full_path, TMP_UNZIP_PATH);
	sprintf(com_cmd, "cp %s %s", image_full_path, TMP_UNZIP_PATH);
	if (system(com_cmd)) {	/* 拷贝失败?? */
		Log.e(TAG, "Copy Update binaray to [%s] failed, what??", TMP_UNZIP_PATH);
		return false;
	} else {	/* Success */

		/* 打开拷贝后的文件 */
		sprintf(image_cp_dst_path, "%s/%s", TMP_UNZIP_PATH, UPDATE_IMAGE_FILE);
		FILE *fp_bin = fopen(image_cp_dst_path, "rb");	/* 打开/XXXX/Insta360_Pro2_Update.bin */
		if (fp_bin) {
        	u8 buf[1024 * 1024];
        	u32 read_len;
        	u32 update_app_len;
        	u32 header_len;
        	u32 content_len;

			u32 content_offset = strlen(FP_KEY) + VERSION_LEN;
        	fseek(fp_bin, content_offset, SEEK_SET);
        	memset(buf, 0, sizeof(buf));
		
			/* 获取压缩文件的长度: 4字节表示压缩文件的长度 */
        	read_len = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp_bin);
        	if (read_len != UPDATE_APP_CONTENT_LEN)  {
            	Log.e(TAG, "get_unzip_update_app: pro_update read update_app content len mismatch(%d %d)", read_len, UPDATE_APP_CONTENT_LEN);
            	goto EXIT;
        	}

        	content_offset += read_len;
        	update_app_len = bytes_to_int(buf);
        	content_offset += update_app_len;
        	fseek(fp_bin, content_offset, SEEK_SET);


        	read_len = fread(buf, 1, HEADER_CONENT_LEN, fp_bin);
        	if (read_len != HEADER_CONENT_LEN) {
            	Log.e(TAG, "get_unzip_update_app: header len mismatch(%d %d)", read_len, HEADER_CONENT_LEN);
            	goto EXIT;
        	}

		
        	header_len = bytes_to_int(buf);

        	if (header_len != sizeof(UPDATE_HEADER)) {
            	Log.e(TAG, "get_unzip_update_app: header content len mismatch1(%u %zd)", read_len, sizeof(UPDATE_HEADER));
            	goto EXIT;
        	}

			/* 从镜像中读取UPDATE_HEADER */
        	memset(buf, 0, sizeof(buf));
       		read_len = fread(buf, 1, header_len, fp_bin);
        	if (read_len != header_len) {
            	Log.e(TAG, "get_unzip_update_app: header content len mismatch2(%d %d)", read_len, header_len);
            	goto EXIT;
        	}

        	memcpy(&gstHeader, buf, header_len);

			/* 检查头部 */
        	if (!check_header_match(&gstHeader)) {
				Log.e(TAG, "get_unzip_update_app: check header match failed...");
            	goto EXIT;
        	}

			/* 得到升级压缩包的长度 */
			content_len = bytes_to_int(gstHeader.len);
			memset(pro_update_path, 0, sizeof(pro_update_path));
			sprintf(pro_update_path, "%s/%s", TMP_UNZIP_PATH, PRO_UPDATE_ZIP);

			/* 提取升级压缩包:    pro2_update.zip */
        	if (gen_file(pro_update_path, content_len, fp_bin)) {	/* 从Insta360_Pro2_Update.bin中提取pro2_update.zip */
            	if (tar_zip(pro_update_path, TMP_UNZIP_PATH) == 0) {	/* 解压压缩包到TMP_UNZIP_PATH目录中 */
                	bRet = true;
					Log.d(TAG, "unzip pro2_update.zip to [%s] success...", TMP_UNZIP_PATH);
           	 	} else {
					Log.e(TAG, "unzip pro_update.zip to [%s] failed...", TMP_UNZIP_PATH);
            	}
			} else {
            	Log.e(TAG, "get update_app.zip %s fail", UPDATE_APP_CONTENT_NAME_FULL_ZIP);
        	}
EXIT:
    		if (fp_bin) {
        		fclose(fp_bin);
   	 		}
    		return bRet;
		} else {
        	Log.e(TAG, "update_app open %s fail", image_full_path);
			return false;
    	}
	}
}



/*************************************************************************
** 方法名称: start_update_app
** 方法功能: 执行更新操作
** 入口参数: 
**		mount_point - 挂载点路径
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
static int start_update_app(const char* mount_point)
{
    int iRet = -1;
	sp<UPDATE_SECTION> pbin_sec;
	std::vector<sp<UPDATE_SECTION>> mSections;
	char update_root_path[512] = {0};
	
	Log.d(TAG, "start_update_app: init_oled_module ...\n");

    init_oled_module();		/* 初始化OLED模块 */
	
    disp_update_icon(ICON_UPGRADE_SCHEDULE00128_64);	/* 显示正在更新 */

	/* 检查电池电量是否充足 
	 * - 如果有充电器没电池呢?
	 */
    if (is_bat_enough()) {	/* 电量充足 */
		
        if (get_unzip_update_app(mount_point))	{	/* 提取解压升级包成功 */

			/* 判断pro_update/bill.list文件是否存在
			 * 系统将根据该文件进行程序升级
			 */
            if (!check_require_exist(TMP_UNZIP_PATH, mSections)) {	/* 检查必备的升级程序是否存在: bill.list */
				iRet = ERR_UPAPP_BILL;
            } else {
				/** 根据mSections的各个section进行更新 */
				sprintf(update_root_path, "%s/%s", TMP_UNZIP_PATH, PRO2_UPDATE_DIR);
				iRet = update_sections(update_root_path, mSections);
			}
        } else {	/* 提取升级包失败 */
			Log.e(TAG, "start_update_app: get update_app form Insta360_Pro_Update.bin failed...");
			iRet = ERR_UPAPP_GET_APP_TAR;
		}
    } else  {	/* 电池电量低 */
        Log.e(TAG, "battery low, can't update...");
        iRet = ERR_UPAPP_BATTERY_LOW;
    }
    return iRet;
}


/*************************************************************************
** 方法名称: clean_tmp_image_file
** 方法功能: 清除临时文件及镜像文件
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
static void clean_tmp_image_file(const char* mount_point)
{
	char image_path[512] = {0};
	char pro_update_path[512] = {0};
	
	/* 删除镜像文件:  Insta360_Pro_Updaete.bin */
	if (update_del_flag == true) {
		sprintf(image_path, "%s/%s", mount_point, UPDATE_IMAGE_FILE);
		unlink(image_path);
		Log.d(TAG, "unlink image file [%s]", image_path);
	}

	/* 删除: rm -rf pro_update.bin   pro_update */
	sprintf(pro_update_path, "rm -rf %s/%s*", mount_point, PRO2_UPDATE_DIR);
	exec_sh(pro_update_path);
	Log.d(TAG, "rm pro_update* ...");
	
    arlog_close();
}


/*
 * 将版本文件写入到系统中
 */
static int update_ver2file(const char* ver)
{
	int iRet = -1;
	u32 write_cnt = 0;
	int fd;

	if (access(VER_FULL_TMP_PATH, F_OK) == 0) {	/* 文件已经存在,先删除 */
		unlink(VER_FULL_TMP_PATH);
	}
	
	/* 1.创建临时文件,将版本写入临时文件:  .sys_ver_tmp */
	fd = open(VER_FULL_TMP_PATH, O_CREAT|O_RDWR, 0644);
	if (fd < 0) {
		Log.e(TAG, "update_ver2file: create sys version timp file failed ...");
		goto EXIT;
	}
	
	/* 2.将版本号写入临时文件 */
	write_cnt = write(fd, ver, strlen(ver));
	if (write_cnt != strlen(ver)) {
		Log.e(TAG, "update_ver2file: write cnt[%d] != actual cnt[%d]", write_cnt, strlen(ver));
		close(fd);
		goto EXIT;
	}

	close(fd);
	
	/* 3. */
	iRet = rename(VER_FULL_TMP_PATH, VER_FULL_PATH);

	/* 拷贝一份到/data/pro_version */
	Log.d(TAG, "update_ver2file: rename result = %d", iRet);

	system("cp /home/nvidia/insta360/etc/.sys_ver /data/pro_version");
	system("chmod 766 /data/pro_version");

	sync();

EXIT:
	return iRet;
}




/*************************************************************************
** 方法名称: proc_update_success
** 方法功能: 升级成功
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: deal_update_result
**
*************************************************************************/
static void proc_update_success(const char* mount_point)
{
	int iRet = -1;
	
	/* 1.提示: 显示升级成功 */	
	disp_update_icon(ICON_UPDATE_SUC128_64);

	/* 2.更新系统的版本(/home/nvidia/insta360/etc/.pro_version) */
	Log.d(TAG, "new image ver: [%s]", property_get(PROP_SYS_IMAGE_VER));

	iRet = update_ver2file(property_get(PROP_SYS_IMAGE_VER));
	if (!iRet) {	
		/* 2.1 写入版本成功,清理删除临时文件及升级文件 */
		clean_tmp_image_file(mount_point);
	}
	
	/* 2.2写入版本失败,不删除升级文件及临时文件,重启后尝试重新升级 */
	  
	/* 3.根据配置是重启or直接启动应用 */
    disp_start_reboot(5);

	system("mv /lib/systemd/system/NetworkManager.service /lib/systemd/");	/* 暂时移除这个服务，测试Direct */

	/* 2018年8月20日：禁止avahi-demon服务，避免分配169.254.xxxx的IP */
	system("mv /etc/avahi /");
	
	start_reboot();			
}	



/*************************************************************************
** 方法名称: proc_bat_low
** 方法功能: 处理由于电量低而导致的升级失败
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: deal_update_result
** 在解压之前会检测电池电量,如果电量过低将直接导致升级失败,因此只需要提示
** 电池电量,重启即可
*************************************************************************/
static void proc_bat_low(const char* mount_point)
{
    msg_util::sleep_ms(1000);
	
    disp_update_err_str("battery low");

    disp_start_reboot(5);
	start_reboot();		/* 重启 */
}

#if 0

/*************************************************************************
** 方法名称: proc_get_unzip_failed
** 方法功能: 处理提取pro_update.zip及解压失败
** 入口参数: 
**		mount_point - 升级设备的挂载点
**		errno - 错误码
** 返 回 值: 无
** 调     用: deal_update_result
** 在解压之前会检测电池电量,如果电量过低将直接导致升级失败,因此只需要提示
** 电池电量,重启即可
*************************************************************************/
static void proc_get_unzip_failed(int errno, const char* mount_point)
{
	Log.d(TAG, "proc_get_unzip_failed ...");
	disp_update_error(errno);
	
	/* 2.删除临时文件及升级镜像 */
	clean_tmp_image_file(mount_point);
	
	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	


}


/*************************************************************************
** 方法名称: proc_bill_error
** 方法功能: 处理由于解析升级清单文件失败而导致的升级失败
** 入口参数: 
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: deal_update_result
** 清单文件不存在或解析失败,说明不是一个合适的升级镜像文件
** 处理方式: 提示错误848, 删除临时文件及镜像文件,重启
** 为了防止该种错误应该在生成镜像的时候检测清单文件是否正确
*************************************************************************/
static void proc_bill_error(const char* mount_point, int errno)
{

	Log.d(TAG, "proc_bill_error ...");

	/* 1.显示升级失败 */
	disp_update_error(errno);

	Log.d(TAG, "remove tmp file and image file...");

	/* 2.删除临时文件及升级镜像 */
	clean_tmp_image_file(mount_point);
	
	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	
}
#endif



static void proc_com_update_error(int err_type, const char* mount_point)
{

	Log.d(TAG, "proc_com_update_error ...");

	/* 1.显示升级失败 */
	disp_update_error(err_type);

	Log.d(TAG, "remove tmp file and image file...");

	/* 2.删除临时文件及升级镜像 */
	clean_tmp_image_file(mount_point);
	
	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	
}


static void proc_update_module_error(int err_type, const char* mount_point)
{

	Log.d(TAG, "proc_com_update_error ...");

	/* 1.显示升级失败 */
	disp_update_error(err_type);

#if 0
	Log.d(TAG, "remove tmp file and image file...");

	/* 2.删除临时文件及升级镜像 */
	clean_tmp_image_file(mount_point);
	
	/* 3.重启 */
    disp_start_reboot(5);
	start_reboot();		/* 重启 */	
#endif
}



/*************************************************************************
** 方法名称: deal_update_result
** 方法功能: 处理升级操作的结果
** 入口参数: 
**		ret - 升级的结果(成功返回ERR_UPDATE_SUCCESS;失败返回错误码)
**		mount_point - 升级设备的挂载点
** 返 回 值: 无
** 调     用: main
**
*************************************************************************/
static void deal_update_result(int ret, const char* mount_point)
{

	Log.d(TAG, "deal_update_result, ret = %d", ret);
	
	switch (ret) {
	case ERR_UPDATE_SUCCESS:	/* 升级成功:  提示升级成功, 清理工作然后重启 */
		proc_update_success(mount_point);
		break;
	
	case ERR_UPAPP_BATTERY_LOW:	/* 电池电量低: 提示电池电量低,然后重启(不需要删除Insta360_Pro_Update.bin) */
		proc_bat_low(mount_point);	
		break;

	case ERR_UPAPP_MODUE:		/* 模组升级失败 */
		proc_update_module_error(ret, mount_point);
		break;


	case ERR_UPAPP_GET_APP_TAR:	/* 从Insta360_Pro_Update.bin中提取pro_update失败:  提示错误,删除临时文件,重启 */
	case ERR_UPAPP_BILL:		/* 清单文件不存在 */
	//case ERR_UPAPP_MODUE:		/* 模组升级失败 */
	case ERR_UPAPP_BIN:			/* 更新可执行程序失败: 提示错误,删除临时文件,重启 */
	case ERR_UPAPP_LIB:
	case ERR_UPAPP_CFG:
	case ERR_UPAPP_DATA:
	case ERR_UPAPP_DEFAULT:
		proc_com_update_error(ret, mount_point);
		break;
	}
}

/*************************************************************************
** 方法名称: main
** 方法功能: update_app的入口函数
** 入口参数: 
**		argc - 参数个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
    int iRet = -1;
	const char* update_image_path = NULL;
	char delete_file_path[512] = {0};

	/* 注册信号处理 */
	registerSig(default_signal_handler);
	signal(SIGPIPE, pipe_signal_handler);


	/* 配置日志 */
    arlog_configure(true, true, UPDATE_APP_LOG_PATH, false);

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "update_app service exit: __system_properties_init() faile, ret = %d\n", iRet);
		return -1;
	}
	
	Log.d(TAG, ">>> Service: update_app starting ^_^ !! <<");
	property_set(PROP_SYS_UA_VER, UAPP_VER);
	
	/* 1.获取升级包的路径：/mnt/udisk1/XXX */
	update_image_path = property_get(PROP_SYS_UPDATE_IMG_PATH);

	Log.d(TAG, "get update image path [%s]", update_image_path);

	/* 2.根据文件夹中是否有flag_delete文件来决定升级成功后是否删除固件 */
	sprintf(delete_file_path, "%s/flag_delete", update_image_path);
	if (access(delete_file_path, F_OK) != 0) {
		update_del_flag = true;
		Log.d(TAG, "flag file [%s] not exist, will delete image if update ok", delete_file_path);
	} else {
		update_del_flag = false;
		Log.d(TAG, "flag file [%s] exist", delete_file_path);

	}

    iRet = start_update_app(update_image_path);		/* 传递的是固件所在的存储路径 */

	/** 根据返回值统一处理 */
	deal_update_result(iRet, update_image_path);
    return iRet;
}



