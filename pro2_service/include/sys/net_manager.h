//
// Created by vans on 16-12-7.
//

#ifndef PROJECT_NET_MANAGER_H
#define PROJECT_NET_MANAGER_H
#include <mutex>
#include <common/sp.h>
struct net_dev_info;
class net_manager
{
public:
    explicit net_manager();
    ~net_manager();
    bool check_net_change(sp<net_dev_info> &new_dev_info);
private:
    void init();
    void deinit();
    void get_dev_name(int dev_type, char *dev_name);
    int GSNet_CheckLinkByType(int eDeviceType);
    unsigned int GSNet_GetIPInfo(int dev_type);
//    sp<net_dev_info> mDevInfo[DEV_MAX];
    //lan ,wlan or 4g
//    int cur_dev = -1;
    std::mutex mMutex;
//    unsigned int org_ip = 0;
//    unsigned int org_type = 0;

    sp<net_dev_info> org_dev_info;
};


#endif //PROJECT_NET_MANAGER_H
