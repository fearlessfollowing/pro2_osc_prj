//
// Created by vans on 16-12-2.
//

#include <sys/stat.h>
#include <future>
#include <vector>
#include <common/include_common.h>
#include <trans/fifo.h>
#include <trans/fifo_event.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/common.h>
#include <util/cJSON.h>
#include <util/bytes_int_convert.h>
#include <hw/oled_handler.h>

//#include "poll_timer.h"
//#include "fifo_struct.h"
//#include "dev_manager.h"

#include <sys/action_info.h>
#include <hw/ins_gpio.h>
#include <log/arlog.h>

#include <system_properties.h>


using namespace std;

#define  FIFO_FROM_CLIENT	"/home/nvidia/insta360/fifo/fifo_read_client"
#define  FIFO_TO_CLIENT		"/home/nvidia/insta360/fifo/fifo_write_client"

#define  TAG "fifo"

#define  FIFO_HEAD_LEN (8)
#define  FIFO_DATA_LEN_OFF (FIFO_HEAD_LEN - 4)

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
    //while boot with usb inserted
    MSG_INIT_SCAN,
    MSG_EXIT,
};

//rec from controller
enum {
    CMD_OLED_DISP_TYPE,
    CMD_OLED_SYNC_INIT,
//    CMD_OLED_POWER_OFF = 17,

    CMD_OLED_DISP_TYPE_ERR = 16,
    CMD_OLED_SET_SN = 18,
    CMD_CONFIG_WIFI = 19,
    //clear all camera state
    //for kill self
            CMD_EXIT = 20
};

//send to controller
enum {
    EVENT_BATTERY = 0,
    EVENT_NET_CHANGE = 1,
    EVENT_OLED_KEY = 2,
    EVENT_DEV_NOTIFY = 3,
    EVENT_SAVE_PATH = 4,
    EVENT_AGEING_TEST = 5,
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
}QR_STRUCT;

static QR_STRUCT mQRInfo[] = {
	100,{{7,4,4,3,1,0},{6,6,5,0,0,1},{6,6,6}}
};

static const char *act_mode[] = {"3d_top_left", "pano"};
static const char *all_mime[] = {"h264", "h265","jpeg", "raw"};
static const char *all_map[] = {"flat","cube"};
//static const char *pic_mime[] = {};
static const int all_frs[] = {24,25,30,60,120};
//static const char *cal_mode[] = {"pano", "3d"};
//kbits/s
//static const int all_brs[] = {150000,100000,50000,40000,30000,25000,20000,10000,8000,5000,3000};
static sp<fifo> mpFIFO = nullptr;
static const int qr_array_len = 4;

static const RES_INFO mResInfos[] =
        {
                //sti
                {7680, {7680, 3840}},//0
                {5760, {5760, 2880}},
                {4096, {4096, 2048}},
                {3840, {3840, 1920}},
                {2880, {2880, 1440}},
                {1920, {1920, 960}}, //5
                {1440, {1440, 720}},
                //org
                {
                 4000, {3000, 3000}
                },
                {
                 3840, {2160, 2160}
                },
                {
                 2560, {1440, 1440}
                },
                {
                 1920, {1080, 1080} //10
                },
                {
                 3200, {2400, 2400}
                },
                {
                 2160, {1620, 1620}
                },
                {
                 1920, {1440, 1440}
                },
                {1280,{960,960}},
                {2160,{1080,1080}} //15
        };

//#ifdef __cplusplus
//extern "C" {  // 即使这是一个C++程序，下列这个函数的实现也要以C约定的风格来搞！
//#endif
////int send_usb_event(void *pV)
////{
////    return mpFIFO->send_usb_event(pV);
////}
//#ifdef __cplusplus
//}
//#endif

static int reboot_cmd = -1;

static void set_gpio_level(unsigned int gpio_num,int val)
{
    if (gpio_is_requested(gpio_num) != 1) {
        gpio_request(gpio_num);
    }
	
    gpio_direction_output(gpio_num, val);
    Log.d(TAG, "set_gpio_level gpio_num %d val %d", gpio_num, val);
}

void init_fifo()
{
    CHECK_EQ(mpFIFO, nullptr);
    mpFIFO = sp<fifo>(new fifo());
    CHECK_NE(mpFIFO, nullptr);
}

void start_all()
{
    if (nullptr != mpFIFO) {
        mpFIFO->start_all();
    }
}

void json_array_to_int_array()
{

}

//static void start_sys_reboot()
//{
//    CHECK_NE(mpFIFO, nullptr);
//    Log.d(TAG,"start_sys_reboot");
//    mpFIFO->stop_all();
//    //confirm all obj deinit
//    Log.d(TAG,"start_sys_reboot reboot_cmd %d",reboot_cmd);
//    switch(reboot_cmd)
//    {
//        case REBOOT_NORMAL:
//            exec_sh("setprop sys.powerctl reboot");
//            break;
//        case REBOOT_SHUTDOWN:
//            exec_sh("setprop sys.powerctl shutdown");
//            break;
//        SWITCH_DEF_ERROR(reboot_cmd);
//    }
//}
#define GET_CJSON_OBJ_ITEM_STR(child, root, key,str,size) \
    child = cJSON_GetObjectItem(root,key); \
    if(child)\
         snprintf(str,size, "%s",child->valuestring);

#define GET_CJSON_OBJ_ITEM_INT(child, root, key,val) \
    child = cJSON_GetObjectItem(root,key); \
    if(child) \
    {\
        val = child->valueint;\
    }

static int get_fr_index(int fr)
{
    int i;
    int max = sizeof(all_frs) / sizeof(all_frs[0]);
    for (i = 0; i < max; i++) {
        if (all_frs[i] == fr) {
            break;
        }
    }
	
    if (i == max) {
        Log.e(TAG, "fr index not found");
        i = 0;
    }
    return i;
}
// 0 --MODE_3D, 1-- MODE_STITCH
static int get_mode_index(char *mode)
{
    int iIndex;
    if(strcmp(mode,"pano") == 0)
    {
        iIndex = 1;
    }
    else
    {
        iIndex = 0;
    }
    return iIndex;
}

//static int get_pic_mime_index(char *mime)
//{
//    int i;
//    int max = sizeof(pic_mime) / sizeof(pic_mime[0]);
//    for (i = 0; i < max; i++)
//    {
//        if (strcmp(pic_mime[i], mime) == 0)
//        {
//            break;
//        }
//    }
//    if (i == max)
//    {
//        Log.e(TAG, "pic mime not found");
//        i = 0;
//    }
//    return i;
//}

static int get_sti_mode(char *map)
{
    int i;
    int max = sizeof(all_map) / sizeof(all_map[0]);
    for (i = 0; i < max; i++) {
        if (strcmp(all_map[i], map) == 0) {
            break;
        }
    }
	
    if (i == max) {
        Log.e(TAG, "map %s not found",map);
        i = 0;
    }
    return i;
}

static int get_mime_index(char *mime)
{
    int i;
    int max = sizeof(all_mime) / sizeof(all_mime[0]);
    for (i = 0; i < max; i++) {
        if (strcmp(all_mime[i], mime) == 0) {
            break;
        }
    }
	
    if (i == max) {
        Log.e(TAG, "mime %s not found",mime);
        i = 0;
    }
    return i;
}

class my_handler : public ARHandler
{
public:
    my_handler(fifo *source) : mHandler(source)
    {
    }

    virtual ~my_handler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override
    {
        mHandler->handleMessage(msg);
    }

private:
    fifo *mHandler;
};

fifo::fifo()
{
    init();
}

fifo::~fifo()
{
    deinit();
}

void fifo::init()
{
    CHECK_EQ(sizeof(mResInfos)/sizeof(mResInfos[0]),RES_MAX);
    CHECK_EQ(sizeof(all_frs)/sizeof(all_frs[0]),ALL_FR_MAX);
    make_fifo();
    init_thread();

//    sp<ARMessage> notification = obtainMessage(MSG_POLL_TIMER);
//    mpPollTimer = sp<poll_timer>(new poll_timer(notification));
//    CHECK_NE(mpPollTimer, nullptr);
//    sp<ARMessage> dev_notify = obtainMessage(MSG_DEV_NOTIFY);
//    mDevManager = sp<dev_manager>(new dev_manager(dev_notify));
//    Log.d(TAG,"start fan control init");
//    mFanControl = sp<fan_control>(new fan_control());

    //set in the end
    sp<ARMessage> notify = obtainMessage(MSG_GET_OLED_KEY);
    mOLEDHandle = sp<oled_handler>(new oled_handler(notify));	
    CHECK_NE(mOLEDHandle, nullptr);

	mInputManager = sp<InputManager>(new InputManager(mOLEDHandle));
    CHECK_NE(mInputManager, nullptr);
	
    //keep at end 0617 to rec fifo from python
    th_read_fifo_ = thread([this]
    {
       read_fifo_thread();
    });
}

//void fifo::req_sync_state()
//{
//    sp<ARMessage> notify = obtainMessage(MSG_GET_OLED_KEY);
//    notify->set<int>("what", oled_handler::OLED_KEY);
//    notify->set<int>("action", ACTION_REQ_SYNC);
//    notify->post();
//    Log.d(TAG, "req_sync_state");
//}

//void fifo::send_dev_manager_scan()
//{
//    sp<ARMessage> notify = obtainMessage(MSG_INIT_SCAN);
//    notify->post();
//}

void fifo::start_all()
{
//    if (mpPollTimer != nullptr)
//    {
//        mpPollTimer->start_timer_thread();
//    }
}

void fifo::stop_all(bool delay)
{
    Log.d(TAG, "stop_all");
//    if (mpPollTimer != nullptr)
//    {
//        mpPollTimer->stop_timer_thread();
//    }
//    mpPollTimer = nullptr;
//    mDevManager = nullptr;

    if (!bReadThread) {
        bReadThread = true;
        if (th_read_fifo_.joinable()) {
            write_exit_for_read();
            th_read_fifo_.join();
        }
    }
    mOLEDHandle = nullptr;
//    if(delay)
//    {
        msg_util::sleep_ms(200);
//    }
    Log.d(TAG, "stop_all3");
}

sp<ARMessage> fifo::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}

void fifo::sendExit()
{
    if (!bExit)
    {
        bExit = true;
        if (th_msg_.joinable())
        {
            if (bWFifoStop)
            {
                // unblock fifo write open while fifo read not happen
                int fd = open(FIFO_TO_CLIENT, O_RDONLY);
                CHECK_NE(fd, -1);
                close(fd);
            }
            obtainMessage(MSG_EXIT)->post();
            th_msg_.join();
        }
        else
        {
            Log.e(TAG, " th_msg_ not joinable ");
        }
    }
}

void fifo::send_err_type_code(int type, int code)
{
    sp<ERR_TYPE_INFO> mInfo = sp<ERR_TYPE_INFO>(new ERR_TYPE_INFO());
    sp<ARMessage> msg = obtainMessage(MSG_DISP_ERR_TYPE);
    mInfo->type =type;
    mInfo->err_code =code;
//    mConfig->bopen = open;
    msg->set<sp<ERR_TYPE_INFO>>("err_type_info", mInfo);
    msg->post();
}

void fifo::send_wifi_config(const char *ssid, const char *pwd, int open)
{
    Log.d(TAG,"send wifi config");
    sp<WIFI_CONFIG> mConfig = sp<WIFI_CONFIG>(new WIFI_CONFIG());
    sp<ARMessage> msg = obtainMessage(MSG_SET_WIFI_CONFIG);
    snprintf(mConfig->ssid, sizeof(mConfig->ssid), "%s", ssid);
    snprintf(mConfig->pwd, sizeof(mConfig->pwd), "%s", pwd);
//    mConfig->bopen = open;
    msg->set<sp<WIFI_CONFIG>>("wifi_config", mConfig);
    msg->post();
}


void fifo::send_power_off()
{
    sp<ARMessage> msg = obtainMessage(MSG_START_POWER_OFF);
    msg->post();
}

void fifo::send_sys_info(sp<SYS_INFO> &mSysInfo)
{
    sp<ARMessage> msg = obtainMessage(MSG_SET_SYS_INFO);
    msg->set<sp<SYS_INFO>>("sys_info", mSysInfo);
    msg->post();
}

void fifo::send_sync_info(sp<struct _sync_init_info_> &mSyncInfo)
{
    sp<ARMessage> msg = obtainMessage(MSG_SET_SYNC_INFO);
    msg->set<sp<SYNC_INIT_INFO>>("sync_info", mSyncInfo);
    msg->post();
}


void fifo::send_disp_str_type(sp<DISP_TYPE> &dis_type)
{
    sp<ARMessage> msg = obtainMessage(MSG_DISP_STR_TYPE);
    msg->set<sp<DISP_TYPE>>("disp_type", dis_type);
    msg->post();
}


/*************************************************************************
** 方法名称: write_fifo
** 方法功能: 发送数据到给osc
** 入口参数: 
**		iEvent - 事件类型
**		str - 数据
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
void fifo::write_fifo(int iEvent, char *str)
{
    get_write_fd();
    char data[4096] = {0x00};
    int total = FIFO_HEAD_LEN;
    int write_len;
    int len = 0;
    //header(only contain content len(4bytes) ) + content

    int_to_bytes(data, iEvent);
    if (str != nullptr) {
        len = strlen(str);
#if 1
        if (len > (int)((sizeof(data) - FIFO_HEAD_LEN))) {
            Log.e(TAG, "fifo len exceed (%d %d)", len, (sizeof(data) - FIFO_HEAD_LEN));
            len = (sizeof(data) - FIFO_HEAD_LEN);
        }
		
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
        memcpy((void *)&data[FIFO_HEAD_LEN],str,len);
#else
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
        snprintf(&data[FIFO_HEAD_LEN], sizeof(data) - FIFO_HEAD_LEN, str, len);
#endif
        total += len;
    } else {
        int_to_bytes(&data[FIFO_DATA_LEN_OFF], len);
    }
	
    write_len = write(write_fd, data, total);
    if (write_len != total) {
        Log.e(TAG, "write fifo len %d but total is %d\n", write_len, total);
        if (write_len == -1) {
            Log.e(TAG, "write fifo broken");
            close_write_fd();
        }
    } 
}


static char save_path[512] = {0};



/*************************************************************************
** 方法名称: handle_oled_notify
** 方法功能: 处理来自UI线程的消息
** 入口参数: 
**		msg - 消息指针
** 返 回 值: 无 
** 调     用: 
** {"name": "camera._calibrationAwb"}
*************************************************************************/
void fifo::handle_oled_notify(const sp<ARMessage> &msg)
{
    char *pSrt = nullptr;
    cJSON *root = cJSON_CreateObject();
    CHECK_NE(root, nullptr);

    int what;
    CHECK_EQ(msg->find<int>("what", &what), true);

    switch (what) {

		/* msg.what = OLED_KEY
	 	 * msg.action = int
	 	 * msg.action_info = sp<ACTION_INFO>	[optional]
	 	 * {"action":[0/9], "action_info": }
	 	 */
        case oled_handler::OLED_KEY: {
            int action;
            sp<ACTION_INFO> mActInfo;
            CHECK_EQ(msg->find<int>("action", &action), true);

			/* {"action": [0/9]} */
			Log.d(TAG, " rec oled action %d", action);
            cJSON_AddNumberToObject(root, "action", action);

			/* 查看msg的"action_info" */
            if (msg->find<sp<ACTION_INFO>>("action_info", &mActInfo)) {
                cJSON *org = cJSON_CreateObject();
                cJSON *param = cJSON_CreateObject();

				/* {
				 *		"action":ACTION_PIC, 
				 *		"parameters": {
				 *			"org": {
				 *				"mime":string, 
				 *				"saveOrigin":true/false, 
				 *				"width":int,
				 *				"height":int,
				 *				"channelLayout":string,
				 *				"mime":string
				 *			}
				 *			"delay":int,
				 *			"burst": {
				 *				"enable":true/false,
				 *				"count":int
				 *			},
				 *			"hdr": {
				 *				"enable":true/false,
				 *				"count":int,
				 *				"min_ev":int,
				 *				"max_ev":int,
				 *			},
				 *		}
				 * }
				 */	
                switch (action) {
                    case ACTION_PIC:
                        cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
                        cJSON_AddNumberToObject(param, "delay", mActInfo->delay);
                        if (mActInfo->stOrgInfo.stOrgAct.mOrgP.burst_count > 0) {
                            cJSON *burst = cJSON_CreateObject();
                            cJSON_AddTrueToObject(burst,"enable");
                            cJSON_AddNumberToObject(burst, "count", mActInfo->stOrgInfo.stOrgAct.mOrgP.burst_count);
                            cJSON_AddItemToObject(param, "burst",burst);
                        } else if(mActInfo->stOrgInfo.stOrgAct.mOrgP.hdr_count > 0) {
                            cJSON *hdr = cJSON_CreateObject();
                            cJSON_AddTrueToObject(hdr,"enable");
                            cJSON_AddNumberToObject(hdr, "count", mActInfo->stOrgInfo.stOrgAct.mOrgP.hdr_count);
                            cJSON_AddNumberToObject(hdr, "min_ev", mActInfo->stOrgInfo.stOrgAct.mOrgP.min_ev);
                            cJSON_AddNumberToObject(hdr, "max_ev", mActInfo->stOrgInfo.stOrgAct.mOrgP.max_ev);
                            cJSON_AddItemToObject(param, "hdr", hdr);
                        }
                        break;
						
                    case ACTION_VIDEO:
                        cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
                        cJSON_AddNumberToObject(org, "framerate", (all_frs[mActInfo->stOrgInfo.stOrgAct.mOrgV.org_fr]));
                        cJSON_AddNumberToObject(org, "bitrate", mActInfo->stOrgInfo.stOrgAct.mOrgV.org_br * 1000);
                        Log.d(TAG,"vid mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
                        if (mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int != 0)
                        {
                            cJSON *tim_lap = cJSON_CreateObject();
                            cJSON_AddTrueToObject(tim_lap, "enable");
                            cJSON_AddNumberToObject(tim_lap, "interval",
                                                    mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
                            cJSON_AddItemToObject(param, "timelapse", tim_lap);
                        }
                        if(mActInfo->stOrgInfo.stOrgAct.mOrgV.logMode == 1)
                        {
                            cJSON_AddNumberToObject(org, "logMode", mActInfo->stOrgInfo.stOrgAct.mOrgV.logMode);
                        }
                        break;
                    case ACTION_LIVE:
                        cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
                        cJSON_AddNumberToObject(org, "framerate", (all_frs[mActInfo->stOrgInfo.stOrgAct.mOrgL.org_fr]));
                        cJSON_AddNumberToObject(org, "bitrate", mActInfo->stOrgInfo.stOrgAct.mOrgL.org_br * 1000);
                        if(mActInfo->stOrgInfo.stOrgAct.mOrgL.logMode == 1)
                        {
                            cJSON_AddNumberToObject(org, "logMode", mActInfo->stOrgInfo.stOrgAct.mOrgL.logMode);
                        }
                        break;
                    SWITCH_DEF_ERROR(action)
                }
				
                if (mActInfo->stOrgInfo.save_org == SAVE_OFF) {
                    cJSON_AddFalseToObject(org, "saveOrigin");
                } else {
                    cJSON_AddTrueToObject(org, "saveOrigin");
                }
				
                cJSON_AddNumberToObject(org, "width", mActInfo->stOrgInfo.w);
                cJSON_AddNumberToObject(org, "height", mActInfo->stOrgInfo.h);

				/*
				 * 2018年5月17日：添加录像到各张子卡
				 */
				cJSON_AddNumberToObject(org, "storage_loc", mActInfo->stOrgInfo.locMode);

				
				/* {"paramters": {"origin":{}} */
				cJSON_AddItemToObject(param, "origin", org);
				Log.d(TAG, " mActInfo->stStiInfo.stich_mode %d", mActInfo->stStiInfo.stich_mode);


				/* {
				 *		"action":ACTION_XX, 
				 *		"parameters": {
				 *			"stiching": {
				 *				"bitrate":int, 
				 *				"samplerate":int, 
				 *				"mime":string,
				 *				"sampleFormat":stirng,
				 *				"channelLayout":string,
				 *				"mime":string
				 *			}
				 *		}
				 * }
				 */				

				if (mActInfo->stStiInfo.stich_mode != STITCH_OFF) {
                    cJSON *sti = cJSON_CreateObject();
                    cJSON_AddStringToObject(sti, "mode", act_mode[mActInfo->mode]);
                    cJSON_AddNumberToObject(sti, "height", mActInfo->stStiInfo.h);
                    cJSON_AddNumberToObject(sti, "width", mActInfo->stStiInfo.w);
					Log.d(TAG, "sti mode %d", mActInfo->stStiInfo.stich_mode);
                    if (STITCH_CUBE == mActInfo->stStiInfo.stich_mode) {
                        cJSON_AddStringToObject(sti, "map", "cube");
                    }
					
                    switch (action) {
                        case ACTION_PIC:
                            cJSON_AddStringToObject(sti, "mime", all_mime[mActInfo->stStiInfo.mime]);
                            if (STITCH_OPTICAL_FLOW == mActInfo->stStiInfo.stich_mode) {
                                cJSON_AddStringToObject(sti, "algorithm", "opticalFlow");
                            }
                            break;
							
                        case ACTION_LIVE:
                            if (mActInfo->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON) {
                                cJSON_AddTrueToObject(sti, "liveOnHdmi");
                            } else {
                                cJSON_AddFalseToObject(sti, "liveOnHdmi");
                            }

                            if (mActInfo->stStiInfo.stStiAct.mStiL.file_save) {
                                cJSON_AddTrueToObject(sti, "fileSave");
                            } else {
                                cJSON_AddFalseToObject(sti, "fileSave");
                            }
							
                            Log.d(TAG, "url format (%d %d)",
                                  strlen(mActInfo->stStiInfo.stStiAct.mStiL.url),
                                  strlen(mActInfo->stStiInfo.stStiAct.mStiL.format));

							if (strlen(mActInfo->stStiInfo.stStiAct.mStiL.url) > 0) {
                                Log.d(TAG,"  url  %s", mActInfo->stStiInfo.stStiAct.mStiL.url);
                                cJSON_AddStringToObject(sti, "_liveUrl", mActInfo->stStiInfo.stStiAct.mStiL.url);
                            }
							
                            if (strlen(mActInfo->stStiInfo.stStiAct.mStiL.format) > 0) {
                                Log.d(TAG,"  format  %s", mActInfo->stStiInfo.stStiAct.mStiL.url);
                                cJSON_AddStringToObject(sti, "format", mActInfo->stStiInfo.stStiAct.mStiL.format);
                            }
                            cJSON_AddNumberToObject(sti, "framerate",
                                                    (all_frs[mActInfo->stStiInfo.stStiAct.mStiL.sti_fr]));
                            cJSON_AddNumberToObject(sti, "bitrate", mActInfo->stStiInfo.stStiAct.mStiL.sti_br * 1000);
                            cJSON_AddStringToObject(sti, "mime", all_mime[mActInfo->stStiInfo.mime]);
                            //no break here
                            break;
							
                        case ACTION_VIDEO:
                            Log.d(TAG, "mActInfo->stStiInfo.stStiAct.mStiV.sti_fr is %d",
                                  mActInfo->stStiInfo.stStiAct.mStiV.sti_fr);
							
                            cJSON_AddNumberToObject(sti, "framerate",
                                                    (all_frs[mActInfo->stStiInfo.stStiAct.mStiV.sti_fr]));
                            cJSON_AddNumberToObject(sti, "bitrate", mActInfo->stStiInfo.stStiAct.mStiV.sti_br * 1000);
                            cJSON_AddStringToObject(sti, "mime", all_mime[mActInfo->stStiInfo.mime]);
                            break;
                        SWITCH_DEF_ERROR(action)
                    }
                    cJSON_AddItemToObject(param, "stiching", sti);
                } else {
                    Log.w(TAG,"sti off");
                }

				/* {
				 *		"action":ACTION_XX, 
				 *		"parameters": {
				 *			"audio": {
				 *				"bitrate":int, 
				 *				"samplerate":int, 
				 *				"mime":string,
				 *				"sampleFormat":stirng,
				 *				"channelLayout":string,
				 *				"mime":string
				 *			}
				 *		}
				 * }
				 */				
                //judge whether has audio
				// Log.d(TAG, "mActInfo->stAudInfo.sample_rate is %d", mActInfo->stAudInfo.sample_rate);
                if (mActInfo->stAudInfo.sample_rate != 0) {
                    //"mime":string,"sampleFormat":string,"channelLayout":string,samplerate:int,bitrate:int}
                    cJSON *aud = cJSON_CreateObject();
                    cJSON_AddNumberToObject(aud, "bitrate", mActInfo->stAudInfo.br);
                    cJSON_AddNumberToObject(aud, "samplerate", mActInfo->stAudInfo.sample_rate);
                    cJSON_AddStringToObject(aud, "mime", mActInfo->stAudInfo.mime);
                    cJSON_AddStringToObject(aud, "sampleFormat", mActInfo->stAudInfo.sample_fmt);
                    cJSON_AddStringToObject(aud, "channelLayout", mActInfo->stAudInfo.ch_layout);
                    cJSON_AddStringToObject(aud, "mime", mActInfo->stAudInfo.mime);
                    cJSON_AddItemToObject(param, "audio", aud);
                } else {
					Log.w(TAG,"aud s 0");
				}

				/* {"action":ACTION_XX, "parameters": {}} */
                cJSON_AddItemToObject(root, "parameters", param);
            } else {
				/* msg中不含"action_info" */
				cJSON *param = nullptr;
                switch (action) {
                    case ACTION_CALIBRATION: {	/* 校验: {"action": ACTION_CALIBRATION} */
						#if 0
						int calibration_mode;
						CHECK_EQ(msg->find<int>("cal_mode", &calibration_mode), true);
						param = cJSON_CreateObject();
						cJSON_AddStringToObject(param, "mode", cal_mode[calibration_mode]);
						#endif
						break;
						
					}

                    case ACTION_SET_OPTION: {	/* 设置选项:  */
                        int type;
                        CHECK_EQ(msg->find<int>("type", &type), true);

						Log.d(TAG, " type is %d", type);
                        switch (type) {
							/* {"action": ACTION_SET_OPTION, "parameters":{"property":"flicker", "value":int} } */
                            case OPTION_FLICKER: { 
                                int flicker;
                                CHECK_EQ(msg->find<int>("flicker", &flicker), true);
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "flicker");
                                cJSON_AddNumberToObject(param, "value", flicker);
								break;
							}

							/* {"action": ACTION_SET_OPTION, "parameters":{"property":"logMode", "mode":int, "effect":int, ""} } */
                            case OPTION_LOG_MODE: {	 /* {"action": ACTION_SET_OPTION, "type": OPTION_FLICKER } */
                                int mode;
                                int effect;
                                cJSON *valObj = cJSON_CreateObject();
                                CHECK_EQ(msg->find<int>("mode", &mode), true);
                                CHECK_EQ(msg->find<int>("effect", &effect), true);
								
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "logMode");
                                cJSON_AddNumberToObject(valObj, "mode", mode);
                                cJSON_AddNumberToObject(valObj, "effect", effect);
                                cJSON_AddItemToObject(param, "value", valObj);
								break;
                            }
							
							/* {"action": ACTION_SET_OPTION, {"property": "fanless", "value": 0/1}} */
                            case OPTION_SET_FAN: {
                                int fan;
                                CHECK_EQ(msg->find<int>("fan", &fan), true);
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "fanless");
                                if (fan == 1) {
                                    cJSON_AddNumberToObject(param, "value", 0);
                                } else {
                                    cJSON_AddNumberToObject(param, "value", 1);
                                }
								break;
                            }

							/* {"action": ACTION_SET_OPTION, {"property": "panoAudio", "value": 0/1}} */
                            case OPTION_SET_AUD: {
                                int aud;
                                CHECK_EQ(msg->find<int>("aud", &aud), true);
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "panoAudio");
                                cJSON_AddNumberToObject(param, "value", aud);
								break;
							}

							/* {"action": ACTION_SET_OPTION, {"property": "stabilization_cfg", "value": 0/1}} */
                            case OPTION_GYRO_ON: {
                                int gyro_on;
                                CHECK_EQ(msg->find<int>("gyro_on", &gyro_on), true);
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "stabilization_cfg");
                                cJSON_AddNumberToObject(param, "value", gyro_on);
								break;
                           }

							/* {"action": ACTION_SET_OPTION, {"property": "logo", "value": 0/1}} */
                            case OPTION_SET_LOGO: {
                                int logo_on;
                                CHECK_EQ(msg->find<int>("logo_on", &logo_on), true);
                                param = cJSON_CreateObject();
                                cJSON_AddStringToObject(param, "property", "logo");
                                cJSON_AddNumberToObject(param, "value", logo_on);
								break;
                            }

							/* {"action": ACTION_SET_OPTION, {"property": "audio_gain", "value": 0/1}} */
                            case OPTION_SET_AUD_GAIN: {
                                int aud_gain;
                                CHECK_EQ(msg->find<int>("aud_gain", &aud_gain), true);
                                param = cJSON_CreateObject();
                                Log.d(TAG,"set aud gain %d",aud_gain);
                                cJSON_AddStringToObject(param, "property", "audio_gain");
                                cJSON_AddNumberToObject(param, "value", aud_gain);
								break;
                            }

                            SWITCH_DEF_ERROR(type)
                        }
						break;
                    }


					/* {"action": ACTION_REQ_SYNC, parameters":{"sn":"0123456", "r_v":"xxxx", "p_v":"ddfff", "k_v":"xxxxx"}} */
                    case ACTION_REQ_SYNC: {
                        sp<REQ_SYNC> mReqSync;
                        CHECK_EQ(msg->find<sp<REQ_SYNC>>("req_sync", &mReqSync), true);
                        param = cJSON_CreateObject();
                        cJSON_AddStringToObject(param, "sn", mReqSync->sn);
                        cJSON_AddStringToObject(param, "r_v", mReqSync->r_v);
                        cJSON_AddStringToObject(param, "p_v", mReqSync->p_v);
                        cJSON_AddStringToObject(param, "k_v", mReqSync->k_v);
						break;
					}

					/* {"action": ACTION_LOW_BAT} */
                    case ACTION_LOW_BAT: {
                        CHECK_EQ(msg->find<int>("cmd", &reboot_cmd), true);
                        Log.d(TAG,"low bat reboot cmd is %d",reboot_cmd);
						break;
					}

					/* {"action": ACTION_LOW_PROTECT} */
					#if 0
                    case ACTION_LOW_PROTECT:
                        break;
					#endif
					
					/* {"action": ACTION_SPEED_TEST, "paramters":{"path":"/media/nvidia/xxxx"}} */
                    case ACTION_SPEED_TEST: {
                        char *path;
                        CHECK_EQ(msg->find<char *>("path", &path), true);
                        param = cJSON_CreateObject();
						Log.d(TAG, "speed path %s", path);
                        cJSON_AddStringToObject(param, "path", path);
						break;
					}

					/* {"action": ACTION_NOISE} */
                    case ACTION_NOISE:
                        break;

					/* {"action": ACTION_GYRO} */
                    case ACTION_GYRO:
                        break;

					/* {"action": ACTION_AGEING} */
                    case ACTION_AGEING:
                        break;


		case ACTION_AWB:
						
			param = cJSON_CreateObject();
			cJSON_AddStringToObject(param, "name", "camera._calibrationAwb");


			break;


					/* {"action": ACTION_POWER_OFF} */
                    case ACTION_POWER_OFF: {
						// stop_all(false);
                        set_gpio_level(85,0);
                        CHECK_EQ(msg->find<int>("cmd", &reboot_cmd), true);
                        Log.d(TAG, "power off reboot cmd is %d", reboot_cmd);
						break;
                    }

                    default:
                        break;
                }

                if (param != nullptr) {
                    cJSON_AddItemToObject(root, "parameters", param);
                }
            }
            pSrt = cJSON_Print(root);

			Log.d(TAG, "oled key req> %s", pSrt);
			
            write_fifo(EVENT_OLED_KEY, pSrt);
        }
            break;


		/* 
		 * msg.what = SAVE_PATH_CHANGE
		 * msg.save_path = sp<SAVE_PATH>
		 * {"path": "/media/nvidia/xxxxxxx"}
		 */
        case oled_handler::SAVE_PATH_CHANGE: {	/* 存储路径发生变化 */
            sp<SAVE_PATH> mSavePath;
            CHECK_EQ(msg->find<sp<SAVE_PATH>>("save_path", &mSavePath), true);
#if 0
            cJSON *subNode = cJSON_CreateObject();
            cJSON_AddStringToObject(subNode, "path", mSavePath->path);
            cJSON_AddNumberToObject(subNode, "remain_pic_num", mSavePath->mRemain->remain_pic_num);
            cJSON_AddNumberToObject(subNode, "remain_min", mSavePath->mRemain->remain_min);
            cJSON_AddNumberToObject(subNode, "remain_sec", mSavePath->mRemain->remain_sec);
            cJSON_AddNumberToObject(subNode, "remain_hour", mSavePath->mRemain->remain_hour);
            cJSON_AddItemToObject(root,"save_path_info",subNode);
#else
            cJSON_AddStringToObject(root, "path", mSavePath->path);
#endif
            pSrt = cJSON_Print(root);

			Log.d(TAG, "rec EVENT_SAVE_PATH event %s", pSrt);
            write_fifo(EVENT_SAVE_PATH, pSrt);
			break;
        }


		
		/* 
		 * msg.what = UPDATE_BAT
		 * msg.bat_info = sp<BAT_INFO>
		 * {"battery_level": 100, "battery_charge": 0/1, "int_tmp": 20, "tmp":20}
		 */
        case oled_handler::UPDATE_BAT: {		/* 电池状态更新 */
            sp<BAT_INFO> mBatInfo;
            CHECK_EQ(msg->find<sp<BAT_INFO>>("bat_info", &mBatInfo), true);

			cJSON_AddNumberToObject(root, "battery_level", mBatInfo->battery_level);
            cJSON_AddNumberToObject(root, "battery_charge", mBatInfo->bCharge);
            cJSON_AddNumberToObject(root, "int_tmp", mBatInfo->int_tmp);
            cJSON_AddNumberToObject(root, "tmp", mBatInfo->tmp);

            pSrt = cJSON_Print(root);
            write_fifo(EVENT_BATTERY, pSrt);
			
            Log.d(TAG, "rec UPDATE_BATTERY battery_level %d bCharge %d", mBatInfo->battery_level, mBatInfo->bCharge);
			break;
        }


		/* 
		 * msg.what = UPDATE_DEV
		 * msg.dev_list = vector<sp<USB_DEV_INFO>>
		 * {"dev_list": {{"dev_type": "sd/usb", "path": "/mnt/sdcard", "name": "sd1"}, {"dev_type": "sd/usb", "path": "/mnt/sdcard", "name": "sd1"}}}
		 */
		
        case oled_handler::UPDATE_DEV: {		/* 新的存储设备插入 */
            vector<sp<USB_DEV_INFO>> mDevList;
            CHECK_EQ(msg->find<vector<sp<USB_DEV_INFO>>>("dev_list", &mDevList), true);

			Log.d(TAG, "update dev %d", mDevList.size());

            if (mDevList.size() > 0) {	/* 存储设备列表大于0 */
                cJSON *root = nullptr;
                char *pSrt = nullptr;
			
                root = cJSON_CreateObject();
                CHECK_NE(root, nullptr);
				
                cJSON *jsonArray = cJSON_CreateArray();
                for (unsigned int i = 0; i < mDevList.size(); i++) {
                    cJSON *sub = cJSON_CreateObject();
                    cJSON_AddStringToObject(sub, "dev_type", mDevList.at(i)->dev_type);
                    cJSON_AddStringToObject(sub, "path", mDevList.at(i)->path);
                    cJSON_AddStringToObject(sub, "name", mDevList.at(i)->name);
                    cJSON_AddItemToArray(jsonArray, sub);
                }
				
                cJSON_AddItemToObject(root, "dev_list", jsonArray);

				pSrt = cJSON_Print(root);

				Log.d(TAG, "rec MSG_DEV_NOTIFY  %s", pSrt);
                write_fifo(EVENT_DEV_NOTIFY, pSrt);
                if (nullptr != pSrt) {
                    free(pSrt);
                }
				
                if (nullptr != root) {
                    cJSON_Delete(root);
                }
            } else {
                write_fifo(EVENT_DEV_NOTIFY);
            }
			break;
        }

        default:
            break;
    }

	if (nullptr != pSrt) {
        free(pSrt);
    }
	
    if (nullptr != root) {
        cJSON_Delete(root);
    }
}



void fifo::handle_poll_change(const sp<ARMessage> &msg)
{
    char *pSrt = nullptr;
    cJSON *root = cJSON_CreateObject();
    CHECK_NE(root, nullptr);

    int what;
    CHECK_EQ(msg->find<int>("what", &what), true);
//    Log.d(TAG," handle_poll_change what %d",what);
    switch (what)
    {
//        case poll_timer::UPDATE_BATTERY:
//        {
//            int battery_level = 0;
//            int temp;
//            bool bCharge = false;
//            CHECK_EQ(msg->find<int>("battery_level", &battery_level) ,true);
//            CHECK_EQ(msg->find<int>("temp", &temp) ,true);
//            CHECK_EQ(msg->find<bool>("battery_charge", &bCharge),true);
//            cJSON_AddNumberToObject(root, "battery_level", battery_level);
//            cJSON_AddNumberToObject(root, "battery_charge", bCharge);
//            pSrt = cJSON_Print(root);
//            write_fifo(EVENT_BATTERY, pSrt);
//            Log.d(TAG, "rec UPDATE_BATTERY battery_level %d bCharge %d",battery_level, bCharge);
//            mOLEDHandle->send_disp_battery(battery_level,bCharge);
//        }
        break;
#if 0
        case poll_timer::UPDATE_NET:
        {
//skip update net to ws
            sp<net_dev_info> mpDevInfo;
            CHECK_EQ(msg->find<sp<net_dev_info>>("net_dev_info", &mpDevInfo), true);
//            Log.d(TAG,"send ip addr 0x%x",mpDevInfo->dev_addr);
            mOLEDHandle->send_disp_ip((int) mpDevInfo->dev_addr,
                                      mpDevInfo->dev_type);
//            Log.d(TAG,"2send ip addr 0x%x",mpDevInfo->dev_addr);
        }
            break;
#endif
        default:
            break;
    }
    if (nullptr != pSrt)
    {
        free(pSrt);
    }
    if (nullptr != root)
    {
        cJSON_Delete(root);
    }
}




/*************************************************************************
** 方法名称: handleMessage
** 方法功能: FIFO交互线程消息处理
** 入口参数: 
**  msg - 消息指针
** 返回值: 无
** 调用:
**
*************************************************************************/
void fifo::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();

	{
        if (MSG_EXIT == what) {		/* 线程退出消息 */
            mLooper->quit();
            close_write_fd();
        } else {
			Log.d(TAG," what is %d", what);
            switch (what) {
		#if 0
			case MSG_POLL_TIMER:
				handle_poll_change(msg);
				break;
		#endif

			/* UI -> FIFO -> OSC */
			case MSG_GET_OLED_KEY: {	/* 来自UI线程的Key */
				handle_oled_notify(msg);
				break;
			}

		#if 0
			case MSG_DEV_NOTIFY: {
				// told oled_handler all the dev_type
				mOLEDHandle->send_update_dev_list(mDevList);
				break;
			}

			case MSG_INIT_SCAN: {
				mDevManager->start_scan();
				break;
			}

		#endif
		
			/* OSC -> FIFO -> UI 
			 * 通知UI进入某个显示界面
			 */
			case MSG_DISP_STR_TYPE: {
				sp<DISP_TYPE> disp_type;
				CHECK_EQ(msg->find<sp<DISP_TYPE>>("disp_type", &disp_type), true);
				mOLEDHandle->send_disp_str(disp_type);
				break;
			}

			/* OSC -> FIFO -> UI 
			 * 通知UI显示指定的错误
			 */
			case MSG_DISP_ERR_TYPE: {
				sp<ERR_TYPE_INFO> mInfo;
				CHECK_EQ(msg->find<sp<ERR_TYPE_INFO>>("err_type_info", &mInfo), true);
				mOLEDHandle->send_disp_err(mInfo);
				break;
			}

			#if 0
			case MSG_DISP_EXT: {
				sp<DISP_EXT> disp_ext;
				CHECK_EQ(msg->find<sp<DISP_EXT>>("disp_ext", &disp_ext), true);
				mOLEDHandle->send_disp_ext(disp_ext);
				break;
			}
			#endif

		
			/* OSC -> FIFO -> UI 
			 * 设置WiFi配置
			 */
	        case MSG_SET_WIFI_CONFIG: {
	            Log.d(TAG,"MSG_SET_WIFI_CONFIG");
	            sp<WIFI_CONFIG> mConfig;
	            CHECK_EQ(msg->find<sp<WIFI_CONFIG>>("wifi_config", &mConfig), true);
	            mOLEDHandle->send_wifi_config(mConfig);
	            break;
	        }

			/* OSC -> FIFO -> UI 
			 * 通知UI设置系统信息
			 */
	        case MSG_SET_SYS_INFO: {
	            sp<SYS_INFO> mSysInfo;
	            CHECK_EQ(msg->find<sp<SYS_INFO>>("sys_info", &mSysInfo), true);
	            mOLEDHandle->send_sys_info(mSysInfo);
	            break;
	        }

			case MSG_START_POWER_OFF:
				// start_sys_reboot();
				break;

			/* OSC -> FIFO -> UI 
			 * 通知UI设置同步信息
			 */
	        case MSG_SET_SYNC_INFO: {
	            sp<SYNC_INIT_INFO> mSyncInfo;
	            CHECK_EQ(msg->find<sp<SYNC_INIT_INFO>>("sync_info", &mSyncInfo), true);
	            mOLEDHandle->send_sync_init_info(mSyncInfo);
	        }
	                    break;
			SWITCH_DEF_ERROR(what)
		    }
        }
    }
}





/*************************************************************************
** 方法名称: init_thread
** 方法功能: 初始化通信线程(FiFo)
** 入口参数: 
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
void fifo::init_thread()
{
    std::promise<bool> pr;
    std::future<bool> reply = pr.get_future();
    th_msg_ = thread([this, &pr] {
                       mLooper = sp<ARLooper>(new ARLooper());
                       mHandler = sp<ARHandler>(new my_handler(this));
                       mHandler->registerTo(mLooper);
                       pr.set_value(true);
                       mLooper->run();
                   });
    CHECK_EQ(reply.get(), true);
}



/*************************************************************************
** 方法名称: read_fifo_thread
** 方法功能: 读取来自osc的消息
** 入口参数: 
** 返 回 值: 无 
** 调     用: 
** 注: 读线程得到的消息最终会投递到交互线程的消息队列中统一处理
*************************************************************************/
void fifo::read_fifo_thread()
{
    char buf[1024];
    int error_times = 0;

    while (true) {
        memset(buf, 0, sizeof(buf));
        get_read_fd();	/* 获取FIFO读端的fd */

		/* 首先读取8字节的头部 */
        int len = read(read_fd, buf, FIFO_HEAD_LEN);
        if (len != FIFO_HEAD_LEN) {	/* 头部读取错误 */
            Log.w(TAG, "2read fifo head mismatch(%d %d)", len, FIFO_HEAD_LEN);
            if (++error_times >= 3) {
                Log.e(TAG, "read fifo broken?");
                close_read_fd();
            }
        } else {
            int msg_what = bytes_to_int(buf);	/* 前4字节代表消息类型: what */
            if (msg_what == CMD_EXIT) {	/* 如果是退出消息 */
				// Log.d(TAG," rec cmd exit");
                break;
            } else {
            	#if 0
				printf(TAG,"debug header :");
				for(int i = 0; i < FIFO_HEAD_LEN; i++) {
					printf("0x%x ",buf[i]);
				}
				#endif
				
				/* 头部的后4字节代表本次数据传输的长度 */
                int content_len = bytes_to_int(&buf[FIFO_DATA_LEN_OFF]);
                CHECK_NE(content_len, 0);

				/* 读取传输的数据 */
                len = read(read_fd, &buf[FIFO_HEAD_LEN], content_len);

				#if 0
				Log.d(TAG, "2read fifo msg what %d content_len %d", msg_what, content_len);
				#endif

				if (len != content_len) {	/* 读取的数据长度不一致 */
                    Log.w(TAG, "3read fifo content mismatch(%d %d)", len, content_len);
                    if (++error_times >= 3) {
                        Log.e(TAG, " 2read fifo broken? ");
                        close_read_fd();
                    }
                } else {
                
                	/* 解析该Json数据 */
                    cJSON *root = cJSON_Parse(&buf[FIFO_HEAD_LEN]);
                    cJSON *subNode = 0;
                    if (!root) {	/* 解析出错 */
                        Log.e(TAG, "cJSON parse string error, func(%s), line(%d)",
                              __FILE__,
                              __LINE__);
                    }
                    Log.d(TAG, "msg_what %d fifo test %s", msg_what, &buf[FIFO_HEAD_LEN]);

					/* 根据消息的类型做出处理 */
                    switch (msg_what) {
                        case CMD_OLED_DISP_TYPE: {	/* 通信UI线程显示指定UI */
                            cJSON *child = nullptr;
							
							/* 解析子节点的数据来构造DISP_TYPE */
                            sp<DISP_TYPE> mDispType = sp<DISP_TYPE>(new DISP_TYPE());
                            subNode = cJSON_GetObjectItem(root, "type");
                            CHECK_NE(subNode, nullptr);
							
                            mDispType->type = subNode->valueint;
                            mDispType->control_act = -1;
                            mDispType->tl_count  = -1;
							
                            subNode = cJSON_GetObjectItem(root, "content");	/* 获取"content"子节点 */
                            if (subNode) {
                                int iArraySize = cJSON_GetArraySize(subNode);
                                int qr_version;
                                int qr_index = -1;
                                int qr_action_index;
                                int sti_res;
                                subNode = subNode->child;
                                CHECK_EQ(subNode->type, cJSON_Number);
                                qr_version = subNode->valueint;
                                for (u32 i = 0; i < sizeof(mQRInfo)/sizeof(mQRInfo[0]); i++) {
                                    if (qr_version == mQRInfo[i].version) {
                                        qr_index = i;
                                        break;
                                    }
                                }
								
                                Log.d(TAG, "qr version %d array size is %d qr_index %d", qr_version, iArraySize, qr_index);
                                if (qr_index != -1) {
                                    subNode = subNode->next;
                                    CHECK_EQ(subNode->type, cJSON_Number);
                                    mDispType->qr_type = subNode->valueint;
//                                    Log.d(TAG,"qr type %d", mDispType->qr_type);
                                    qr_action_index = (mDispType->qr_type - ACTION_PIC);
                                    Log.d(TAG,"qr action index %d iArraySize %d "
                                                  "mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size %d ",
                                          qr_action_index,
                                          iArraySize,
                                          mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size);
									
                                    if (iArraySize == mQRInfo[qr_index].astQRInfo[qr_action_index].qr_size) {
                                        int org_res;
                                        mDispType->mAct = sp<ACTION_INFO>(new ACTION_INFO());
                                        subNode = subNode->next;
                                        CHECK_EQ(subNode->type, cJSON_Number);
                                        mDispType->mAct->size_per_act = subNode->valueint;

                                        Log.d(TAG,"size per act %d", mDispType->mAct->size_per_act);
                                        //org
                                        subNode = subNode->next;
                                        CHECK_EQ(subNode->type, cJSON_Array);
                                        iArraySize = cJSON_GetArraySize(subNode);
                                        if (iArraySize > 0)
                                        {
                                            CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].org_size);
                                            child = subNode->child;
//                                                CHECK_NE(child, nullptr);
                                            CHECK_EQ(child->type, cJSON_Number);
                                            org_res = child->valueint;
                                            mDispType->mAct->stOrgInfo.w = mResInfos[org_res].w;
                                            //org 3d or pano h is same 170721
                                            mDispType->mAct->stOrgInfo.h = mResInfos[org_res].h[0];
                                            //skip mime
                                            child = child->next;
//                                                CHECK_NE(child, nullptr);
                                            CHECK_EQ(child->type, cJSON_Number);
                                            mDispType->mAct->stOrgInfo.mime = child->valueint;

                                            switch (mDispType->qr_type)
                                            {
                                                // old [1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm]]
//                                          new [v,1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm], [enable, count, step], [enable, count]]
                                                case ACTION_PIC:
                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.save_org = child->valueint;

                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->delay = child->valueint;
                                                    break;
//                                        [resolution, mime, framerate, originBitrate,saveOrigin]
                                                case ACTION_VIDEO:
                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_fr = child->valueint;

                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_br = child->valueint;

                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.save_org = child->valueint;

                                                    child = child->next;
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.logMode = child->valueint;
                                                    break;
//                                        [resolution, mime, framerate, originBitrate,saveOrigin]
                                                case ACTION_LIVE:
                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_fr = child->valueint;

                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_br = child->valueint;

                                                    child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.save_org = child->valueint;

                                                    child = child->next;
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.logMode = child->valueint;
                                                    break;
                                                SWITCH_DEF_ERROR(mDispType->qr_type)
                                            }
                                        }
                                        else
                                        {
                                            Log.w(TAG,"no org mDispType->qr_type %d",mDispType->qr_type);
                                        }
                                        //sti
                                        subNode = subNode->next;
                                        CHECK_EQ(subNode->type, cJSON_Array);
                                        iArraySize = cJSON_GetArraySize(subNode);
                                        switch (mDispType->qr_type)
                                        {
                                            //[resolution, mime, mode, algorithm]]
                                            case ACTION_PIC:
                                                if (iArraySize > 0)
                                                {
                                                    CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
                                                    child = subNode->child;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    sti_res = child->valueint;

                                                    //skip mime
                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stStiInfo.mime = child->valueint;

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->mode = child->valueint;

                                                    mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
                                                    mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stStiInfo.stich_mode = child->valueint;
                                                    Log.d(TAG, "2qr pic info %d,[%d,%d,%d,%d,%d],[%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->size_per_act,
                                                          mDispType->mAct->stOrgInfo.w,
                                                          mDispType->mAct->stOrgInfo.h,
                                                          mDispType->mAct->stOrgInfo.mime,
                                                          mDispType->mAct->stOrgInfo.save_org,
                                                          mDispType->mAct->delay,
                                                          mDispType->mAct->stStiInfo.w,
                                                          mDispType->mAct->stStiInfo.h,
                                                          mDispType->mAct->stStiInfo.mime,
                                                          mDispType->mAct->mode,
                                                          mDispType->mAct->stStiInfo.stich_mode);
                                                }
                                                else
                                                {
                                                    mDispType->mAct->stStiInfo.stich_mode = STITCH_OFF;
                                                    Log.d(TAG, "3qr pic org %d,[%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->size_per_act,
                                                          mDispType->mAct->stOrgInfo.w,
                                                          mDispType->mAct->stOrgInfo.h,
                                                          mDispType->mAct->stOrgInfo.mime,
                                                          mDispType->mAct->stOrgInfo.save_org,
                                                          mDispType->mAct->delay);
                                                }
                                                //[count, min_ev,max_ev] -- HDR
                                                subNode = subNode->next;
                                                iArraySize = cJSON_GetArraySize(subNode);
                                                if (iArraySize > 0)
                                                {
                                                    CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].hdr_size);
                                                    child = subNode->child;

                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.hdr_count = child->valueint;

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.min_ev = child->valueint;

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.max_ev = child->valueint;

                                                    Log.d(TAG,"hdr %d %d %d",
                                                          mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.hdr_count,
                                                          mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.min_ev,
                                                          mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.max_ev);
                                                }
                                                subNode = subNode->next;
                                                //[count] -- Burst
                                                iArraySize = cJSON_GetArraySize(subNode);
                                                if (iArraySize > 0)
                                                {
                                                    CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].burst_size);
                                                    child = subNode->child;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count = child->valueint;
                                                    Log.d(TAG,"burst count %d",
                                                          mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count);
                                                }
                                                break;
                                            case ACTION_VIDEO:
//                                              [resolution, mime, mode, framerate, stichBitrate]]
                                                if (iArraySize > 0)
                                                {
                                                    CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
                                                    child = subNode->child;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    sti_res = child->valueint;
                                                    //skip mime
                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stStiInfo.mime = child->valueint;

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->mode = child->valueint;

                                                    mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
                                                    mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[ mDispType->mAct->mode];

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_fr = child->valueint;

                                                    child = child->next;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_br = child->valueint;

                                                    //force to stitch normal
                                                    mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
                                                    Log.d(TAG, "qr video sti info [%d,%d,%d,%d,%d,%d]",
                                                          mDispType->mAct->stStiInfo.w,
                                                          mDispType->mAct->stStiInfo.h,
                                                          mDispType->mAct->stStiInfo.mime,
                                                          mDispType->mAct->mode,
                                                          mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_fr,
                                                          mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_br);
                                                }
                                                else
                                                {
                                                    mDispType->mAct->stStiInfo.stich_mode = STITCH_OFF;
                                                    Log.d(TAG, "stich off");
                                                }
                                                Log.d(TAG, "qr vid org info %d,[%d,%d,%d,%d,%d,%d]",
                                                      mDispType->mAct->size_per_act,
                                                      mDispType->mAct->stOrgInfo.w,
                                                      mDispType->mAct->stOrgInfo.h,
                                                      mDispType->mAct->stOrgInfo.mime,
                                                      mDispType->mAct->stOrgInfo.save_org,
                                                      mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_fr,
                                                      mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_br);

                                                subNode = subNode->next;
                                                //[interval] -- timelapse
                                                iArraySize = cJSON_GetArraySize(subNode);
                                                if(iArraySize > 0)
                                                {
                                                    CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].timelap_size);
                                                    child = subNode->child;
//                                                    CHECK_NE(child, nullptr);
                                                    CHECK_EQ(child->type, cJSON_Number);
                                                    mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int = child->valueint *1000;
                                                    Log.d(TAG,"tim_lap_int  %d",
                                                          mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
                                                    if(mDispType->mAct->size_per_act == 0)
                                                    {
                                                        mDispType->mAct->size_per_act = 10;
                                                    }
                                                }
                                                break;
//                                          [resolution, mime, mode, framerate, stichBitrate, hdmiState]
                                            case ACTION_LIVE:
//                                                iArraySize = cJSON_GetArraySize(subNode);
                                                CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);

                                                child = subNode->child;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                sti_res = child->valueint;
                                                //skip mime
                                                child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                mDispType->mAct->stStiInfo.mime = child->valueint;

                                                child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                mDispType->mAct->mode = child->valueint;

                                                child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_fr = child->valueint;

                                                child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_br = child->valueint;

                                                child = child->next;
//                                                CHECK_NE(child, nullptr);
                                                CHECK_EQ(child->type, cJSON_Number);
                                                mDispType->mAct->stStiInfo.stStiAct.mStiL.hdmi_on = child->valueint;

                                                Log.d(TAG, "qr live org info [%d,%d,%d,%d,%d]",
                                                      mDispType->mAct->stOrgInfo.w,
                                                      mDispType->mAct->stOrgInfo.h,
                                                      mDispType->mAct->stOrgInfo.mime,
                                                      mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_fr,
                                                      mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_br);

                                                //for url
                                                subNode = subNode->next;
                                                CHECK_EQ(subNode->type, cJSON_String);
                                                if (strlen(subNode->valuestring) > 0)
                                                {
                                                    snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.url,
                                                             sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.url),
                                                             "%s",
                                                             subNode->valuestring);
                                                }
                                                subNode = cJSON_GetObjectItem(root, "proExtra");
                                                if(subNode)
                                                {
                                                    child = cJSON_GetObjectItem(subNode, "saveStitch");
                                                    if(child)
                                                    {
                                                        if(SAVE_OFF == child->valueint)
                                                        {
                                                            mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 0;
                                                        }
                                                        else
                                                        {
                                                            mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 1;
                                                        }
                                                    }
                                                    child = cJSON_GetObjectItem(subNode, "format");
                                                    if(child)
                                                    {
                                                        snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.format,sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.format),
                                                                 "%s",child->valuestring);
                                                    }
                                                    child = cJSON_GetObjectItem(subNode, "map");
                                                    if(child)
                                                    {
                                                        mDispType->mAct->stStiInfo.stich_mode = get_sti_mode(child->valuestring);
                                                    }
                                                    else
                                                    {
                                                        //force to stitch normal
                                                        mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
                                                    }
                                                }
                                                else
                                                {
                                                    //force to stitch normal
                                                    mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
                                                }
                                                if(mDispType->mAct->stStiInfo.stich_mode == STITCH_CUBE)
                                                {
                                                    mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];
                                                    mDispType->mAct->stStiInfo.w = mDispType->mAct->stStiInfo.h*3/2;
                                                }
                                                else
                                                {
                                                    mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];
                                                    mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
                                                }
                                                Log.d(TAG, "qr live info [%d,%d,%d,%d,%d,%d %d %d] url %s",
                                                      mDispType->mAct->stStiInfo.w,
                                                      mDispType->mAct->stStiInfo.h,
                                                      mDispType->mAct->stStiInfo.mime,
                                                      mDispType->mAct->mode,
                                                      mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_fr,
                                                      mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_br,
                                                      mDispType->mAct->stStiInfo.stStiAct.mStiL.hdmi_on,
                                                      mDispType->mAct->stStiInfo.stich_mode,
                                                      mDispType->mAct->stStiInfo.stStiAct.mStiL.url);
                                                break;
                                            SWITCH_DEF_ERROR(mDispType->qr_type)
                                        }
                                    }
                                    else
                                    {
                                        mDispType->qr_type = -1;
                                        mDispType->mAct = nullptr;
                                        mDispType->type = QR_FINISH_UNRECOGNIZE;
                                    }
                                }
                                else
                                {
                                    mDispType->qr_type = -1;
                                    mDispType->mAct = nullptr;
                                    mDispType->type = QR_FINISH_UNRECOGNIZE;
                                }
                            }
                            else {
                                subNode = cJSON_GetObjectItem(root, "req");	/* 含有"req"子节点 */
                                if (subNode) {
                                    Log.d(TAG, "rec req type %d", mDispType->type);
                                    GET_CJSON_OBJ_ITEM_INT(child, subNode, "action", mDispType->qr_type)
//                                    mDispType->qr_type = child->valueint;
//                                    Log.d(TAG, "fifo control action %d",
//                                          mDispType->qr_type);
//                                    GET_CJSON_OBJ_ITEM_INT(child, subNode, "param")

									/* 获取"param"子节点 */
                                    child = cJSON_GetObjectItem(subNode,"param");
                                    CHECK_NE(child,nullptr);

									/* 使用ACTION_INFO结构来存储"param"节点的内容 */
                                    sp<ACTION_INFO> mAI = sp<ACTION_INFO>(new ACTION_INFO());
                                    memset(mAI.get(), 0, sizeof(ACTION_INFO));

									/* 获取"param"的子节点"origin"(原片相关信息) */
                                    cJSON *org = cJSON_GetObjectItem(child, "origin");	
                                    if (org) {	/* 原片参数存在 */
										/* 原片的宽,高,是否存储 */
                                        bool bSaveOrg;
                                        GET_CJSON_OBJ_ITEM_INT(subNode, org, "width", mAI->stOrgInfo.w)
                                        GET_CJSON_OBJ_ITEM_INT(subNode, org, "height", mAI->stOrgInfo.h)
                                        GET_CJSON_OBJ_ITEM_INT(subNode, org, "saveOrigin", bSaveOrg)
										Log.d(TAG, "bSave org %d", mAI->stOrgInfo.save_org);
                                        if (bSaveOrg) {
                                            mAI->stOrgInfo.save_org = SAVE_DEF;
                                        } else {
                                            mAI->stOrgInfo.save_org= SAVE_OFF;
                                        };
										
                                        Log.d(TAG, "org %d %d", mAI->stOrgInfo.w, mAI->stOrgInfo.h);

										/* 获取"origin"的子节点"mime" */
                                        subNode = cJSON_GetObjectItem(org, "mime");
                                        if (subNode) {
                                            mAI->stOrgInfo.mime = get_mime_index(subNode->valuestring);
                                        }

										Log.d(TAG, "qr type %d", mDispType->qr_type);
                                        switch (mDispType->qr_type) {
                                            case ACTION_PIC:
                                                if (mAI->stOrgInfo.w >= 7680) {
                                                    mAI->size_per_act = 20;
                                                } else if (mAI->stOrgInfo.w >= 5760) {
                                                    mAI->size_per_act = 15;
                                                } else {
                                                    mAI->size_per_act = 10;
                                                }
                                                break;
												
                                            case ACTION_VIDEO:
                                                subNode = cJSON_GetObjectItem(org, "framerate");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgV.org_fr = get_fr_index(
                                                            subNode->valueint);
                                                }

                                                subNode = cJSON_GetObjectItem(org,"bitrate");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgV.org_br = subNode->valueint / 1000;
                                                }

                                                if (mAI->stOrgInfo.save_org != SAVE_OFF) {
                                                    //exactly should divide 8,but actually less,so divide 10
                                                    mAI->size_per_act =
                                                            (mAI->stOrgInfo.stOrgAct.mOrgV.org_br * 6) / 10;
                                                } else {
                                                    // no br ,seems timelapse
                                                    mAI->size_per_act = 30;
                                                }
//                                                Log.d(TAG,"5qr type %d",mDispType->qr_type);
                                                subNode = cJSON_GetObjectItem(org,"logMode");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgV.logMode = subNode->valueint;
                                                }
                                                break;

                                            case ACTION_LIVE:
                                                subNode = cJSON_GetObjectItem(org, "framerate");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgV.org_fr = get_fr_index(
                                                            subNode->valueint);
                                                }
                                                subNode = cJSON_GetObjectItem(org,"bitrate");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgV.org_br = subNode->valueint / 1000;
                                                }
                                                subNode = cJSON_GetObjectItem(root,"logMode");
                                                if (subNode) {
                                                    mAI->stOrgInfo.stOrgAct.mOrgL.logMode = subNode->valueint;
                                                }
                                                break;
												
                                            SWITCH_DEF_ERROR(mDispType->qr_type)
                                        }
                                    }

									/* "param"的子节点"stiching" 拼接相关参数 */
                                    cJSON *sti = cJSON_GetObjectItem(child, "stiching");
                                    if (sti) {
                                        char sti_mode[32];
                                        GET_CJSON_OBJ_ITEM_INT(subNode, sti, "width", mAI->stStiInfo.w)
                                        GET_CJSON_OBJ_ITEM_INT(subNode, sti, "height", mAI->stStiInfo.h)

                                        Log.d(TAG,"stStiInfo.sti_res is (%d %d)", mAI->stStiInfo.w, mAI->stStiInfo.h);

                                        GET_CJSON_OBJ_ITEM_STR(subNode,sti,"mode",sti_mode,sizeof(sti_mode));
                                        mAI->mode = get_mode_index(sti_mode);
                                        subNode = cJSON_GetObjectItem(sti,"mime");
                                        if (subNode) {
                                            mAI->stStiInfo.mime = get_mime_index(subNode->valuestring);
                                        }
										
                                        subNode = cJSON_GetObjectItem(sti, "map");
                                        if (subNode) {
                                            mAI->stStiInfo.stich_mode = get_sti_mode(subNode->valuestring);
                                        }
										
                                        Log.d(TAG, " mode (%d %d)", mAI->mode, mAI->stStiInfo.stich_mode);
                                        switch (mDispType->qr_type) {
                                            case ACTION_PIC:
                                                subNode = cJSON_GetObjectItem(sti, "algorithm");
                                                if (subNode) {
                                                    mAI->stStiInfo.stich_mode = STITCH_OPTICAL_FLOW;
                                                } else {
                                                    mAI->stStiInfo.stich_mode = STITCH_NORMAL;
                                                }
												
                                                //3d
                                                if (mAI->mode == 0) {
                                                    if (mAI->stStiInfo.w >= 7680) {
                                                        mAI->size_per_act = 60;
                                                    } else if (mAI->stStiInfo.w >= 5760) {
                                                        mAI->size_per_act = 45;
                                                    } else {
                                                        mAI->size_per_act = 30;
                                                    }
                                                } else {
                                                    if (mAI->stStiInfo.w >= 7680) {
                                                        mAI->size_per_act = 30;
                                                    } else if (mAI->stStiInfo.w >= 5760) {
                                                        mAI->size_per_act = 25;
                                                    } else {
                                                        mAI->size_per_act = 20;
                                                    }
                                                }
                                                break;
												
                                            case ACTION_VIDEO:
                                                subNode = cJSON_GetObjectItem(sti, "framerate");
                                                if (subNode) {
                                                    mAI->stStiInfo.stStiAct.mStiV.sti_fr = get_fr_index(subNode->valueint);
                                                }

                                                subNode = cJSON_GetObjectItem(sti, "bitrate");
                                                if(subNode) {
                                                    mAI->stStiInfo.stStiAct.mStiV.sti_br = subNode->valueint / 1000;
                                                }
												
                                                mAI->stStiInfo.stich_mode = STITCH_NORMAL;
                                                //exclude timelapse 170831
                                                if (mAI->stOrgInfo.save_org != SAVE_OFF) {
                                                    mAI->size_per_act +=
                                                            mAI->stStiInfo.stStiAct.mStiV.sti_br / 10;
                                                }
                                                break;
												
                                            case ACTION_LIVE:
                                                subNode = cJSON_GetObjectItem(sti, "framerate");
                                                if (subNode) {
                                                    mAI->stStiInfo.stStiAct.mStiL.sti_fr = get_fr_index(subNode->valueint);
                                                }
                                                subNode = cJSON_GetObjectItem(sti, "bitrate");
                                                if (subNode) {
                                                    mAI->stStiInfo.stStiAct.mStiL.sti_br = subNode->valueint / 1000;
                                                }
                                                GET_CJSON_OBJ_ITEM_STR(subNode,sti,"_liveUrl",mAI->stStiInfo.stStiAct.mStiL.url,sizeof(mAI->stStiInfo.stStiAct.mStiL.url));
                                                GET_CJSON_OBJ_ITEM_STR(subNode,sti,"format",mAI->stStiInfo.stStiAct.mStiL.format,sizeof(mAI->stStiInfo.stStiAct.mStiL.format));
                                                GET_CJSON_OBJ_ITEM_INT(subNode,sti,"liveOnHdmi",mAI->stStiInfo.stStiAct.mStiL.hdmi_on)
                                                GET_CJSON_OBJ_ITEM_INT(subNode,sti,"fileSave",mAI->stStiInfo.stStiAct.mStiL.file_save)
                                                break;
                                            SWITCH_DEF_ERROR(mDispType->qr_type)
                                        }
                                    }
                                    else {
                                        mAI->stStiInfo.stich_mode = STITCH_OFF;
                                    }
									
                                    cJSON *aud = cJSON_GetObjectItem(child, "audio");
                                    if (aud) {
                                        GET_CJSON_OBJ_ITEM_STR(subNode,aud,"mime",mAI->stAudInfo.mime,sizeof(mAI->stAudInfo.mime))
                                        GET_CJSON_OBJ_ITEM_STR(subNode,aud,"sampleFormat",mAI->stAudInfo.sample_fmt,sizeof(mAI->stAudInfo.sample_fmt))
                                        GET_CJSON_OBJ_ITEM_STR(subNode,aud,"channelLayout",mAI->stAudInfo.ch_layout,sizeof(mAI->stAudInfo.ch_layout))
                                        GET_CJSON_OBJ_ITEM_INT(subNode,aud,"samplerate",mAI->stAudInfo.sample_rate)
                                        GET_CJSON_OBJ_ITEM_INT(subNode,aud,"bitrate",mAI->stAudInfo.br)

                                    }

                                    cJSON *del = cJSON_GetObjectItem(child, "delay");
                                    if (del) {
                                        mAI->delay = del->valueint;
                                    }
									
                                    cJSON *tl = cJSON_GetObjectItem(child, "timelapse");
                                    if (tl) {
                                        GET_CJSON_OBJ_ITEM_INT(subNode,tl,"interval",
                                                               mAI->stOrgInfo.stOrgAct.mOrgV.tim_lap_int)
                                    }
									
                                    cJSON *hdr_j = cJSON_GetObjectItem(child, "hdr");
                                    if (hdr_j) {
                                        GET_CJSON_OBJ_ITEM_INT(subNode,hdr_j,"count",mAI->stOrgInfo.stOrgAct.mOrgP.hdr_count)
                                        GET_CJSON_OBJ_ITEM_INT(subNode,hdr_j,"min_ev",mAI->stOrgInfo.stOrgAct.mOrgP.min_ev)
                                        GET_CJSON_OBJ_ITEM_INT(subNode,hdr_j,"max_ev",mAI->stOrgInfo.stOrgAct.mOrgP.max_ev)
                                    }

                                    cJSON *bur = cJSON_GetObjectItem(child, "burst");
                                    if (bur) {
                                        GET_CJSON_OBJ_ITEM_INT(subNode, bur, "count",mAI->stOrgInfo.stOrgAct.mOrgP.burst_count)
                                    }

                                    cJSON *props = cJSON_GetObjectItem(child, "properties");
                                    if (props) {
                                        GET_CJSON_OBJ_ITEM_INT(subNode, props, "audio_gain",mAI->stProp.audio_gain);
                                        Log.d(TAG,"aud_gain %d",mAI->stProp.audio_gain);
                                    } else {
                                        mAI->stProp.audio_gain = 96;
                                    }
									
                                    Log.d(TAG,"tl type size (%d %d %d)",
                                          mAI->stOrgInfo.stOrgAct.mOrgV.tim_lap_int,
                                          mDispType->type,mAI->size_per_act);

									mDispType->mAct = mAI;

									switch (mDispType->type) {
                                        case START_LIVE_SUC:	/* 16, 启动录像成功 */
                                            mDispType->control_act = ACTION_LIVE;
                                            break;
										
                                        case CAPTURE:			/* 拍照 */
                                            mDispType->control_act = ACTION_PIC;
                                            break;
										
                                        case START_REC_SUC:		/* 1, 启动录像成功 */
                                            mDispType->control_act = ACTION_VIDEO;
                                            break;
										
                                        case SET_CUS_PARAM:		/* 46, 设置自定义参数 */
                                            mDispType->control_act = CONTROL_SET_CUSTOM;
                                            break;
										
                                        SWITCH_DEF_ERROR(mDispType->type);
                                    }
                                } else {
									/* 没有"req"子节点 */
                                    subNode = cJSON_GetObjectItem(root, "tl_count");
                                    if (subNode) {
                                        mDispType->tl_count = subNode->valueint;
										Log.d(TAG,"get tl count %d", mDispType->tl_count);
                                    }
									
                                    mDispType->qr_type = -1;
									// mDispType->control_act = -1;
                                    mDispType->mAct = nullptr;
                                }
                            }
							
							Log.d(TAG, "mDispType type (%d %d %d)",
								mDispType->type, mDispType->qr_type, mDispType->control_act);

							/* 发送显示信息给UI线程 */
							send_disp_str_type(mDispType);
                        }
                            break;

							
                        case CMD_OLED_SET_SN: {	/* 设置系统信息:(SN和UUID) */
                            sp<SYS_INFO> mSysInfo = sp<SYS_INFO>(new SYS_INFO());
                            GET_CJSON_OBJ_ITEM_STR(subNode, root, "sn", mSysInfo->sn, sizeof(mSysInfo->sn));
                            GET_CJSON_OBJ_ITEM_STR(subNode, root, "uuid", mSysInfo->uuid, sizeof(mSysInfo->uuid));
                            Log.d(TAG, "CMD_OLED_SET_SN sn %s uuid %s", mSysInfo->sn, mSysInfo->uuid);
                            send_sys_info(mSysInfo);
							break;
                        }

						#if 0
						case CMD_OLED_POWER_OFF:
							Log.d(TAG,"rec power off finish");
							send_power_off();
							break;
						#endif

						
                        case CMD_OLED_SYNC_INIT: {	/* 给UI发送同步信息: state, a_v, h_v, c_v */
                            sp<SYNC_INIT_INFO> mSyncInfo = sp<SYNC_INIT_INFO>(new SYNC_INIT_INFO());
                            subNode = cJSON_GetObjectItem(root, "state");
                            if (subNode) {
                                mSyncInfo->state = subNode->valueint;
                            }
							
                            subNode = cJSON_GetObjectItem(root, "a_v");
                            if (subNode) {
                                snprintf(mSyncInfo->a_v, sizeof(mSyncInfo->a_v),
                                         "%s", subNode->valuestring);
                            }
							
                            subNode = cJSON_GetObjectItem(root, "h_v");
                            if (subNode) {
                                snprintf(mSyncInfo->h_v, sizeof(mSyncInfo->h_v), "%s", subNode->valuestring);
                            }
                            subNode = cJSON_GetObjectItem(root, "c_v");
                            if (subNode) {
                                snprintf(mSyncInfo->c_v, sizeof(mSyncInfo->c_v), "%s", subNode->valuestring);
                            }
                            send_sync_info(mSyncInfo);
							break;
                        }

                        case CMD_OLED_DISP_TYPE_ERR: {	/* 给UI发送显示错误信息:  错误类型和错误码 */
                            int type;
                            int err_code;
                            Log.d(TAG,"CMD_OLED_DISP_TYPE_ERR rec");
                            GET_CJSON_OBJ_ITEM_INT(subNode, root, "type", type);
                            GET_CJSON_OBJ_ITEM_INT(subNode, root, "err_code", err_code);
                            Log.d(TAG, "type %d err_code %d", type, err_code);
                            send_err_type_code(type, err_code);
							break;
                        }

                        case CMD_CONFIG_WIFI: {		/* 发送WiFi的配置信息: SSID和密码 */
                            char ssid[128];
                            char pwd[64];
						
                            GET_CJSON_OBJ_ITEM_STR(subNode,root,"ssid",ssid, sizeof(ssid));
                            GET_CJSON_OBJ_ITEM_STR(subNode,root,"pwd",pwd, sizeof(pwd));
                            Log.d(TAG, "ssid %s pwd %s %d", ssid, pwd,strlen(pwd));
                            send_wifi_config(ssid, pwd);
							break;
                        }

                        default:
                            break;
                    }

                    if (0 != root) {
                        cJSON_Delete(root);
                    }
                }
            }
        }
    }
    close_read_fd();
}



void fifo::write_exit_for_read()
{
    char buf[32];
    memset(buf, 0, sizeof(buf));
    int cmd = CMD_EXIT;
    int_to_bytes(buf, cmd);

    int fd = open(FIFO_FROM_CLIENT, O_WRONLY);
    CHECK_NE(fd, -1);
    int len = write(fd, buf, FIFO_HEAD_LEN);
    //pipe broken
    CHECK_EQ(len, FIFO_HEAD_LEN);
//    Log.d(TAG,"write_exit_for_read over");
    close(fd);
}

void fifo::deinit()
{
    Log.d(TAG, "deinit");
    stop_all();
    Log.d(TAG, "deinit2");
    sendExit();
    Log.d(TAG, "deinit3");
    arlog_close();
}

void fifo::close_read_fd()
{
    if (read_fd != -1)
    {
        close(read_fd);
        read_fd = -1;
    }
}

void fifo::close_write_fd()
{
    if (write_fd != -1)
    {
        close(write_fd);
        write_fd = -1;
    }
}

int fifo::get_read_fd()
{
    if (read_fd == -1)
    {
//        Log.d(TAG, " read_fd fd %d", read_fd);
//        bRFifoStop = true;
        read_fd = open(FIFO_FROM_CLIENT, O_RDONLY);
        CHECK_NE(read_fd, -1);
//        bRFifoStop = false;
//        Log.d(TAG, "2 read_fd fd %d", read_fd);
    }
    return read_fd;
}

int fifo::get_write_fd()
{
    if (write_fd == -1)
    {
//        Log.d(TAG, " write fd %d", write_fd);
        bWFifoStop = true;
        write_fd = open(FIFO_TO_CLIENT, O_WRONLY);
        CHECK_NE(write_fd, -1);
        bWFifoStop = false;
//        Log.d(TAG, "2 write fd %d", write_fd);
    }
    return write_fd;
}

int fifo::make_fifo()
{
    if (access(FIFO_FROM_CLIENT, F_OK) == -1) {
        if (mkfifo(FIFO_FROM_CLIENT, 0777)) {
            Log.d("make fifo:%s fail", FIFO_FROM_CLIENT);
            return INS_ERR;
        }
    }

    if (access(FIFO_TO_CLIENT, F_OK) == -1) {
        if (mkfifo(FIFO_TO_CLIENT, 0777)) {
            Log.d("make fifo:%s fail", FIFO_TO_CLIENT);
            return INS_ERR;
        }
    }
    return INS_OK;
}
