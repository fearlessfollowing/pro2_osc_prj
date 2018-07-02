#ifndef PROJECT_NET_MANAGER_H
#define PROJECT_NET_MANAGER_H

#include <mutex>
#include <common/sp.h>
struct net_dev_info;


enum {
	NET_LINK_CONNECT = 0x10,
	NET_LINK_DISCONNECT,
	NET_LINK_MAX,
};

class net_manager {
public:
    explicit net_manager();
    ~net_manager();
    bool check_net_change(sp<net_dev_info> &new_dev_info);

private:
    void init();
    void deinit();
    void get_dev_name(int dev_type, char *dev_name);
	int get_net_link_state(int dev_type);
	unsigned int get_ipaddr_by_dev_type(int dev_type);
    std::mutex mMutex;
    sp<net_dev_info> org_dev_info;
};


#endif /* PROJECT_NET_MANAGER_H */
