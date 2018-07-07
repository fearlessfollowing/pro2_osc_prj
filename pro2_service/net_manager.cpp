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


#define NET_POLL_INTERVAL 2000



/*********************************** NetDev **********************************/
NetDev::NetDev(int iType, int iWkMode, int iState, bool activeFlag, string ifName):
		mDevType(iType),
		mWorkMode(iWkMode),	
		mLinkState(iState),
		mActive(activeFlag),
		mCurIpAddr(0),
		mSaveIpAddr(0),
		mDevName(ifName)
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

int NetDev::netdevClose()
{
    Log.d(TAG, "NetDev -> netdevClose");
	return 0;
}


int NetDev::getNetdevSavedLink()
{
	return mLinkState;
}

void NetDev::setNetdevSavedLink(int linkState)
{
	mLinkState = linkState;
}


void NetDev::storeNetDevIp()
{
	mSaveIpAddr = mCurIpAddr;
}

void NetDev::resumeNetDevIp()
{
	mCurIpAddr = mSaveIpAddr;
}

unsigned int NetDev::getCurIpAddr()
{
	return mCurIpAddr;
}


void NetDev::setCurIpAddr(uint32_t ip)
{
	mCurIpAddr = ip;
	
}


unsigned int NetDev::getSavedIpAddr()
{
	return mSaveIpAddr;
}


int NetDev::getNetdevLinkFrmPhy()
{
    int skfd = -1;
    int err = NET_LINK_ERROR;
    int ret = 0;
    struct ifreq ifr;

	struct ethtool_value edata;

    memset((u8 *)&ifr, 0, sizeof(struct ifreq));
    sprintf(ifr.ifr_name, "%s", getDevName().c_str());

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


uint32_t NetDev::getNetDevIpFrmPhy()
{
	uint32_t ucA0 = 0, ucA1 = 0, ucA2 = 0, ucA3 = 0;
    int skfd = -1;
    struct ifreq ifr;
    char acAddr[64];
    struct sockaddr_in *addr;
	
    uint32_t ip = 0;

	strcpy(ifr.ifr_name, mDevName.c_str());
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
        ip = (ucA0 << 24) | (ucA1 << 16) | (ucA2 << 8) | ucA3;
    }	
    return ip;
}


bool NetDev::setNetDevIpToPhy(uint32_t ip)
{

	return true;
}


bool NetDev::getNetDevActiveState()
{
    return mActive;
}


int NetDev::setNetDevActiveState(bool state)
{
	mActive = state;
	return 0;
}


string& NetDev::getDevName()
{
	return mDevName;
}



/************************************* Ethernet Dev ***************************************/

EtherNetDev::EtherNetDev(string name):NetDev(DEV_LAN, WIFI_WORK_MODE_STA, NET_DEV_STAT_INACTIVE, false, name)
{
}

EtherNetDev::~EtherNetDev()
{

}


int EtherNetDev::netdevOpen()
{
    /* ifconfig ethX up 
 	 *
 	 */
 	 
	return 0;
}

int EtherNetDev::netdevClose()
{
    /* ifconfig ethX down */
	return 0;
}



/************************************* WiFi Dev ***************************************/

WiFiNetDev::WiFiNetDev(int work_mode, string name):NetDev(DEV_LAN, work_mode, NET_DEV_STAT_INACTIVE, false, name)
{

}

WiFiNetDev::~WiFiNetDev()
{

}

int WiFiNetDev::netdevOpen()
{
    /* Maybe need reload driver */
	return 0;
}

int WiFiNetDev::netdevClose()
{
    /* Maybe need rmmod driver */
	return 0;
}

#if 0

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


#endif




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


void NetManager::removeNetDev(sp<NetDev> & netdev)
{
	vector<sp<NetDev>>::iterator itor;
	
	for (itor = mDevList.begin(); itor != mDevList.end(); itor++) {
		if (*itor == netdev) {
			mDevList.erase(itor);
			break;
		}
	}
}

bool NetManager::checkNetDevHaveRegistered(sp<NetDev> & netdev)
{

	for (uint32_t i = 0; i < mDevList.size(); i++) {
		if (mDevList.at(i) == netdev || mDevList.at(i)->getDevName() == netdev->getDevName()) {
			return true;
		}
	}
	return false;
}


void NetManager::sendNetPollMsg()
{
	postNetMessage(mPollMsg, NET_POLL_INTERVAL);
}


/*
 * 处理以太网网卡
 */
void NetManager::processEthernetEvent(sp<NetDev>& etherDev) 
{
	/*
	 * 1.链路状态,主要针对以太网设备
	 * 初始化时,网卡的链路为断开状态,当前网卡链路状态发生改变时
	 * 由断开到连接: 
	 *	-> 设置网卡的IP地址
	 *		-> Static:
	 *			如果 mCurIpAddr,mSaveIpAddr为0,将其设置为192.168.1.188,并设置到UI
	 *			如果 mCurIpAddr = 0, mSaveIpAddr != 0, 将mSaveIpAddr设置为mCurIpAddr,并设置到UI
	 *		-> DHCP: 
	 *			如果 mCurIpAddr,mSaveIpAddr为0, 调用DHCP
	 * 由连接到断开:
	 *	将mCurIpAddr保存到mSaveIpAddr, 将mCurIpAddr设置为0,发送mCurIpAddr到UI
	 * 
	 * 2.IP地址的变化: (主要针对DHCP)
	 * 	-> 如果从网卡获取的IP地址跟mCurIpAddr不一致,将网卡实际地址设置为mCurIpAddr,发送UI消息
	 */
	int iCurLinkState = etherDev->getNetdevLinkFrmPhy();
	if (etherDev->getNetdevSavedLink() != iCurLinkState) {	/* 链路发生变化 */

		Log.d(TAG, "NetManger: netdev[%s] link state changed", etherDev->getDevName().c_str());

		if (iCurLinkState == NET_LINK_CONNECT) {	/* Disconnect -> Connect */
			Log.d(TAG, ">>> link connect");

			/* Static */
			if (1) {
				Log.d(TAG, "get ip use static");
				/* 只有构造设备时会将mCurIpAddr与mSavedIpAdrr设置为0 */
				if (etherDev->getCurIpAddr() == etherDev->getSavedIpAddr()) { /* mCurIpAddr == mSaveIpAddr */
					/* setip 192.168.1.188,设置mCurIpAddr为192.168.1.188,并且将该地址设置Phy中 */
					//etherDev->setCurIpAddr();	// ip = 192.168.1.188
				} else {	/* mCurIpAddr != mSaveIpAddr */
					etherDev->resumeNetDevIp();	/* 将保存的IP恢复到mCurIpAddr */
				}
				
				/* 将地址设置到网卡,并将地址发送给UI */
			} else {
				Log.d(TAG, "get ip use dhcp");

			}
		} else {	/* Connect -> Disconnect */
			//etherDev->storeNetDevIp();
			//etherDev->setNetdevIp(0);	
			
			/* 发送0.0.0.0到UI */
		}

		etherDev->setNetdevSavedLink(iCurLinkState);
	} else {	/* 链路未发生变化 */
		/* IP发生变化
		 * Static - 链路未发生变化时,IP地址不会发生变化
		 * DHCP   - 可能IP地址会发生变化
		 */
		if (etherDev->getCurIpAddr() != etherDev->getNetDevIpFrmPhy()) {
			etherDev->setCurIpAddr(etherDev->getNetDevIpFrmPhy());	// ip = 192.168.1.188
			
			/* 发送IP到UI
			*/
		}
	}

	
}




void NetManager::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	
	Log.d(TAG, "NetManager get msg what %d", what);

	switch (what) {
		case NETM_POLL_NET_STATE: {		/* 轮询网络设备的状态 */
			/* 检查系统中所有处于激活状态的网卡
			 * 如果链路发生变化
			 * 如果IP发生变化
			 */
			
			vector<sp<NetDev>>::iterator itor;
			vector<sp<NetDev>> tmpList;

			tmpList.clear();
			
			for (itor = mDevList.begin(); itor != mDevList.end(); itor++) {
				if ((*itor)->getNetDevActiveState()) {
					tmpList.push_back(*itor);
				}
			}

			/*
			 * 链路发送了改变
			 * 对于RJ45: 网线的插入和拔出
			 * 插入: 如果之前已经有保存的IP地址(值不为0),直接使用之前保存的IP地址(地址变化,发送消息给UI)
			 *		 如果之前没有保存过IP地址: 
			 *		 	如果是静态IP模式,设置网卡的IP地址(如: 192.168.1.188),并且通知UI显示该IP地址
			 *		 	如果是DHCP模式,如果之前已经DHCP过,直接使用之前已经DHCP获得的地址
			 * 拔出: 保存之前设置的IP地址,将当前设备IP地址设置为0(如果IP地址发生了变化,发生消息给UI)
			 *
			 * WiFi:
			 * 打开:
			 *	AP模式: IP地址是固定的()
			 * 关闭:
			 */
			for (itor = tmpList.begin(); itor != tmpList.end(); itor++) {

			}

			sendNetPollMsg();	/* 继续发送轮询消息 */
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
				if (checkNetDevHaveRegistered(tmpNet) == false) {
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
			
			Log.d(TAG, "NetManager -> unregister net device...");
			sp<NetDev> tmpNet;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			if (checkNetDevHaveRegistered(tmpNet) == true) {
				/* 从注册列表中移除该网络设备 */
				removeNetDev(tmpNet);
			} else {
				Log.e(TAG, "NetManager: netdev [%s] not registered yet", tmpNet->getDevName().c_str());
			}
			break;
		}


		case NETM_STARTUP_NETDEV: {		/* 启动网络设备 */
			
			sp<NetDev> tmpNet;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			if (checkNetDevHaveRegistered(tmpNet) == true) {
				if (tmpNet->getNetDevActiveState() == true) {
					Log.d(TAG, "NetManager: netdev [%s] have actived, ignore this command", tmpNet->getDevName());
				} else {
					if (tmpNet->netdevOpen() == 0) {
						tmpNet->setNetDevActiveState(true);
					} else {
						Log.d(TAG, "NetManager: netdev[%s] active failed...", tmpNet->getDevName());
					}
				}
			}
			break;
		}


		case NETM_CLOSE_NETDEV: {		/* 关闭网络设备 */
			
			sp<NetDev> tmpNet;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);

			if (checkNetDevHaveRegistered(tmpNet) == true) {
				if (tmpNet->getNetDevActiveState() == false) {
					Log.d(TAG, "NetManager: netdev [%s] have inactived, ignore this command", tmpNet->getDevName());
				} else {
					if (tmpNet->netdevClose() == 0) {
						tmpNet->setNetDevActiveState(false);
					} else {
						Log.d(TAG, "NetManager: netdev[%s] inactive failed...", tmpNet->getDevName());
					}
				}
			}
			break;
		}


		case NETM_SET_NETDEV_IP: {	/* 设备设备IP地址(DHCP/static) */
			#if 0
			/* 根据是全局开关是DHCP还是Static */		
			sp<NetDev> tmpNet;
			uint32_t ipaddr;
			CHECK_EQ(msg->find<sp<NetDev>>("netdev", &tmpNet), true);
			CHECK_EQ(msg->find<uint32_t>("ip", &ipaddr), true);

			if (checkNetDevHaveRegistered(tmpNet) == true) {
				if (1) {	/* DHCP */
					//tmpNet->setNetDevIpByDhcp();
				} else {	/* Static */
					tmpNet->setNetdevIp(ipaddr);
				}
			} else {
				Log.e(TAG, "NetManager: netdev[%s] not reigstered...", tmpNet->getDevName());
			}
			#endif
			break;
		}


		case NETM_LIST_NETDEV: {
			sp<NetDev> tmpDev;
			
			for (uint32_t i = 0; i < mDevList.size(); i++) {
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
			Log.d(TAG, "NetManager: netmanager exit loop...");
			mLooper->quit();
			break;
		}


		default:
			Log.d(TAG, "NetManager: Unsupport Message recieve");
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
	uint32_t i;
	int ret = 0;
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
		ret = -1;
	}

	return ret;
}

void NetManager::unregisterNetDev(sp<NetDev>& netDev)
{
	sp<NetDev> tmpDev = netDev;
	
    unique_lock<mutex> lock(mMutex);
    for (uint32_t i = 0; i < mDevList.size(); i++) {
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


sp<NetDev>& NetManager::getNetDevByname(std::string& devName)
{	
	sp<NetDev> tmpDev = nullptr;

    unique_lock<mutex> lock(mMutex);
    for (uint32_t i = 0; i < mDevList.size(); i++) {
        if (mDevList.at(i)->getDevName() == devName) {
			tmpDev = mDevList.at(i);
			break;
		}
    }
	return tmpDev;
}


void NetManager::postNetMessage(sp<ARMessage>& msg, int interval)
{
	msg->setHandler(mHandler);
	msg->postWithDelayMs(interval);
}


NetManager::NetManager(): mState(NET_MANAGER_STAT_INIT), 
							  mCurdev(NULL), 
							  mExit(false)
{
    Log.d(TAG, "construct NetManager....");
	
	mPollMsg = obtainMessage(NETM_POLL_NET_STATE);
	mDevList.clear();
}


NetManager::~NetManager()
{
    Log.d(TAG, "deconstruct NetManager....");

    /* stop all net devices */

    /* unregister all net devices */

    mState = NET_MANAGER_STAT_DESTORYED;
}

