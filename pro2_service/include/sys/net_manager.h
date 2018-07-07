#ifndef PROJECT_NET_MANAGER_H
#define PROJECT_NET_MANAGER_H

#include <mutex>
#include <common/sp.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>


/*
 * 网络设备的链路状态
 */
enum {
    NET_LINK_CONNECT = 0x10,
    NET_LINK_DISCONNECT,
    NET_LINK_ERROR,
    NET_LINK_MAX,
};


/*
 * 网络设备的类型
 */
enum {
    DEV_LAN,        /* 以太网设备 */
    DEV_WLAN,       /* WIFI设备 */
    DEV_4G,         /* 4G设备 */
    DEV_MAX
};

enum {
    NET_DEV_STAT_ACTIVE = 0x20,
    NET_DEV_STAT_INACTIVE,
    NET_DEV_STAT_MAX,
};


enum {
    WIFI_WORK_MODE_AP = 0x30,
    WIFI_WORK_MODE_STA,
    WIFI_WORK_MODE_MAX,
};


class NetDev {
public:

	NetDev(int iType, int iWkMode, int iState, bool activeFlag, std::string ifName);
    ~NetDev();

	/* 打开/关闭网络设备 */
    int netdevOpen();
    int netdevClose();

	/* 获取/设置保存的链路状态 */
    int getNetdevSavedLink();
	void setNetdevSavedLink(int linkState);

	/* 获取网络设备的链路状态 */
    int getNetdevLinkFrmPhy();							


	/* 获取/设置网卡的激活状态 */
	bool getNetDevActiveState();
	int setNetDevActiveState(bool state);


	/* 将当前有效的网卡地址保存起来 */
	void storeNetDevIp();

	/* 将保存起来有效的网卡地址恢复到mCurIpAddr及硬件中 */
	void resumeNetDevIp();

	unsigned int getCurIpAddr();				
	void setCurIpAddr(uint32_t ip);			/* 设置当前的IP地址(除了更新mCurIpAddr,还会将地址更新到网卡硬件中) */

	/* 获取/设置Phy IP地址 */
	uint32_t getNetDevIpFrmPhy();
	bool setNetDevIpToPhy(uint32_t ip);


	/* 获取保存的IP地址,用于快速恢复网卡的地址 */	
	uint32_t getSavedIpAddr();


	/* 获取网卡的设备名 */
	std::string& getDevName();


private:

    int             mDevType;			/* 网卡设备的类型 */
	int 			mWorkMode;			/* 工作模式 */

    int             mLinkState;			/* 链路状态,初始化时为DISCONNECT状态 */


	/* RJ45: 从构造开始一直处于激活状态
	 * WiFi: 根据保存的配置来决定其处于激活或关闭状态
	 */
    bool            mActive;				/* 网卡的状态 */


	/* 构造网卡设备时,mCurIpAddr，mSaveIpAddr = 0
	 * 
	 */
    unsigned int    mCurIpAddr;			/* 当前的IP地址,与网卡的实际地址保持一致 */
	unsigned int 	mSaveIpAddr;		/* 链路状态变化时,保存上次的IP地址 */

    std::string     mDevName;
};



class EtherNetDev: public NetDev {
public:
    EtherNetDev(std::string ifName);
    ~EtherNetDev();

    int netdevOpen();
    int netdevClose();

};


class WiFiNetDev: public NetDev {
public:
    WiFiNetDev(int iWorkMode, std::string ifName);
    ~WiFiNetDev();

    /* Open WiFi Net Device */
    int netdevOpen();

    /* Close WiFi Net Device */
    int netdevClose();

};


/*
 * 网络设备管理器
 */
class NetManager {

public:
    static sp<NetManager> getNetManagerInstance();      	/* Signal Mode */

	void startNetManager();
	void stopNetManager();

    int registerNetdev(sp<NetDev>& netDev);
    void unregisterNetDev(sp<NetDev>& netDev);

    int getSysNetdevCnt();

    sp<NetDev>& getNetDevByname(std::string& devName);

	sp<ARMessage> obtainMessage(uint32_t what);

	void handleMessage(const sp<ARMessage> &msg);

    /*
     * - 启动管理器
     * - 停止管理器
     * - 启动指定名称的网卡
     * - 停止指定名称的网卡
     * - 设置指定网卡的固定IP
     * - 设置指定网卡的状态
     * - 获取指定网卡的状态
     */
    void postNetMessage(sp<ARMessage>& msg, int interval = 0);
    ~NetManager();
    NetManager();

private:

	bool checkNetDevHaveRegistered(sp<NetDev> &);
	void removeNetDev(sp<NetDev> &);
	void processEthernetEvent(sp<NetDev>& etherDev);


	void sendNetPollMsg();

    int mState;
	bool mExit;

	sp<ARLooper> mLooper;
    sp<ARHandler> mHandler;

	sp<ARMessage> mPollMsg;					/* 轮询消息 */

    std::thread mThread;                    /* 网络管理器线程 */

    std::mutex mMutex;                      /* 访问网络设备的互斥锁 */
    std::vector<sp<NetDev>> mDevList;       /* 网络设备列表 */
    sp<NetDev> mCurdev;                     /* 当前需要在屏幕上显示IP地址的激活设备 */
};


#endif /* PROJECT_NET_MANAGER_H */
