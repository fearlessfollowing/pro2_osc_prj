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
#include <string>
#include <stdlib.h>

#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>

#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>

using namespace std;

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


static int exec_cmd(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
       Log.e(TAG, "exec_cmd>> system %s error\n", str);
    } else {
        // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
                iRet = 0;
            } else {
                Log.e(TAG, "exec_cmd>> %s fail script exit code: %d\n", str, WEXITSTATUS(status));
				Log.e(TAG, "exec_cmd>> errno = %d, %s\n", errno, strerror(errno));
            }
        } else {
            Log.e(TAG, "exec_cmd>> exit status %s error  = [%d]\n", str, WEXITSTATUS(status));
        }
    }
    return iRet;
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


Volume* VolumeManager::isSupportedDev(const char* busAddr)
{
    u32 i = 0;
    Volume* tmpVol = NULL;
    for (i = 0; i < mVolumes.size(); i++) {
        tmpVol = mVolumes.at(i);
        if (tmpVol) {
            if (strstr(tmpVol->pBusAddr, busAddr)) {   /* 只要含有字串，认为支持 */
                Log.d(TAG, "[%s: %d] Volume Addr: %s, Current dev Addr: %s", __FILE__, __LINE__, tmpVol->pBusAddr, busAddr);
                break;
            }
        }
    }
    return tmpVol;
}




bool VolumeManager::extractMetadata(const char* devicePath, char* volFsType, int iLen)
{
    bool bResult = true;

    string cmd;
    cmd = "blkid";
    cmd += " -c /dev/null ";
    cmd += devicePath;

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        Log.e(TAG, "Failed to run %s: %s", cmd.c_str(), strerror(errno));
        bResult = false;
        goto done;
    }

    char line[1024];
    char value[128];
     
    if (fgets(line, sizeof(line), fp) != NULL) {
		//blkid identified as /dev/block/vold/179:14: LABEL="ROCKCHIP" UUID="0FE6-0808" TYPE="vfat"
        Log.d(TAG, "blkid identified as %s", line);
		
        #if 0
        char* start = strstr(line, "UUID=");
        if (start != NULL && sscanf(start + 5, "\"%127[^\"]\"", value) == 1) {
            setUuid(value);
        } else {
            setUuid(NULL);
        }

        start = strstr(line, "LABEL=");
        if (start != NULL && sscanf(start + 6, "\"%127[^\"]\"", value) == 1) {
            setUserLabel(value);
        } else {
            setUserLabel(NULL);
        }
        #else 
        char* pType = strstr(line, "TYPE=");
        char* ptType = strstr(line, "PTTYPE=");
        if (pType) {

            if (ptType) {
                Log.d(TAG, "ptType - pType = %d", ptType - pType);

                if (abs(ptType - pType) == 2) {
                    bResult = false;
                }
            }

            pType += strlen("TYPE=") + 1;
            for (u32 i = 0; i < iLen; i++) {
                if (pType[i] != '"') {
                     volFsType[i] = pType[i];                   
                } else {
                    break;  /* 遇到第一个空格作为截至符 */
                }
            }
        }
        Log.d(TAG, "Parse File system type: %s", volFsType);
        #endif

    } else {
        Log.w(TAG, "blkid failed to identify %s", devicePath);
        bResult = false;
    }

    pclose(fp);

done:
    return bResult;
}

bool VolumeManager::clearMountPath(const char* mountPath)
{
    char cmd[128] = {0};
    
    if (access(mountPath, F_OK) != 0) {
        mkdir(mountPath, 0777);
    } else {
        sprintf(cmd, "rm -rf %s/*", mountPath);
        system(cmd);
    }
}




bool VolumeManager::isValidFs(const char* devName, Volume* pVol)
{
    char cDevNodePath[128] = {0};
    char cVolFsType[64] = {0};
    bool bResult = false;

    memset(pVol->cDevNode, 0, sizeof(pVol->cDevNode));
    memset(pVol->cVolFsType, 0, sizeof(pVol->cVolFsType));


    /* 1.检查是否存在/dev/devName该设备文件
     * blkid -c /dev/null /dev/devName - 获取其文件系统类型TYPE="xxxx"
     */

    sprintf(cDevNodePath, "/dev/%s", devName);
    Log.d(TAG, "[%s: %d] dev node path: %s", __FILE__, __LINE__, cDevNodePath);

    if (access(cDevNodePath, F_OK) == 0) {
        Log.d(TAG, "dev node path exist %s", cDevNodePath);

        if (extractMetadata(cDevNodePath, pVol->cVolFsType, sizeof(pVol->cVolFsType))) {
            strcpy(pVol->cDevNode, cDevNodePath);
            bResult = true;
        }
    } else {
        Log.e(TAG, "dev node[%s] not exist, what's wrong", cDevNodePath);
    }
    return bResult;
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
    Volume* tmpVol = NULL;

    switch (evt->getAction()) {
        case NETLINK_ACTION_ADD: {

            /* 1.检查，检查该插入的设备是否在系统的支持范围内 */
            if ((tmpVol = isSupportedDev(evt->getBusAddr()))) {
                /* 2.检查卷对应的槽是否已经被挂载，如果已经挂载说明上次卸载出了错误
                 * 需要先进行强制卸载操作否则会挂载不上
                 */
                if (isValidFs(evt->getDevNodeName(), tmpVol)) {
                    if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                        Log.e(TAG, "[%s: %d] Volume Maybe unmount failed, last time", __FILE__, __LINE__);
                        if (!unmountVolume(tmpVol, true)) { /* 强制卸载失败 */
                            Log.e(TAG, "[%s: %d] Force umount volume[%s] failed !!!", __FILE__, __LINE__, tmpVol->pMountPath);
                            return;
                        } else {
                            tmpVol->iVolState == VOLUME_STATE_NOMEDIA;
                        }
                    }

                    Log.d(TAG, "[%s: %d] dev[%s] mount point[%s]", __FILE__, __LINE__, tmpVol->cDevNode, tmpVol->pMountPath);

                    if (!mountVolume(tmpVol, evt)) {
                        Log.e(TAG, "mount device[%s] failed, reason [%d]", tmpVol->cDevNode, errno);
                    } else {
                        Log.d(TAG, "mount device[%s] on path [%s] success", tmpVol->cDevNode, tmpVol->pMountPath);
                        tmpVol->iVolState = VOLUME_STATE_MOUNTED;
                    }
                }
            }
            break;
        }

        case NETLINK_ACTION_REMOVE: {
            break;
        }


    }    

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
int VolumeManager::mountVolume(Volume* pVol, NetlinkEvent* pEvt)
{
    int iRet = 0;


    /* 如果使能了Check,在挂载之前对卷进行check操作 */
    char cTmpDevNode[COM_NAME_MAX_LEN] = {0};
    sprintf(cTmpDevNode, "fsck %s", pEvt->getDevNodeName());
    system(cTmpDevNode);

    /* 挂载点为非挂载状态，但是挂载点有其他文件，会先删除 */
    clearMountPath(pVol->pMountPath);

    #ifdef ENABLE_USE_SYSTEM_VOL_MOUNTUMOUNT
    char cMountCmd[512] = {0};
    sprintf(cMountCmd, "mount %s %s", pVol->cDevNode, pVol->pMountPath);
    for (int i = 0; i < 3; i++) {
        iRet = exec_cmd(cMountCmd);
        if (!iRet) {
            break;
        } else {
            Log.e(TAG, "[%s: %d] Mount [%s] failed, errno[%d]", __FILE__, __LINE__, pVol->pMountPath, errno);
        }
        msg_util::sleep_ms(200);
    }
    #endif

    return iRet;
}

/*
 * unmountVolume - 卸载/强制卸载卷
 */
int VolumeManager::unmountVolume(Volume* pVol, bool force)
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


