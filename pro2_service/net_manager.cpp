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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <prop_cfg.h>

#include <trans/fifo.h>

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


/*********************************** NetDev **********************************/
NetDev::NetDev(int iType, int iWkMode, int iState, bool activeFlag, string ifName):
		mDevType(iType),
		mWorkMode(iWkMode),	
		mLinkState(iState),
		mActive(activeFlag),
        mDevName(ifName)
        //mHaveCachedDhcp(false)

{
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    memset(mSaveIpAddr, 0, sizeof(mSaveIpAddr));
    //memset(mCachedDhcpAddr, 0, sizeof(mCachedDhcpAddr));

    Log.d(TAG, "++> constructor net device");
}

NetDev::~NetDev()
{
    Log.d(TAG, "++> deconstructor net device");
}


void NetDev::flushDhcpAddr()
{
    memset(mCachedDhcpAddr, 0, sizeof(mCachedDhcpAddr));
    mHaveCachedDhcp = false;
}

bool NetDev::isCachedDhcpAddr()
{
    return mHaveCachedDhcp;
}

const char* NetDev::getCachedDhcpAddr()
{
    return mCachedDhcpAddr;
}

void NetDev::setCachedDhcpAddr(const char* ipAddr)
{
    memset(mCachedDhcpAddr, 0, sizeof(mCachedDhcpAddr));
    strcpy(mCachedDhcpAddr, ipAddr);
    mHaveCachedDhcp = true;
}

int NetDev::getNetDevType()
{
    return mDevType;
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

const char* NetDev::getCurIpAddr()
{
    return mCurIpAddr;
}

const char* NetDev::getSaveIpAddr()
{
    return mSaveIpAddr;
}


void NetDev::storeCurIp2Saved()
{
    memset(mSaveIpAddr, 0, sizeof(mSaveIpAddr));
    strcpy(mSaveIpAddr, mCurIpAddr);
}

void NetDev::resumeSavedIp2CurAndPhy(bool bUpPhy)
{
    Log.d(TAG, "saved ip: %s", mSaveIpAddr);
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    strcpy(mCurIpAddr, mSaveIpAddr);
    if (bUpPhy) {
        setNetDevIp2Phy(mCurIpAddr);
    }
}


void NetDev::setCurIpAddr(const char* ip, bool bUpPhy = true)
{
    memset(mCurIpAddr, 0, sizeof(mCurIpAddr));
    strcpy(mCurIpAddr, ip);
    if (bUpPhy) {
        setNetDevIp2Phy(mCurIpAddr);
    }
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
        Log.i(TAG, "Link detected: %d\n", edata.data);

        if (edata.data == 1) {
            err = NET_LINK_CONNECT;
        } else {
            err = NET_LINK_DISCONNECT;
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


const char* NetDev::getNetDevIpFrmPhy()
{
    int skfd = -1;
    struct ifreq ifr;
    struct sockaddr_in *addr;

    uint32_t ip = 0;

    strcpy(ifr.ifr_name, mDevName.c_str());


    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        goto ERR;
    }

    if (ioctl(skfd, SIOCGIFADDR, &ifr) == -1) {
        close(skfd);
        goto ERR;
    }

    close(skfd);
    addr = (struct sockaddr_in *)(&ifr.ifr_addr);

    return inet_ntoa(addr->sin_addr);

ERR:
    return NULL;
}


bool NetDev::setNetDevIp2Phy(const char* ip)
{
    bool ret = false;
    struct ifreq ifr;
    struct sockaddr_in *p = NULL;

    memset(&ifr, 0, sizeof(ifr));

    p = (struct sockaddr_in *)&(ifr.ifr_addr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        Log.e(TAG, "open socket failed....");
    } else {
        strcpy(ifr.ifr_name, getDevName().c_str());
        p->sin_family = AF_INET;

        inet_aton(ip, &(p->sin_addr));

        if (ioctl(sockfd, SIOCSIFADDR, &ifr)) {
            Log.e(TAG, "setNetDevIp2Phy -> [%s:%s] failed", getDevName().c_str(), ip);
        } else {
            Log.d(TAG, "setNetDevIp2Phy -> [%s:%s] Success", getDevName().c_str(), ip);

            ifr.ifr_flags |= IFF_UP;
            if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) == -1)
                   Log.d(TAG, "setNetDevIp2Phy FAILED");
               else
                   Log.d(TAG, "setNetDevIp2Phy OK");

            ret = true;

        }
        close(sockfd);
    }
    return ret;

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


void NetDev::getIpByDhcp()
{
    char cmd[512] = {0};

    system("killall dhclient");
    sprintf(cmd, "dhclient %s &", mDevName.c_str());
    system(cmd);
}


void NetDev::processPollEvent(sp<NetDev>& netdev)
{
    Log.d(TAG, "Netdev -> processPollEvent");
}


void NetDev::postDevInfo2Ui()
{
    sp<DEV_IP_INFO> pInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    strcpy(pInfo->cDevName, getDevName().c_str());
    strcpy(pInfo->ipAddr, getCurIpAddr());

    sp<ARMessage> msg = (sp<ARMessage>)(new ARMessage(OLED_DISP_IP));
    msg->set<sp<DEV_IP_INFO>>("info", pInfo);

    NetManager::getNetManagerInstance()->sendIpInfo2Ui(msg);
}


/************************************* Ethernet Dev ***************************************/

EtherNetDev::EtherNetDev(string name):NetDev(DEV_LAN, WIFI_WORK_MODE_STA, NET_LINK_DISCONNECT, true, name)
{
    Log.d(TAG, "constructor ethernet device...");
}

EtherNetDev::~EtherNetDev()
{
    Log.d(TAG, "deconstructor ethernet device...");
}


int EtherNetDev::netdevOpen()
{
    char cmd[512] = {0};

    Log.d(TAG, "EtherNetdev Open ....");
    sprintf(cmd, "ifconfig %s up", getDevName().c_str());
    system(cmd);

	return 0;
}

int EtherNetDev::netdevClose()
{
    char cmd[512] = {0};

    Log.d(TAG, "Ethernetdev Close...");
    sprintf(cmd, "ifconfig %s down", getDevName().c_str());
    system(cmd);

	return 0;
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
void EtherNetDev::processPollEvent(sp<NetDev>& etherDev)
{

    int iCurLinkState = etherDev->getNetdevLinkFrmPhy();

    Log.i(TAG, "ethernet current state: %s", (iCurLinkState == NET_LINK_CONNECT) ? "Connect": "Disconnect");

    if (etherDev->getNetdevSavedLink() != iCurLinkState) {	/* 链路发生变化 */

        Log.d(TAG, "NetManger: netdev[%s] link state changed", etherDev->getDevName().c_str());

        if (iCurLinkState == NET_LINK_CONNECT) {	/* Disconnect -> Connect */
            Log.i(TAG, "++++>>> link connect");


            Log.d(TAG, "current ip [%s], saved ip [%s]", etherDev->getCurIpAddr(), etherDev->getSaveIpAddr());
            int iCurIpLen = strlen(etherDev->getCurIpAddr());
            int iSaveIpLen = strlen(etherDev->getSaveIpAddr());

            /* 只有构造设备时会将mCurIpAddr与mSavedIpAdrr设置为"0" */
            if ((iCurIpLen == iSaveIpLen) && !strcmp(etherDev->getCurIpAddr(), etherDev->getSaveIpAddr())) {
                if (0) {    /* Static */
                    etherDev->setCurIpAddr(DEFAULT_ETH0_IP, true);
                } else {    /* DHCP */
                    etherDev->setCurIpAddr("0.0.0.0", true);    /* 避免屏幕没有任何显示: 0.0.0.0 */
                    etherDev->getIpByDhcp();
                }
            } else {	/* mCurIpAddr != mSaveIpAddr */
                Log.d(TAG, ">>>> Resume mSaveIpAddr to mCurIpAddr");
                etherDev->resumeSavedIp2CurAndPhy(true);
            }
            /* 将地址设置到网卡,并将地址发送给UI */
            Log.d(TAG, ">>>> send our ip to UI thread ....");
            etherDev->postDevInfo2Ui();

        } else {	/* Connect -> Disconnect */
            Log.i(TAG, "++++>>> link disconnect");

            etherDev->storeCurIp2Saved();
            etherDev->setCurIpAddr("0.0.0.0", true);

            /* 发送0.0.0.0到UI */
            Log.d(TAG, ">>>> send our ip(0.0.0.0) to UI thread .... ");
            etherDev->postDevInfo2Ui();
        }

        etherDev->setNetdevSavedLink(iCurLinkState);
    } else {	/* 链路未发生变化 */
        /* IP发生变化
         * Static - 链路未发生变化时,IP地址不会发生变化
         * DHCP   - 可能IP地址会发生变化
         */
        if (etherDev->getNetdevSavedLink() == NET_LINK_CONNECT) {   /* Connected */
            Log.d(TAG, "+++++>>> link not changed(Connected), check ip haved changed ....");
            if (etherDev->getNetDevIpFrmPhy() && strcmp(etherDev->getNetDevIpFrmPhy(), etherDev->getCurIpAddr())) {  /* Ip changed */

                Log.d(TAG, ">>>> send our ip(%s) to UI thread .... ", etherDev->getNetDevIpFrmPhy());
#if 0
                if (etherDev->isCachedDhcpAddr() == false || strcmp(etherDev->getCachedDhcpAddr(), etherDev->getNetDevIpFrmPhy())) {
                    etherDev->setCachedDhcpAddr(etherDev->getNetDevIpFrmPhy());
                }
#endif
                etherDev->setCurIpAddr(etherDev->getNetDevIpFrmPhy(), false);
                etherDev->postDevInfo2Ui();
            }
        } else {
            Log.i(TAG, "+++++>>> link not changed(Disconnected), do nothing.");
        }
    }
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


void WiFiNetDev::processPollEvent(sp<NetDev>& wifiDev)
{

}




class NetManagerHandler : public ARHandler {
public:
    NetManagerHandler(NetManager *source): mNetManager(source) {
    }

    virtual ~NetManagerHandler() override {
    }

    virtual void handleMessage(const sp<ARMessage> & msg) override {
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
    if (mPollMsg)
        postNetMessage(mPollMsg, NET_POLL_INTERVAL);
}



string NetManager::convWhat2Msg(uint32_t what)
{
    string msg;
    switch (what) {
        case NETM_POLL_NET_STATE:
            msg = "NETM_POLL_NET_STATE";
            break;

        case NETM_REGISTER_NETDEV:
            msg = "NETM_REGISTER_NETDEV";
            break;

        case NETM_UNREGISTER_NETDEV:
            msg = "NETM_UNREGISTER_NETDEV";
            break;

        case NETM_STARTUP_NETDEV:
            msg = "NETM_STARTUP_NETDEV";
            break;

        case NETM_CLOSE_NETDEV:
            msg = "NETM_CLOSE_NETDEV";
            break;

        case NETM_SET_NETDEV_IP:
            msg = "NETM_SET_NETDEV_IP";
            break;

        case NETM_LIST_NETDEV:
            msg = "NETM_LIST_NETDEV";
            break;

        case NETM_EXIT_LOOP:
            msg = "NETM_EXIT_LOOP";
            break;

        default:
            msg = "Unkown Msg";
            break;
    }

    return msg;
}



void NetManager::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	
    Log.d(TAG, "NetManager get msg what %s", convWhat2Msg(what).c_str());

	switch (what) {
		case NETM_POLL_NET_STATE: {		/* 轮询网络设备的状态 */
			/* 检查系统中所有处于激活状态的网卡
			 * 如果链路发生变化
			 * 如果IP发生变化
			 */
			
            if (!mPollMsg) {
                mPollMsg = msg->dup();
            }

			vector<sp<NetDev>>::iterator itor;
			vector<sp<NetDev>> tmpList;
            sp<NetDev> tmpDev;

			tmpList.clear();
			
			for (itor = mDevList.begin(); itor != mDevList.end(); itor++) {
				if ((*itor)->getNetDevActiveState()) {
					tmpList.push_back(*itor);
				}
			}

            for (uint32_t i = 0; i < tmpList.size(); i++) {
                tmpDev = tmpList.at(i);
                tmpDev->processPollEvent(tmpDev);
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


        /* NET_IP_INFO(name, ipaddr)
         *
         */
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
            #else
            Log.d(TAG, "______=+++++++++++++++ set ip>>>>");
            sp<DEV_IP_INFO> tmpIpInfo = NULL;
            sp<NetDev> tmpNetDev = NULL;
            CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);

            Log.d(TAG, "net [%s], ip %s, mode =%d, type = %d",tmpIpInfo->cDevName, tmpIpInfo->ipAddr, tmpIpInfo->iDhcp, tmpIpInfo->iDevType);
#if 1
            /* get netdev used dev name */
            tmpNetDev = getNetDevByType(tmpIpInfo->iDevType);
            if (tmpNetDev) {
                if (tmpIpInfo->iDhcp) { /* DHCP */
                    /* Need set our ip 0.0.0.0 first??? */
                    tmpNetDev->getIpByDhcp();
                } else {    /* Static */
                    tmpNetDev->setCurIpAddr("192.168.3.13", true);
                }
                Log.d(TAG, ">>>>>>> get device ");
            }
          #endif
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

sp<NetDev>& NetManager::getNetDevByType(int iType)
{
    uint32_t i;
    {
        unique_lock<mutex> lock(mMutex);
        for (uint32_t i = 0; i < mDevList.size(); i++) {
            Log.d(TAG, "dev name: %s", mDevList.at(i)->getDevName().c_str());
            if (mDevList.at(i)->getNetDevType() == iType) {
                return mDevList.at(i);
            }
        }
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
    return mDevList.size();
}


sp<NetDev>& NetManager::getNetDevByname(const char* devName)
{	
    uint32_t i;
    {
        unique_lock<mutex> lock(mMutex);
        for (i = 0; i < mDevList.size(); i++) {
            Log.d(TAG, "dev name: %s", mDevList.at(i)->getDevName().c_str());
            if (!strncmp(mDevList.at(i)->getDevName().c_str(), devName, strlen(devName))) {
                return mDevList.at(i);
            }
        }
    }
}


void NetManager::postNetMessage(sp<ARMessage>& msg, int interval)
{
	msg->setHandler(mHandler);
	msg->postWithDelayMs(interval);
}


void NetManager::sendIpInfo2Ui(sp<ARMessage>& msg)
{
    /* Get Global UI object */
    Log.d(TAG, "NetManager: send ip info to ui");
    fifo::getSysTranObj()->sendUiMessage(msg);
}


NetManager::NetManager(): mState(NET_MANAGER_STAT_INIT), 
							  mCurdev(NULL), 
                              mPollMsg(NULL),
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

