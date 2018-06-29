/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: main.cpp
** 功能描述: 自动挂载服务(支持自动挂载SD卡，U盘等)
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <mutex>

#include <common/include_common.h>
#include <common/sp.h>
#include <common/check.h>
#include <log/arlog.h>
#include <system_properties.h>
#include <unistd.h>
#include <dirent.h>
#include <prop_cfg.h>

using namespace std;

/*
 * 程序启动时需要对已经生成的设备文件进行扫描并挂载
 * 需要挂载的有两大类:
 * - SD卡(mmcblk1)
 * - U盘或USB移动硬盘
 */

/*
 * 新增支持挂载的文件系统类型
 * ext2,3,4
 * exfat(perfect),
 * fat
 * ntfs
 */

std::mutex devMutex;

#define WATCH_PATH "/dev"

#define VOLD_VER "v2.2"

#define TAG "vold"
#define VOLD_LOG_PATH "/home/nvidia/insta360/log/vold_log"


#define MAX_FILES 			1000
#define EPOLL_COUNT 		20
#define MAXCOUNT 			500
#define EPOLL_SIZE_HINT 	8


typedef struct t_name_fd {
    int fd;
    char name[30];
} T_name_fd;


/* 默认支持的文件系统格式(Window,Linux兼容: exFat, NTFS, FAT32等)
 * 1.在启动服务时,需要扫描已经存在的设备(mmcblk1xx, sdxx),如果设备已经存在,需要将其挂载
 * 2.服务启动后,需要动态的检测系统的分区的变化情况并作出相应的动作
 * - 挂载规则:
 * 1.如果磁盘没有分区,直接挂载磁盘
 * 2.如果磁盘有分区,默认挂载第一个分区
 * 3.SD卡默认挂载路径: /mnt/sdcard/
 * 4.U盘/USB接口硬盘默认挂载路径: /mnt/udisk
 * SD卡 -> mmcblkX[px]
 * U盘  -> sdX[1-4]
 */

enum {
	ACTION_ADD = 1,
	ACTION_REMOVE = 2,
	ACTION_CHANGE = 3,
	ACTION_MAX
};


enum {
	MEDIA_TYPE_SD = 0,
	MEDIA_TYPE_USB = 1,
	MEDIA_TYPE_MAX
};

enum {
	DEV_TYPE_DISK = 1,
	DEV_TYPE_PART = 2,
	DEV_TYPE_MAX
};

struct stMountSlotObj {
	int iMediaType;					/* 媒介类型 */
	bool hasMounted;				/* 是否已经挂载标志 */
	const char* cMountPointPath;	/* 挂载点的挂载路径 */
	const char* devPropName;
	const char* devNodeName;		/* 挂载的设备节点名 */
};

static const char* default_mount_devices[] = {
	"/dev/mmcblk1",
	"/dev/mmcblk1p1",
	"/dev/mmcblk1p2",
	"/dev/sda",
	"/dev/sda1",
	"/dev/sdb",
	"/dev/sdb1",
	"/dev/sdc",
	"/dev/sdc1",
	"/dev/sdd",
	"/dev/sdd1",
};


T_name_fd  t_name_fd[100];
int count_name_fd;
static char *epoll_files[MAX_FILES];
static struct epoll_event mPendingEventItems[EPOLL_COUNT];

static int mINotifyFd, mEpollFd, i;
static char inotifyBuf[MAXCOUNT];
static char epollBuf[MAXCOUNT];
static int gCurDiskCnt = 0;


/*
 * 最大支持9个卡槽（SD卡/USB磁盘
 */
struct stMountSlotObj mountObjs[] = {
	{MEDIA_TYPE_SD, false, "/mnt/sdcard",  PROP_SYS_DEV_DISK0, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk1", PROP_SYS_DEV_DISK1, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk2", PROP_SYS_DEV_DISK2, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk3", PROP_SYS_DEV_DISK3, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk4", PROP_SYS_DEV_DISK4, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk5", PROP_SYS_DEV_DISK5, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk6", PROP_SYS_DEV_DISK6, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk7", PROP_SYS_DEV_DISK7, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk8", PROP_SYS_DEV_DISK8, NULL},
	{MEDIA_TYPE_USB, false, "/mnt/udisk9", PROP_SYS_DEV_DISK9, NULL},
};

#define BLK_DEV_TMP_FILE "/tmp/.blk_tmp"


#define MAX_MOUNT_POINT (sizeof(mountObjs) /sizeof(mountObjs[0]))


static int getfdFromName(char* name)
{
    int i;
    for (i = 0; i < MAX_FILES; i++)
    {
        if (!epoll_files[i])
            continue;

        if (0 == strcmp(name, epoll_files[i]))
        {
            return i;
        }
    }
    return -1;
}


static int exec_cmd(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status)
    {
       Log.e(TAG, "exec_cmd>> system %s error\n", str);
    }
    else
    {
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status))
        {
            if (0 == WEXITSTATUS(status))
            {
                iRet = 0;
            }
            else
            {
                Log.e(TAG, "exec_cmd>> %s fail script exit code: %d\n", str, WEXITSTATUS(status));
				Log.e(TAG, "exec_cmd>> errno = %d, %s\n", errno, strerror(errno));
            }
        }
        else
        {
            Log.e(TAG, "exec_cmd>> exit status %s error  = [%d]\n", str, WEXITSTATUS(status));
        }
    }
    return iRet;
}


bool isDevNeedMount(const char* deviceName, char* fsType, int iLen)
{
	bool needMount = false;

	char line[1024] = {0};
	char* pLine = NULL;
	char* pdevNaneEnd = NULL;
	char devPath[128] = {0};
	char* pFsType = NULL;
	
	/* 
	 * 读取"/tmp/.blk_tmp"文件,去除设备文件路径字段与deviceName进行匹配
	 * 批评成功,查看是否有TYPE字段(如果有说明改设备可以挂载)
	 */
	FILE *fp = fopen(BLK_DEV_TMP_FILE, "rb");
	if (fp)
	{
		while ((pLine = fgets(line, sizeof(line), fp)))
		{
			line[strlen(line) - 1] = '\0';	/* 将换行符替换成'\0' */

			pdevNaneEnd = strstr(line, ":");
			snprintf(devPath, pdevNaneEnd - &line[0] + 1, "%s", line);
			
			if (strcmp(devPath, deviceName) != 0)
			{
				continue;
			}
			else
			{
				/* 检查是否有"TYPE"字段 */
				pFsType = strstr(line, "TYPE=");
				if (NULL == pFsType)
				{
					continue;
				}
				else
				{
					//if (strstr(line, "PTTYPE=") == NULL)
					//{
						/* 获取文件系统类型 */
						pFsType += strlen("TYPE=") + 1;
						for (int i = 0; i < iLen; i++)
						{
							if (pFsType[i] != '"')
							{
								fsType[i] = pFsType[i];
							}
							else
							{
								break;
							}
						}
						needMount = true;
						break;
					}
//				}
			}
		}

		fclose(fp);
	}
	else
	{
		Log.e(TAG, "file [%s] not exist??", BLK_DEV_TMP_FILE);
	}
	return needMount;
}

const char* getDeviceMountPath(const char* devName, int* index)
{
	const char* mountPath = NULL;
	
	/* 是SD卡还是U盘设备 */
	if (strstr(devName, "mmc") != NULL)
	{
		if (mountObjs[MEDIA_TYPE_SD].hasMounted == false)
		{
			mountObjs[MEDIA_TYPE_SD].devNodeName = strdup(devName);
			mountPath = mountObjs[MEDIA_TYPE_SD].cMountPointPath;
			*index = 0;
		}
	}
	else if (strstr(devName, "sd") != NULL)
	{
		/* Max support 9 usb device now ... */
		int i = 0;
		for (i = 0;  i < PROP_MAX_DISK_SLOT_NUM; i++)
		{
			if (mountObjs[i].iMediaType == MEDIA_TYPE_USB && mountObjs[i].hasMounted == false) {
				mountObjs[i].devNodeName = strdup(devName);
				mountPath = mountObjs[i].cMountPointPath;
				*index = i;
				break;
			}
		}
	} 

	return mountPath;
}

/*
 * 执行添加操作
 */
static void handleAddAction(const char* deviceName)
{
	int iRet = -1;
	bool needMounted = false;
	char fsType[64] = {0};
	char mount_cmd[512] = {0};
	const char* mountPath = NULL;
	int iIndex = 0;
	char cDiskNum[32] = {0};

	unique_lock<mutex> lock(devMutex);

	needMounted = isDevNeedMount(deviceName, fsType, 64);
	if (needMounted)
	{
		/* 调用挂载函数进行挂载 */
		mountPath = getDeviceMountPath(deviceName, &iIndex);
		if (mountPath)
		{
			Log.d(TAG, "mount: devname[%s], path[%s], fstype[%s]\n", deviceName, mountPath, fsType);
			#if 0
			iRet = mount(deviceName, mountPath, fsType, 0, NULL);
			if (iRet)
			{
				printf("mount failed, reason [%s] \n", strerror(errno));
			}
			else
			{
				printf("mount success ...\n");
				mountObjs[iMediaType].hasMounted = true;
			}
			#else
			for (int i = 0; i < 3; i++)
			{
				sprintf(mount_cmd, "mount %s %s", deviceName, mountPath);
				iRet = exec_cmd(mount_cmd);
				if (iRet)
				{
					Log.e(TAG, "mount device[%s] failed, reason [%s]", deviceName, strerror(errno));
				}
				else
				{
					Log.d(TAG, "mount device[%s] on path [%s] success", deviceName, mountPath);
					mountObjs[iIndex].hasMounted = true;
					gCurDiskCnt += 1;
					sprintf(cDiskNum, "%d", gCurDiskCnt);
					property_set(PROP_SYS_DISK_NUM, cDiskNum);
					property_set(mountObjs[iIndex].devPropName, mountObjs[iIndex].devNodeName);
					break;
				}
				usleep(500 * 1000);
			}

			#endif
		}
	}

}

static void handleRmAction(const char* deviceName)
{
	int iRet = -1;
	char cDiskNum[32] = {0};
	unique_lock<mutex> lock(devMutex);

	printf("rm device name: %s\n", deviceName);
	
	for (u32 i = 0; i < MAX_MOUNT_POINT; i++)
	{
		if ((mountObjs[i].devNodeName != NULL) &&  strcmp(deviceName, mountObjs[i].devNodeName) == 0 && mountObjs[i].hasMounted == true)
		{
			iRet = umount2(mountObjs[i].cMountPointPath, MNT_FORCE);
			if (iRet)
			{
				Log.e(TAG, "umout failed, reason [%s]\n", strerror(errno));
			}
			else
			{
				Log.d(TAG, "umount path[%s] success", mountObjs[i].cMountPointPath);
				mountObjs[i].hasMounted = false;
				if (mountObjs[i].devNodeName)
				{
					gCurDiskCnt -= 1;
					if (gCurDiskCnt < 0)
						gCurDiskCnt = 0;

					sprintf(cDiskNum, "%d", gCurDiskCnt);
					property_set(PROP_SYS_DISK_NUM, cDiskNum);
					property_set(mountObjs[i].devPropName, "none");
					free((void*)mountObjs[i].devNodeName);
					mountObjs[i].devNodeName = NULL;
				}
			}
			break;			
		}
	}
}


static void handleMonitorAction(int action, const char* deiceName)
{
	switch (action)
	{
		case ACTION_ADD:
			handleAddAction(deiceName);
			break;

		case ACTION_REMOVE:
			handleRmAction(deiceName);
			break;

		case ACTION_CHANGE:
		default:
			Log.e(TAG, "Unkown deal method...\n");
			break;
	}
}

/*
 * 运行挂载管理服务前需要扫描已经生成的设备文件,然后将其挂载
 */
static void scanAndMount()
{
	/* 扫描"/proc/partitions"文件,检测已经插入的SD卡和U盘并将其挂载好 
 	 * 依次读取各行(使用过滤规则将其中没用的行直接过滤,直接处理有效的行)
 	 */
	system("blkid > /tmp/.blk_tmp");


	/* 挂载已经存在的USB和SD卡(目前只支持一类一个设备) */
	for (u32 i = 0; i < sizeof(default_mount_devices) / sizeof(default_mount_devices[0]); i++)
	{
		handleAddAction(default_mount_devices[i]);
	}

}


static void checkMountPoint()
{
	for (int i = 0; i < MEDIA_TYPE_MAX; i++)
	{
		if (access(mountObjs[i].cMountPointPath, F_OK) != 0)
		{
			Log.d(TAG, "mkdir mount point -> [%s]", mountObjs[i].cMountPointPath);
			mkdir(mountObjs[i].cMountPointPath, 0766);
		}
	}
}


int main(int argc, char** argv)
{
	struct epoll_event eventItem;
	struct inotify_event inotifyEvent;
	struct inotify_event* curInotifyEvent;
	
	u32 readCount = 0;
	char devPath[256] = {0};


	/* 属性及日志系统初始化 */
	arlog_configure(true, true, VOLD_LOG_PATH, false);    /* 配置日志 */

	if (__system_properties_init())		/* 属性区域初始化 */
	{
		Log.e(TAG, "update_check service exit: __system_properties_init() failed");
		return -1;
	}
	
	property_set("sys.vold_ver", VOLD_VER);

	Log.d(TAG, ">>>>> Vold Manager Service, sarting <<<<<");

	/** 检查所有的挂载点是否存在,如果不存在则创建 */
	checkMountPoint();

	/** 执行一次扫描挂载动作(避免开机时插入的设备被遗漏) */
	scanAndMount();

	/* 1.初始化inotify和epoll */
	mINotifyFd = inotify_init();
	mEpollFd = epoll_create(EPOLL_SIZE_HINT);

	/* 2.添加监控的目录(监听/dev/下设备文件的变化<当有设备插入时会生成设备文件;当有设备拔出时会删除设备文件> */
	inotify_add_watch(mINotifyFd, WATCH_PATH, IN_DELETE | IN_CREATE);

	/* 3.将Inotify添加到epoll中 */
	eventItem.events = EPOLLIN;
	eventItem.data.fd = mINotifyFd;
	epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mINotifyFd, &eventItem);

	Log.d(TAG, "Starting watch /dev files changed now ...");

	while (true)
	{

		/* 4.等待事件的带来 */
		int pollResult = epoll_wait(mEpollFd, mPendingEventItems, EPOLL_COUNT, -1);
		
		Log.d(TAG, "epoll wait event num = %d\n", pollResult);

		/* 依次处理各个事件 */
		for (i = 0; i < pollResult; i++)
		{
			
			/* 有文件的变化,如文件的创建/删除 */
			if (mPendingEventItems[i].data.fd == mINotifyFd)
			{
				/* 读取inotify事件，查看是add 文件还是remove文件，add 需要将其添加到epoll中去，remove 需要从epoll中移除 */
				readCount  = 0;
				readCount = read(mINotifyFd, inotifyBuf, MAXCOUNT);
                if (readCount <  sizeof(inotifyEvent))
                {
		            Log.e(TAG, "error inofity event");
		            return -1;
                }

                // cur 指针赋值
                curInotifyEvent = (struct inotify_event*)inotifyBuf;

                while (readCount >= sizeof(inotifyEvent))
                {
                	if (curInotifyEvent->len > 0)
                	{
                		/* 生成临时文件.blk_tmp文件 */
                		usleep(50 * 1000);	/* delay 50ms */
                		system("blkid > /tmp/.blk_tmp");
                		usleep(50 * 1000);

                		memset(devPath, 0, sizeof(devPath));
                		sprintf(devPath, "/dev/%s", curInotifyEvent->name);
						
                		if (curInotifyEvent->mask & IN_CREATE)
                		{
                			/* 有新设备插入,根据设备文件执行挂载操作 */
                			handleMonitorAction(ACTION_ADD, devPath);
                		}
                		else if (curInotifyEvent->mask & IN_DELETE)
                		{
                			/* 有设备拔出,执行卸载操作 */
                			handleMonitorAction(ACTION_REMOVE, devPath);
                		}
                	}
                	curInotifyEvent --;
                	readCount -= sizeof(inotifyEvent);
                }
			}
			else
			{	//6. 其他原有的fd发生变化
				Log.d(TAG, "file changer------------------");
				readCount = 0;
				readCount = read(mPendingEventItems[i].data.fd, epollBuf, MAXCOUNT);
				if (readCount > 0)
				{
					epollBuf[readCount] = '\0';
					Log.d(TAG, "file can read, fd: %d, countent:%s",mPendingEventItems[i].data.fd,epollBuf);
				}
			}
		}
	}
	return 0;
}


