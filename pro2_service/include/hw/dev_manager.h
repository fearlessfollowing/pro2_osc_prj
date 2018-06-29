//
// Created by vans on 16-12-23.
//

#ifndef PROJECT_DEV_MANAGER_H
#define PROJECT_DEV_MANAGER_H
#include <thread>
#include <vector>
#include <common/sp.h>
#include <hw/usb_dev_info.h>

struct net_link_info;
class ARMessage;

enum
{
    SET_STORAGE_SD,
    SET_STORAGE_USB,
    SET_STORAGE_MAX
};

enum
{
    ADD,
    REMOVE,
    CHANGE,
};

class dev_manager
{
public:
    explicit dev_manager(const sp<ARMessage> &notify);
    ~dev_manager();
    bool start_scan(int action = CHANGE);
    int check_block_mount(char *dev);
private:

    void dev_change(int action = CHANGE);
    bool parseAsciiNetlinkMessage(char *buffer,int size);
    void uevent_detect_thread();
    void init();
    void start_detect_thread();
    void deinit();
    void stop_detect_thread();
    void handle_block_event(sp<struct net_link_info> &mLink);

    void send_notify_msg(std::vector<sp<USB_DEV_INFO>> &dev_list);
//    void add_internal_info(std::vector <sp<USB_DEV_INFO>> &mList);
    std::thread th_detect_;
    sp<ARMessage> mNotify;

    int mCtrlPipe[2];

    bool bThExit_ = false;
    unsigned int org_dev_count = 0;

};
#endif //PROJECT_DEV_MANAGER_H
