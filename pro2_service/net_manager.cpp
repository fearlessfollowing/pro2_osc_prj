#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <common/include_common.h>

#include <util/ARHandler.h>
#include <util/ARMessage.h>


#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

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
NetDev::NetDev(int iType, int iState, bool activeFlag, string ifName):
			devType(iType),
			linkState(iState),
			active(activeFlag),
			ipAddr(0),
			devName(ifName)
{
    Log.d(TAG, "++> constructor net device");
}

NetDev::~NetDev()
{
    Log.d(TAG, "++> deconstructor net device");
}

int NetDev::netdevOpen()
{
    Log.d(TAG, "NetDev -> netdevOpen");
	return 0;
}

void NetDev::netdevClose()
{
    Log.d(TAG, "NetDev -> netdevClose");
}


int NetDev::getNetdevLink()
{
    int skfd = -1;
    int err = NET_LINK_ERROR;
    int ret = 0;
    struct ifreq ifr;

	struct ethtool_value edata;

    memset((u8 *)&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", devName.c_str());

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

    ret = ioctl(skfd, 0x8946, &ifr);
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


unsigned int NetDev::getNetdevIpaddr()
{
    return ipAddr;
}


void NetDev::setNetdevIpaddr(unsigned int ipaddr)
{
    /* set ipaddr to device */

    ipAddr = ipaddr;
}

int NetDev::getNetDevActiveState()
{
    return active;
}


std::string NetDev::getDevName()
{
	return devName;
}



/************************************* Ethernet Dev ***************************************/

EtherNetDev::EtherNetDev(std::string name):NetDev(DEV_LAN, NET_DEV_STAT_INACTIVE, false, name)
{
}

EtherNetDev::~EtherNetDev()
{

}


int EtherNetDev::netdevOpen()
{
    /* ifconfig ethX up */
	return 0;
}

void EtherNetDev::netdevClose()
{
    /* ifconfig ethX down */
}



/************************************* WiFi Dev ***************************************/

WiFiNetDev::WiFiNetDev(int work_mode, string name):NetDev(DEV_LAN, NET_DEV_STAT_INACTIVE, false, name),
													   mWorkMode(work_mode)
{

}

WiFiNetDev::~WiFiNetDev()
{

}

int WiFiNetDev::netdevOpen()
{
    /* Maybe need reload driver */
}

void WiFiNetDev::netdevClose()
{
    /* Maybe need rmmod driver */
}

void WiFiNetDev::setWifiWorkMode(int mode)
{
    /* If used different firmware, need changed it here... */
    mWorkMode = mode;
}

int WiFiNetDev::getWifiWorkMode()
{
    return mWorkMode;
}



#if 0
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
#endif


static sp<NetManager> gSysNetManager = NULL;
static bool gInitNetManagerThread = false;
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

#define NETM_EXIT_LOOP 			0x100		/* 退出消息循环 */
#define NETM_REGISTER_NETDEV 	0x101		/* 注册网络设备 */
#define NETM_UNREGISTER_NETDEV	0x102		/* 注销网络设备 */
#define NETM_STARTUP_NETDEV		0x103		/* 启动网络设备 */
#define NETM_CLOSE_NETDEV		0x104		/* 关闭网络设备 */
#define NETM_SET_NETDEV_IP		0x105		/* 设置设备的IP地址 */
#define NETM_LIST_NETDEV		0x106		/* 列出所有注册的网络设备 */
#define NETM_POLL_NET_STATE		0x107

#define NETM_NETDEV_MAX_COUNT	10

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


sp<ARMessage> NetManager::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}


void NetManager::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	bool bHaveRegistered = false;
	
	Log.d(TAG, "NetManager get msg what %d", what);

	switch (what) {
		case NETM_POLL_NET_STATE: {		/* 轮询网络设备的状态 */
			break;
		}
	

		case NETM_REGISTER_NETDEV: {	/* 注册网络设备 */
			Log.d(TAG, "NetManager -> register net device...");
			/*
			 * msg.what = NETM_REGISTER_NETDEV
			 * msg."netdev" = sp<NetDev>
			 */
			sp<NetDev> tmpNet;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			/* 检查该网络设备是否已经被注册过,及管理器管理的网卡是否达到上限 */
			if (mDevList.size() > NETM_NETDEV_MAX_COUNT) {
				Log.e(TAG, "NetManager registered netdev is maxed...");
			} else {
				for (int i = 0; i < mDevList.size(); i++) {
					if (mDevList.at(i) == tmpNet || mDevList.at(i)->getDevName() == tmpNet->getDevName()) {
						Log.e(TAG, "NetManager same name netdev have registered...");
						bHaveRegistered = true;
						break;
					}
				}

				if (!bHaveRegistered) {
					Log.d(TAG, "NetManager: netdev[%s] register now", tmpNet->getDevName().c_str());
					mDevList.push_back(tmpNet);
					if (tmpNet->getNetDevActiveState() == true) {	/* 激活状态 */
						tmpNet->netdevOpen();	/* 打开网络设备 */
					}
				}				
			}			
			break;
		}


		case NETM_UNREGISTER_NETDEV: {	/* 注销网络设备 */

			break;
		}


		case NETM_STARTUP_NETDEV: {		/* 启动网络设备 */
			break;
		}


		case NETM_CLOSE_NETDEV: {		/* 关闭网络设备 */
			break;
		}


		case NETM_SET_NETDEV_IP: {		/* 设备设备IP地址(DHCP/static) */
			break;
		}



		case NETM_LIST_NETDEV: {
			sp<NetDev> tmpDev;
			
			for (int i = 0; i < mDevList.size(); i++) {
				tmpDev = mDevList.at(i);
				Log.d(TAG, "--------------- NetManager List Netdev ------------------");
				Log.d(TAG, "Name: %s", tmpDev->getDevName().c_str());
				Log.d(TAG, "IP: %s", tmpDev->getDevName().c_str());
				Log.d(TAG, "Link: %s", tmpDev->getDevName().c_str());
				Log.d(TAG, "State: %s", tmpDev->getDevName().c_str());
				Log.d(TAG, "---------------------------------------------------------");
			}

			break;
		}

		case NETM_EXIT_LOOP: {
			Log.d(TAG, "netmanager exit loop...");
			mLooper->quit();
			break;
		}


		default:
			Log.d(TAG, "Unsupport Message recieve ...");
			break;
	}
}


void NetManager::startNetManager()
{
	if (gInitNetManagerThread == false) {
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
		gInitNetManagerThread = true;
		Log.d(TAG, "startNetManager .... success!!!");
	} else {
		Log.d(TAG, "NetManager thread have exist");
	}	
}


void NetManager::stopNetManager()
{
	if (gInitNetManagerThread == true) {
		if (!mExit) {
			mExit = true;
			if (mThread.joinable()) {
				obtainMessage(NETM_EXIT_LOOP)->post();
				mThread.join();
				gInitNetManagerThread = false;
			} else {
				Log.d(TAG, "NetManager thread not joinable");
			}
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
    for (int i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i) == netDev) {
			tmpDev->netdevClose();
			//mDevList.erase(i);
			break;
		}
    }

}

int NetManager::getSysNetdevCnt()
{
    unique_lock<mutex> lock(mMutex);
    return mDevList.size();
}


sp<NetDev> NetManager::getNetDevByname(std::string& devName)
{
	sp<NetDev> tmpDev = NULL;
	
    unique_lock<mutex> lock(mMutex);
    for (int i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i)->getDevName() == devName) {
			tmpDev = mDevList.at(i);
			break;
		}
    }

	return tmpDev;
}


void NetManager::postNetMessage(sp<ARMessage>& msg)
{
	msg->setHandler(mHandler);
	msg->post();
}


NetManager::NetManager(): mState(NET_MANAGER_STAT_INIT), 
							  mCurdev(NULL), 
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

