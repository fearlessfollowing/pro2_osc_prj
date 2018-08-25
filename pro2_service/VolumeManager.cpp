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


#include <sys/types.h>
#include <sys/wait.h>


#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>

#include <sys/Process.h>
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
    mListenerMode = VOLUME_MANAGER_LISTENER_MODE_NETLINK;
#else
    mListenerMode = VOLUME_MANAGER_LISTENER_MODE_INOTIFY;
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

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
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
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, mListenerMode);
    }
    return bResult;
}


bool VolumeManager::stop()
{
    bool bResult = false;

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
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
        Log.d(TAG, "[%s: %d] VolumeManager Not Support Listener Mode[%d]", __FILE__, __LINE__, mListenerMode);
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
            tmpVol = isSupportedDev(evt->getBusAddr());
            
            if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {
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

                    if (mountVolume(tmpVol, evt)) {
                        Log.e(TAG, "mount device[%s] failed, reason [%d]", tmpVol->cDevNode, errno);
                    } else {
                        Log.d(TAG, "mount device[%s] on path [%s] success", tmpVol->cDevNode, tmpVol->pMountPath);
                        tmpVol->iVolState = VOLUME_STATE_MOUNTED;
                    }
                }
            }
            break;
        }

        /* 移除卷 */
        case NETLINK_ACTION_REMOVE: {
            tmpVol = isSupportedDev(evt->getBusAddr());
            
            if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) { 
                 unmountVolume(tmpVol, true);    /* 卸载卷 */
            }
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
 * - 成功返回0; 失败返回-1
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
            iRet = -1;
        }
        msg_util::sleep_ms(200);
    }
    #endif

    return iRet;
}


int VolumeManager::doUnmount(const char *path, bool force)
{
    int retries = 10;

    if (mDebug) {
        Log.d(TAG, "Unmounting {%s}, force = %d", path, force);
    }

    while (retries--) {
        if (!umount(path) || errno == EINVAL || errno == ENOENT) {
            Log.i(TAG, "%s sucessfully unmounted", path);
            return 0;
        }

        int action = 0;
        if (force) {
            if (retries == 1) {
                action = 2;     // SIGKILL
            } else if (retries == 2) {
                action = 1;     // SIGHUP
            }
        }

        Log.e(TAG, "Failed to unmount %s (%s, retries %d, action %d)", path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);

        usleep(1000*30);
    }

    errno = EBUSY;
    Log.e(TAG, "Giving up on unmount %s (%s)", path, strerror(errno));
    return -1;
}



/*
 * unmountVolume - 卸载/强制卸载卷
 */
int VolumeManager::unmountVolume(Volume* pVol, bool force)
{
    int iRet = 0;

    if (pVol->iVolState != VOLUME_STATE_MOUNTED) {
        Log.e(TAG, "Volume [%s] unmount request when not mounted, state[0x%x]", pVol->pMountPath, pVol->iVolState);
        errno = EINVAL;
        return -2;
    }

    pVol->iVolState = VOLUME_STATE_UNMOUNTING;
    usleep(1000 * 1000);    // Give the framework some time to react

    if (doUnmount(pVol->pMountPath, force) != 0) {
        Log.e(TAG, "Failed to unmount %s (%s)", pVol->pMountPath, strerror(errno));
        goto out_mounted;
    }

    Log.i(TAG, "[%s: %d] %s unmounted successfully", __FILE__, __LINE__, pVol->pMountPath);

    pVol->iVolState = VOLUME_STATE_IDLE;
    return 0;

out_mounted:
    Log.e(TAG, "[%s: %d] Unmount Volume[%s] Failed", __FILE__, __LINE__, pVol->pMountPath);
    pVol->iVolState = VOLUME_STATE_MOUNTED;     /* 卸载失败 */
    return -1;
}


static int forkExecvp(int argc, char* argv[], int *status)
{
    int iRet = 0;
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        Log.e(TAG, "Failed to fork");
        iRet = -1;
        goto err_fork;
    } else if (pid == 0) {  /* 子进程: 使用execvp来替代执行的命令 */
        char* argv_child[argc + 1];
        memcpy(argv_child, argv, argc * sizeof(char *));
        argv_child[argc] = NULL;

        if (execvp(argv_child[0], argv_child)) {
            Log.e(TAG, "executing %s failed: %s\n", argv_child[0], strerror(errno));
        }
    } else {
        Log.d(TAG, ">>>>> Parent Process");

        iRet = waitpid(pid, status, WNOHANG);  /* 在此处等待子进程的退出 */
        if (iRet < 0) {
            iRet = errno;
            Log.e(TAG, "waitpid failed with %s\n", strerror(errno));
        }
    }

err_fork:
    return iRet;
}


int VolumeManager::checkFs(const char *fsPath) 
{
    bool rw = true;
    int pass = 1;
    int rc = 0;

    do {
        const char *args[4];
        int status;

        args[0] = "fsck";
        args[1] = "-p";
        args[2] = "-f";
        args[3] = fsPath;

        rc = forkExecvp(ARRAY_SIZE(args), (char **)args, &status);
        if (rc != 0) {
            Log.e(TAG, "Filesystem check failed due to logwrap error");
            errno = EIO;
            return -1;
        }

        if (!WIFEXITED(status)) {
            Log.e(TAG, "Filesystem check did not exit properly");
            errno = EIO;
            return -1;
        }

        status = WEXITSTATUS(status);

        switch(status) {
        case 0:
            Log.d(TAG, "Filesystem check completed OK");
            return 0;

        case 2:
            Log.d(TAG, "Filesystem check failed (not a FAT filesystem)");
            errno = ENODATA;
            return -1;

        default:
            Log.d(TAG, "Filesystem check failed (unknown exit code %d)", status);
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}


#if 0
int VolumeManager::formatFs2Ext4(const char *fsPath, unsigned int numSectors, const char *mountpoint) 
{
    int fd;
    const char *args[7];
    int rc;
    int status;

    args[0] = MKEXT4FS_PATH;
    args[1] = "-J";
    args[2] = "-a";
    args[3] = mountpoint;

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors * 512);
        const char *size = tmp;
        args[4] = "-l";
        args[5] = size;
        args[6] = fsPath;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    } else {
        args[4] = fsPath;
        rc = android_fork_execvp(5, (char **)args, &status, false, true);
    }
	
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    if (rc != 0) {
        SLOGE("Filesystem (ext4) format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem (ext4) formatted OK");
        return 0;
    } else {
        SLOGE("Format (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}


int VolumeManager::formatFs2Exfat(const char *fsPath, unsigned int numSectors, const char *mountpoint) 
{
    int fd;
    const char *args[7];
    int rc;
    int status;

    args[0] = MKEXT4FS_PATH;
    args[1] = "-J";
    args[2] = "-a";
    args[3] = mountpoint;

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors * 512);
        const char *size = tmp;
        args[4] = "-l";
        args[5] = size;
        args[6] = fsPath;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    } else {
        args[4] = fsPath;
        rc = android_fork_execvp(5, (char **)args, &status, false, true);
    }
	
    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false, true);
    if (rc != 0) {
        SLOGE("Filesystem (ext4) format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem (ext4) format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem (ext4) formatted OK");
        return 0;
    } else {
        SLOGE("Format (ext4) failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}

#endif

/*
 * 格式化类型: 默认为exFat
 */
int VolumeManager::formatVolume(Volume* pVol, bool wipe)
{
    int iRet = 0;

    if (pVol->iVolState == VOLUME_STATE_NOMEDIA) {
        errno = ENODEV;
        return -1;       
    } else if (pVol->iVolState != VOLUME_STATE_IDLE) {
        errno = EBUSY;
        return -1;
    }

    if (pVol->iVolState == VOLUME_STATE_MOUNTED) {
        errno = EBUSY;
        return -1;
    }

    pVol->iVolState == VOLUME_STATE_FORMATTING;

    if (mDebug) {
        Log.i(TAG, "Formatting volume %s (%s)", pVol->cDevNode, pVol->pMountPath);
    }

#if 0
    if (Fat::format(devicePath, 0, wipe, label)) {
        Log.e(TAG, "Failed to format (%s)", strerror(errno));
        goto err;
    }
#endif

    system("sync");
    iRet = 0;

err:
    pVol->iVolState == VOLUME_STATE_IDLE;
    return iRet;
}


void VolumeManager::updateRemoteTfsInfo(std::vector<sp<Volume>>& mList)
{

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


