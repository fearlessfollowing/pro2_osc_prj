#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

#include <common/include_common.h>
#include <sys/net_manager.h>
#include <util/bytes_int_convert.h>
#include <string>

using namespace std;

#define TAG "NetManager"

//#define ENABLE_IP_DEBUG

struct ethtool_value {
    __uint32_t cmd;
    __uint32_t data;
};


/*********************************** NetDev **********************************/


NetDev::NetDev(int type, int state, bool active_flag, std::string if_name):
    dev_type(type),
    link_stat(state),
    active(active_flag),
    dev_name(if_name),
    addr(0)
{
    Log.d(TAG, "++> constructor net device");
}

NetDev::~NetDev()
{
    Log.d(TAG, "++> deconstructor net device");
}

NetDev::netdev_open()
{
    Log.d(TAG, "NetDev -> netdev_open");
}

NetDev::netdev_close()
{
    Log.d(TAG, "NetDev -> netdev_close");
}


int NetDev::get_dev_link()
{
    int skfd = -1;
    int err = NET_LINK_ERROR;
    int ret = 0;
    struct ifreq ifr;

    memset((u8 *)&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", dev_name.c_str());

    if (strlen(ifr.ifr_name) == 0) {
        goto RET_VALUE;
    }

    if (skfd == -1) {
        if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            goto RET_VALUE;
        }
    }

    edata.cmd = 0x0000000A;
    ifr.ifr_data = (caddr_t)&edata;

    ret = ioctl(fd, 0x8946, &ifr);
    if (ret == 0) {
        Log.d(TAG, "Link detected: %s\n", edata.data ? "yes" : "no");

        if (edata.data == 1) {
            err == NET_LINK_CONNECT;
        } else {
            err == NET_LINK_DISCONNECT;
        }
    } else {
        Log.e(TAG, "Cannot get link status");
    }

RET_VALUE:

    if (skfd != -1) {
        close(skfd);
    }
    return err;
}


unsigned int NetDev::get_dev_ipaddr()
{
    return addr;
}


void NetDev::set_dev_ipaddr(unsigned int ipaddr)
{
    /* set ipaddr to device */

    addr = ipaddr;
}

int NetDev::get_dev_active()
{
    return active;
}


/************************************* Ethernet Dev ***************************************/

EtherNetDev::EtherNetDev(std::string name)
{
    NetDev(DEV_LAN, NET_DEV_STAT_INACTIVE, false, name);
}

EtherNetDev::~EtherNetDev()
{

}


int EtherNetDev::netdev_open()
{
    /* ifconfig ethX up */
}

void EtherNetDev::netdev_close()
{
    /* ifconfig ethX down */
}



/************************************* WiFi Dev ***************************************/

WiFiNetDev::WiFiNetDev(int work_mode, string dev_name)
{

}

WiFiNetDev::~WiFiNetDev()
{

}

int WiFiNetDev::netdev_open()
{
    /* Maybe need reload driver */
}

void WiFiNetDev::netdev_close()
{
    /* Maybe need rmmod driver */
}

void WiFiNetDev::set_wifi_work_mode(int mode)
{
    /* If used different firmware, need changed it here... */
    work_mode = mode;
}

int WiFiNetDev::get_wifi_work_mode()
{
    return work_mode;
}



net_manager::net_manager()
{
    init();
}

net_manager::~net_manager()
{
    deinit();
}


void net_manager::init()
{
    for (int i = 0; i < DEV_MAX; i++) {
        sp<net_dev_info> tmp_net = sp<net_dev_info>(new net_dev_info(DEV_LAN + i, NET_LINK_DISCONNECT, NET_DEV_STAT_INACTIVE, 0));
        Log.d(TAG, ">>> register net dev type [%d]", DEV_LAN + i);
        net_devs.push_back(tmp_net);
    }
}

void net_manager::deinit()
{

    for (int i = 0; i < DEV_MAX; i++) {
        net_devs.pop_back();
    }
}


/*
 * 网络状态改变
 * 1.链路状态的改变: (由连接到断开; 由断开到连接)
 * 2.IP地址的变化
 * eth0: 由连接到断开, IP地址将强制设置为0.0.0.0
 *		 由断开到连接, 如果时Direct模式,IP地址强制设置为192.168.1.188; 如果是DHCP模式,调用dhclient为eth0
 *		 动态获取IP
 */

bool net_manager::check_net_change(sp<net_dev_info> &new_dev_info)
{
    bool bUpdate = false;
    bool bFoundIP = false;
	int net_link = NET_LINK_MAX;
	
    sp<net_dev_info> found_dev = sp<net_dev_info>(new net_dev_info());

	for (int i = 0; i < DEV_MAX; i++) {
        if (get_net_link_state(i, net_link) == 0) {
            unsigned int addr = GSNet_GetIPInfo(i);
            if (addr != 0) {
                found_dev->dev_type = i;
                found_dev->dev_addr = addr;
                found_dev->net_link = net_link;
                bFoundIP = true;
                break;
            }
        }
    }

    if (bFoundIP) {
		
#ifdef ENABLE_IP_DEBUG
        u8 ip[32];
        int_to_ip(found_dev->dev_addr, ip, sizeof(ip));
#endif

        if (found_dev->dev_addr == org_dev_info->dev_addr &&
                found_dev->dev_type == org_dev_info->dev_type &&
                found_dev->net_link == org_dev_info->net_link) {

		#ifdef ENABLE_IP_DEBUG
            Log.d(TAG, "met same addr type %d %s", found_dev->dev_type, ip);
		#endif

			//still send disp for wlan still error
            bUpdate = true;
        } else {

		#ifdef ENABLE_IP_DEBUG
            Log.d(TAG, "found new addr type %d %s", found_dev->dev_type, ip);
		#endif
            memcpy(org_dev_info.get(), found_dev.get(), sizeof(net_dev_info));
            bUpdate = true;
        }
    } else {
    
        // first check or net link disconnect
        if (org_dev_info->dev_addr != 0 || org_dev_info->net_link == -1) {
            // net from connect to disconnect
            bUpdate = true;
            org_dev_info->dev_addr = 0;
            org_dev_info->net_link = GSNet_CheckLinkByType(org_dev_info->dev_type);
        }
    }

    if (bUpdate) {

	#ifdef ENABLE_IP_DEBUG
        u8 ip[32];
        int_to_ip(org_dev_info->dev_addr,ip,sizeof(ip));
        Log.d(TAG," net update %d %s", org_dev_info->dev_type, ip);
	#endif
        memcpy(new_dev_info.get(), org_dev_info.get(), sizeof(net_dev_info));
    }

    {
        u8 org_ip[32];
        int_to_ip(found_dev->dev_addr, org_ip, sizeof(org_ip));
    }

    return bUpdate;
}




/*************************************************************************
** 方法名称: get_dev_name
** 方法功能: 根据网络设备的类型,获取对应的网络设备名
** 入口参数: 
**		dev_type - 设备类型(DEV_LAN, DEV_WLAN, DEV_4G)
**		dev_name - 设备名称
** 返 回 值: 无 
**
**
*************************************************************************/
void net_manager::get_dev_name(int dev_type, char *dev_name)
{
    switch (dev_type) {
        case DEV_LAN:
            sprintf(dev_name, "eth%d", 0);
            break;
		
        case DEV_WLAN:
            sprintf(dev_name, "%s", "wlan0");
            break;
		
        case DEV_4G:
            sprintf(dev_name, "ppp%d", 0);
            break;
		
        default:
            Log.e(TAG,"error net dev %d", dev_type);
            break;
    }
}



int net_manager::get_net_link_state(int dev_type, int & link_stat)
{
    int skfd = -1;
	int err = 0;
    struct ifreq ifr;
	
    memset((u8 *)&ifr, 0, sizeof(struct ifreq));

    get_dev_name(dev_type, ifr.ifr_name);
    if (strlen(ifr.ifr_name) == 0) {
		err = -1;
        goto RET_VALUE;
    }
	
    if (skfd == -1) {
        if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			err = -1;
            goto RET_VALUE;
        }
    }

    edata.cmd = 0x0000000A;
    ifr.ifr_data = (caddr_t)&edata;

    err = ioctl(fd, 0x8946, &ifr);
    if (err == 0) {
        Log.d(TAG, "Link detected: %s\n", edata.data ? "yes" : "no");
    } else {
        Log.e(TAG, "Cannot get link status");
	}

	if (edata.data == 1) {
		link_stat == NET_LINK_CONNECT;
	} else {
		link_stat == NET_LINK_DISCONNECT;
	}

RET_VALUE:

    if (skfd != -1) {
        close(skfd);
    }
    return err;
}



unsigned int net_manager::get_ipaddr_by_dev_type(int dev_type)
{
    unsigned int ucA0 = 0, ucA1 = 0, ucA2 = 0, ucA3 = 0;

    int skfd = -1;
    struct ifreq ifr;
    char acAddr[64];
    struct sockaddr_in *addr;
	
    unsigned int ip = 0;
    get_dev_name(dev_type, ifr.ifr_name);

    if (strlen(ifr.ifr_name) == 0) {
        return 0;
    }

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return 0;
    }

    if (ioctl(skfd, SIOCGIFADDR, &ifr) == -1) {
        close(skfd);
        return 0;
    }
	
    close(skfd);
    addr = (struct sockaddr_in *)(&ifr.ifr_addr);
    strcpy(acAddr, inet_ntoa(addr->sin_addr));
	
    if (4 == sscanf(acAddr, "%u.%u.%u.%u", &ucA0, &ucA1, &ucA2, &ucA3)) {
        ip = (ucA0 << 24)| (ucA1 << 16)|(ucA2 << 8)| ucA3;
    }	
    return ip;
}


static sp<NetManager> gSysNetManager = NULL;
static std::mutex gSysNetMutex;


enum {
    NET_MANAGER_STAT_INIT,
    NET_MANAGER_STAT_START,
    NET_MANAGER_STAT_RUNNING,
    NET_MANAGER_STAT_STOP,
    NET_MANAGER_STAT_STOPED,
    NET_MANAGER_STAT_DESTORYED,
    NET_MANAGER_STAT_MAX
};

#define EXIT_LOOP 0x100


class NetManagerHandler : public ARHandler {
public:
    NetManagerHandler(NetManager *source): mNetManager(source)
    {
    }

    virtual ~NetManagerHandler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> & msg) override
    {
        mNetManager->handleMessage(msg);
    }
	
private:
    NetManager* mNetManager;
};


sp<NetManager> NetManager::getNetManagerInstance()
{
    unique_lock<mutex> lock(gSysNetMutex);
    if (gSysNetManager != NULL) {
        return gSysNetManager;
    } else {
        gSysNetManager = sp<NetManager> (new NetManager());
    }
    return gSysNetManager;
}


void NetManager::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	Log.d(TAG, "NetManager get msg what %d", what);

	switch (what) {
		case EXIT_LOOP: {
			Log.d(TAG, "netmanager exit loop...");
			mLooper->quit();
			break;
		}

		/*
		 * - 启动网络设备
		 * - 停止网络设备
		 */

		default:
			break;
	}
}


void NetManager::startNetManager()
{
	if (NULL == mThread) {
		std::promise<bool> pr;
		std::future<bool> reply = pr.get_future();
		mThread = thread([this, &pr]
					   {
						   mLooper = sp<ARLooper>(new ARLooper());
						   mHandler = sp<ARHandler>(new NetManagerHandler(this));
						   mHandler->registerTo(mLooper);
						   pr.set_value(true);
						   mLooper->run();
					   });
		CHECK_EQ(reply.get(), true);

	} else {
		Log.d(TAG, "NetManager thread have exist");
	}	
}


void NetManager::stopNetManager()
{
	if (!mExit) {
		mExit = true;
		if (mThread.joinable()) {
			obtainMessage(EXIT_LOOP)->post();
			mThread.join();
			mThread = NULL;
		} else {
			Log.d(TAG, "NetManager thread not joinable");
		}
	}
}

int NetManager::registerNetdev(sp<NetDev>& netDev)
{
	int i;

    unique_lock<mutex> lock(mMutex);
    for (i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i)->getDevName() == netDev->getDevName()) {
			break;
		}
    }

	if (i >= mDevList.size()) {
		mDevList.push_back(netDev);
		Log.d(TAG, "register net device [%s]", netDev->getDevName());

	} else {
		Log.d(TAG, "net device [%s] have existed", netDev->getDevName());
	}
}

void NetManager::unregisterNetDev(sp<NetDev>& netDev)
{
	sp<NetDev> tmpDev = netDev;
	
    unique_lock<mutex> lock(mMutex);
    for (i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i) == netDev) {
			tmpDev->netdevClose();
			mDevList.erase(i);
			break;
		}
    }

}

int NetManager::getSysNetdevCnt()
{
    unique_lock<mutex> lock(mMutex);
    return mDevList.size();
}


sp<NetDev> NetManager::getNetDevByname(std::string devName)
{
	sp<NetDev> tmpDev = NULL;
	
    unique_lock<mutex> lock(mMutex);
    for (int i = 0; i < mDevList.size(); i++) {
        if (devName != NULL && mDevList.at(i)->dev_name == devName) {
			tmpDev = mDevList.at(i);
			break;
		}
    }

	return tmpDev;
}


int NetManager::postNetMessage(sp<ARMessage>& msg)
{

}


NetManager::NetManager(): mState(NET_MANAGER_STAT_INIT), 
							  mCurdev(NULL), 
							  mThread(NULL),
							  mExit(false)
{
    Log.d(TAG, "construct NetManager....");
	mDevList.clear();
}


NetManager::~NetManager()
{
    Log.d(TAG, "deconstruct NetManager....");

    /* stop all net devices */

    /* unregister all net devices */

    mState = NET_MANAGER_STAT_DESTORYED;
}



class NetManager {

public:


    /*
     * - 启动管理器
     * - 停止管理器
     * - 启动指定名称的网卡
     * - 停止指定名称的网卡
     * - 设置指定网卡的固定IP
     * - 设置指定网卡的状态
     * - 获取指定网卡的状态
     */


private:
    NetManager();
    ~NetManager();



    int mState;

    std::thread mThread;                    /* 网络管理器线程 */

    std::mutex mMutex;                      /* 访问网络设备的互斥锁 */
    std::vector<sp<NetDev>> mDevList;       /* 网络设备列表 */
    sp<NetDev> mCurdev;                     /* 当前需要在屏幕上显示IP地址的激活设备 */
};
