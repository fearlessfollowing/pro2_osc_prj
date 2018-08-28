/**************************************************************************************************
** 项	 目: PRO2
** 文件名称: update_tool.cpp
** 功能描述: 用于生成升级镜像文件(如Insta360_Pro_Update.bin)
** 创建日期: 2017-03-24
** 文件版本: V1.1
** 作     者: skymixos, ws
** 修改记录:
** V1.0			ws			2017-03-24			创建文件
** V2.0			skymixos	2018年4月18日			添加注释,并做修改(版本号)
** V2.1			skymixos	2018年4月26日			支持USB/SD卡升级
** V2.2			skymixos	2018年5月2日			精简update_check功能
** V2.3			skymixos	2018年5月17日			等待挂载超时,精简代码
** V2.4			skymixos	2018年5月26日			增多通过属性系统配置等待延时
** V2.5			skymixos	2018年6月9日			将涉及的属性名统一移动到/include/prop_cfg.h中，便于管理
** V2.6			skymixos	2018年7月27日			提取update_app.zip到/tmp目录下
***************************************************************************************************/


/* 检查镜像是否存在
 * update_check - 在系统上电后运行(只运行一次),检查SD卡或者U-Disk的顶层目录中是否存在Insta360_Pro_Update.bin
 * - 如果不存在,直接启动系统(通过设置属性: "sys.uc_start_app" = true)
 * - 如果存在
 *		|-- 初始化OLED模块,校验Insta360_Pro_Update.bin是否合法
 *				|--- 检验失败,提示校验失败, 启动App
 *				|--- 校验成功,版本检查
 *						|--- 版本检查未通过,提示版本原因,启动App
 *						|--- 版本检查通过
 *								|--- 提取镜像包
 *										|--- 失败,提示提取失败,重启或启动App
 *										|--- 成功,提取update_app,并运行该App
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/statfs.h>
#include <sys/stat.h>
#include <sys/ins_types.h>
#include <util/msg_util.h>
#include <common/sp.h>
#include <sys/sig_util.h>
#include <update/update_util.h>
#include <update/dbg_util.h>
#include <log/stlog.h>
#include <update/update_oled.h>
#include <util/md5.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <string>

#include <sys/types.h>
#include <dirent.h>
#include <prop_cfg.h>


#undef  TAG
#define TAG "update_check"

using namespace std;

/*
 * 升级程序的日志文件路径
 */
#define UPDATE_LOG_PATH 		"/home/nvidia/insta360/log/uc_log" 
#define UPDATE_APP_ZIP 			"update_app.zip"
#define UPDATE_APP_DEST_PATH	"/usr/local/bin"

#define UPDATE_APP_TMP_PATH		"/tmp"		/* 提取update_app.zip存放的目的位置 */
#define UPDATE_IMG_DEST_PATH	"/tmp"

#define UPDAE_CHECK_VER		"V2.9"

static const char* rm_devies_list[] = {
	"/dev/mmcblk1", "/dev/sd",
};

static const char* gUpdateDevPrefix[] = { "mmcblk1", "sd"};
static char gDevNodeFullPath[512];			/* 升级设备的设备节点全路径，如: /dev/sdX */
static char gUpdateImageRootPath[256];


static bool is_removable_device(char *dev)
{
    bool ret = false;
    for (u32 i = 0; i < sizeof(rm_devies_list) / sizeof(rm_devies_list[0]); i++) {
        if (strstr(dev, rm_devies_list[i]) == dev) {
            ret = true;
            break;
        }
    }
    return ret;
}


static const char *get_key()
{
    return FP_KEY;
}

#if 0
/*************************************************************************
** 方法名称: check_free_space
** 方法功能: 检查SD卡的空闲空间是否足够
** 入口参数: 
**		fs_path - 文件系统的根目录
**		limit - 剩余空间的最小阈值
** 返 回 值: 空间足够返回true;否则返回false
** 调     用: main
**
*************************************************************************/
static bool check_free_space(char* fs_path, u32 limit)
{
    bool bAllow = false;
    struct statfs diskInfo;

    statfs(fs_path, &diskInfo);
    u64 blocksize = diskInfo.f_bsize;    				// 每个block里包含的字节数
    u64 availableDisk = diskInfo.f_bavail * blocksize;   // 可用空间大小
    u32 size_M = (u32) (availableDisk >> 20);

    if (size_M > limit) {
        bAllow = true;
    } else {
        Log.e(TAG, "update no space free size_M %u limit %u", size_M, limit);
    }
    return bAllow;
}
#endif


static bool check_is_digit(const char* data)
{
	const char* p = data;
	while (*p != '\0') {
	    if (*p >= '0' && *p <= '9') {
	        p++;
        } else {
	        break;
	    }
    }

    if (p >= data + strlen(data))
	    return true;
    else
        return false;
}


static bool conv_str2ver(const char* str_ver, SYS_VERSION* pVer)
{
    const char* phead = str_ver;
    const char* pmajor_end = NULL;
    const char* pminor_end = NULL;
    const char* prelease_end = phead + strlen(str_ver);

    char major[32] = {0};
    char minor[32] = {0};
    char release[32] = {0};

    pmajor_end = strstr(phead, ".");
    if (pmajor_end == NULL) {
	    return false;
    }

    strncpy(major, phead, pmajor_end - phead);
	if (check_is_digit(major) == false) {
        return false;
    }
    pVer->major_ver = atoi(major);
	Log.d(TAG, "board major: %d", pVer->major_ver);

    phead = pmajor_end + 1;
    pminor_end = strstr(phead, ".");
    if (pminor_end == NULL) {
        return false;
    }

    strncpy(minor, phead, pminor_end - phead);
    if (check_is_digit(minor) == false) {
        return false;
    }
    pVer->minor_ver = atoi(minor);
	Log.d(TAG, "board minor: %d", pVer->minor_ver);


    phead = pminor_end + 1;
    strncpy(release, phead, prelease_end - phead);
    if (check_is_digit(release) == false) {
        return false;
    }
    pVer->release_ver = atoi(release);
	Log.d(TAG, "board release: %d", pVer->release_ver);

    return true;
}


static bool isNeedUpdate(SYS_VERSION* old_ver, SYS_VERSION* cur_ver)
{
	bool isUpdate = false;
	Log.d(TAG, "board version [%d.%d.%d]", old_ver->major_ver, old_ver->minor_ver, old_ver->release_ver);
	Log.d(TAG, "image version [%d.%d.%d]", cur_ver->major_ver, cur_ver->minor_ver, cur_ver->release_ver);
	
	if (cur_ver->major_ver > old_ver->major_ver) {	/* 主版本号更新,不用比较次版本号和修订版本号 */
		Log.d(TAG, "new major version found, update!");
		isUpdate = true;
	} else if (cur_ver->major_ver == old_ver->major_ver) {	/* 主版本一致需要比较次版本号 */
		if (cur_ver->minor_ver > old_ver->minor_ver) {	/* 次版本号更新,不用比较修订版本号 */
			Log.d(TAG, "major is equal, but minor is new, update!");
			isUpdate = true;
		} else if (cur_ver->minor_ver == old_ver->minor_ver) {	/* 主,次版本号一致,比较修订版本号 */
			if (cur_ver->release_ver > old_ver->release_ver) {
				Log.d(TAG, "new release version found, update!");
				isUpdate = true;
			} else {
				Log.d(TAG, "old relase version, just pass!");
				isUpdate = false;
			}
		} else {
			Log.d(TAG, "cur minor is oleder than board, jost pass!");
			isUpdate = false;
		}	
	} else {
		Log.d(TAG, "cur major is older than board, just pass!");
		isUpdate = false;
	}
	
	return isUpdate;
}


/*************************************************************************
** 方法名称: version_check
** 方法功能: 固件版本检查
** 入口参数: 
**		new_version - 镜像的版本
** 返 回 值: 版本更新返回0;否则返回
** 调     用: get_update_app
**
*************************************************************************/
static int version_check(SYS_VERSION* pVer)
{
    int bRet = 0;
	SYS_VERSION old_ver;
	char ver_str[128] = {0};
	const char* pSysVer = nullptr;
	
	/*
	 * 版本文件不存在(第一次升级??),直接通过
	 * 版本文件存在,解析版本错误
	 */
	if ((pSysVer = property_get(PROP_SYS_FIRM_VER)) ) {
		/* 将属性字符串转换为SYS_VERSION结果进行比较 */
		sprintf(ver_str, "%s", pSysVer);
		Log.d(TAG, "version_check: version str[%s]", pSysVer);
		conv_str2ver(pSysVer, &old_ver);
		if (isNeedUpdate(&old_ver, pVer)) {
			bRet = 0;
		} else {
			bRet = 2;
		}
	} else {
		Log.d(TAG, "version file not exist, maybe first update!");
	}

    return bRet;
}



/*************************************************************************
** 方法名称: get_unzip_update_check
** 方法功能: 从镜像文件中提取update_app,并替换掉系统中存在的update_app
** 入口参数: 
** 返 回 值: 成功返回0;失败返回-1
** 调     用: main
**
*************************************************************************/
static int get_unzip_update_check(const char* image_path)
{
    int bRet = -1;
    FILE *fp = nullptr;
    u8 buf[1024 * 1024];
    const char *key = get_key();
    u32 update_app_len;
    u32 read_len;
	char update_app_name[1024];
	char ver_str[128] = {0};
	SYS_VERSION* pVer = NULL;
	

	memset(update_app_name, 0, sizeof(update_app_name));
	sprintf(update_app_name, "%s/%s", image_path, UPDATE_IMAGE_FILE);
	
    if (!check_file_key_md5(update_app_name)) {	/* 文件的MD5值进行校验: 校验失败返回-1 */
        Log.e(TAG, "md5 check err [%s]", update_app_name);
        // disp_update_error(ERR_CHECK);
        // goto EXIT;
    }

	Log.d(TAG, "check file [%s] MD5 success...", update_app_name);
	
    fp = fopen(update_app_name, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        Log.e(TAG, "open pro_update [%s] fail", update_app_name);
        disp_update_error(ERR_OPEN_BIN);
        goto EXIT;
    }

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    read_len = fread(buf, 1, strlen(key), fp);
    if (read_len != strlen(key)) {
        Log.e(TAG, "read key len mismatch(%u %zd)", read_len, strlen(key));
        disp_update_error(ERR_READ_KEY);
        goto EXIT;
    }
	
    if (strcmp((const char *) buf, key) != 0) {
        Log.e(TAG, "key mismatch(%s %s)\n", key, buf);
        disp_update_error(ERR_KEY_MISMATCH);
        goto EXIT;
    }

	/* 提取比较版本 */
    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, sizeof(SYS_VERSION), fp);
    if (read_len != sizeof(SYS_VERSION)) {
        Log.e(TAG, "read version len mismatch(%u 1)", read_len);
        disp_update_error(ERR_READ_VER_LEN);
        goto EXIT;
    }

	pVer = (SYS_VERSION*)buf;
	Log.d(TAG, "image version: [%d.%d.%d]", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	
	sprintf(ver_str, "%d.%d.%d", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	property_set(PROP_SYS_IMAGE_VER, ver_str);

    bRet = version_check(pVer);
    if (bRet != 0) {
        goto EXIT;
    }

	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    read_len = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (read_len != UPDATE_APP_CONTENT_LEN) {
        disp_update_error(ERR_READ_APP_LEN);
        Log.e(TAG, "update app len mismatch(%d %d)\n", read_len, UPDATE_APP_CONTENT_LEN);
        goto EXIT;
    }
	
    update_app_len = bytes_to_int(buf);

	/* 从更新文件中得到update_app.zip */
	memset(update_app_name, 0, sizeof(update_app_name));
	sprintf(update_app_name, "%s/%s", UPDATE_APP_TMP_PATH, UPDATE_APP_ZIP);
	Log.d(TAG, "update app zip full path: %s", update_app_name);
	
    if (gen_file(update_app_name, update_app_len, fp)) {
		
		/* 将update_app直接解压到/usr/local/bin/目录下 */
        if (tar_zip(update_app_name, UPDATE_APP_DEST_PATH) == 0) {	/* 直接将其解压到/usr/local/bin目录下 */
			Log.d(TAG, "unzip update_app success...");
			bRet = 0;
        } else {	/* 解压update_app.zip文件出错 */
			Log.e(TAG, "unzip update_app.zip failed...");
            disp_update_error(ERR_TAR_APP_ZIP);
        }
    } else {	/* 提取update_app.zip文件出错 */
		Log.e(TAG, "gen_file update_app.zip failed...");
        disp_update_error(ERR_GET_APP_ZIP);
    }
	
EXIT:
    if (fp) {
        fclose(fp);
    }
	Log.e(TAG, "get_update_app, ret = %d", bRet);
    return bRet;
}



static int read_line(int fd, void *vptr, int maxlen)
{
    int n, rc;
    char c;
    char *ptr;

    ptr = (char *)vptr;
    for (n = 1; n < maxlen; n++) {
        again:
			
        if ((rc = read(fd, &c, 1)) == 1) {
            //not add '\n' to buf
            if (c == '\n')
                break;
            *ptr++ = c;

        } else if (rc == 0) {
            *ptr = 0;
            return (n - 1);
        } else {
            if (errno == EINTR) {
                Log.d(TAG, "ad line error");
                goto again;
            }
            return -1;
        }
    }
    *ptr = 0;
    return(n);
}

/*************************************************************************
** 方法名称: search_updatebin_from_rmdev
** 方法功能: check_update服务的入口
** 入口参数: 
**		argc - 命令行参数的个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
static bool search_updatebin_from_rmdev() 
{
	/* 1.通过查询"/proc/mounts"来查询已经挂载的文件系统(可移动存储设备) */
	bool bRet = false;
	char image_path[512];
	
	int fd = open("/proc/mounts", O_RDONLY);
	
	if (fd > 0) {

		char buf[1024];
		int iLen = -1;
		char *delim = (char *)" ";
		struct stat fileStat;

		memset(buf, 0, sizeof(buf));
		
		while ((iLen = read_line(fd, buf, sizeof(buf))) > 0) {

			char *p = strtok(buf, delim);	/* 提取"cat /proc/mounts"的低0列(设备文件) */
			if (p != nullptr) {
				
				if (is_removable_device(p)) {	/* 检查该设备文件是否为可移动存储设备(mmcblkX, sdX) */
					p = strtok(NULL, delim);	/* 获取该移动设备的挂载点 */
					if (p) {	/* 挂载点存在 */
						Log.d(TAG, "mount point [%s] is exist..", p);
						
						/* 判断挂载点的根路径下是否存在升级文件(Insta360_Pro2_Update.bin) */
						memset(image_path, 0, sizeof(image_path));
						memset(gUpdateImageRootPath, 0, sizeof(gUpdateImageRootPath));
						sprintf(gUpdateImageRootPath, "%s", p);

						sprintf(image_path, "%s/%s", p, UPDATE_IMAGE_FILE);

						Log.i(TAG, "image_path [%s]", image_path);
						if (access(image_path, F_OK) != 0) {
							continue;
						}

						if (stat(image_path, &fileStat)) {
							continue;
						} else {
							if (S_ISREG(fileStat.st_mode)) {
								Log.d(TAG, "[%s: %d] regular file", __FILE__, __LINE__);
							} else {
								Log.d(TAG, "[%s: %d] Not regular file, pass it", __FILE__, __LINE__);
								continue;
							}
						}
						bRet = true;
					} else {
						Log.d(TAG, "no mount path?");
					}
				}
			}
			memset(buf, 0, sizeof(buf));
		}
		close(fd);
	}
	return bRet;
}

/*
 * 1.检查设备文件存在
 * 2.挂载设备文件
 * 3.检查是否存在升级文件
 * 4.拷贝设备文件到EMMC
 * 5.卸载设备，启动update_app
 */

/*************************************************************************
** 方法名称: check_update_device_insert
** 方法功能: 检查系统中是否有可移动设备插入到系统
** 入口参数: 
**		无
** 返回值: 有升级设备插入返回true;否则返回false
** 调 用: main
**
*************************************************************************/
static bool check_update_device_insert()
{
    bool bFound = false;
    char devname[512];
    char *filename;
    DIR *dir;
    struct dirent *de;

    const char *dirname = "/dev";

    dir = opendir(dirname);

    if (dir == NULL)
        return false;

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        strcpy(filename, de->d_name);
		for (u32 i = 0; i < sizeof(gUpdateDevPrefix) / sizeof(gUpdateDevPrefix[0]); i++) {
			if (strstr(filename, gUpdateDevPrefix[i]) != NULL) {
				bFound = true;
				memset(gDevNodeFullPath, 0, 512);
				sprintf(gDevNodeFullPath, "/dev/%s", filename);
				break;
			}
		}
    }
    closedir(dir);
    return bFound;
}


#define MAX_WAIT_TIMES	20



/*************************************************************************
** 方法名称: wait_update_dev_mounted
** 方法功能: 等待指定的设备节点挂载成功
** 入口参数: 
**		dev_node - 设备节点名称
**		timeout - 等待超时时间（）
** 返回值: 是否成功挂载标志
** 调 用: main
**
*************************************************************************/
static bool wait_update_dev_mounted(char* dev_node, int timeout)
{
	int wait_step = 500;	
	int i_cnt = 0;
	
	while (true) {
		int fd = open("/proc/mounts", O_RDONLY);
		if (fd < 0) {
			Log.e(TAG, "open /proc/mounts failed..");
			return false;
		}

		char buf[1024];
		int iLen = -1;
		char *delim = (char *)" ";

		memset(buf, 0, sizeof(buf));
		while ((iLen = read_line(fd, buf, sizeof(buf))) > 0) {
			char *p = strtok(buf, delim);	/* 提取"cat /proc/mounts"的低0列(设备文件) */
			if (p != nullptr) {
				if (strstr(p, dev_node) != NULL) {	/* 找到第一个挂载上的USB/SD卡设备 */
					Log.d(TAG, "device[%s] have mounted ....", p);
					close(fd);
					return true;
				}			
			}
			memset(buf, 0, sizeof(buf));
		}
		
		close(fd);
		msg_util::sleep_ms(wait_step);
		i_cnt++;

		if (i_cnt >= MAX_WAIT_TIMES) {
			Log.d(TAG, "wait device mount timeout ...");
			break;
		}
	}
	return false;
}


static void err_reboot()
{
	/** 关闭日志,关闭显示 */
 	arlog_close();
	start_reboot();
}



/*************************************************************************
** 方法名称: set_sys_ver_prop
** 方法功能: 检查固件的版本，并将版本号写入属性系统中
** 入口参数: 
**		ver_path - 固件版本文件的全路径名
** 返回值: 无
** 调 用: main
**
*************************************************************************/
static void set_sys_ver_prop(const char* ver_path)
{
	char* pret = nullptr;
	char sys_ver[128];
	
	/* 读取版本文件 */
	if (access(ver_path, F_OK) == 0) {
		FILE* fp = fopen(ver_path, "rb");
		if (fp) {
			pret = fgets(sys_ver, sizeof(sys_ver), fp);
			if (pret) {
				str_trim(sys_ver);
				Log.d(TAG, "get sys_ver [%s]", sys_ver);
				property_set(PROP_SYS_FIRM_VER, sys_ver);
			}
			fclose(fp);
		}
	} else {
		property_set(PROP_SYS_FIRM_VER, "0.0.0");
	}
}


/*************************************************************************
** 方法名称: main
** 方法功能: check_update服务的入口
** 入口参数: 
**		argc - 命令行参数的个数
**		argv - 参数列表
** 返 回 值: 成功返回0;失败返回-1
** 调     用: OS
**
*************************************************************************/
int main(int argc, char **argv)
{
	int iRet = -1;
	bool found = false;
	bool result = false;
	const char* pUcDelayStr = NULL;	
	long iDelay = 0;
	char com_cmd[1024] = {0};
	char cp_src_path[1024] = {0};
	char cp_dst_path[1024] = {0};
	int iCpCnt = 0;

	/* 注册信号处理函数 */
    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

	arlog_configure(true, true, UPDATE_LOG_PATH, false);	/* 配置日志 */

	iRet = __system_properties_init();		/* 属性区域初始化 */
	if (iRet) {
		Log.e(TAG, "update_check service exit: __system_properties_init() faile, ret = %d", iRet);
		return -1;
	}

	Log.d(TAG, "Service: update_check starting ^_^ !!");

	property_set(PROP_SYS_UC_VER, UPDAE_CHECK_VER);


	/** 读取系统的固件版本并写入到属性系统中 */
	set_sys_ver_prop(VER_FULL_PATH);

	/*
 	 * 系统启动后USB的挂载需要一些时间,因此可通过属性系统来配置update_check服务的等待时间
	 * prop: "ro.delay_uc_time"
	 */
	pUcDelayStr = property_get("ro.delay_uc_time");	
	if (pUcDelayStr != NULL) {
		iDelay = atol(pUcDelayStr);
		if (iDelay < 0 || iDelay > 20)
			iDelay = 0;
		Log.d(TAG, "service update_check service delay [%d]s", iDelay);
		sleep(iDelay);
	}

	/*
	 * 1.检查/dev/目录下mmcblkXpX 和sdX是否存在,如果存在说明SD卡或硬盘已经存在
	 * 此时需要等待系统将磁盘挂载后再往下执行(会出现有设备文件不挂载情况?)
	 */
	found = check_update_device_insert();
	if (found) {	/* 升级设备存在,等待升级挂载之后再进行后续操作 */
		
		Log.d(TAG, "update device node exist, but need confirm it's have mounted...");
		Log.d(TAG, "device node [%s]\n", gDevNodeFullPath);

		/** 等待SD卡挂载成功（挂载操作由systemd-udevd服务完成）*/
		result = wait_update_dev_mounted(gDevNodeFullPath, 5000);
		if (!result) {
			Log.e(TAG, "wait timeout, maybe need more time to wait mounted...");
			goto EXIT;
		}
		
		/** SD卡挂载完成后,检查根目录下是否存在Insta360_Pro_Update.bin */
		result = search_updatebin_from_rmdev();
		if (!result) {
			Log.e(TAG, "update image file not exist, start app now...");
			goto EXIT;
		}

		/* 将升级文件拷贝到/tmp目录下 */
		sprintf(cp_src_path, "%s/%s", gUpdateImageRootPath, UPDATE_IMAGE_FILE);
		sprintf(cp_dst_path, "%s/%s", "/tmp", UPDATE_IMAGE_FILE);
		Log.d(TAG, "[%s: %d] Copy image source path name: %s", __FILE__, __LINE__, cp_src_path);
		Log.d(TAG, "[%s: %d] Copy image dest path name: %s", __FILE__, __LINE__, cp_dst_path);

		/* 拷贝升级文件到/tmp下 */
		sprintf(com_cmd, "cp -f %s %s", cp_src_path, cp_dst_path);
		for (int i = 0; i < 3; i++) {
			if (system(com_cmd)) {
				iCpCnt++;
				Log.e(TAG, "[%s: %d] Exec Copy Image from disk to System(/tmp) failed", __FILE__, __LINE__);
				msg_util::sleep_ms(500);
			} else {
				Log.d(TAG, "[%s: %d] Exec Copy Image from disk to System(/tmp) Success", __FILE__, __LINE__);
				break;
			}
		}

		if (iCpCnt >= 3) {
			Log.e(TAG, "[%s:%d] Copy retry 3 times failed, What's wrong, reboot now", __FILE__, __LINE__);
			system("reboot");
		} else {

			property_set("update_image_path", UPDATE_IMG_DEST_PATH);
			
			/** 升级文件存在 */
			init_oled_module();

			Log.e(TAG, "update_image_path[%s]", property_get("update_image_path"));
			
			#if 0
			/* 检查磁盘空间是否足够 */
			memset(image_path, 0, sizeof(image_path));
			sprintf(image_path, "%s/%s", gUpdateImageRootPath, UPDATE_IMAGE_FILE);
			sprintf(upate_check_path, "%s/%s", gUpdateImageRootPath, UPDATE_APP_ZIP);
			u32 bin_file_size = get_file_size(image_path);	/* 得到升级文件的大小 */
			#endif


       		iRet = get_unzip_update_check(UPDATE_IMG_DEST_PATH);	/* 提取用于系统更新的应用: update_app */
			if (iRet == 0) {	/* 提取update_app成功(会将其放入到/usr/local/bin/app/下) */
				
				/* 设置属性: "sys.uc_update_app"为true
					* 让monitor服务启动update_app服务来进行具体的更新操作 
					*/
				property_set(PROP_UC_START_UPDATE, "true");	/* 启动update_app服务 */
				Log.d(TAG, "update_check normal exit now ...");
				arlog_close();	
				return 0;
			} else if (iRet == 2) {	/* 版本低于目标板 */
				Log.d(TAG, "image version smaller than board...");
				goto EXIT;
			} else {	/* 提取update_app失败,提示错误并重启 */
				Log.e(TAG, "get_update_app fail\n");
				disp_update_error(ERR_GET_APP);
				err_reboot();
			}	
		}	
	} else {
		Log.d(TAG, "update device not insert yet....");
	}

EXIT:

	/* 直接启动应用
	 * 1.升级设备不存在
	 * 2.升级镜像不存在
	 * 3.升级镜像提取错误
	 */
	Log.d(TAG, "needn't startup update_app, start app directly...");	

	arlog_close();	/* 关闭日志 */		
	
	system("killall vold_test");

	/* 卸载挂载的升级设备 */
	string cmd = "umount ";
	cmd += gDevNodeFullPath;
	system(cmd.c_str());

	property_set(PROP_UC_START_APP, "true");	/* 不需要重启,直接通知init进程启动其他服务 */	

	/* 关闭动画服务 */
	property_set(PROP_BOOTAN_NAME, "false");
	
	return 0;
}


