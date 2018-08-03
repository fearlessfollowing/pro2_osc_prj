#ifndef INC_360PRO_SERVICE_FIFO_H
#define INC_360PRO_SERVICE_FIFO_H

#include <hw/MenuUI.h>
#include <common/sp.h>
#include <util/cJSON.h>

class ARLooper;
class ARHandler;
class ARMessage;
class oled_handler;


struct _disp_type_;
class fan_control;
struct _sys_info_;
struct _sync_init_info_;


#define  TAG "fifo"

#define  FIFO_HEAD_LEN (8)
#define  FIFO_DATA_LEN_OFF (FIFO_HEAD_LEN - 4)


/*
 * FIFO与UI线程交互的消息
 */
enum {
//    MSG_POLL_TIMER,
    MSG_GET_OLED_KEY,
    MSG_DEV_NOTIFY,

//    MSG_DISP_STR,
    MSG_DISP_STR_TYPE,
    MSG_DISP_ERR_TYPE,

    //    MSG_DISP_EXT,
    MSG_SET_WIFI_CONFIG,
    MSG_SET_SYS_INFO,
    MSG_SET_SYNC_INFO,

    MSG_START_POWER_OFF,

    // while boot with usb inserted
    MSG_INIT_SCAN,

    MSG_TRAN_INNER_UPDATE_TF,
    MSG_EXIT,
};


/*
 * 接收来自Http服务器的消息
 */
enum {
    CMD_OLED_DISP_TYPE = 0,
    CMD_OLED_SYNC_INIT = 1,

    CMD_OLED_DISP_TYPE_ERR = 16,

    CMD_OLED_POWER_OFF = 17,

    CMD_OLED_SET_SN = 18,

    CMD_CONFIG_WIFI = 19,
    //clear all camera state
    
	CMD_EXIT = 20,
    
    CMD_WEB_UI_TF_NOTIFY = 30,

};


//send to controller
enum {
    EVENT_BATTERY = 0,
    EVENT_NET_CHANGE = 1,
    EVENT_OLED_KEY = 2,
    EVENT_DEV_NOTIFY = 3,
    EVENT_SAVE_PATH = 4,
    EVENT_AGEING_TEST = 5,
    EVENT_QUERY_STORAGE = 6,
};


typedef struct _res_info_ {
    int w;
    int h[2];
} RES_INFO;

typedef struct _qr_info_ {
    int qr_size;
    int org_size;
    int stich_size;
    int hdr_size;
    int burst_size;
    int timelap_size;
} QR_INFO;

typedef struct _qr_struct_ {
    int version;
    QR_INFO astQRInfo[3];
} QR_STRUCT;


class fifo {
public:
    ~fifo();

    void start_all();
    void stop_all(bool delay = true);
    void handleMessage(const std::shared_ptr<ARMessage> &msg);

    static sp<fifo> getSysTranObj();

    void postTranMessage(sp<ARMessage>& msg, int interval = 0);

    //sp<ARMessage> dupMessage();

    void sendUiMessage(sp<ARMessage>& msg);

private:

    fifo();

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

    sp<ARMessage> notify;

    sp<ARMessage> obtainMessage(uint32_t what);
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

    void handleStitchProgress(sp<struct _disp_type_>& mDispType, cJSON *subNode);
    void handleSetting(sp<struct _disp_type_>& mDispType, cJSON *subNode);
    void handleReqFormHttp(sp<struct _disp_type_>& mDispType, cJSON *root, cJSON *subNode);
    void handleQrContent(sp<DISP_TYPE>& mDispType, cJSON* root, cJSON *subNode);
    
    void handleUiTakeVidReq(sp<ACTION_INFO>& mActInfo, cJSON *root, cJSON *param);
    void handleUiTakePicReq(sp<ACTION_INFO>& mActInfo, cJSON* root, cJSON *param);
    void handleUiTakeLiveReq(sp<ACTION_INFO>& mActInfo, cJSON *root, cJSON *param);


    void handleUiReqWithNoAction(cJSON *root, int action, const sp<ARMessage>& msg);

    sp<MenuUI> mOLEDHandle;
	
    bool bWFifoStop = false;

    //msg thread
    bool bExit = false;
    bool bReadThread = false;
};

#endif //INC_360PRO_SERVICE_FIFO_H
