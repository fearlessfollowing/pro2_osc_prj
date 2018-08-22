#ifndef _VOLUMEMANAGER_H
#define _VOLUMEMANAGER_H

#include <pthread.h>


enum {
    VOLUME_MANAGER_LISTENER_MODE_NETLINK,
    VOLUME_MANAGER_LISTENER_MODE_INOTIFY,
    VOLUME_MANAGER_LISTENER_MODE_MAX,
};


/*
 * 底层: 接收Netlink消息模式, 监听设备文件模式
 */
class VolumeManager {
private:
    static VolumeManager*   sInstance;

private:
    SocketListener*         mBroadcaster;

    VolumeCollection*       mVolumes;

    bool                    mDebug;

    int                    mVolManagerDisabled;

public:
    virtual ~VolumeManager();

    int         start();
    int         stop();

    void        handleBlockEvent(NetlinkEvent *evt);

    int         addVolume(Volume *v);

    int         listVolumes(SocketClient *cli, bool broadcast);

    int         mountVolume(const char *label);

    int         unmountVolume(const char *label, bool force, bool revert, bool badremove = false);

    int         formatVolume(const char *label, bool wipe);
    void        disableVolumeManager(void) { mVolManagerDisabled = 1; }



    Volume*     getVolumeForFile(const char *fileName);

    void        setDebug(bool enable);

    void        setBroadcaster(SocketListener *sl) { mBroadcaster = sl; }
    SocketListener *getBroadcaster() { return mBroadcaster; }

    static VolumeManager *Instance();

    Volume *lookupVolume(const char *label);

    int getNumDirectVolumes(void);
    int getDirectVolumeList(struct volume_info *vol_list);

    int mkdirs(char* path);

private:
    int         iListenerMode;          /* 监听模式 */

    VolumeManager();
    void readInitialState();
    bool isMountpointMounted(const char *mp);
};

#endif