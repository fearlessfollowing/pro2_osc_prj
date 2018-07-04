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

#if 0
/*
 * 网络设备
 */
struct net_dev_info {
    int             dev_type;           /* 网络设备的类型 */
    int             net_link;           /* 网络设备的链路状态 */
    int             active_stat;        /* 网络设备的激活状态 */
    unsigned int    dev_addr;           /* 网络设备的IP地址 */
};

#endif

class NetDev {
public:

    NetDev(int iType, int iState, bool activeFlag, std::string ifName);
    ~NetDev();

    int netdevOpen();						/* 打开网络设备 */
    void netdevClose();						/* 关闭网络设备 */
    int getNetdevLink();					/* 获取网络设备的链路状态 */
    unsigned int getNetdevIpaddr();			/* 获取网络设备的IP地址 */
    void setNetdevIpaddr(unsigned int ipAddr);	/* 设置网络设备的IP地址 */
	int getNetDevActiveState();				/* 获取设备的激活状态 */

	std::string getDevName();

private:
    int             devType;
    int             linkState;
    bool            active;
    unsigned int    ipAddr;
    std::string     devName;
};



class EtherNetDev: public NetDev {
public:
    EtherNetDev(std::string ifName);
    ~EtherNetDev();

    int netdevOpen();
    void netdevClose();

};


class WiFiNetDev: public NetDev {
public:
    WiFiNetDev(int iWorkMode, std::string ifName);
    ~WiFiNetDev();

    /* Open WiFi Net Device */
    int netdevOpen();

    /* Close WiFi Net Device */
    void netdevClose();

    void setWifiWorkMode(int mode);
    int getWifiWorkMode();

private:
    int	mWorkMode;  /* AP or STA */
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

    sp<NetDev> getNetDevByname(std::string& devName);

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
    void postNetMessage(sp<ARMessage>& msg);
    ~NetManager();
    NetManager();

private:

    int mState;
	bool mExit;

	sp<ARLooper> mLooper;
    sp<ARHandler> mHandler;

    std::thread mThread;                    /* 网络管理器线程 */

    std::mutex mMutex;                      /* 访问网络设备的互斥锁 */
    std::vector<sp<NetDev>> mDevList;       /* 网络设备列表 */
    sp<NetDev> mCurdev;                     /* 当前需要在屏幕上显示IP地址的激活设备 */
};


#endif /* PROJECT_NET_MANAGER_H */
