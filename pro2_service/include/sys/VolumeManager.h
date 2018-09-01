#ifndef _VOLUMEMANAGER_H
#define _VOLUMEMANAGER_H

#include <pthread.h>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <util/ARMessage.h>
#include <sys/NetlinkEvent.h>

#include <sys/Mutex.h>

enum {
    VOLUME_MANAGER_LISTENER_MODE_NETLINK,
    VOLUME_MANAGER_LISTENER_MODE_INOTIFY,
    VOLUME_MANAGER_LISTENER_MODE_MAX,
};


#define VOLUME_NAME_MAX     32

#ifndef COM_NAME_MAX_LEN
#define COM_NAME_MAX_LEN    64
#endif


#ifndef WIFEXITED
#define WIFEXITED(status)	(((status) & 0xff) == 0)
#endif /* !defined WIFEXITED */

#ifndef WEXITSTATUS
#define WEXITSTATUS(status)	(((status) >> 8) & 0xff)
#endif /* !defined WEXITSTATUS */

#define ARRAY_SIZE(x)	    (sizeof(x) / sizeof(x[0]))


/* 卷:
 * 子系统:      USB/SD
 *             所属的总线（BUS）
 * 容量相关:    总线/剩余/可用/索引
 * 测速相关：   是否已经测速
 * 挂载相关:    该设备的挂载点(由所属的bus-port-subport决定)
 * 状态相关:    未插入,已挂载,正在格式化,格式化完成
 */


/*
 * 卷所属的子系统 - SD/USB两类
 */
enum {
    VOLUME_SUBSYS_SD,
    VOLUME_SUBSYS_USB,
    VOLUME_SUBSYS_MAX,
};


enum {
	VOLUME_TYPE_NV = 0,
	VOLUME_TYPE_MODULE = 1,
	VOLUME_TYPE_MAX
};


enum {
	VOLUME_SPEED_TEST_SUC = 0,
	VOLUME_SPEED_TEST_FAIL = -1,
};


enum {
    VOLUME_STATE_INIT       = -1,
    VOLUME_STATE_NOMEDIA    = 0,
    VOLUME_STATE_IDLE       = 1,
    VOLUME_STATE_PENDING    = 2,
    VOLUME_STATE_CHECKNG    = 3,
    VOLUME_STATE_MOUNTED    = 4,
    VOLUME_STATE_UNMOUNTING = 5,
    VOLUME_STATE_FORMATTING = 6,
    VOLUME_STATE_DISABLED   = 7,    /* 卷被禁止, 可以不上报UI */
};

enum {
    VOLUME_SLOT_SWITCH_ENABLE   = 1,
    VOLUME_SLOT_SWITCH_DISABLED = 2,
    VOLUME_SLOT_SWITCH_MAX,
};

enum {
    VOLUME_ACTION_ADD = 1,
    VOLUME_ACTION_REMOVE = 2,
    VOLUME_ACTION_UNSUPPORT = 3,
    VOLUME_ACTION_MAX
};


enum {
    VOLUME_PRIO_LOW     = 0,
    VOLUME_PRIO_SD      = 1,        /* USB3.0 内部设备（SD） */
    VOLUME_PRIO_UDISK   = 2,        /* USB3.0 移动硬盘 */
    VOLUME_PRIO_MAX,
};

enum {
    VOLUME_MANAGER_WORKMODE_NORMAL = 0,
    VOLUME_MANAGER_WORKMODE_UDISK  = 1,
    VOLUME_MANAGER_WORKMODE_MAX,
};


/*
 * Volume - 逻辑卷
 */
typedef struct stVol {
    int             iVolSubsys;                         /* 卷的子系统： USB/SD */
    const char*     pBusAddr;                           /* 总线地址: USB - "1-2.3" */
    const char*     pMountPath;                         /* 挂载点：挂载点与总线地址是一一对应的 */

    char            cVolName[COM_NAME_MAX_LEN];         /* 卷的名称 */
    char            cDevNode[COM_NAME_MAX_LEN];         /* 设备节点名: 如'/dev/sdX' */
    char            cVolFsType[COM_NAME_MAX_LEN];

	int		        iType;                              /* 用于表示是内部设备还是外部设备 */
	int		        iIndex;			                    /* 索引号（对于模组上的小卡有用） */
    int             iPrio;                              /* 卷的优先级 */
    int             iVolState;                          /* 卷所处的状态: No_Media/Init/Mounted/Formatting */
    int             iVolSlotSwitch;                     /* 是否使能该接口槽 */

    u64             uTotal;			                    /* 总容量:  (单位为MB) */
    u64             uAvail;			                    /* 剩余容量:(单位为MB) */
	int 	        iSpeedTest;		                    /* 1: 已经测速通过; 0: 没有进行测速或测速未通过 */
} Volume;


static Volume gSysVols[] = {
    {   /* SD卡 - 3.0 */
        VOLUME_SUBSYS_SD,
        "usb2-2,usb2-2.2",
        "/mnt/sdcard",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_NV,
        0,
        VOLUME_PRIO_SD,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,      /* 机身后面的SD卡: 默认为使能状态 */
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* Udisk1 - 2.0/3.0 */
        VOLUME_SUBSYS_USB,
        "usb2-1,usb1-2.1",           /* 接3.0设备时的总线地址 */
        "/mnt/udisk1",
        {0},             /* 动态生成 */
        {0},  
        {0},

        VOLUME_TYPE_NV,
        0,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,      /* 机身底部的USB接口: 默认为使能状态 */        
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* Udisk2 - 2.0/3.0 */
        VOLUME_SUBSYS_USB,
        "usb2-3",           /* 3.0 */
        "/mnt/udisk2",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_NV,
        0,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_DISABLED,      /* 机身顶部的USB接口: 默认为禁止状态 */         
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD1 */
        VOLUME_SUBSYS_SD,
        "usb1-2.3",                         /* usb1-3.2 usb1-2.3 */
        "/mnt/mSD1",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        1,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF1: 默认为使能状态 */         
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD2 */
        VOLUME_SUBSYS_SD,
        "usb1-2.2",
        "/mnt/mSD2",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        2,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF2: 默认为使能状态 */          
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD3 */
        VOLUME_SUBSYS_SD,
        "usb1-3.3",
        "/mnt/mSD3",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        3,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF3: 默认为使能状态 */           
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD4 */
        VOLUME_SUBSYS_SD,
        "usb1-3.2",
        "/mnt/mSD4",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        4,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF4: 默认为使能状态 */           
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD5 */
        VOLUME_SUBSYS_SD,
        "usb1-3.1",
        "/mnt/mSD5",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        5,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF5: 默认为使能状态 */           
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD6 */
        VOLUME_SUBSYS_SD,
        "usb1-2.4",
        "/mnt/mSD6",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        6,
        VOLUME_PRIO_LOW,

        VOLUME_STATE_INIT,
        VOLUME_SLOT_SWITCH_ENABLE,          /* TF6: 默认为使能状态 */           
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },          

};


class NetlinkEvent;

/* 1.接收客户端发送的进入U盘模式命令
 * 2.将命令转发给UI
 * 3.UI设置gpio，然后给模组上电
 * 4.全部模组挂载成功，
 */

/*
 * 底层: 接收Netlink消息模式, 监听设备文件模式
 * - 挂载/卸载/格式化
 * - 列出所有卷
 * - 获取指定名称的卷
 */
class VolumeManager {

public:
    virtual ~VolumeManager();


    static u32 lefSpaceThreshold;

    /*
     * 启动/停止卷管理器
     */
    bool        start();
    bool        stop();

    /*
     * 处理块设备事件的到来
     */
    int         handleBlockEvent(NetlinkEvent *evt);


    void        listVolumes();

    
    int         mountVolume(Volume* pVol, NetlinkEvent* pEvt);

    int         unmountVolume(Volume* pVol, NetlinkEvent* pEvt, bool force);

    int         doUnmount(const char *path, bool force);

    int         formatVolume(Volume* pVol, bool wipe);
    
    void        disableVolumeManager(void) { mVolManagerDisabled = 1; }

    void        setDebug(bool enable);

    Volume*     isSupportedDev(const char* busAddr);

    bool        extractMetadata(const char* devicePath, char* volFsType, int iLen);

    bool        checkMountPath(const char* mountPath);

    bool        isValidFs(const char* devName, Volume* pVol);

    int         checkFs(Volume* pVol);

    void        updateVolumeSpace(Volume* pVol);

    void        syncTakePicLeftSapce(u32 uLeftSize);

    /*
     * 检查是否存在本地卷
     */
    bool        checkLocalVolumeExist();
    u64         getLocalVolLeftSize(bool bUseCached = false);
    const char* getLocalVolMountPath();

    void        setVolCurPrio(Volume* pVol, NetlinkEvent* pEvt);
    void        setSavepathChanged(int iAction, Volume* pVol);

    std::vector<Volume*>& getCurSavepathList();

    void        syncLocalDisk();

    /*
     * 检查是否所有的TF卡都存在
     */
    bool        checkAllTfCardExist();
    u64         calcRemoteRemainSpace(bool bFactoryMode = false);

    void        updateLocalVolSpeedTestResult(int iResult);
    void        updateRemoteVolSpeedTestResult(Volume* pVol);
    bool        checkAllmSdSpeedOK();
    bool        checkLocalVolSpeedOK();

    bool        checkSavepathChanged();

    void        setSavepathChanged(Volume* pVol);
    bool        volumeIsTfCard(Volume* pVol);


    /*
     * 更新mSD的查询结果
     */
    void        updateRemoteTfsInfo(std::vector<sp<Volume>>& mList);

    /*
     * 更新所有卷的测速状态
     */
    void        updateVolumesSpeedTestState(std::vector<sp<Volume>>& mList);


    /*
     * 更新远端卷的拔插处理
     */
    int         handleRemoteVolHotplug(std::vector<sp<Volume>>& volChangeList);

    void        sendDevChangeMsg2UI(int iAction, int iType, std::vector<Volume*>& devList);

    void        setNotifyRecv(sp<ARMessage> notify);

    Volume*     lookupVolume(const char *label);

    /*
     * 获取远端存储卷列表
     */
    std::vector<Volume*>& getRemoteVols();
    std::vector<Volume*>& getLocalVols();

    std::vector<Volume*>& getSysStorageDevList();

    bool isMountpointMounted(const char *mp);


    /*
     * U盘模式
     */
    void        enterUdiskMode();
    void        exitUdiskMode();
    void        checkAllUdiskIdle();
    int         checkAllUdiskMounted();

    bool        checkEnteredUdiskMode();

    int         getVolumeManagerWorkMode();
    void        setVolumeManagerWorkMode(int iWorkMode);

    int         getCurHandleAddUdiskVolCnt();
    int         getCurHandleRemoveUdiskVolCnt();


    /*
     * 录像/直播存片 时间接口
     */
    u64         getRecSec();
    void        incOrClearRecSec(bool bClrFlg = false);
    void        setRecLeftSec(u64 leftSecs);
    bool        decRecLeftSec();
    u64         getRecLeftSec();

    u64         getLiveRecSec();
    void        incOrClearLiveRecSec(bool bClrFlg = false);

    void        setLiveRecLeftSec(u64 leftSecs);
    void        decLiveRecLeftSec();
    u64         getLiveRecLeftSec();

    /*
     * 转换秒数为'00:00:00'格式字符串
     */
    void        convSec2TimeStr(u64 secs, char* strBuf, int iLen);

    static VolumeManager *Instance();

private:
    int                     mListenerMode;                  /* 监听模式 */
    Volume*                 mCurrentUsedLocalVol;           /* 当前被使用的本地卷 */
    Volume*                 mSavedLocalVol;                 /* 上次保存 */
    bool                    mBsavePathChanged;              /* 本地存储设备路径是否发生改变 */

    static VolumeManager*   sInstance;

    std::vector<Volume*>    mVolumes;       /* 管理系统中所有的卷 */

    std::vector<Volume*>    mLocalVols;     /* 管理系统中所有的卷 */
    std::vector<Volume*>    mModuleVols;    /* 模组卷 */

    std::vector<Volume*>    mCurSaveVolList;

    std::vector<Volume*>    mSysStorageVolList;

    bool                    mDebug;

    int                     mVolManagerDisabled;

    int                     mModuleVolNum;

    std::mutex				mLocaLDevLock;
    std::mutex              mRemoteDevLock;

    u64                     mReoteRecLiveLeftSize = 0;                  /* 远端设备(小卡)的录像,直播剩余时间 */

    int                     mVolumeManagerWorkMode;                     /* 卷管理器的工作模式: U盘模式;普通模式 */

    int                     mHandledAddUdiskVolCnt;
    int                     mHandledRemoveUdiskVolCnt;

    u64                     mRecLeftSec;                                /* 当前挡位可录像的剩余时长 */
    u64                     mRecSec;    

    
    u64                     mLiveRecLeftSec;                            /* 当前挡位直播存片的剩余时长 */
    u64                     mLiveRecSec;

    pthread_t               mThread;			

	sp<ARMessage>	        mNotify;
    VolumeManager();

    pthread_t               mFileMonitorThread;
    int                     mFileMonitorPipe[2];
    bool                    initFileMonitor();
    bool                    deInitFileMonitor();

public:
    void                    runFileMonitorListener();


};




#endif