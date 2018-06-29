//
// Created by vans on 16-12-7.
//
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
//#define WLAN_PRIOR

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
    org_dev_info = sp<net_dev_info>(new net_dev_info{0,-1,0});
}

void net_manager::deinit()
{

}

bool net_manager::check_net_change(sp<net_dev_info> &new_dev_info)
{
    bool bUpdate = false;
//    int link = 0;
    bool bFoundIP = false;

    sp<net_dev_info> found_dev = sp<net_dev_info>(new net_dev_info());

	for (int i = 0; i< DEV_MAX; i++)
    {
        if (GSNet_CheckLinkByType(i) == 1)
        {
            unsigned int addr = GSNet_GetIPInfo(i);
            if (addr != 0)
            {
                found_dev->dev_type = i;
                found_dev->dev_addr = addr;
                found_dev->net_link = 1;
                bFoundIP = true;
                break;
            }
        }
    }

    if (bFoundIP)
    {
#ifdef ENABLE_IP_DEBUG
        u8 ip[32];
        int_to_ip(found_dev->dev_addr,ip,sizeof(ip));
#endif
        if( found_dev->dev_addr == org_dev_info->dev_addr &&
                found_dev->dev_type == org_dev_info->dev_type &&
                found_dev->net_link == org_dev_info->net_link)
        {
#ifdef ENABLE_IP_DEBUG
            Log.d(TAG, "met same addr type %d %s", found_dev->dev_type, ip);
#endif
            //still send disp for wlan still error
            bUpdate = true;
        }
        else
        {
#ifdef ENABLE_IP_DEBUG
            Log.d(TAG, "found new addr type %d %s", found_dev->dev_type, ip);
#endif
//            org_dev_info->dev_addr = found_dev->dev_addr;
//            org_dev_info->dev_type = found_dev->dev_type;
//            org_dev_info->net_link = found_dev->net_link;
            memcpy(org_dev_info.get(),found_dev.get(),sizeof(net_dev_info));
            bUpdate = true;
        }
    }
    else
    {
        //first check or net link disconnect
        if (org_dev_info->dev_addr != 0 || org_dev_info->net_link == -1)
        {
            // net from connect to disconnect
            bUpdate = true;
            org_dev_info->dev_addr = 0;
            org_dev_info->net_link = GSNet_CheckLinkByType(org_dev_info->dev_type);
        }
    }

    if (bUpdate)
    {
	#ifdef ENABLE_IP_DEBUG
        u8 ip[32];
        int_to_ip(org_dev_info->dev_addr,ip,sizeof(ip));
        Log.d(TAG," net update %d %s", org_dev_info->dev_type, ip);
	#endif
        memcpy(new_dev_info.get(),org_dev_info.get(),sizeof(net_dev_info));
    }

    {
        u8 org_ip[32];
        int_to_ip(found_dev->dev_addr,org_ip,sizeof(org_ip));
    }

    return bUpdate;
}

void net_manager::get_dev_name(int dev_type, char *dev_name)
{
    switch (dev_type)
    {
        case DEV_LAN:
            sprintf(dev_name,"eth%d",0);
            break;
		
        case DEV_WLAN:
            sprintf(dev_name,"%s","wlan0");
            break;
		
        case DEV_4G:
            sprintf(dev_name,"ppp%d",0);
            break;
		
        default:
            Log.e(TAG,"error net dev %d", dev_type);
            break;
    }
}

int net_manager::GSNet_CheckLinkByType(int eDeviceType)
{
    int   skfd = -1;
//	int i,mii_val[32];
//	int   bmsr;
    struct   ifreq   ifr;
    int istatus = 0;
	
//	if(iInitLinkCheck == 1)
//	{
//		GSOS_MutexCreate(&link_mutex);
//		iInitLinkCheck = 0;
//	}
    memset((u8 *)&ifr,0,sizeof(struct ifreq));
    get_dev_name(eDeviceType,ifr.ifr_name);
    if (strlen(ifr.ifr_name) == 0)
    {
        goto RET_VALUE;
    }
	
    if (skfd == -1)
    {
        /*   Open   a   basic   socket.   */
        if((skfd = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))   <   0)
        {
            goto RET_VALUE;
        }
    }
	
    if (ioctl(skfd, SIOCGIFFLAGS, &ifr)   <   0)//no device or device down
    {
//        printf("SIOCGMIIPHY----   on   '%s'   failed:   %s\n",
//               ifr.ifr_name,   strerror(errno));
        goto RET_VALUE;
    }
#if 0
    /*   Some   bits   in   the   BMSR   are   latched,   but   we   can't   rely   on   being
		the   only   reader,   so   only   the   current   values   are   meaningful   */
	mdio_read(skfd, &ifr,  MII_BMSR);
    for (i = 0; i < 8; i++) {
		mii_val[i]   =   mdio_read(skfd, &ifr,  i);
	}

	/*   Descriptive   rename.   */
	bmsr   =   mii_val[MII_BMSR];
	istatus = ((bmsr   &   MII_BMSR_LINK_VALID)==MII_BMSR_LINK_VALID);
#else
    istatus = (ifr.ifr_flags & IFF_RUNNING) == IFF_RUNNING;
#endif

//	printf("####%s     %s\n",   ifr.ifr_name,   istatus ?   "link   ok"   :   "no   link");
//	printf("other way get status=%d\n",interface_detect_beat_ethtool(skfd,ifr.ifr_name));
    //	close(skfd);
RET_VALUE:

    if (skfd != -1) {
        close(skfd);
    }
    return istatus;
}

unsigned int net_manager::GSNet_GetIPInfo(int dev_type)
{
    unsigned int	ucA0 = 0, ucA1 = 0, ucA2 = 0, ucA3 = 0;
    int   skfd = -1;
    struct ifreq ifr;
    char acAddr[64];
    struct sockaddr_in *addr;
    unsigned int ip = 0;
    get_dev_name(dev_type,ifr.ifr_name);

    if (strlen(ifr.ifr_name) == 0) {
        return 0;
    }

    if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
        return 0;
    }

    if (ioctl(skfd,SIOCGIFADDR,&ifr) == -1) {
        close(skfd);
        return 0;
    }
	
    close(skfd);
    addr = (struct sockaddr_in *)(&ifr.ifr_addr);
    strcpy(acAddr, inet_ntoa(addr->sin_addr));
	
//    Log.d(TAG,"get ip dev %s ip %s",ifr.ifr_name,acAddr);
    if (4 == sscanf(acAddr, "%u.%u.%u.%u", &ucA0, &ucA1, &ucA2, &ucA3)) {
        ip = (ucA0 << 24)| (ucA1 << 16)|(ucA2 << 8)| ucA3;
    }
	
    return ip;
}

