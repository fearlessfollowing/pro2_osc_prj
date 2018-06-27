//
// Created by vans on 16-12-2.
//

#ifndef INC_360PRO_SERVICE_FIFO_H
#define INC_360PRO_SERVICE_FIFO_H

#include <hw/InputManager.h>
#include <common/sp.h>

class ARLooper;
class ARHandler;
class ARMessage;
class oled_handler;

//class poll_timer;
//class usb_hp;
//class dev_manager;

struct _disp_type_;
class fan_control;
struct _sys_info_;
struct _sync_init_info_;

class fifo
{
public:
    explicit fifo();
    ~fifo();
    void start_all();
    void stop_all(bool delay = true);
    void handleMessage(const std::shared_ptr<ARMessage> &msg);
private:
    sp<ARLooper> mLooper;
    sp<ARHandler> mHandler;
    std::thread th_msg_;
    std::thread th_read_fifo_;
    int write_fd = -1;
    int read_fd = -1;
    void init();
    void deinit();
    int make_fifo();
    void init_thread();
    void sendExit();
//    void req_sync_state();
//    void send_dev_manager_scan();
    sp<ARMessage> obtainMessage(uint32_t what);
    void handle_poll_change(const sp<ARMessage> &msg);
    void handle_oled_notify(const sp<ARMessage> &msg);
    int get_write_fd();
    int get_read_fd();
    void close_read_fd();
    void close_write_fd();
    void read_fifo_thread();
    void write_exit_for_read();
    void write_fifo(int iEvent , char *str = nullptr);
    void send_disp_str_type(sp<struct _disp_type_> &dis_type);
    void send_wifi_config(const char *ssid, const char *pwd,int open = 1);
    void send_err_type_code(int type, int code);
    void send_power_off();
    void send_sys_info(sp<struct _sys_info_> &mSysInfo);
    void send_sync_info(sp<struct _sync_init_info_> &mSyncInfo);
    sp<oled_handler> mOLEDHandle;

	sp<InputManager> mInputManager;
	
//    sp<poll_timer> mpPollTimer;
//    sp<dev_manager> mDevManager;
//    sp<fan_control> mFanControl;

//    bool bRFifoStop = false;
    bool bWFifoStop = false;
    //msg thread
    bool bExit = false;
    bool bReadThread = false;
};

#endif //INC_360PRO_SERVICE_FIFO_H
