#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include <dirent.h>

#include <log/stlog.h>

#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 


#undef  TAG
#define TAG "Vold"

VolumeManager *VolumeManager::sInstance = NULL;


static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}


static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if(d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}


VolumeManager* VolumeManager::Instance() 
{
    if (!sInstance)
        sInstance = new VolumeManager();
    return sInstance;
}


VolumeManager::VolumeManager() 
{
    mDebug = false;
	Volume* tmpVol = NULL;

#ifdef ENABLE_VOLUME_MANAGER_USE_NETLINK
    iListenerMode = VOLUME_MANAGER_LISTENER_MODE_NETLINK;
#else
    iListenerMode = VOLUME_MANAGER_LISTENER_MODE_INOTIFY;
#endif

    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();

    /* 根据类型将各个卷加入到系统多个Vector中 */
    for (int i = 0; i < sizeof(gSysVols) / sizeof(gSysVols[0]); i++) {
        tmpVol = &gSysVols[i];  
        mVolumes.push_back(tmpVol);

        if (tmpVol->iType == VOLUME_TYPE_MODULE) {
            mLocalVols.push_back(tmpVol);
        } else {
            mModuleVols.push_back(tmpVol);
        }
    }
}


bool VolumeManager::start()
{
    bool bResult = false;

    Log.d(TAG, "[%s: %d] Start VolumeManager now ....", __FILE__, __LINE__);

    if (iListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            Log.e(TAG, "[%s: %d] Unable to create NetlinkManager", __FILE__, __LINE__);
        } else {
            if (nm->start()) {
                Log.e(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                coldboot("/sys/block");
                bResult = true;
            }
        }
    
    } else {
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, iListenerMode);
    }
    return bResult;
}


bool VolumeManager::stop()
{
    bool bResult = false;

    if (iListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            Log.e(TAG, "[%s: %d] Unable to create NetlinkManager", __FILE__, __LINE__);
        } else {
            if (nm->stop()) {
                Log.e(TAG, "Unable to start NetlinkManager (%s)", strerror(errno));
            } else {
                bResult = true;
            }
        }
    
    } else {
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, iListenerMode);
    }
    return bResult;
}



VolumeManager::~VolumeManager()
{
    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();
}


/*
 * 处理来自底层的事件
 * 1.Netlink
 * 2.inotify(/dev)
 */
void VolumeManager::handleBlockEvent(NetlinkEvent *evt)
{
    /*
     * 1.根据NetlinkEvent信息来查找对应的卷
     * 2.根据事件的类型作出（插入，拔出做响应的处理）
     *  - 插入: 是磁盘还是分区 以及文件系统类型(blkid -c /dev/null /dev/xxx)
     *  - 拔出: 是磁盘还是分区 以及文件系统类型(blkid -c /dev/null /dev/xxx)
     * 挂载: 是否需要进行磁盘检查(fsck),处理
     * 真正挂咋
     * 卸载: 需要处理卸载不掉的情况(杀掉所有打开文件的进程???))
     * 挂载成功后/卸载成功后,通知UI(SD/USB attached/detacheed)
     */
    Log.d(TAG, ">>>>>>>>>>>>>>>>>> handleBlockEvent");
    
}

int VolumeManager::listVolumes()
{
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mVolumes.size(); i++) {
        tmpVol = mVolumes.at(i);
        if (tmpVol) {
            Log.d(TAG, "Volume type: %s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "VOLUME_SUBSYS_SD": "VOLUME_SUBSYS_USB" );
            Log.d(TAG, "Volume bus: %s", tmpVol->pBusAddr);
            Log.d(TAG, "Volume mountpointer: %s", tmpVol->pMountPath);
            Log.d(TAG, "Volume devnode: %s", tmpVol->cDevNode);

            Log.d(TAG, "Volume Type %d", tmpVol->iType);
            Log.d(TAG, "Volume index: %d", tmpVol->iIndex);
            Log.d(TAG, "Volume state: %d", tmpVol->iVolState);

            Log.d(TAG, "Volume total %d", tmpVol->uTotal);
            Log.d(TAG, "Volume avail: %d", tmpVol->uAvail);
            Log.d(TAG, "Volume speed: %d", tmpVol->iSpeedTest);
        }
    }
}

/*
 * 挂载卷
 */
int VolumeManager::mountVolume(Volume* pVol)
{
    int iRet = 0;

    return iRet;
}

int VolumeManager::unmountVolume(const char *label, bool force, bool revert, bool badremove)
{
    int iRet = 0;

    return iRet;
}

/*
 * 格式化类型: 默认为exFat
 */
int VolumeManager::formatVolume(const char *label, bool wipe)
{
    int iRet = 0;

    return iRet;
}

void VolumeManager::setDebug(bool enable)
{
    mDebug = enable;
}


Volume* lookupVolume(const char *label)
{
    Volume* tmpVol = NULL;
    
    return tmpVol;
}


