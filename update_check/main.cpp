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

#define UPDAE_CHECK_VER		"V3.0"


static const char *get_key()
{
    return FP_KEY;
}




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
static bool upateVerCheck(SYS_VERSION* pVer)
{
    bool bRet = true;

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
		return isNeedUpdate(&old_ver, pVer);
	} else {
		Log.d(TAG, "version file not exist, maybe first update!");
	}

    return bRet;
}



/*************************************************************************
** 方法名称: setLastFirmVer2Prop
** 方法功能: 检查固件的版本，并将版本号写入属性系统中
** 入口参数: 
**		ver_path - 固件版本文件的全路径名
** 返回值: 无
** 调 用: main
**
*************************************************************************/
static void setLastFirmVer2Prop(const char* ver_path)
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


static int copyUpdateFile2Memory(const char* dstFile, const char* srcFile)
{
	int status;

    const char *args[4];
    args[0] = "/bin/cp";
    args[1] = "-f";
    args[2] = srcFile;
	args[3] = dstFile

	Log.d(TAG, "[%s: %d] Copy Cmd [%s %s %s %s]", __FILE__, __LINE__, args[0], args[1], args[2], args[3]);

    iRet = forkExecvpExt(ARRAY_SIZE(args), (char **)args, &status, false);
    if (iRet != 0) {
        Log.e(TAG, "copyUpdateFile2Memory failed due to logwrap error");
        return -1;
    }

    if (!WIFEXITED(status)) {
        Log.e(TAG, "mocopyUpdateFile2Memoryunt sub process did not exit properly");
        return -1;
    }

    status = WEXITSTATUS(status);
    if (status == 0) {
        Log.d(TAG, ">>>> copyUpdateFile2Memory OK");
        return 0;
    } else {
        Log.e(TAG, ">>> copyUpdateFile2Memory failed (unknown exit code %d)", status);
        return -1;
    }
}


enum {
	ERROR_SUCCESS = 0,
	ERROR_MD5_CHECK = -1,
	ERROR_OPEN_UPDATE_FILE = -2,
	ERROR_READ_LEN = -3,
	ERROR_KEY_MISMATCH = -4,
	ERROR_LOW_VERSION = -5,
	ERROR_GET_UPDATE_APP_ZIP = -6,
	ERROR_UNZIP_UPDATE_APP = -7,
	
};

/*************************************************************************
** 方法名称: getUpdateApp
** 方法功能: 从镜像文件中提取update_app,并替换掉系统中存在的update_app
** 入口参数: 
** 返 回 值: 成功返回0;失败返回-1
** 调     用: main
**
*************************************************************************/
static int getUpdateApp(const char* pUpdateFilePathName)
{
    int bRet = -1;
    FILE *fp = nullptr;

    u8 buf[1024 * 1024];

    const char *key = get_key();
    u32 update_app_len;
    u32 read_len;
	u32 uReadLen = 0;
	char update_app_name[1024];
	char ver_str[128] = {0};
	SYS_VERSION* pVer = NULL;
	

	memset(update_app_name, 0, sizeof(update_app_name));
	sprintf(update_app_name, "%s/%s", image_path, UPDATE_IMAGE_FILE);
	
    if (!check_file_key_md5(pUpdateFilePathName)) {		/* 文件的MD5值进行校验: 校验失败返回-1 */
        Log.e(TAG, "[%s: %d] Update File MD5 Check Error", __FILE__, __LINE__);
		return ERROR_MD5_CHECK;
    }

	Log.d(TAG, "[%s: %d] Update File[%s] MD5 Check Success", __FILE__, __LINE__, pUpdateFilePathName);
	
    fp = fopen(pUpdateFilePathName, "rb");	
    if (!fp) {	/* 文件打开失败返回-1 */
        Log.e(TAG, "[%s: %d] Open Update File[%s] fail", __FILE__, __LINE__, pUpdateFilePathName);
        return ERROR_OPEN_UPDATE_FILE;
    }

    memset(buf, 0, sizeof(buf));
    fseek(fp, 0L, SEEK_SET);

	/* 读取文件的PF_KEY */
    uReadLen = fread(buf, 1, strlen(key), fp);
    if (uReadLen != strlen(key)) {
        Log.e(TAG, "[%s: %d] Read key len mismatch(%u %zd)", __FILE__, __LINE__, uReadLen, strlen(key));
		fclose(fp);
		return ERROR_READ_LEN;
    }
	
    if (strcmp((const char *)buf, key) != 0) {
        Log.e(TAG, "[%s: %d] key mismatch(%s %s)", __FILE__, __LINE__, key, buf);
		fclose(fp);
		return ERROR_KEY_MISMATCH;
    }

	/* 提取比较版本 */
    memset(buf, 0, sizeof(buf));
    uReadLen = fread(buf, 1, sizeof(SYS_VERSION), fp);
    if (uReadLen != sizeof(SYS_VERSION)) {
        Log.e(TAG, "[%s: %d] read version len mismatch(%u 1)", read_len);
        fclose(fp);
		return ERROR_READ_LEN;
    }

	pVer = (SYS_VERSION*)buf;
	Log.d(TAG, "image version: [%d.%d.%d]", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	
	sprintf(ver_str, "%d.%d.%d", pVer->major_ver, pVer->minor_ver, pVer->release_ver);
	property_set(PROP_SYS_IMAGE_VER, ver_str);


	if (upateVerCheck(pVer) == false) {
		Log.d(TAG, "[%s: %d] Need not Update version", __FILE__, __LINE__);
		fclose(fp);
		return ERROR_LOW_VERSION;
	}


	/* 提取update_app.zip文件的长度 */
    memset(buf, 0, sizeof(buf));
    uReadLen = fread(buf, 1, UPDATE_APP_CONTENT_LEN, fp);
    if (uReadLen != UPDATE_APP_CONTENT_LEN) {
        Log.e(TAG, "[%s: %d] update app len mismatch(%d %d)", __FILE__, __LINE__, read_len, UPDATE_APP_CONTENT_LEN);
        fclose(fd);
		return ERROR_READ_LEN;
    }
	
	int iUpdateAppLen = bytes_to_int(buf);
	string updateAppPathName = "/tmp";
	updateAppPathName += UPDATE_APP_ZIP;
	const char* pUpdateAppPathName = updateAppPathName.c_str();

	Log.d(TAG, "[%s: %d] update_app.zip full path: %s", pUpdateAppPathName);
	
    if (gen_file(pUpdateAppPathName, iUpdateAppLen, fp)) {
		
		/* 将update_app直接解压到/usr/local/bin/目录下 */
        if (tar_zip(pUpdateAppPathName, UPDATE_APP_DEST_PATH) == 0) {	/* 直接将其解压到/usr/local/bin目录下 */
			Log.d(TAG, "[%s: %d] unzip update_app to [%s] Success", __FILE__, __LINE__, UPDATE_APP_DEST_PATH);
			fclose(fp);
			return ERROR_SUCCESS;
        } else {	/* 解压update_app.zip文件出错 */
			Log.e(TAG, "[%s: %d] unzip update_app.zip failed...", __FILE__, __LINE__);
            fclose(fp);
			return ERROR_UNZIP_UPDATE_APP;
        }
    } else {	/* 提取update_app.zip文件出错 */
		Log.e(TAG, "[%s: %d] extrac update_app.zip failed...", __FILE__, __LINE__);
        fclose(fp);
		return ERROR_GET_UPDATE_APP_ZIP;
    }

	return 0;
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

	/* update_check中清除清空挂载目录中的一切 */
	system("rm -rf /mnt/*");

	/** 启动卷管理器,用于挂载升级设备 */
    VolumeManager* vm = VolumeManager::Instance();
    if (vm) {
        Log.d(TAG, "[%s: %d] +++++++++++++ Start Vold(2.4) Manager +++++++++++", __FILE__, __LINE__);
        vm->start();
    }

	/** 读取系统的固件版本并写入到属性系统中 */
	setLastFirmVer2Prop(VER_FULL_PATH);

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

	if (vm->checkLocalVolumeExist()) {	/* 卷存在，并且已经挂载 */
		string updateFilePathName = vm->getLocalVolMountPath();
		updateFilePathName += UPDATE_IMAGE_FILE;

		const char* pUpdateFilePathName = updateFilePathName.c_str();
		Log.d(TAG, "[%s: %d] Update file path name -> %s", __FILE__, __LINE__, pUpdateFilePathName);

		if (access(pUpdateFilePathName, F_OK) == 0) {
			struct stat fileStat;
			if (stat(pUpdateFilePathName, &fileStat)) {
				Log.e(TAG, "[%s: %d] stat file prop failed", __FILE__, __LINE__);
				goto err_stat;
			} else {
				if (S_ISREG(fileStat.st_mode)) {
					Log.d(TAG, "[%s: %d] regular file", __FILE__, __LINE__);

					string dstUpdateFilePath = "/tmp/";
					dstUpdateFilePath += UPDATE_IMAGE_FILE;
					const char* pDstUpdateFilePath = dstUpdateFilePath.c_str();
					
					/* 拷贝升级文件到/tmp */
					copyUpdateFile2Memory(pDstUpdateFilePath, pUpdateFilePathName);

					int i, iError = 0;
					for (i = 0; i < 3; i++) {
						iError = getUpdateApp(pDstUpdateFilePath);
						if (iError == ERROR_SUCCESS || iError == ERROR_LOW_VERSION) {
							break;
						}
					}
					
					if (i < 3) {	/* 成功提取或者版本低 */
						if (iError == ERROR_SUCCESS) {
							vm->unmountCurLocalVol();
							property_set(PROP_UC_START_UPDATE, "true");	/* 启动update_app服务 */
							Log.d(TAG, "[%s: %d] Enter the real update program", __FILE__, __LINE__);
							arlog_close();	
							return 0;
						} else {	/* 直接启动APP */
							Log.d(TAG, "[%s: %d] Skip this version, start app now...", __FILE__, __LINE__);
							goto err_low_ver;
						}
					} else {
						Log.e(TAG, "[%s: %d] Parse update file Failed", __FILE__, __LINE__);
						arlog_close();	

						/* 提示失败的原因，并倒计时重启 */
						vm->unmountCurLocalVol();
						init_oled_module();
						disp_update_error(ERR_GET_APP);
						disp_start_reboot(5);
					}
				} else {
					Log.d(TAG, "[%s: %d] Update file is Not regular file", __FILE__, __LINE__);
					goto err_regula_update_file;
				}
			}
		} else {	/* 升级文件不存在 */
			Log.e(TAG, "[%s: %d] Update file [%s] Not exist in current update device", __FILE__, __LINE__, pUpdateFilePathName);
			goto no_update_file;
		}

	} else {
		Log.d(TAG, "[%s: %d] Local storage device Not exist or Not Mounted, start app now ...");
		goto no_update_device;
	}


err_low_ver:
err_regula_update_file:
err_stat:
no_update_file:
	vm->unmountCurLocalVol();

no_update_device:

	arlog_close();	/* 关闭日志 */		
	property_set(PROP_UC_START_APP, "true");	/* 不需要重启,直接通知init进程启动其他服务 */	
	property_set(PROP_BOOTAN_NAME, "false");	/* 关闭动画服务 */
	return 0;
}


