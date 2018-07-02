#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

#include <common/include_common.h>
#include <sys/net_manager.h>
#include <trans/fifo_struct.h>
#include <util/bytes_int_convert.h>

using namespace std;

enum {

#ifdef WLAN_PRIOR
    DEV_WLAN,
    DEV_LAN,
#else
    DEV_LAN,
    DEV_WLAN,
#endif

    DEV_4G,
    DEV_MAX
};

#define TAG "net_manager"


struct ethtool_value {
    __uint32_t cmd;
    __uint32_t data;
};



//#define ENABLE_IP_DEBUG
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
    //set net_link -1 to force update while first time
    org_dev_info = sp<net_dev_info>(new net_dev_info{0, -1, 0});
}

void net_manager::deinit()
{

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

