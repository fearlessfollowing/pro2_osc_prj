//
// Created by vans on 16-12-6.
//
#if 0
#include "include_common.h"
#include "poll_timer.h"
#include "net_manager.h"
#include "msg_util.h"
#include "fifo_struct.h"
#include "ARMessage.h"

using namespace std;
#define TAG "poll_timer"

poll_timer::poll_timer(const sp<ARMessage> &mNotifyMsg):mNotify(mNotifyMsg)
{
    init();
}

poll_timer::~poll_timer()
{
    deinit();
}

void poll_timer::init()
{
    mpNetManager = sp<net_manager>(new net_manager());
}

void poll_timer::start_timer_thread()
{
    th_timer_ = thread([this]
                       {
                           timer_thread();
                       });
}

void poll_timer::stop_timer_thread()
{
//    Log.d(TAG,"stop_timer_thread bExitTimer %d",bExitTimer);
    if(!bExitTimer)
    {
        bExitTimer = true;
        if (th_timer_.joinable())
        {
            th_timer_.join();
        }
    }
//    Log.d(TAG,"stop_timer_thread bExitTimer %d over",bExitTimer);
}

void poll_timer::check_net_status()
{
    sp<net_dev_info> mpDevInfo = sp<net_dev_info>(new net_dev_info());
    if(mpNetManager->check_net_change(mpDevInfo))
    {
//        Log.d(TAG,"net dev info addr 0x%p"
//                      " mNotify addr 0x%p",
//              mpDevInfo.get(),mNotify.get());
        sp<ARMessage> msg = mNotify->dup();
        //update net in oled
        msg->set<int>("what", UPDATE_NET);
        msg->set<sp<net_dev_info>>("net_dev_info",mpDevInfo);
        msg->post();
    }
}

void poll_timer::timer_thread()
{
    while(!bExitTimer)
    {
        //remove battery temporally
        check_net_status();
        msg_util::sleep_ms(1 * 1000);
    }
}

void poll_timer::deinit()
{
    stop_timer_thread();
}
#endif