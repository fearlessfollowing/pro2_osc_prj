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
#include <vector>

#include <sys/vfs.h>    /* or <sys/statfs.h> */


#include <sys/types.h>
#include <sys/wait.h>


#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>
#include <util/ARMessage.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>

#include <sys/inotify.h>

#include <sys/mount.h>


using namespace std;

#undef  TAG
#define TAG "Vold"

#define MAX_FILES 			1000
#define EPOLL_COUNT 		20
#define MAXCOUNT 			500
#define EPOLL_SIZE_HINT 	8

#define CtrlPipe_Shutdown 0
#define CtrlPipe_Wakeup   1

VolumeManager *VolumeManager::sInstance = NULL;

u32 VolumeManager::lefSpaceThreshold = 1024U;
int     mCtrlPipe[2];

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
            Log.e(TAG, "executing %s failed: %s", argv_child[0], strerror(errno));
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

/*
 * 卷管理器 - 僵死挂载点的清除(设备文件已经删除,但是挂载点还处于挂载状态,由于没有接收到内核的消息)
 * 设备名 -> 卷
 * ADD/REMORE - 目前只管REMOVE
 */





VolumeManager::VolumeManager() 
{
    mDebug = true;

	Volume* tmpVol = NULL;

#ifdef ENABLE_VOLUME_MANAGER_USE_NETLINK
    mListenerMode = VOLUME_MANAGER_LISTENER_MODE_NETLINK;
#else
    mListenerMode = VOLUME_MANAGER_LISTENER_MODE_INOTIFY;
#endif

    mVolumes.clear();
    mLocalVols.clear();
    mModuleVols.clear();

    /* 挂载点初始化 */
    /*
     * 重新初始化挂载点
     */
    
    mkdir("/mnt/mSD1", 0777);
    mkdir("/mnt/mSD2", 0777);
    mkdir("/mnt/mSD3", 0777);
    mkdir("/mnt/mSD4", 0777);
    mkdir("/mnt/mSD5", 0777);
    mkdir("/mnt/mSD6", 0777);
    mkdir("/mnt/sdcard", 0777);
    mkdir("/mnt/udisk1", 0777);
    mkdir("/mnt/udisk2", 0777);

    umount2("/mnt/mSD1", MNT_FORCE);
    umount2("/mnt/mSD2", MNT_FORCE);
    umount2("/mnt/mSD3", MNT_FORCE);
    umount2("/mnt/mSD4", MNT_FORCE);
    umount2("/mnt/mSD5", MNT_FORCE);
    umount2("/mnt/mSD6", MNT_FORCE);
    umount2("/mnt/sdcard", MNT_FORCE);
    umount2("/mnt/udisk1", MNT_FORCE);
    umount2("/mnt/udisk2", MNT_FORCE);

    Log.d(TAG, "[%s: %d] Umont All device now .....", __FILE__, __LINE__);

    /*
     * 初始化与模组交互的两个GPIO
     */
    system("echo 456 > /sys/class/gpio/export");
    system("echo 478 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio456/direction");
    system("echo out > /sys/class/gpio/gpio478/direction");

    mCurrentUsedLocalVol = NULL;
    mSavedLocalVol = NULL;
    mBsavePathChanged = false;

    /* 根据类型将各个卷加入到系统多个Vector中 */
    for (int i = 0; i < sizeof(gSysVols) / sizeof(gSysVols[0]); i++) {
        tmpVol = &gSysVols[i];  
        mVolumes.push_back(tmpVol);

        if (tmpVol->iType == VOLUME_TYPE_MODULE) {
            mModuleVols.push_back(tmpVol);
            mModuleVolNum++;
        } else {
            mLocalVols.push_back(tmpVol);
        }
    }

    Log.d(TAG, "[%s: %d] Module Volume size = %d", __FILE__, __LINE__, mModuleVolNum);
}


void handleDevRemove(const char* pDevNode)
{
    u32 i;
    Volume* tmpVol = NULL;
    string devNodePath = "/dev/";
    devNodePath += pDevNode;

    VolumeManager *vm = VolumeManager::Instance();
    vector<Volume*>& tmpVector = vm->getRemoteVols();

    Log.d(TAG, "[%s: %d] handleDevRemove -> [%s]", __FILE__, __LINE__, pDevNode);
    Log.d(TAG, "[%s: %d] Remote vols size = %d", __FILE__, __LINE__, tmpVector.size());

    /* 处理TF卡的移除 */
    for (i = 0; i < tmpVector.size(); i++) {
        tmpVol = tmpVector.at(i);
    
        Log.d(TAG, "[%s: %d] Volue[%d] -> %s", __FILE__, __LINE__, i, tmpVol->cDevNode);
    
        if (tmpVol && !strcmp(tmpVol->cDevNode, devNodePath.c_str())) {
            NetlinkEvent *evt = new NetlinkEvent();

            evt->setAction(NETLINK_ACTION_REMOVE);
            evt->setSubsys(VOLUME_SUBSYS_USB);
            evt->setBusAddr(tmpVol->pBusAddr);
            evt->setDevNodeName(pDevNode);
            
            vm->handleBlockEvent(evt);
            
            delete evt;
        }
    }
}


void runListener()
{
    int iFd;
    int iRes;
	u32 readCount = 0;
    char inotifyBuf[MAXCOUNT];
    char epollBuf[MAXCOUNT];

	struct inotify_event inotifyEvent;
	struct inotify_event* curInotifyEvent;

    iFd = inotify_init();
    if (iFd < 0) {
        Log.e(TAG, "[%s: %d] inotify init failed...", __FILE__, __LINE__);
        return;
    }

    iRes = inotify_add_watch(iFd, "/dev", IN_CREATE | IN_DELETE);
    if (iRes < 0) {
        Log.e(TAG, "[%s: %d] inotify_add_watch /dev failed", __FILE__, __LINE__);
        return;
    }    

    while (true) {
        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_ZERO(&read_fds);

        FD_SET(mCtrlPipe[0], &read_fds);	
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];

        FD_SET(iFd, &read_fds);	
        if (iFd > max)
            max = iFd;

        if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {	
            if (errno == EINTR)
                continue;
            sleep(1);
            continue;
        } else if (!rc)
            continue;

        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                Log.d(TAG, "[%s: %d] VolumeManager notify our exit now ...", __FILE__, __LINE__);
                break;
            }
            continue;
        }

        if (FD_ISSET(iFd, &read_fds)) {	
            /* 读取inotify事件，查看是add 文件还是remove文件，add 需要将其添加到epoll中去，remove 需要从epoll中移除 */
            readCount  = 0;
            readCount = read(iFd, inotifyBuf, MAXCOUNT);
            if (readCount <  sizeof(inotifyEvent)) {
                Log.e(TAG, "error inofity event");
                continue;
            }

            curInotifyEvent = (struct inotify_event*)inotifyBuf;

            while (readCount >= sizeof(inotifyEvent)) {
                if (curInotifyEvent->len > 0) {

                    string devNode = "/dev/";
                    devNode += curInotifyEvent->name;

                    if (curInotifyEvent->mask & IN_CREATE) {
                        /* 有新设备插入,根据设备文件执行挂载操作 */
                        // handleMonitorAction(ACTION_ADD, devPath);
                        Log.d(TAG, "[%s: %d] [%s] Insert", __FILE__, __LINE__, devNode.c_str());
                    } else if (curInotifyEvent->mask & IN_DELETE) {
                        /* 有设备拔出,执行卸载操作 
                         * 由设备名找到对应的卷(地址，子系统，挂载路径，设备命) - 构造出一个NetlinkEvent事件
                         */
                        Log.d(TAG, "[%s: %d] [%s] Remove", __FILE__, __LINE__, devNode.c_str());
                        handleDevRemove(curInotifyEvent->name);
                    }
                }
                curInotifyEvent--;
                readCount -= sizeof(inotifyEvent);
            }
        }
    }
}

void* threadStart(void *obj) 
{
    runListener();		
    pthread_exit(NULL);
    return NULL;
}


void VolumeManager::checkAllUdiskIdle()
{
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol) {
            if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                Log.d(TAG, "[%s: %d] Current Volume(%s) is Mouted State, force unmount now....", __FILE__, __LINE__, tmpVol->pMountPath);
                if (doUnmount(tmpVol->pMountPath, true)) {
                    Log.e(TAG, "[%s: %d] Force Unmount Volume Failed!!!", __FILE__, __LINE__);
                }
                tmpVol->iVolState = VOLUME_STATE_IDLE;
            }
        }
    }
}

/*
 * 确保所有的U-Disk都挂载上了
 */
int VolumeManager::checkAllUdiskMounted()
{
    int iMountedCnt = 0;
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol) {
            if (tmpVol->iVolState == VOLUME_STATE_MOUNTED) {
                iMountedCnt++;
            }
        }
    }
    return iMountedCnt;
}

void VolumeManager::enterUdiskMode()
{
    /* 1.检查所有卷的状态，如果非IDLE状态（MOUNTED状态），先进行强制卸载操作
     * 1.将gpio456, gpio478设置为1，0
     * 2.调用power_manager power_on给所有的模组上电
     * 3.等待所有的模组挂上
     * 4.检查是否所有的模组都挂载成功
     */
    Log.d(TAG, "[%s: %d] Enter U-disk Mode now ...", __FILE__, __LINE__);
    
    mHandledAddUdiskVolCnt = 0;
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_UDISK);
    
    checkAllUdiskIdle();

    system("echo 1 > /sys/class/gpio/gpio456/value");   /* gpio456 = 1 */
    system("power_manager power_on");

    /* TODO: 等待所有的U盘都挂载上 */
}

bool VolumeManager::checkEnteredUdiskMode()
{
    if (mHandledAddUdiskVolCnt == mModuleVolNum) {
        return true;
    } else {
        return false;
    }
}


void VolumeManager::setVolumeManagerWorkMode(int iWorkMode)
{
    mVolumeManagerWorkMode = iWorkMode;
}

int VolumeManager::getVolumeManagerWorkMode()
{
    return mVolumeManagerWorkMode;
}

int VolumeManager::getCurHandleAddUdiskVolCnt()
{
    return mHandledAddUdiskVolCnt;
}

int VolumeManager::getCurHandleRemoveUdiskVolCnt()
{
    return mHandledRemoveUdiskVolCnt;
}


void VolumeManager::exitUdiskMode()
{
    /* 1.卸载掉所有的U盘
     * 2.给模组断电
     */
    u32 i;
    int iResult = -1;
    Volume* tmpVol = NULL;

    Log.d(TAG, "[%s: %d] Exit U-disk Mode now ...", __FILE__, __LINE__);
    
    mHandledRemoveUdiskVolCnt = 0;

    /* 处理TF卡的移除 */
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (i = 0; i < mModuleVols.size(); i++) {

            tmpVol = mModuleVols.at(i);
        
            Log.d(TAG, "[%s: %d] Volue[%d] -> %s", __FILE__, __LINE__, i, tmpVol->cDevNode);
            if (tmpVol) {

                NetlinkEvent *evt = new NetlinkEvent();        
                evt->setEventSrc(NETLINK_EVENT_SRC_APP);
                evt->setAction(NETLINK_ACTION_REMOVE);
                evt->setSubsys(VOLUME_SUBSYS_USB);
                evt->setBusAddr(tmpVol->pBusAddr);
                evt->setDevNodeName(tmpVol->cDevNode);            
                iResult = handleBlockEvent(evt);
                if (iResult) {
                    Log.d(TAG, "[%s: %d] Remove Device Failed ...", __FILE__, __LINE__);
                }       
                delete evt;
            }
        }
    }

    msg_util::sleep_ms(1000);

    system("power_manager power_off");    
    system("echo 0 > /sys/class/gpio/gpio456/value");   /* gpio456 = 0 */
    
    setVolumeManagerWorkMode(VOLUME_MANAGER_WORKMODE_NORMAL);
}

/*
 * 卷管理器新增功能: 2018年8月31日
 * 1.监听/mnt下的文件变化   - (何时创建/删除文件，将其记录在日志中)
 * 2.监听磁盘的容量变化     - (本地的正在使用的以及远端的小卡)
 * 
 */


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

            #ifdef ENABLE_REMOVE_LISTEN_THREAD
                if (pipe(mCtrlPipe)) {		
                    Log.e(TAG, "pipe failed (%s)", strerror(errno));
                } else {
                    if (pthread_create(&mThread, NULL, threadStart, NULL)) {	
                        Log.e(TAG, "pthread_create (%s)", strerror(errno));
                    } else {
                        Log.d(TAG, "[%s: %d] Create Dev notify Thread....", __FILE__, __LINE__);
                    }                      
                }
            #endif

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

#ifdef ENABLE_REMOVE_LISTEN_THREAD
    char c = CtrlPipe_Shutdown;		
    int  rc;	

    rc = TEMP_FAILURE_RETRY(write(mCtrlPipe[1], &c, 1));
    if (rc != 1) {
        Log.e(TAG, "Error writing to control pipe (%s)", strerror(errno));
    }

    void *ret;
    if (pthread_join(mThread, &ret)) {	
        Log.e(TAG, "Error joining to listener thread (%s)", strerror(errno));
    }
	
    close(mCtrlPipe[0]);	
    close(mCtrlPipe[1]);
    mCtrlPipe[0] = -1;
    mCtrlPipe[1] = -1;

#endif

    if (mListenerMode == VOLUME_MANAGER_LISTENER_MODE_NETLINK) {
        NetlinkManager* nm = NULL;
        if (!(nm = NetlinkManager::Instance())) {	
            Log.e(TAG, "[%s: %d] Unable to create NetlinkManager", __FILE__, __LINE__);
        } else {
            /* 停止监听线程 */
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


/*
 * 获取系统中的当前存储设备列表
 */
vector<Volume*>& VolumeManager::getSysStorageDevList()
{
    vector<Volume*>& localVols = getCurSavepathList();
    vector<Volume*>& remoteVols = getRemoteVols();
    Volume* tmpVol = NULL;

    Log.d(TAG, "[%s: %d] >>>>>> getSysStorageDevList", __FILE__, __LINE__);

    mSysStorageVolList.clear();

    for (u32 i = 0; i < localVols.size(); i++) {
        tmpVol = localVols.at(i);
        if (tmpVol) {
            mSysStorageVolList.push_back(tmpVol);
        }
    }

    for (u32 i = 0; i < remoteVols.size(); i++) {
        tmpVol = remoteVols.at(i);
        if (tmpVol && tmpVol->uTotal > 0) {
            mSysStorageVolList.push_back(tmpVol);
        }
    }

    Log.d(TAG, "[%s: %d] Current System Storage list size = %d", __FILE__, __LINE__, mSysStorageVolList.size());
    return mSysStorageVolList;
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



/*************************************************************************
** 方法名称: clearMountPath
** 方法功能: 清除挂载点
** 入口参数: 
**      mountPath - 挂载点路径
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 在清除挂载点前，需要判断该挂载点已经被挂载，如果已经被挂载先对其卸载
** 如果被卸载成功或未被挂载,检查该挂载点是否干净,如果不干净,对其进行清除操作
*************************************************************************/
bool VolumeManager::clearMountPath(const char* mountPath)
{
    char cmd[128] = {0};
    
    if (access(mountPath, F_OK) != 0) {     /* 挂载点不存在,创建挂载点 */
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



/*************************************************************************
** 方法名称: handleBlockEvent
** 方法功能: 处理来自底层的卷块设备事件
** 入口参数: 
**      evt - NetlinkEvent对象
** 返回值: 无
** 调 用: 
** 处理来自底层的事件: 1.Netlink; 2.inotify(/dev)
*************************************************************************/
int VolumeManager::handleBlockEvent(NetlinkEvent *evt)
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
    Log.d(TAG, ">>>>>>>>>>>>>>>>>> handleBlockEvent(action: %d) <<<<<<<<<<<<<<<", evt->getAction());
    Volume* tmpVol = NULL;
    int iResult = 0;

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
                        unmountVolume(tmpVol, evt, true);
                        tmpVol->iVolState == VOLUME_STATE_INIT;
                    }

                    Log.d(TAG, "[%s: %d] dev[%s] mount point[%s]", __FILE__, __LINE__, tmpVol->cDevNode, tmpVol->pMountPath);
                    if ( (getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                        mHandledAddUdiskVolCnt++;  /* 不能确保所有的卷都能挂载(比如说卷已经损坏) */
                    }

                    if (mountVolume(tmpVol, evt)) {
                        Log.e(TAG, "mount device[%s -> %s] failed, reason [%d]", tmpVol->cDevNode, tmpVol->pMountPath, errno);
                        iResult = -1;
                    } else {
                        Log.d(TAG, "mount device[%s] on path [%s] success", tmpVol->cDevNode, tmpVol->pMountPath);

                        tmpVol->iVolState = VOLUME_STATE_MOUNTED;

                        /* 如果是TF卡,不需要做如下操作 */
                        if (volumeIsTfCard(tmpVol) == false) {
                            setVolCurPrio(tmpVol, evt);
                            setSavepathChanged(VOLUME_ACTION_ADD, tmpVol);
                            sendDevChangeMsg2UI(VOLUME_ACTION_ADD, tmpVol->iVolSubsys, getCurSavepathList());
                        }
                    }
                }
            } else {
                Log.d(TAG, "[%s: %d] Not Support Device Addr[%s] or Slot Not Enable[%d]", __FILE__, __LINE__, evt->getBusAddr(), tmpVol->iVolSlotSwitch);
            }
            break;
        }

        /* 移除卷 */
        case NETLINK_ACTION_REMOVE: {
            tmpVol = isSupportedDev(evt->getBusAddr());            
            if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE)) {  /* 该卷被使能 */ 

                if ( (getVolumeManagerWorkMode() == VOLUME_MANAGER_WORKMODE_UDISK) && volumeIsTfCard(tmpVol)) {
                    mHandledRemoveUdiskVolCnt++;  /* 不能确保所有的卷都能挂载(比如说卷已经损坏) */
                }

                iResult = unmountVolume(tmpVol, evt, true);
                if (!iResult) {    /* 卸载卷成功 */

                    tmpVol->iVolState = VOLUME_STATE_INIT;
                    if (volumeIsTfCard(tmpVol) == false) {
                        setVolCurPrio(tmpVol, evt); /* 重新修改该卷的优先级 */
                        setSavepathChanged(VOLUME_ACTION_REMOVE, tmpVol);   /* 检查是否修改当前的存储路径 */
                        /* 发送存储设备移除,及当前存储设备路径的消息 */
                        sendDevChangeMsg2UI(VOLUME_ACTION_REMOVE, tmpVol->iVolSubsys, getCurSavepathList());
                    }
                } else {
                    Log.d(TAG, "[%s: %d] Unmount Failed!!", __FILE__, __LINE__);
                    iResult = -1;
                }
            } else {
                Log.e(TAG, "[%s: %d] unmount volume Failed, Reason = %d", __FILE__, __LINE__, iResult);
            }
            break;
        }
    }  
    return iResult;  
}



/*************************************************************************
** 方法名称: getCurSavepathList
** 方法功能: 返回当前的本地存储设备列表
** 入口参数: 
** 返回值: 本地存储设备列表
** 调 用: 
** 根据卷的传递的地址来改变卷的优先级
*************************************************************************/
vector<Volume*>& VolumeManager::getCurSavepathList()
{
    Volume* tmpVol = NULL;
    struct statfs diskInfo;
    u64 totalsize = 0;
    u64 used_size = 0;

    mCurSaveVolList.clear();

    for (u32 i = 0; i < mLocalVols.size(); i++) {
        tmpVol = mLocalVols.at(i);
        if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolState == VOLUME_STATE_MOUNTED) ) { 

            statfs(tmpVol->pMountPath, &diskInfo);

            u64 blocksize = diskInfo.f_bsize;                                   //每个block里包含的字节数
            totalsize = blocksize * diskInfo.f_blocks;                          // 总的字节数，f_blocks为block的数目
            used_size = (diskInfo.f_blocks - diskInfo.f_bfree) * blocksize;     // 可用空间大小
            used_size = used_size >> 20;

            memset(tmpVol->cVolName, 0, sizeof(tmpVol->cVolName));
            sprintf(tmpVol->cVolName, "%s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "sd": "usb");
            tmpVol->uTotal = totalsize >> 20;                 /* 统一将单位转换MB */
            tmpVol->uAvail = tmpVol->uTotal - used_size;
            tmpVol->iType = VOLUME_TYPE_NV;
            
            mCurSaveVolList.push_back(tmpVol);
        }
    }
    return mCurSaveVolList;
}


bool VolumeManager::volumeIsTfCard(Volume* pVol) 
{
    if (!strcmp(pVol->pMountPath, "/mnt/mSD1") 
        || !strcmp(pVol->pMountPath, "/mnt/mSD2")
        || !strcmp(pVol->pMountPath, "/mnt/mSD3")
        || !strcmp(pVol->pMountPath, "/mnt/mSD4")
        || !strcmp(pVol->pMountPath, "/mnt/mSD5")
        || !strcmp(pVol->pMountPath, "/mnt/mSD6")) {
        return true;
    } else {
        return false;
    }

}


/*************************************************************************
** 方法名称: setVolCurPrio
** 方法功能: 重新设置指定卷的优先级
** 入口参数: 
**      pVol - 卷对象
**      pEvt - Netlink事件对象
** 返回值: 无 
** 调 用: 
** 根据卷的传递的地址来改变卷的优先级
*************************************************************************/
void VolumeManager::setVolCurPrio(Volume* pVol, NetlinkEvent* pEvt)
{
    /* 根据卷的地址重置卷的优先级 */
    if (!strncmp(pEvt->getBusAddr(), "2-2", strlen("2-2"))) {
        pVol->iPrio = VOLUME_PRIO_SD;
    } else if (!strncmp(pEvt->getBusAddr(), "2-1", strlen("2-1"))) {
        pVol->iPrio = VOLUME_PRIO_UDISK;
    } else if (!strncmp(pEvt->getBusAddr(), "1-2.1", strlen("1-2.1"))) {
        pVol->iPrio = VOLUME_PRIO_LOW;
    } else if (!strncmp(pEvt->getBusAddr(), "2-3", strlen("2-3"))) {
        pVol->iPrio = VOLUME_PRIO_UDISK;
    } else {
        pVol->iPrio = VOLUME_PRIO_LOW;
    }

}

void VolumeManager::syncLocalDisk()
{
    string cmd = "sync -f ";
    if (mCurrentUsedLocalVol != NULL) {
        cmd += mCurrentUsedLocalVol->pMountPath;
        system(cmd.c_str());
    }
}




/*************************************************************************
** 方法名称: setSavepathChanged
** 方法功能: 检查是否修改当前的存储路径
** 入口参数: 
**      iAction - 事件类型(Add/Remove)
**      pVol - 触发事件的卷
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::setSavepathChanged(int iAction, Volume* pVol)
{
    Volume* tmpVol = NULL;

    switch (iAction) {
        case VOLUME_ACTION_ADD: {   
            if (mCurrentUsedLocalVol == NULL) {
                mCurrentUsedLocalVol = pVol;
                mBsavePathChanged = true;       /* 表示存储设备路径发生了改变 */
                
                Log.d(TAG, "[%s: %d] Fist Local Volume Insert, Current Save path [ %s]", 
                                            __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath);

            } else {    /* 本来已有本地存储设备，根据存储设备的优先级来判断否需要改变存储路径 */

                /* 检查是否有更高速的设备插入，如果有 */
                for (u32 i = 0; i < mLocalVols.size(); i++) {
                    tmpVol = mLocalVols.at(i);
                    if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol->iVolSlotSwitch == VOLUME_STATE_MOUNTED)) {

                        /* 挑选优先级更高的设备作为当前的存储设备 */
                        if (tmpVol->iPrio > mCurrentUsedLocalVol->iPrio) {
                
                            Log.d(TAG, "[%s: %d] New high prio Volume insert, Changed current save path [%s -> %s]", 
                                                        __FILE__, __LINE__, mCurrentUsedLocalVol->pMountPath, tmpVol->pMountPath);
                            mCurrentUsedLocalVol = tmpVol;
                            mBsavePathChanged = true;
                        }
                    }
                }
            }
            break;
        }

        case VOLUME_ACTION_REMOVE: {
            
            if (pVol == mCurrentUsedLocalVol) { /* 移除的是当前存储路径,需要从剩余的存储设备列表中选择优先级最高的存储设备 */
                Volume* oldVol = NULL;
                if (mCurrentUsedLocalVol) {
                    oldVol = mCurrentUsedLocalVol;
                    mCurrentUsedLocalVol->iPrio = VOLUME_PRIO_LOW;

                    for (u32 i = 0; i < mLocalVols.size(); i++) {
                        tmpVol = mLocalVols.at(i);
                        if (tmpVol && (tmpVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (tmpVol != mCurrentUsedLocalVol)) {
                            
                            /* 挑选优先级更高的设备作为当前的存储设备 */
                            if (tmpVol->iPrio > mCurrentUsedLocalVol->iPrio) {
                                mCurrentUsedLocalVol = tmpVol;
                                mBsavePathChanged = true;
                            }
                        }
                    } 

                    if (mCurrentUsedLocalVol == oldVol) {   /* 只有一个本地卷被挂载 */
                        mCurrentUsedLocalVol = NULL;
                        mBsavePathChanged = true;
                    }

                } else {
                    Log.e(TAG, "[%s: %d] Remove Volume Not exist ?????", __FILE__, __LINE__);
                }
            } else {    /* 移除的不是当前存储路径，不需要任何操作 */
                Log.d(TAG, "[%s: %d] Remove Volume[%s] not Current save path, Do nothing", __FILE__, __LINE__);
            }
            break;
        }
    }

}



/*************************************************************************
** 方法名称: listVolumes
** 方法功能: 列出系统中所有的卷
** 入口参数: 
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::listVolumes()
{
    Volume* tmpVol = NULL;

    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol) {
            Log.d(TAG, "Volume type: %s", (tmpVol->iVolSubsys == VOLUME_SUBSYS_SD) ? "VOLUME_SUBSYS_SD": "VOLUME_SUBSYS_USB" );
            Log.d(TAG, "Volume bus: %s", tmpVol->pBusAddr);
            Log.d(TAG, "Volume mountpointer: %s", tmpVol->pMountPath);
            Log.d(TAG, "Volume devnode: %s", tmpVol->cDevNode);

            Log.d(TAG, "Volume Type %d", tmpVol->iType);
            Log.d(TAG, "Volume index: %d", tmpVol->iIndex);
            Log.d(TAG, "Volume state: %d", tmpVol->iVolState);

            Log.d(TAG, "Volume total %d MB", tmpVol->uTotal);
            Log.d(TAG, "Volume avail: %d MB", tmpVol->uAvail);
            Log.d(TAG, "Volume speed: %d MB", tmpVol->iSpeedTest);
        }
    }
}


void VolumeManager::setNotifyRecv(sp<ARMessage> notify)
{
    mNotify = notify;
}



/*************************************************************************
** 方法名称: sendDevChangeMsg2UI
** 方法功能: 发送存储设备变化的通知给UI
** 入口参数: 
**      iAction - ADD/REMOVE
**      iType - 存储设备类型(SD/USB)
**      devList - 发生变化的存储设备列表
** 返回值: 无 
** 调 用: 
** 
*************************************************************************/
void VolumeManager::sendDevChangeMsg2UI(int iAction, int iType, vector<Volume*>& devList)
{
    sp<ARMessage> msg = mNotify->dup();

    msg->set<int>("action", iAction);    
    msg->set<int>("type", iType);    
    msg->set<vector<Volume*>>("dev_list", devList);
    msg->post();    
}



/*************************************************************************
** 方法名称: checkLocalVolumeExist
** 方法功能: 检查本地的当前存储设备是否存在
** 入口参数: 
** 返回值: 存在返回true;否则返回false 
** 调 用: 
** 
*************************************************************************/
bool VolumeManager::checkLocalVolumeExist()
{
    if (mCurrentUsedLocalVol) {
        return true;
    } else {
        return false;
    }
}


/*************************************************************************
** 方法名称: getLocalVolMountPath
** 方法功能: 获取当前的存储路径
** 入口参数: 
** 返回值: 当前存储路径 
** 调 用: 
** 
*************************************************************************/
const char* VolumeManager::getLocalVolMountPath()
{
    if (mCurrentUsedLocalVol) {
        return mCurrentUsedLocalVol->pMountPath;
    } else {
        return "none";
    }
}



/*************************************************************************
** 方法名称: getLocalVolLeftSize
** 方法功能: 计算当前存储设备的剩余容量
** 入口参数: 
**     bUseCached - 是否使用缓存的剩余空间 
** 返回值: 剩余空间
** 调 用: 
** 
*************************************************************************/
u64 VolumeManager::getLocalVolLeftSize(bool bUseCached)
{
    if (mCurrentUsedLocalVol) {
        if (bUseCached == false) {
            updateVolumeSpace(mCurrentUsedLocalVol);
        } 
        return mCurrentUsedLocalVol->uAvail;
    } else {
        return 0;
    }    
}



/*************************************************************************
** 方法名称: checkAllTfCardExist
** 方法功能: 检查是否所有的TF卡存在
** 入口参数: 
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 
*************************************************************************/
bool VolumeManager::checkAllTfCardExist()
{
    Volume* tmpVolume = NULL;
    int iExitNum = 0;
    
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            tmpVolume = mModuleVols.at(i);
            if (tmpVolume->uTotal > 0) {     /* 总容量大于0,表示卡存在 */
                iExitNum++;
            }
        }
    }

    if (iExitNum >= mModuleVolNum) {
        return true;
    } else {
        return false;
    }
}


void VolumeManager::calcRemoteRemainSpace(bool bFactoryMode)
{
    u64 iTmpMinSize = ~0UL;
    
    if (bFactoryMode) {
        mReoteRecLiveLeftSize = 1024 * 256;    /* 单位为MB， 256GB */
    } else {
        {
            unique_lock<mutex> lock(mRemoteDevLock);
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                if (iTmpMinSize > mModuleVols.at(i)->uAvail) {
                    iTmpMinSize = mModuleVols.at(i)->uAvail;
                }
            }
        }
        mReoteRecLiveLeftSize = iTmpMinSize;

    }
    Log.d(TAG, "[%s: %d] remote left space [%d]M", __FILE__, __LINE__, mReoteRecLiveLeftSize);

}


u64 VolumeManager::getRemoteVolLeftMinSize()
{
    return mReoteRecLiveLeftSize; 
}



/*************************************************************************
** 方法名称: updateLocalVolSpeedTestResult
** 方法功能: 更新本地卷的测速结果
** 入口参数: 
**      iResult - 
** 返回值: 所有TF卡存在返回true;否则返回false
** 调 用: 
** 
*************************************************************************/
void VolumeManager::updateLocalVolSpeedTestResult(int iResult)
{
    if (mCurrentUsedLocalVol) { /* 在根目录的底层目录创建'.pro_suc' */
        Log.d(TAG, "[%s: %d] >>>> Create Speet Test Success File [.pro_suc]", __FILE__, __LINE__);
        mCurrentUsedLocalVol->iSpeedTest = iResult;
        if (iResult) {
            Log.d(TAG, "[%s: %d] Speed test suc, create pro_suc Now...", __FILE__, __LINE__);
            string cmd = "touch ";
            cmd += mCurrentUsedLocalVol->pMountPath;
            cmd += "/.pro_suc";
            system(cmd.c_str());
        }
    }
}

void VolumeManager::updateRemoteVolSpeedTestResult(Volume* pVol)
{
    Volume* tmpVol = NULL;

    unique_lock<mutex> lock(mRemoteDevLock); 
    for (u32 i = 0; i < mModuleVols.size(); i++) {
        tmpVol = mModuleVols.at(i);
        if (tmpVol && pVol) {
            if (tmpVol->iIndex == pVol->iIndex) {
                tmpVol->iSpeedTest = pVol->iSpeedTest;
            }
        }  
    }
}

bool VolumeManager::checkAllmSdSpeedOK()
{
    Volume* tmpVolume = NULL;
    int iExitNum = 0;
    {
        unique_lock<mutex> lock(mRemoteDevLock);
        for (u32 i = 0; i < mModuleVols.size(); i++) {
            tmpVolume = mModuleVols.at(i);
            if ((tmpVolume->uTotal > 0) && tmpVolume->iSpeedTest) {     /* 总容量大于0,表示卡存在 */
                iExitNum++;
            }
        }
    }

    if (iExitNum >= mModuleVolNum) {
        return true;
    } else {
        return false;
    }    
}


bool VolumeManager::checkLocalVolSpeedOK()
{
    if (mCurrentUsedLocalVol) {
        return (mCurrentUsedLocalVol->iSpeedTest == 1) ? true : false;
    } else {
        return false;
    }
}

/*
 * checkSavepathChanged - 本地存储路径是否发生改变
 */
bool VolumeManager::checkSavepathChanged()
{
    return mBsavePathChanged;
}


int VolumeManager::handleRemoteVolHotplug(vector<sp<Volume>>& volChangeList)
{
    Volume* tmpSourceVolume = NULL;
    int iAction = VOLUME_ACTION_UNSUPPORT;

    if (volChangeList.size() > 1) {
        Log.e(TAG, "[%s: %d] Hotplug Remote volume num than 1", __FILE__, __LINE__);
    } else {
        sp<Volume> tmpChangedVolume = volChangeList.at(0);

        {
            unique_lock<mutex> lock(mRemoteDevLock); 
            for (u32 i = 0; i < mModuleVols.size(); i++) {
                tmpSourceVolume = mModuleVols.at(i);
                if (tmpChangedVolume && tmpSourceVolume) {
                    if (tmpChangedVolume->iIndex == tmpSourceVolume->iIndex) {
                        tmpSourceVolume->uTotal     = tmpChangedVolume->uTotal;
                        tmpSourceVolume->uAvail     = tmpChangedVolume->uAvail;
                        tmpSourceVolume->iSpeedTest = tmpChangedVolume->iSpeedTest;
                        if (tmpSourceVolume->uTotal > 0) {
                            Log.d(TAG, "[%s: %d] TF Card Add action", __FILE__, __LINE__);
                            iAction = VOLUME_ACTION_ADD;
                        } else {
                            iAction = VOLUME_ACTION_REMOVE;
                            Log.d(TAG, "[%s: %d] TF Card Remove action", __FILE__, __LINE__);
                        }                        
                        break;
                    }
                }
            }  
        }
    }
    Log.d(TAG, "[%s: %d] handleRemoteVolHotplug return action: %d", __FILE__, __LINE__, iAction);
    return iAction;
}


/*
 * 挂载卷
 * - 成功返回0; 失败返回-1
 */
int VolumeManager::mountVolume(Volume* pVol, NetlinkEvent* pEvt)
{
    int iRet = 0;
    unsigned long flags;

    flags = MS_DIRSYNC | MS_NOATIME;

    /* 如果使能了Check,在挂载之前对卷进行check操作 */
    string fsckStr = "fsck /dev/";
    fsckStr += pEvt->getDevNodeName();
    system(fsckStr.c_str());


    /* 挂载点为非挂载状态，但是挂载点有其他文件，会先删除 */
    // clearMountPath(pVol->pMountPath);

    Log.d(TAG, "[%s: %d] >>>>> Filesystem type: %s", __FILE__, __LINE__, pVol->cVolFsType);

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
    #else

    #if 0
    msg_util::sleep_ms(2000);

    Log.e(TAG, "[%s: %d] Mount [%s -> %s, flags 0x%x]", 
                __FILE__, __LINE__, pVol->cDevNode, pVol->pMountPath, flags);

    if (access(pVol->cDevNode, F_OK) != 0) {
        Log.e(TAG, "[%s: %d] Dev node [%s] not exist", __FILE__, __LINE__, pVol->cDevNode);
    }

    if (!strncmp(pVol->cVolFsType, "exfat", strlen("exfat"))) {  /* EXFAT挂载 */
        return mount(pVol->cDevNode, pVol->pMountPath, "fuseblk", 0, NULL);
    } else if (!strncmp(pVol->cVolFsType, "ext4", strlen("ext4"))) {

    } else if (!strncmp(pVol->cVolFsType, "ext3", strlen("ext3"))) {

    } else if (!strncmp(pVol->cVolFsType, "ext2", strlen("ext2"))) {

    } else if (!strncmp(pVol->cVolFsType, "vfat", strlen("vfat"))) {

    } else if (!strncmp(pVol->cVolFsType, "ntfs", strlen("ntfs"))) {

    } else {
        Log.e(TAG, "[%s: %s] Not support Filesystem type[%s]", __FILE__, __LINE__, pVol->cVolFsType);
        iRet = -1;
    }

    #else

    int status;
    const char *args[3];
    args[0] = "/bin/mount";
    args[1] = pVol->cDevNode;
    args[2] = pVol->pMountPath;

    iRet = forkExecvp(ARRAY_SIZE(args), (char **)args, &status);
    Log.d(TAG, "[%s: %d] forkExecvp return val 0x%x", __FILE__, __LINE__, iRet);
    #endif


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
int VolumeManager::unmountVolume(Volume* pVol, NetlinkEvent* pEvt, bool force)
{
    int iRet = 0;

    if (pEvt->getEventSrc() == NETLINK_EVENT_SRC_KERNEL) {
        string devnode = "/dev/";
        devnode += pEvt->getDevNodeName();
        Log.d(TAG, "[%s: %d] umount volume devname[%s], event devname[%s]", __FILE__, __LINE__, pVol->cDevNode, devnode.c_str());

        if (strcmp(pVol->cDevNode, devnode.c_str())) {   /* 设备名不一致,直接返回 */
            return -1;
        }
    }

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


/*
 * 更新指定卷的存储容量信息
 */
void VolumeManager::updateVolumeSpace(Volume* pVol) 
{
    struct statfs diskInfo;
    u64 totalsize = 0;
    u64 used_size = 0;

    /* 卡槽使能并且卷已经被挂载 */
    if ((pVol->iVolSlotSwitch == VOLUME_SLOT_SWITCH_ENABLE) && (pVol->iVolState == VOLUME_STATE_MOUNTED)) {
        if (!statfs(pVol->pMountPath, &diskInfo)) {
            u64 blocksize = diskInfo.f_bsize;                                   //每个block里包含的字节数
            totalsize = blocksize * diskInfo.f_blocks;                          // 总的字节数，f_blocks为block的数目
            pVol->uTotal = (totalsize >> 20);
            pVol->uAvail = ((diskInfo.f_bfree * blocksize) >> 20);
            Log.d(TAG, "[%s: %d] Local Volume Tatol size = %ld MB", __FILE__, __LINE__, pVol->uTotal);
            Log.d(TAG, "[%s: %d] Local Volume Avail size = %ld MB", __FILE__, __LINE__, pVol->uAvail);

        } else {
            Log.d(TAG, "[%s: %d] statfs failed ...", __FILE__, __LINE__);
        }
    } else {
        Log.d(TAG, "[%s: %d] Current Local Vol May Locked or Not Mounted!", __FILE__, __LINE__);
    }
}



void VolumeManager::syncTakePicLeftSapce(u32 uLeftSize)
{
    if (mCurrentUsedLocalVol) {
        Log.d(TAG, "[%s: %d] Update mCurrentUsedLocalVol Left size: %ld MB", __FILE__, __LINE__, uLeftSize + lefSpaceThreshold);
        mCurrentUsedLocalVol->uAvail = uLeftSize + lefSpaceThreshold;
    }
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
    {
        unique_lock<mutex> lock(mRemoteDevLock);   

        sp<Volume> tmpVolume = NULL;
        Volume* localVolume = NULL;

        for (u32 i = 0; i < mList.size(); i++) {
            tmpVolume = mList.at(i);

            for (u32 j = 0; j < mModuleVols.size(); j++) {
                localVolume = mModuleVols.at(j);
                if (tmpVolume && localVolume && (tmpVolume->iIndex == localVolume->iIndex)) {
                    memset(localVolume->cVolName, 0, sizeof(localVolume->cVolName));
                    
                    strcpy(localVolume->cVolName, tmpVolume->cVolName);
                    localVolume->uTotal = tmpVolume->uTotal;
                    localVolume->uAvail = tmpVolume->uAvail;
                    localVolume->iSpeedTest = tmpVolume->iSpeedTest;
                }
            }
        }
    }
}


vector<Volume*>& VolumeManager::getRemoteVols()
{
    vector<Volume*>& remoteVols = mModuleVols;
    return remoteVols;
}


vector<Volume*>& VolumeManager::getLocalVols()
{
    vector<Volume*>& localVols = mLocalVols;
    return localVols;
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


