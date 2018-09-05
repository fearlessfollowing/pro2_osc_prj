/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: fifo.cpp
** 功能描述: 输入管理器（用于处理按键事件）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/

#include <sys/stat.h>
#include <future>
#include <vector>
#include <common/include_common.h>
#include <trans/fifo.h>
#include <trans/fifo_event.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <common/common.h>
#include <util/bytes_int_convert.h>
#include <hw/MenuUI.h>

#include <sys/action_info.h>
#include <hw/ins_gpio.h>
#include <log/arlog.h>

#include <system_properties.h>
#include <sys/VolumeManager.h>

#include <prop_cfg.h>

#include <json/value.h>
#include <json/json.h>



using namespace std;

static QR_STRUCT mQRInfo[] = {
	100, {{7,4,4,3,1,0}, {6,6,5,0,0,1}, {6,6,6}}
};

static const char *act_mode[] = {"3d_top_left", "pano"};

static const char *all_mime[] = {"h264", "h265", "jpeg", "raw", "raw+jpeg"};

static const char *all_map[] = {"flat", "cube"};

static const int all_frs[] = {24, 25, 30, 60, 120, 5};

//static const char *cal_mode[] = {"pano", "3d"};
//kbits/s
//static const int all_brs[] = {150000,100000,50000,40000,30000,25000,20000,10000,8000,5000,3000};

static sp<fifo> mpFIFO = nullptr;
static const int qr_array_len = 4;

static const RES_INFO mResInfos[] = {
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


static int reboot_cmd = -1;



static sp<fifo> gSysTranObj = NULL;
static std::mutex gSysTranMutex;
static Mutex gSendLock;


sp<fifo>& fifo::getSysTranObj()
{
    unique_lock<mutex> lock(gSysTranMutex);
    if (gSysTranObj != NULL) {
        return gSysTranObj;
    } else {
        gSysTranObj = sp<fifo> (new fifo());
        CHECK_NE(gSysTranObj, nullptr);
    }
    return gSysTranObj;
}


void init_fifo()
{
    /*
    CHECK_EQ(mpFIFO, nullptr);
    mpFIFO = sp<fifo>(new fifo());
    CHECK_NE(mpFIFO, nullptr);
    */
    fifo::getSysTranObj();
}


void fifo::sendUiMessage(sp<ARMessage>& msg)
{
    mOLEDHandle->postUiMessage(msg);
}

void start_all()
{

#if 0
    if (nullptr != mpFIFO) {
        mpFIFO->start_all();
    }
#ele
    fifo::getSysTranObj()->start_all();
#endif

}


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

#define GET_CJSON_OBJ_ITEM_DOUBLE(child, root, key,val) \
    child = cJSON_GetObjectItem(root,key); \
    if(child) \
    {\
        val = child->valuedouble;\
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
    if (strcmp(mode, "pano") == 0) {
        iIndex = 1;
    } else {
        iIndex = 0;
    }
    return iIndex;
}

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

class my_handler : public ARHandler {
public:
    my_handler(fifo *source) : mHandler(source) {
    }

    virtual ~my_handler() override {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override {
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
    CHECK_EQ(sizeof(mResInfos) / sizeof(mResInfos[0]), RES_MAX);
    CHECK_EQ(sizeof(all_frs) / sizeof(all_frs[0]), ALL_FR_MAX);
	
    make_fifo();
    init_thread();

    // //set in the end
    notify = obtainMessage(MSG_UI_KEY);

    mOLEDHandle = (sp<MenuUI>)(new MenuUI(notify)); //oled_handler::getSysUiObj(notify);
    CHECK_NE(mOLEDHandle, nullptr);

    th_read_fifo_ = thread([this] { read_fifo_thread(); });

    Log.d(TAG, "fifo::init() ... OK");
}



void fifo::start_all()
{

}

void fifo::stop_all(bool delay)
{
    Log.d(TAG, "stop_all");

    if (!bReadThread) {
        bReadThread = true;
        if (th_read_fifo_.joinable()) {
            write_exit_for_read();
            th_read_fifo_.join();
        }
    }
    mOLEDHandle = nullptr;

    msg_util::sleep_ms(200);
    Log.d(TAG, "stop_all3");
}

sp<ARMessage> fifo::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}

void fifo::sendExit()
{
    if (!bExit) {
        bExit = true;
        if (th_msg_.joinable()) {
            if (bWFifoStop) {
                // unblock fifo write open while fifo read not happen
                int fd = open(FIFO_TO_CLIENT, O_RDONLY);
                CHECK_NE(fd, -1);
                close(fd);
            }
            obtainMessage(MSG_EXIT)->post();
            th_msg_.join();
        } else {
            Log.e(TAG, " th_msg_ not joinable ");
        }
    }
}


#if 0
void fifo::send_err_type_code(int type, int code)
{
    sp<ERR_TYPE_INFO> mInfo = sp<ERR_TYPE_INFO>(new ERR_TYPE_INFO());
    sp<ARMessage> msg = obtainMessage(MSG_DISP_ERR_TYPE);
    mInfo->type = type;
    mInfo->err_code =code;
    msg->set<sp<ERR_TYPE_INFO>>("err_type_info", mInfo);
    msg->post();
}


void fifo::send_wifi_config(const char *ssid, const char *pwd, int open)
{
    Log.d(TAG, "send wifi config");
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
#endif


/*
 * 来自UI，需要往外发送的消息 - 将消息丢入本地消息循环中
 * 来自Web的消息, 将json转换为消息直接丢入UI线程的消息队列中
 */



/*************************************************************************
** 方法名称: write_fifo
** 方法功能: 发送数据到给osc
** 入口参数: 
**		iEvent - 事件类型
**		str - 数据
** 返 回 值: 无 
** 调 用: 
**
*************************************************************************/
void fifo::write_fifo(int iEvent, const char *str)
{

    AutoMutex _l(gSendLock);

    char data[4096] = {0x00};
    int total = FIFO_HEAD_LEN;
    int write_len;
    int len = 0;

    get_write_fd();

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

#define ACTION_NAME(n) case n: return #n
const char *getActionName(int iAction)
{
    switch (iAction) {
        ACTION_NAME(ACTION_REQ_SYNC);
        ACTION_NAME(ACTION_PIC);
        ACTION_NAME(ACTION_VIDEO);
        ACTION_NAME(ACTION_LIVE);
        ACTION_NAME(ACTION_PREVIEW);
        ACTION_NAME(ACTION_CALIBRATION);
        ACTION_NAME(ACTION_QR);
        ACTION_NAME(ACTION_SET_OPTION);
        ACTION_NAME(ACTION_LOW_BAT);
        ACTION_NAME(ACTION_SPEED_TEST);
        ACTION_NAME(ACTION_POWER_OFF);
        ACTION_NAME(ACTION_GYRO);
        ACTION_NAME(ACTION_NOISE);
        ACTION_NAME(ACTION_CUSTOM_PARAM);
        ACTION_NAME(ACTION_LIVE_ORIGIN);
        ACTION_NAME(ACTION_AGEING);
        ACTION_NAME(ACTION_AWB);
        ACTION_NAME(ACTION_SET_STICH);
        ACTION_NAME(ACTION_QUERY_STORAGE);
        ACTION_NAME(ACTION_FORMAT_TFCARD);

#ifdef ENABLE_MENU_STITCH_BOX
        ACTION_NAME(MENU_STITCH_BOX);
#endif

        ACTION_NAME(ACTION_QUIT_UDISK_MODE);
        ACTION_NAME(ACTION_UPDATE_REC_LEFT_SEC);
        ACTION_NAME(ACTION_UPDATE_LIVE_REC_LEFT_SEC);


    default: return "Unkown Action";
    }    
}

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

#if 0
void fifo::handleUiTakePicReq(sp<ACTION_INFO>& mActInfo, cJSON* root, cJSON *param)
{
    Log.d(TAG, ">>>> ACTION_PIC: mime index = %d", mActInfo->stOrgInfo.mime);

    cJSON *org = cJSON_CreateObject();

    cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
    #if 0
    cJSON_AddNumberToObject(param, "delay", mActInfo->delay);
    #else 
    cJSON_AddNumberToObject(param, "delay", 0);
    #endif

    if (mActInfo->stOrgInfo.stOrgAct.mOrgP.burst_count > 0) {
        cJSON *burst = cJSON_CreateObject();
        cJSON_AddTrueToObject(burst,"enable");
        cJSON_AddNumberToObject(burst, "count", mActInfo->stOrgInfo.stOrgAct.mOrgP.burst_count);
        cJSON_AddItemToObject(param, "burst",burst);

    } else if (mActInfo->stOrgInfo.stOrgAct.mOrgP.hdr_count > 0) {
        cJSON *hdr = cJSON_CreateObject();
        cJSON_AddTrueToObject(hdr,"enable");
        cJSON_AddNumberToObject(hdr, "count", mActInfo->stOrgInfo.stOrgAct.mOrgP.hdr_count);
        cJSON_AddNumberToObject(hdr, "min_ev", mActInfo->stOrgInfo.stOrgAct.mOrgP.min_ev);
        cJSON_AddNumberToObject(hdr, "max_ev", mActInfo->stOrgInfo.stOrgAct.mOrgP.max_ev);
        cJSON_AddItemToObject(param, "hdr", hdr);
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

    if (mActInfo->stStiInfo.stich_mode != STITCH_OFF) {
        cJSON *sti = cJSON_CreateObject();
        cJSON_AddStringToObject(sti, "mode", act_mode[mActInfo->mode]);
        cJSON_AddNumberToObject(sti, "height", mActInfo->stStiInfo.h);
        cJSON_AddNumberToObject(sti, "width", mActInfo->stStiInfo.w);
        Log.d(TAG, "sti mode %d", mActInfo->stStiInfo.stich_mode);
        if (STITCH_CUBE == mActInfo->stStiInfo.stich_mode) {
            cJSON_AddStringToObject(sti, "map", "cube");
        }

        cJSON_AddStringToObject(sti, "mime", all_mime[mActInfo->stStiInfo.mime]);
        if (STITCH_OPTICAL_FLOW == mActInfo->stStiInfo.stich_mode) {
            cJSON_AddStringToObject(sti, "algorithm", "opticalFlow");
        }
        cJSON_AddItemToObject(param, "stiching", sti);
    } else {
        Log.w(TAG, "sti off");        
    }

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
        Log.w(TAG, "aud s 0");
    }

    cJSON_AddItemToObject(root, "parameters", param);

}



void fifo::handleUiTakeVidReq(sp<ACTION_INFO>& mActInfo, cJSON* root, cJSON *param)
{
    cJSON *org = cJSON_CreateObject();
    cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
    cJSON_AddNumberToObject(org, "framerate", (all_frs[mActInfo->stOrgInfo.stOrgAct.mOrgV.org_fr]));
    cJSON_AddNumberToObject(org, "bitrate", mActInfo->stOrgInfo.stOrgAct.mOrgV.org_br * 1000);
    Log.d(TAG,"vid mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
    if (mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int != 0) {
        cJSON *tim_lap = cJSON_CreateObject();
        cJSON_AddTrueToObject(tim_lap, "enable");
        cJSON_AddNumberToObject(tim_lap, "interval",
                                mActInfo->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
        cJSON_AddItemToObject(param, "timelapse", tim_lap);
    }
    
    if (mActInfo->stOrgInfo.stOrgAct.mOrgV.logMode == 1) {
        cJSON_AddNumberToObject(org, "logMode", mActInfo->stOrgInfo.stOrgAct.mOrgV.logMode);
    }

    if (mActInfo->stOrgInfo.save_org == SAVE_OFF) {
        cJSON_AddFalseToObject(org, "saveOrigin");
    } else {
        cJSON_AddTrueToObject(org, "saveOrigin");
    }
    
    cJSON_AddNumberToObject(org, "width", mActInfo->stOrgInfo.w);
    cJSON_AddNumberToObject(org, "height", mActInfo->stOrgInfo.h);


    cJSON_AddNumberToObject(org, "storage_loc", mActInfo->stOrgInfo.locMode);


    /* {"paramters": {"origin":{}} */
    cJSON_AddItemToObject(param, "origin", org);

    Log.d(TAG, " mActInfo->stStiInfo.stich_mode %d", mActInfo->stStiInfo.stich_mode);

    if (mActInfo->stStiInfo.stich_mode != STITCH_OFF) {
        cJSON *sti = cJSON_CreateObject();
        cJSON_AddStringToObject(sti, "mode", act_mode[mActInfo->mode]);
        cJSON_AddNumberToObject(sti, "height", mActInfo->stStiInfo.h);
        cJSON_AddNumberToObject(sti, "width", mActInfo->stStiInfo.w);
        Log.d(TAG, "sti mode %d", mActInfo->stStiInfo.stich_mode);
        if (STITCH_CUBE == mActInfo->stStiInfo.stich_mode) {
            cJSON_AddStringToObject(sti, "map", "cube");
        }						

        Log.d(TAG, "mActInfo->stStiInfo.stStiAct.mStiV.sti_fr is %d",
                mActInfo->stStiInfo.stStiAct.mStiV.sti_fr);
        
        cJSON_AddNumberToObject(sti, "framerate",
                                (all_frs[mActInfo->stStiInfo.stStiAct.mStiV.sti_fr]));
        cJSON_AddNumberToObject(sti, "bitrate", mActInfo->stStiInfo.stStiAct.mStiV.sti_br * 1000);
        cJSON_AddStringToObject(sti, "mime", all_mime[mActInfo->stStiInfo.mime]);

        cJSON_AddItemToObject(param, "stiching", sti);
    } else {
        Log.w(TAG, "sti off");
    }   


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

}


void fifo::handleUiTakeLiveReq(sp<ACTION_INFO>& mActInfo, cJSON *root, cJSON *param)
{


    cJSON *org = cJSON_CreateObject();
    cJSON_AddStringToObject(org, "mime", all_mime[mActInfo->stOrgInfo.mime]);
    cJSON_AddNumberToObject(org, "framerate", (all_frs[mActInfo->stOrgInfo.stOrgAct.mOrgL.org_fr]));
    cJSON_AddNumberToObject(org, "bitrate", mActInfo->stOrgInfo.stOrgAct.mOrgL.org_br * 1000);
    if (mActInfo->stOrgInfo.stOrgAct.mOrgL.logMode == 1) {
        cJSON_AddNumberToObject(org, "logMode", mActInfo->stOrgInfo.stOrgAct.mOrgL.logMode);
    }

    if (mActInfo->stOrgInfo.save_org == SAVE_OFF) {
        cJSON_AddFalseToObject(org, "saveOrigin");
    } else {
        cJSON_AddTrueToObject(org, "saveOrigin");
    }
    


    cJSON_AddNumberToObject(org, "width", mActInfo->stOrgInfo.w);
    cJSON_AddNumberToObject(org, "height", mActInfo->stOrgInfo.h);

    cJSON_AddNumberToObject(org, "storage_loc", mActInfo->stOrgInfo.locMode);

    /* {"paramters": {"origin":{}} */
    cJSON_AddItemToObject(param, "origin", org);

    Log.d(TAG, " mActInfo->stStiInfo.stich_mode %d", mActInfo->stStiInfo.stich_mode);

    if (mActInfo->stStiInfo.stich_mode != STITCH_OFF) {
        cJSON *sti = cJSON_CreateObject();
        cJSON_AddStringToObject(sti, "mode", act_mode[mActInfo->mode]);
        cJSON_AddNumberToObject(sti, "height", mActInfo->stStiInfo.h);
        cJSON_AddNumberToObject(sti, "width", mActInfo->stStiInfo.w);
        Log.d(TAG, "sti mode %d", mActInfo->stStiInfo.stich_mode);
        if (STITCH_CUBE == mActInfo->stStiInfo.stich_mode) {
            cJSON_AddStringToObject(sti, "map", "cube");
        }
					
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

        cJSON_AddItemToObject(param, "stiching", sti);
    } else {
        Log.w(TAG, "sti off");
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
        Log.w(TAG, "aud s 0");
    }

    /* {"action":ACTION_XX, "parameters": {}} */
    cJSON_AddItemToObject(root, "parameters", param); 

}
#endif


void fifo::handleUiKeyReq(int action, const sp<ARMessage>& msg)
{
    int iIndex = -1;
    Json::FastWriter writer;
    string sendDataStr;

    Json::Value rootNode;
    Json::Value paramNode;

    switch (action) {
        
        case ACTION_PIC: {
            Json::Value* pTakePicJson;
            CHECK_EQ(msg->find<Json::Value*>("take_pic", &pTakePicJson), true);

            rootNode["action"] = ACTION_PIC;
            rootNode["parameters"] = *pTakePicJson;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Pic: %s", sendDataStr.c_str());

            break;
        }


        case ACTION_VIDEO: {
            Json::Value* pTakeVideoJson;
            CHECK_EQ(msg->find<Json::Value*>("take_video", &pTakeVideoJson), true);

            rootNode["action"] = ACTION_VIDEO;
            rootNode["parameters"] = *pTakeVideoJson;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Video: %s", sendDataStr.c_str());

            break;
        }

        case ACTION_LIVE: {
            Json::Value* pTakeLiveJson;
            CHECK_EQ(msg->find<Json::Value*>("take_video", &pTakeLiveJson), true);

            rootNode["action"] = ACTION_LIVE;
            rootNode["parameters"] = *pTakeLiveJson;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Live: %s", sendDataStr.c_str());
            break;
        }


        /* 设置模板参数:
         * {"action": 18, "parameters":{"audio_gain":XX, "len_param": {}, "gamma_param": "XXXX"}}
         */

        case ACTION_CUSTOM_PARAM: {
            Json::Value* pSetCustomerArg;
            Json::Value root;
            CHECK_EQ(msg->find<Json::Value*>("set_customer_param", &pSetCustomerArg), true);

            rootNode["action"] = ACTION_PIC;
            rootNode["parameters"] = (*pSetCustomerArg)["properties"];

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Set customer param: %s", sendDataStr.c_str());
            break;
        }
        
        case ACTION_LIVE_ORIGIN: {
            rootNode["action"] = ACTION_LIVE_ORIGIN;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Live origin: %s", sendDataStr.c_str());
            break;
        }
						
        case ACTION_CALIBRATION: {
            rootNode["action"] = ACTION_CALIBRATION;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Calibration: %s", sendDataStr.c_str());
            break;
        }


        /* {"action": ACTION_REQ_SYNC, parameters":{"sn":"0123456", "r_v":"xxxx", "p_v":"ddfff", "k_v":"xxxxx"}} */
        case ACTION_REQ_SYNC: {
            sp<REQ_SYNC> mReqSync;
            CHECK_EQ(msg->find<sp<REQ_SYNC>>("req_sync", &mReqSync), true);

            paramNode["sn"] = mReqSync->sn;
            paramNode["r_v"] = mReqSync->r_v;  
            paramNode["p_v"] = mReqSync->p_v;
            paramNode["k_v"] = mReqSync->k_v;
            rootNode["action"] = ACTION_REQ_SYNC;
            rootNode["parameters"] = *pTakePicJson;

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Req Sync: %s", sendDataStr.c_str());

            break;
        }

        
        /* {"action": ACTION_LOW_BAT} */
        case ACTION_LOW_BAT: {
            CHECK_EQ(msg->find<int>("cmd", &reboot_cmd), true);
            Log.d(TAG, "low bat reboot cmd is %d", reboot_cmd);

            rootNode["action"] = ACTION_LOW_BAT;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Low Battery: %s", sendDataStr.c_str());

            break;
        }

        #if 0
        /* {"action": ACTION_LOW_PROTECT} */

        case ACTION_LOW_PROTECT:
            break;
        #endif
					
        /* {"action": ACTION_SPEED_TEST, "paramters":{"path":"/media/nvidia/xxxx"}} ACTION_SPEED_TEST*/
        case ACTION_SPEED_TEST: {
            const char *path;
            CHECK_EQ(msg->find<const char *>("path", &path), true);

            paramNode["path"] = path;
            rootNode["action"] = ACTION_SPEED_TEST;
            rootNode["parameters"] = paramNode

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action Speed test: %s", sendDataStr.c_str());
            break;
        }

        /* {"action": ACTION_NOISE} */
        case ACTION_NOISE: 
        case ACTION_GYRO:
        case ACTION_AGEING:
        case ACTION_QUIT_UDISK_MODE:
        case ACTION_QUERY_STORAGE: 
        case ACTION_SET_STICH: 
        case case ACTION_POWER_OFF: {
            rootNode["action"] = action;
            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action %s: %s", getActionName(action), sendDataStr.c_str());           
            break;
        }

        case ACTION_AWB: {       
            paramNode["name"] = "camera._calibrationAwb";
            rootNode["action"] = ACTION_AWB;
            rootNode["parameters"] = paramNode

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action AWB: %s", sendDataStr.c_str());
            break;
        }

        case ACTION_UPDATE_REC_LEFT_SEC: {
            u64 recLefSecs;
            u64 liveRecLeftSecs;
            u64 recSecs;
            u64 liveSecs;

            CHECK_EQ(msg->find<u64>("rec_left_sec", &recLefSecs), true);
            CHECK_EQ(msg->find<u64>("live_rec_left_sec", &liveRecLeftSecs), true);
            CHECK_EQ(msg->find<u64>("rec_sec", &recSecs), true);
            CHECK_EQ(msg->find<u64>("live_rec_sec", &liveSecs), true);


            paramNode["rec_left_sec"]       = (u32)recLefSecs;
            paramNode["live_rec_left_sec"]  = (u32)liveRecLeftSecs;
            paramNode["rec_sec"]            = (u32)recSecs;
            paramNode["live_rec_sec"]       = (u32)liveSecs;

            rootNode["action"] = ACTION_UPDATE_REC_LEFT_SEC;
            rootNode["parameters"] = paramNode;

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action ACTION_UPDATE_REC_LEFT_SEC: %s", sendDataStr.c_str());


            break;
        }


        /* 
            {
                "action":ACTION_FORMAT_TFCARD, 
                "parameters": {
                    "name": "camera._formatCameraMoudle",
                    "parameters":{
                        "index": 1
                    }
                }
            }
        */
        case ACTION_FORMAT_TFCARD: {    /* 格式化请求 */
            CHECK_EQ(msg->find<int>("index", &iIndex), true);

            Json::Value indexNode;

            indexNode["index"] = iIndex;
            paramNode["name"] = "camera._formatCameraMoudle";
            paramNode["parameters"] = indexNode;

            rootNode["action"] = ACTION_FORMAT_TFCARD;
            rootNode["parameters"] = paramNode;

            sendDataStr = writer.write(rootNode);
		    Log.d(TAG, "Action ACTION_FORMAT_TFCARD: %s", sendDataStr.c_str());
            break;
        }


        case ACTION_SET_OPTION: {
            int type;
            CHECK_EQ(msg->find<int>("type", &type), true);

            Log.d(TAG, " type is %d", type);
            switch (type) {
                /* {"action": ACTION_SET_OPTION, "parameters":{"property":"flicker", "value":int} } */
                case OPTION_FLICKER: { 
                    int flicker;
                    CHECK_EQ(msg->find<int>("flicker", &flicker), true);

                    paramNode["property"] = "flicker";
                    paramNode["value"] = flicker;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());

                    break;
                }

                /* {"action": ACTION_SET_OPTION, "parameters":{"property":"logMode", "mode":int, "effect":int, ""} } */
                case OPTION_LOG_MODE: {	 /* {"action": ACTION_SET_OPTION, "type": OPTION_FLICKER } */
                    int mode;
                    int effect;

                    CHECK_EQ(msg->find<int>("mode", &mode), true);
                    CHECK_EQ(msg->find<int>("effect", &effect), true);
                    

                    Json::Value valNode;

                    valNode["mode"] = mode;
                    valNode["effect"] = effect;

                    paramNode["property"] = "logMode";
                    paramNode["value"] = valNode;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());
                    break;
                }
                
                /* {"action": ACTION_SET_OPTION, {"property": "fanless", "value": 0/1}} */
                case OPTION_SET_FAN: {
                    int fan;
                    CHECK_EQ(msg->find<int>("fan", &fan), true);

                    paramNode["property"] = "fanless";
                    paramNode["value"] = (fan == 1) ? 0 : 1;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());                    
                    break;
                }

                /* {"action": ACTION_SET_OPTION, {"property": "panoAudio", "value": 0/1}} */
                case OPTION_SET_AUD: {
                    int aud;
                    CHECK_EQ(msg->find<int>("aud", &aud), true);

                    paramNode["property"] = "panoAudio";
                    paramNode["value"] = aud;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());                    
                     
                    break;
                }

                /* {"action": ACTION_SET_OPTION, {"property": "stabilization_cfg", "value": 0/1}} */
                case OPTION_GYRO_ON: {
                    int gyro_on;
                    CHECK_EQ(msg->find<int>("gyro_on", &gyro_on), true);

                    paramNode["property"] = "stabilization_cfg";
                    paramNode["value"] = gyro_on;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());                    
                    break;
                }

                /* {"action": ACTION_SET_OPTION, {"property": "logo", "value": 0/1}} */
                case OPTION_SET_LOGO: {
                    int logo_on;
                    CHECK_EQ(msg->find<int>("logo_on", &logo_on), true);

                    paramNode["property"] = "logo";
                    paramNode["value"] = logo_on;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());                    
            
                    break;
                }

                /*
                 {
                     "action":OPTION_SET_VID_SEG,
                     "parameters":{
                         "property": "video_fragment",
                         "value": 0/1
                     }
                 }
                 */
                case OPTION_SET_VID_SEG: {
                    int video_fragment;
                    CHECK_EQ(msg->find<int>("video_fragment", &video_fragment), true);
                   
                    paramNode["property"] = "video_fragment";
                    paramNode["value"] = video_fragment;
                    rootNode["action"] = ACTION_SET_OPTION;
                    rootNode["parameters"] = paramNode; 

                    sendDataStr = writer.write(rootNode);
		            Log.d(TAG, "Action ACTION_SET_OPTION: %s", sendDataStr.c_str());                    

                    break;
                }

            #if 0
                case OPTION_SET_AUD_GAIN: {
                    sp<CAM_PROP> mCamProp;
                    CHECK_EQ(msg->find<sp<CAM_PROP>>("cam_prop", &mCamProp), true);
                    param = cJSON_CreateObject();
                    Log.d(TAG,"set aud gain %d",
                            mCamProp->audio_gain);
                    cJSON_AddStringToObject(param, "property", "audio_gain");
                    cJSON_AddNumberToObject(param, "value", mCamProp->audio_gain);
                     break;
                }
            #endif
                               
                SWITCH_DEF_ERROR(type)
            }
            break;
        }

        default:
            break;
    }

    write_fifo(EVENT_OLED_KEY, sendDataStr.c_str());
}



/*************************************************************************
** 方法名称: handleUiNotify
** 方法功能: 处理来自UI线程的消息
** 入口参数: 
**		msg - 消息指针
** 返 回 值: 无 
** 调     用: 
** {"name": "camera._calibrationAwb"}
*************************************************************************/
void fifo::handleUiNotify(const sp<ARMessage> &msg)
{
    int what;
    CHECK_EQ(msg->find<int>("what", &what), true);

    switch (what) {

		/* msg.what = OLED_KEY
	 	 * msg.action = int
	 	 * msg.action_info = sp<ACTION_INFO>	[optional]
	 	 * {"action":[0/9], "action_info": }
	 	 */
	 	 
        case MenuUI::OLED_KEY: {
            int action;
            CHECK_EQ(msg->find<int>("action", &action), true);

			/* {"action": [0/9]} */
			Log.d(TAG, "FIFO RECV OLED ACTION [%s]", getActionName(action));
            handleUiKeyReq(action, msg);
            break;
        }
			
		
		/* 
		 * msg.what = UPDATE_BAT
		 * msg.bat_info = sp<BAT_INFO>
		 * {"battery_level": 100, "battery_charge": 0/1, "int_tmp": 20, "tmp":20}
		 */
        case MenuUI::UPDATE_BAT: {		/* 电池状态更新 */
            sp<BAT_INFO> mBatInfo;
            Json::FastWriter writer;
            string sendDataStr;

            CHECK_EQ(msg->find<sp<BAT_INFO>>("bat_info", &mBatInfo), true);

            Json::Value rootNode;
            rootNode["battery_level"]   = mBatInfo->battery_level;
            rootNode["battery_charge"]  = (mBatInfo->bCharge == true) ? 1: 0;            
            rootNode["int_tmp"]         =  mBatInfo->int_tmp;
            rootNode["tmp"]             =  mBatInfo->tmp;            

            sendDataStr = writer.write(rootNode);

            Log.d(TAG, "-------- FIFO RECV UPDATE_BAT");
            write_fifo(EVENT_BATTERY, pSrt);
			break;
        }

        default:
            break;
    }
}



void fifo::postTranMessage(sp<ARMessage>& msg)
{
    msg->setHandler(mHandler);
    msg->post();
}


void fifo::handleSavePathChanged(const sp<ARMessage>& msg)
{
    sp<SAVE_PATH> mSavePath;
    CHECK_EQ(msg->find<sp<SAVE_PATH>>("save_path", &mSavePath), true);

    Log.d(TAG, "[%s: %d] <<--------------------------- [SavePathChanged Message] %s", __FILE__, __LINE__, mSavePath->path);
    write_fifo(EVENT_SAVE_PATH, mSavePath->path);   
}

void fifo::handleUpdateDevList(const sp<ARMessage>& msg)
{
    sp<SAVE_PATH> mSavePath;
    CHECK_EQ(msg->find<sp<SAVE_PATH>>("dev_list", &mSavePath), true);

    Log.d(TAG, "[%s: %d] <<--------------------------- [UpdateDevList Message] %s", __FILE__, __LINE__, mSavePath->path);
    write_fifo(EVENT_DEV_NOTIFY, mSavePath->path);   
}



#define MESSAGE_NAME(n) case n: return #n
const char *getMessageName(int iMessage)
{
    switch (iMessage) {
        MESSAGE_NAME(MSG_UI_KEY);
        MESSAGE_NAME(MSG_DEV_NOTIFY);
        MESSAGE_NAME(MSG_DISP_STR_TYPE);
        MESSAGE_NAME(MSG_DISP_ERR_TYPE);
        MESSAGE_NAME(MSG_SET_WIFI_CONFIG);
        MESSAGE_NAME(MSG_SET_SYS_INFO);
        MESSAGE_NAME(MSG_SET_SYNC_INFO);
        MESSAGE_NAME(MSG_START_POWER_OFF);
        MESSAGE_NAME(MSG_SAVE_PATH_CHANGE);
        MESSAGE_NAME(MSG_UPDATE_CURRENT_SAVE_LIST);
        MESSAGE_NAME(MSG_TRAN_INNER_UPDATE_TF);
        MESSAGE_NAME(MSG_EXIT);

    default: return "Unkown Message";
    }    
}


/*************************************************************************
** 方法名称: handleMessage
** 方法功能: FIFO交互线程消息处理
** 入口参数: 
**      msg - 消息指针
** 返回值: 无
** 调用:
** 改动: 让消息循环只处理来自UI线程的消息 - 2018年9月5日
*************************************************************************/
void fifo::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();

	{
        if (MSG_EXIT == what) {		/* 线程退出消息 */
            mLooper->quit();
            close_write_fd();
        } else {

            switch (what) {

			/* UI -> FIFO -> OSC */
			case MSG_UI_KEY: {	/* 来自UI线程的Key */
				handleUiNotify(msg);
				break;
			}


            case MSG_SAVE_PATH_CHANGE: {    /* 来自VolumeManager */
                handleSavePathChanged(msg);                
                break;
            }

            case MSG_UPDATE_CURRENT_SAVE_LIST: {
                handleUpdateDevList(msg);
                break;
            }

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
** 调 用: 
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



#define RECV_MSG(n) case n: return #n
const char *getRecvMsgName(int iMessage)
{
    switch (iMessage) {
        RECV_MSG(CMD_OLED_DISP_TYPE);
        RECV_MSG(CMD_OLED_SET_SN);
        RECV_MSG(CMD_OLED_SYNC_INIT);
        RECV_MSG(CMD_OLED_DISP_TYPE_ERR);
        RECV_MSG(CMD_CONFIG_WIFI);
        RECV_MSG(ACTION_CALIBRATION);
        RECV_MSG(CMD_WEB_UI_TF_CHANGED);
        RECV_MSG(CMD_WEB_UI_TF_NOTIFY);
        RECV_MSG(CMD_WEB_UI_TF_FORMAT);


    default: return "Unkown Message Type";
    }    
}


#define DISPLAY_TYPE(n) case n: return #n
const char *getDispType(int iType)
{
    switch (iType) {
        DISPLAY_TYPE(START_RECING);
        DISPLAY_TYPE(START_REC_SUC);
        DISPLAY_TYPE(START_REC_FAIL);
        DISPLAY_TYPE(STOP_RECING);
        DISPLAY_TYPE(STOP_REC_SUC);

        DISPLAY_TYPE(STOP_REC_FAIL);
        DISPLAY_TYPE(CAPTURE);
        DISPLAY_TYPE(CAPTURE_SUC);
        DISPLAY_TYPE(CAPTURE_FAIL);
        DISPLAY_TYPE(COMPOSE_PIC);

        DISPLAY_TYPE(COMPOSE_PIC_FAIL);
        DISPLAY_TYPE(COMPOSE_PIC_SUC);
        DISPLAY_TYPE(COMPOSE_VIDEO);
        DISPLAY_TYPE(COMPOSE_VIDEO_FAIL);
        DISPLAY_TYPE(COMPOSE_VIDEO_SUC);

        DISPLAY_TYPE(STRAT_LIVING);
        DISPLAY_TYPE(START_LIVE_SUC);
        DISPLAY_TYPE(START_LIVE_FAIL);
        DISPLAY_TYPE(STOP_LIVING);
        DISPLAY_TYPE(STOP_LIVE_SUC);


        DISPLAY_TYPE(STOP_LIVE_FAIL);
        DISPLAY_TYPE(PIC_ORG_FINISH);
        DISPLAY_TYPE(START_LIVE_CONNECTING);
        DISPLAY_TYPE(START_CALIBRATIONING);
        DISPLAY_TYPE(CALIBRATION_SUC);

        DISPLAY_TYPE(CALIBRATION_FAIL);
        DISPLAY_TYPE(START_PREVIEWING);
        DISPLAY_TYPE(START_PREVIEW_SUC);
        DISPLAY_TYPE(START_PREVIEW_FAIL);
        DISPLAY_TYPE(STOP_PREVIEWING);

        DISPLAY_TYPE(STOP_PREVIEW_SUC);
        DISPLAY_TYPE(STOP_PREVIEW_FAIL);
        DISPLAY_TYPE(START_QRING);
        DISPLAY_TYPE(START_QR_SUC);
        DISPLAY_TYPE(START_QR_FAIL);

        DISPLAY_TYPE(STOP_QRING);
        DISPLAY_TYPE(STOP_QR_SUC);
        DISPLAY_TYPE(STOP_QR_FAIL);
        DISPLAY_TYPE(QR_FINISH_CORRECT);
        DISPLAY_TYPE(QR_FINISH_ERROR);

        DISPLAY_TYPE(CAPTURE_ORG_SUC);
        DISPLAY_TYPE(CALIBRATION_ORG_SUC);
        DISPLAY_TYPE(SET_CUS_PARAM);
        DISPLAY_TYPE(QR_FINISH_UNRECOGNIZE);
        DISPLAY_TYPE(TIMELPASE_COUNT);

        DISPLAY_TYPE(START_NOISE_SUC);
        DISPLAY_TYPE(START_NOISE_FAIL);
        DISPLAY_TYPE(START_NOISE);
        DISPLAY_TYPE(START_LOW_BAT_SUC);
        DISPLAY_TYPE(START_LOW_BAT_FAIL);

        DISPLAY_TYPE(LIVE_REC_OVER);
        DISPLAY_TYPE(SET_SYS_SETTING);
        DISPLAY_TYPE(STITCH_PROGRESS);
        DISPLAY_TYPE(START_BLC);
        DISPLAY_TYPE(STOP_BLC);

        DISPLAY_TYPE(START_GYRO);
        DISPLAY_TYPE(START_GYRO_SUC);
        DISPLAY_TYPE(START_GYRO_FAIL);
        DISPLAY_TYPE(SPEED_TEST_SUC);
        DISPLAY_TYPE(SPEED_TEST_FAIL);

        DISPLAY_TYPE(SPEED_START);
        DISPLAY_TYPE(SYNC_REC_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_PIC_CAPTURE_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_PIC_STITCH_AND_PREVIEW);
        DISPLAY_TYPE(SYNC_LIVE_AND_PREVIEW);

        DISPLAY_TYPE(SYNC_LIVE_CONNECT_AND_PREVIEW);
        DISPLAY_TYPE(START_STA_WIFI_FAIL);
        DISPLAY_TYPE(STOP_STA_WIFI_FAIL);
        DISPLAY_TYPE(START_AP_WIFI_FAIL);
        DISPLAY_TYPE(STOP_AP_WIFI_FAIL);

        DISPLAY_TYPE(START_QUERY_STORAGE);
        DISPLAY_TYPE(START_QUERY_STORAGE_SUC);
        DISPLAY_TYPE(START_QUERY_STORAGE_FAIL);
        DISPLAY_TYPE(START_BPC);
        DISPLAY_TYPE(STOP_BPC);

        DISPLAY_TYPE(ENTER_UDISK_MODE);
        DISPLAY_TYPE(EXIT_UDISK_MODE);
        DISPLAY_TYPE(EXIT_UDISK_DONE);
        DISPLAY_TYPE(RESET_ALL_CFG);
        DISPLAY_TYPE(MAX_TYPE);

    default: 
        return "Display Type";
    }    
}


void fifo::handleQrContent(sp<DISP_TYPE>& mDispType, cJSON* root, cJSON *subNode)
{
	int iArraySize = cJSON_GetArraySize(subNode);
	int qr_version;
	int qr_index = -1;
	int qr_action_index;
	int sti_res;
	cJSON *child = nullptr;

	subNode = subNode->child;
	CHECK_EQ(subNode->type, cJSON_Number);
	qr_version = subNode->valueint;
	
	for (u32 i = 0; i < sizeof(mQRInfo) / sizeof(mQRInfo[0]); i++) {
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
		
		Log.d(TAG,"qr type %d", mDispType->qr_type);
		qr_action_index = (mDispType->qr_type - ACTION_PIC);
		Log.d(TAG, "qr action index %d iArraySize %d "
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

			Log.d(TAG, "size per act %d", mDispType->mAct->size_per_act);
			//org
			subNode = subNode->next;
			CHECK_EQ(subNode->type, cJSON_Array);
                                        
			iArraySize = cJSON_GetArraySize(subNode);
			if (iArraySize > 0) {
				CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].org_size);
				child = subNode->child;
				CHECK_EQ(child->type, cJSON_Number);
				
				org_res = child->valueint;
				mDispType->mAct->stOrgInfo.w = mResInfos[org_res].w;

				// org 3d or pano h is same 170721
				mDispType->mAct->stOrgInfo.h = mResInfos[org_res].h[0];

				// skip mime
				child = child->next;
				CHECK_EQ(child->type, cJSON_Number);
				mDispType->mAct->stOrgInfo.mime = child->valueint;

				switch (mDispType->qr_type) {
					// old [1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm]]
					// new [v,1, Predicate, [resolution, mime, saveOrigin, delay], [resolution, mime, mode, algorithm], [enable, count, step], [enable, count]]
					case ACTION_PIC:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->delay = child->valueint;
						break;

					// [resolution, mime, framerate, originBitrate,saveOrigin]
					case ACTION_VIDEO:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_fr = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.org_br = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.logMode = child->valueint;
						break;
						
					// [resolution, mime, framerate, originBitrate,saveOrigin]
					case ACTION_LIVE:
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_fr = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.org_br = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.save_org = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgL.logMode = child->valueint;
						break;
						
					SWITCH_DEF_ERROR(mDispType->qr_type)
				}
			} else {
				Log.w(TAG, "no org mDispType->qr_type %d", mDispType->qr_type);
			}

			//sti
			subNode = subNode->next;
			CHECK_EQ(subNode->type, cJSON_Array);
			iArraySize = cJSON_GetArraySize(subNode);
			switch (mDispType->qr_type) {
				//[resolution, mime, mode, algorithm]]
				case ACTION_PIC:
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						sti_res = child->valueint;

						//skip mime
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stStiInfo.mime = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->mode = child->valueint;

						mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];

						child = child->next;
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
					} else {
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
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].hdr_size);
						
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.hdr_count = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.min_ev = child->valueint;

						child = child->next;
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
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].burst_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
							
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count = child->valueint;
						Log.d(TAG, "burst count %d",
										mDispType->mAct->stOrgInfo.stOrgAct.mOrgP.burst_count);
					}
					break;

				case ACTION_VIDEO:
					// [resolution, mime, mode, framerate, stichBitrate]]
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						sti_res = child->valueint;
						
						//skip mime
						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						mDispType->mAct->stStiInfo.mime = child->valueint;

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->mode = child->valueint;

						mDispType->mAct->stStiInfo.w = mResInfos[sti_res].w;
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[ mDispType->mAct->mode];

						child = child->next;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stStiInfo.stStiAct.mStiV.sti_fr = child->valueint;

						child = child->next;
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
					} else {
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
					if (iArraySize > 0) {
						CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].timelap_size);
						child = subNode->child;
						CHECK_EQ(child->type, cJSON_Number);
						
						mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int = child->valueint *1000;
						Log.d(TAG,"tim_lap_int  %d", mDispType->mAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
						if (mDispType->mAct->size_per_act == 0) {
							mDispType->mAct->size_per_act = 10;
						}
					}
					break;
					
				// [resolution, mime, mode, framerate, stichBitrate, hdmiState]
				case ACTION_LIVE:
					// iArraySize = cJSON_GetArraySize(subNode);
					CHECK_EQ(iArraySize, mQRInfo[qr_index].astQRInfo[qr_action_index].stich_size);

					child = subNode->child;
					CHECK_EQ(child->type, cJSON_Number);
					
					sti_res = child->valueint;
					//skip mime
					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->stStiInfo.mime = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->mode = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_fr = child->valueint;

					child = child->next;
					CHECK_EQ(child->type, cJSON_Number);
					
					mDispType->mAct->stStiInfo.stStiAct.mStiL.sti_br = child->valueint;

					child = child->next;
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
					if (strlen(subNode->valuestring) > 0) {
						snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.url,
									sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.url),
									"%s",
									subNode->valuestring);
					}
					
					subNode = cJSON_GetObjectItem(root, "proExtra");
					if (subNode) {
						child = cJSON_GetObjectItem(subNode, "saveStitch");
						if (child) {
							if (SAVE_OFF == child->valueint) {
								mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 0;
							} else {
								mDispType->mAct->stStiInfo.stStiAct.mStiL.file_save = 1;
							}
						}
						
						child = cJSON_GetObjectItem(subNode, "format");
						if (child) {
							snprintf(mDispType->mAct->stStiInfo.stStiAct.mStiL.format,sizeof(mDispType->mAct->stStiInfo.stStiAct.mStiL.format),
                                                                 "%s",child->valuestring);
						}
						child = cJSON_GetObjectItem(subNode, "map");
						if (child) {
							mDispType->mAct->stStiInfo.stich_mode = get_sti_mode(child->valuestring);
						} else {
							//force to stitch normal
							mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
						}
					} else {
						//force to stitch normal
						mDispType->mAct->stStiInfo.stich_mode = STITCH_NORMAL;
					}
					
					if (mDispType->mAct->stStiInfo.stich_mode == STITCH_CUBE) {
						mDispType->mAct->stStiInfo.h = mResInfos[sti_res].h[mDispType->mAct->mode];
						mDispType->mAct->stStiInfo.w = mDispType->mAct->stStiInfo.h*3/2;
					} else {
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
		} else {
			mDispType->type = QR_FINISH_UNRECOGNIZE;
		}
	} else {
		mDispType->type = QR_FINISH_UNRECOGNIZE;
	}
    
}


/*
 * 处理来自HTTP的请求
 */
void fifo::handleReqFormHttp(sp<DISP_TYPE>& mDispType, cJSON *root, cJSON *subNode)
{
    cJSON *child = nullptr;

	Log.d(TAG, "rec req type %d", mDispType->type);     // rec req type 6
	GET_CJSON_OBJ_ITEM_INT(child, subNode, "action", mDispType->qr_type)

	/* 获取"param"子节点 */
	child = cJSON_GetObjectItem(subNode, "param");
	CHECK_NE(child, nullptr);

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
		}
										
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
				subNode = cJSON_GetObjectItem(org,"framerate");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgV.org_fr = get_fr_index(subNode->valueint);
				}

				subNode = cJSON_GetObjectItem(org, "bitrate");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgV.org_br = subNode->valueint / 1000;
				}

				if (mAI->stOrgInfo.save_org != SAVE_OFF) {
					//exactly should divide 8,but actually less,so divide 10
					mAI->size_per_act = (mAI->stOrgInfo.stOrgAct.mOrgV.org_br * 6) / 10;
				} else {
					// no br ,seems timelapse
					mAI->size_per_act = 30;
				}

				subNode = cJSON_GetObjectItem(org, "logMode");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgV.logMode = subNode->valueint;
				}
				break;

			case ACTION_LIVE:
				subNode = cJSON_GetObjectItem(org,"framerate");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgV.org_fr = get_fr_index(subNode->valueint);
				}

				subNode = cJSON_GetObjectItem(org, "bitrate");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgV.org_br = subNode->valueint / 1000;
				}
				subNode = cJSON_GetObjectItem(root, "logMode");
				if (subNode) {
					mAI->stOrgInfo.stOrgAct.mOrgL.logMode = subNode->valueint;
				}
				break;
			SWITCH_DEF_ERROR(mDispType->qr_type)
		}
	}

	cJSON *sti = cJSON_GetObjectItem(child, "stiching");
	if (sti) {
		char sti_mode[32];
		GET_CJSON_OBJ_ITEM_INT(subNode, sti, "width", mAI->stStiInfo.w)
		GET_CJSON_OBJ_ITEM_INT(subNode, sti, "height", mAI->stStiInfo.h)
		Log.d(TAG, "stStiInfo.sti_res is (%d %d)", mAI->stStiInfo.w, mAI->stStiInfo.h);
		GET_CJSON_OBJ_ITEM_STR(subNode, sti, "mode", sti_mode, sizeof(sti_mode));
		mAI->mode = get_mode_index(sti_mode);
		

		subNode = cJSON_GetObjectItem(sti, "mime");
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

				// 3d
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
				if (subNode) {
					mAI->stStiInfo.stStiAct.mStiV.sti_br = subNode->valueint / 1000;
				}
												
				mAI->stStiInfo.stich_mode = STITCH_NORMAL;

				// exclude timelapse 170831
				if (mAI->stOrgInfo.save_org != SAVE_OFF) {
					mAI->size_per_act += mAI->stStiInfo.stStiAct.mStiV.sti_br / 10;
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
				GET_CJSON_OBJ_ITEM_STR(subNode, sti, "_liveUrl", mAI->stStiInfo.stStiAct.mStiL.url,sizeof(mAI->stStiInfo.stStiAct.mStiL.url));
				GET_CJSON_OBJ_ITEM_STR(subNode, sti, "format", mAI->stStiInfo.stStiAct.mStiL.format,sizeof(mAI->stStiInfo.stStiAct.mStiL.format));
				GET_CJSON_OBJ_ITEM_INT(subNode, sti, "liveOnHdmi", mAI->stStiInfo.stStiAct.mStiL.hdmi_on)
				GET_CJSON_OBJ_ITEM_INT(subNode, sti, "fileSave", mAI->stStiInfo.stStiAct.mStiL.file_save)
				break;
			SWITCH_DEF_ERROR(mDispType->qr_type)
		}
	} else {
		mAI->stStiInfo.stich_mode = STITCH_OFF;
	}
									
	cJSON *aud = cJSON_GetObjectItem(child, "audio");
	if (aud) {
		GET_CJSON_OBJ_ITEM_STR(subNode, aud, "mime", mAI->stAudInfo.mime,sizeof(mAI->stAudInfo.mime))
		GET_CJSON_OBJ_ITEM_STR(subNode, aud, "sampleFormat", mAI->stAudInfo.sample_fmt,sizeof(mAI->stAudInfo.sample_fmt))
		GET_CJSON_OBJ_ITEM_STR(subNode, aud, "channelLayout", mAI->stAudInfo.ch_layout,sizeof(mAI->stAudInfo.ch_layout))
		GET_CJSON_OBJ_ITEM_INT(subNode, aud, "samplerate", mAI->stAudInfo.sample_rate)
		GET_CJSON_OBJ_ITEM_INT(subNode, aud, "bitrate", mAI->stAudInfo.br)
	}

	cJSON *del = cJSON_GetObjectItem(child, "delay");
	if (del) {
		mAI->delay = del->valueint;
	}
									
	cJSON *tl = cJSON_GetObjectItem(child, "timelapse");
	if (tl) {
		GET_CJSON_OBJ_ITEM_INT(subNode, tl, "interval", mAI->stOrgInfo.stOrgAct.mOrgV.tim_lap_int)
	}
									
	cJSON *hdr_j = cJSON_GetObjectItem(child, "hdr");
	if (hdr_j) {
		GET_CJSON_OBJ_ITEM_INT(subNode, hdr_j, "count", mAI->stOrgInfo.stOrgAct.mOrgP.hdr_count)
		GET_CJSON_OBJ_ITEM_INT(subNode, hdr_j, "min_ev", mAI->stOrgInfo.stOrgAct.mOrgP.min_ev)
		GET_CJSON_OBJ_ITEM_INT(subNode, hdr_j, "max_ev", mAI->stOrgInfo.stOrgAct.mOrgP.max_ev)
	}

	cJSON *bur = cJSON_GetObjectItem(child, "burst");
	if (bur) {
		GET_CJSON_OBJ_ITEM_INT(subNode, bur, "count",mAI->stOrgInfo.stOrgAct.mOrgP.burst_count)
	}

	cJSON *props = cJSON_GetObjectItem(child, "properties");
	//set as default
	mAI->stProp.audio_gain = 96;
	if (props) {
		GET_CJSON_OBJ_ITEM_INT(subNode, props, "audio_gain", mAI->stProp.audio_gain);
		Log.d(TAG, "aud_gain %d", mAI->stProp.audio_gain);
		
		// {"aaa_mode":2,"ev_bias":0,"wb":0,"long_shutter":1-60(s),"shutter_value":21,"iso_value","value":7,"brightness","value":87,"saturation","value":156,"sharpness","value":4,"contrast","value":143}
		subNode = cJSON_GetObjectItem(props, "len_param");
		if (subNode) {
			Log.d(TAG, "found len param %s", cJSON_Print(subNode));
			snprintf(mAI->stProp.len_param,sizeof(mAI->stProp.len_param),"%s",cJSON_Print(subNode));
		}

		subNode = cJSON_GetObjectItem(props, "gamma_param");
		if (subNode) {
			Log.d(TAG, "subNode->valuestring %s", subNode->valuestring);
			memcpy(mAI->stProp.mGammaData, subNode->valuestring, strlen(subNode->valuestring));
			Log.d(TAG, "mAI->stProp.mGammaData %s", mAI->stProp.mGammaData);
		}
	}
									
	Log.d(TAG, "tl type size (%d %d %d)",
                                    mAI->stOrgInfo.stOrgAct.mOrgV.tim_lap_int,
                                    mDispType->type, 
                                    mAI->size_per_act);

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
}


void fifo::handleSetting(sp<DISP_TYPE>& mDispType, cJSON *subNode)
{

    cJSON *child = nullptr;
    mDispType->mSysSetting = sp<SYS_SETTING>(new SYS_SETTING());

    memset(mDispType->mSysSetting.get(), -1, sizeof(SYS_SETTING));

    GET_CJSON_OBJ_ITEM_INT(child, subNode, "flicker", mDispType->mSysSetting->flicker);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "speaker", mDispType->mSysSetting->speaker);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "led_on", mDispType->mSysSetting->led_on);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "fan_on", mDispType->mSysSetting->fan_on);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "aud_on", mDispType->mSysSetting->aud_on);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "aud_spatial", mDispType->mSysSetting->aud_spatial);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "set_logo", mDispType->mSysSetting->set_logo);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "gyro_on", mDispType->mSysSetting->gyro_on);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "video_fragment", mDispType->mSysSetting->video_fragment);

    Log.d(TAG, "%d %d %d %d %d %d %d %d %d",
        mDispType->mSysSetting->flicker,
        mDispType->mSysSetting->speaker,
        mDispType->mSysSetting->led_on,
        mDispType->mSysSetting->fan_on,
        mDispType->mSysSetting->aud_on,
        mDispType->mSysSetting->aud_spatial,
        mDispType->mSysSetting->set_logo,
        mDispType->mSysSetting->gyro_on,
        mDispType->mSysSetting->video_fragment);

}


#if 0
void fifo::handleStitchProgress(sp<DISP_TYPE>& mDispType, cJSON *subNode)
{
    cJSON *child = nullptr;

    mDispType->mStichProgress = sp<STICH_PROGRESS>(new STICH_PROGRESS());
    //finish
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "task_over", mDispType->mStichProgress->task_over);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "total_cnt", mDispType->mStichProgress->total_cnt);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "successful_cnt", mDispType->mStichProgress->successful_cnt);
    GET_CJSON_OBJ_ITEM_INT(child, subNode, "failing_cnt", mDispType->mStichProgress->failing_cnt);
    GET_CJSON_OBJ_ITEM_DOUBLE(child, subNode, "runing_task_progress", mDispType->mStichProgress->runing_task_progress);
}
#endif


void fifo::handleSetting(sp<struct _disp_type_>& mDispType, Json::Value& reqNode)
{
    cJSON *child = nullptr;
    mDispType->mSysSetting = sp<SYS_SETTING>(new SYS_SETTING());

    memset(mDispType->mSysSetting.get(), -1, sizeof(SYS_SETTING));

    if (reqNode["flicker"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["flicker"].asInt();
    }

    if (reqNode["speaker"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["speaker"].asInt();
    }

    if (reqNode["led_on"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["led_on"].asInt();
    }

    if (reqNode["fan_on"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["fan_on"].asInt();
    }

    if (reqNode["aud_on"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["aud_on"].asInt();
    }

    if (reqNode["aud_spatial"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["aud_spatial"].asInt();
    }

    if (reqNode["set_logo"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["set_logo"].asInt();
    }

    if (reqNode["gyro_on"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["gyro_on"].asInt();
    }

    if (reqNode["video_fragment"].isInt()) {
        mDispType->mSysSetting->flicker = reqNode["video_fragment"].asInt();
    }

    Log.d(TAG, "%d %d %d %d %d %d %d %d %d",
                mDispType->mSysSetting->flicker,
                mDispType->mSysSetting->speaker,
                mDispType->mSysSetting->led_on,
                mDispType->mSysSetting->fan_on,
                mDispType->mSysSetting->aud_on,
                mDispType->mSysSetting->aud_spatial,
                mDispType->mSysSetting->set_logo,
                mDispType->mSysSetting->gyro_on,
                mDispType->mSysSetting->video_fragment);    
}


/*
 * 处理来自HTTP的请求
 */
void fifo::handleReqFormHttp(sp<DISP_TYPE>& mDispType, Json::Value& reqNode)
{
	
    if (reqNode["action"].isNull() == false) {
        /* 设置Customer时，使用该字段来区分是拍照,录像，直播 */
        mDispType->qr_type = reqNode["action"].asInt();
    }

    if (reqNode["param"].isNull() == false) {
        mDispType->jsonArg = reqNode["param"];
    }
						
	switch (mDispType->type) {
		case START_LIVE_SUC: {	/* 16, 启动录像成功 */
            Log.d(TAG, "[%s: %d] Client control Live", __FILE__, __LINE__);
        	mDispType->control_act = ACTION_LIVE;
			break;
        }
										
		case CAPTURE: {			/* 拍照 */
            Log.d(TAG, "[%s: %d] Client control Capture", __FILE__, __LINE__);
			mDispType->control_act = ACTION_PIC;
			break;
        }
										
		case START_REC_SUC:	{	/* 1, 启动录像成功 */
            Log.d(TAG, "[%s: %d] Client control Video", __FILE__, __LINE__);
			mDispType->control_act = ACTION_VIDEO;
			break;
        }
											
		case SET_CUS_PARAM:	{	/* 46, 设置自定义参数 */
            Log.d(TAG, "[%s: %d] Client control Set Customer", __FILE__, __LINE__);
			mDispType->control_act = CONTROL_SET_CUSTOM;
			break;
        }
										
		SWITCH_DEF_ERROR(mDispType->type);
	}	   
}



void fifo::parseAndDispatchRecMsg(int iMsgType, Json::Value& jsonData)
{
    Json::FastWriter writer;
    string data = writer.write(jsonData);

    Log.d(TAG, "[%s: %d] ---->>>> Recv Message type[%s], data[%s]", __FILE__, __LINE__,
                getRecvMsgName(iMsgType), data.c_str());

    switch (iMsgType) {
        case CMD_OLED_DISP_TYPE: {	/* 通信UI线程显示指定UI */
        
            sp<DISP_TYPE> mDispType = (sp<DISP_TYPE>)(new DISP_TYPE());
            if (jsonData["type"].isNull()) {
            } else {
                mDispType->type = jsonData["type"].asInt();
                Log.d(TAG, "[%s: %d] ----------->> Display Type: %s", __FILE__, __LINE__, getDispType(mDispType->type));                
            }

            mDispType->mSysSetting = nullptr;
            mDispType->mStichProgress = nullptr;
            mDispType->mAct = nullptr;
            mDispType->control_act = -1;
            mDispType->tl_count  = -1;
            mDispType->qr_type  = -1;

            if (jsonData["content"].isNull() == false) {
                Log.d(TAG, "[%s: %d] Qr Function Not implement now ..", __FILE__, __LINE__);
                // handleQrContent(mDispType, root, subNode);
            } else if (jsonData["req"].isNull() == false) {
                handleReqFormHttp(mDispType, jsonData["req"]);
            } else if (jsonData["sys_setting"].isNull() == false) {
                handleSetting(mDispType, jsonData["sys_setting"]);
            } else if (jsonData["tl_count"].isNull() == false) {
                mDispType->tl_count = jsonData["tl_count"].asInt();
            } else {
                // Log.e(TAG, "[%s: %d] ---------Unkown Error", __FILE__, __LINE__);
            }

            mOLEDHandle->send_disp_str(mDispType);
            break;
        }

        case CMD_OLED_SET_SN: {
            sp<SYS_INFO> mSysInfo = sp<SYS_INFO>(new SYS_INFO());
            
            if (jsonData["sn"].isString()) {
                snprintf(mSysInfo->sn, sizeof(mSysInfo->sn), "%s", jsonData["sn"].asCString());    
                Log.d(TAG, "[%s: %d] Recv SN: %s", __FILE__, __LINE__, mSysInfo->sn);
            }

            if (jsonData["uuid"].isString()) {
                snprintf(mSysInfo->uuid, sizeof(mSysInfo->uuid), "%s", jsonData["uuid"].asCString());    
                Log.d(TAG, "[%s: %d] Recv SN: %s", __FILE__, __LINE__, mSysInfo->uuid);
            }
            mOLEDHandle->send_sys_info(mSysInfo);
            break;
        }


        case CMD_OLED_SYNC_INIT: {	/* 给UI发送同步信息: state, a_v, h_v, c_v */
            sp<SYNC_INIT_INFO> mSyncInfo = sp<SYNC_INIT_INFO>(new SYNC_INIT_INFO());
            if (jsonData["state"].isNull() == false) {
                mSyncInfo->state = jsonData["state"].asInt();
            }
            
            if (jsonData["a_v"].isNull() == false) {
                snprintf(mSyncInfo->a_v, sizeof(mSyncInfo->a_v), "%s", jsonData["a_v"].asCString());
            }            
            
            if (jsonData["h_v"].isNull() == false) {
                snprintf(mSyncInfo->h_v, sizeof(mSyncInfo->h_v), "%s", jsonData["h_v"].asCString());
            }                

            if (jsonData["c_v"].isNull() == false) {
                snprintf(mSyncInfo->c_v, sizeof(mSyncInfo->c_v), "%s", jsonData["c_v"].asCString());
            }                

            mOLEDHandle->send_sync_init_info(mSyncInfo);
            break;
        }    
   

        case CMD_OLED_DISP_TYPE_ERR: {	/* 给UI发送显示错误信息:  错误类型和错误码 */
            sp<ERR_TYPE_INFO> mInfo = sp<ERR_TYPE_INFO>(new ERR_TYPE_INFO());

            if (jsonData["type"].isNull() == false) {
                mInfo->type = jsonData["type"].asInt();
            }    

            if (jsonData["err_code"].isNull() == false) {
                mInfo->err_code = jsonData["err_code"].asInt();
            }    
            mOLEDHandle->send_disp_err(mInfo);
            break;
        }

        case CMD_WEB_UI_TF_CHANGED: {   /* 暂时每次只能解析一张卡的变化 */  

            Log.d(TAG, "[%s:%d] Get Tfcard Changed....", __FILE__, __LINE__);      

            int iModuleArray = 0;
            std::vector<sp<Volume>> storageList;
            
            storageList.clear();

            /*  
             * {'module': {'storage_total': 61024, 'storage_left': 47748, 'pro_suc': 1, 'index': 1}}
             */
            if (jsonData["module"].isNull() == false) {
                sp<Volume> tmpVol = (sp<Volume>)(new Volume());

                if (jsonData["module"]["index"].isInt()) {
                    tmpVol->iIndex = jsonData["module"]["index"].asInt();
                }

                if (jsonData["module"]["storage_total"].isInt()) {
                    tmpVol->uTotal = jsonData["module"]["storage_total"].asInt();
                }

                if (jsonData["module"]["storage_left"].isInt()) {
                    tmpVol->uAvail = jsonData["module"]["storage_left"].asInt();
                }

                snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
                storageList.push_back(tmpVol);

                /* 直接将消息丢入UI线程的消息队列中 */
                mOLEDHandle->sendTfStateChanged(storageList);                
            } else {
                Log.d(TAG, "[%s:%d] get module json node[module] failed", __FILE__, __LINE__);                               
            }
            break;
        }


        /* example:
            {
                "name": "camera._queryStorage", 
                "sequence": 39, 
                "state": "done", 
                "results": {
                    "storagePath": "/mnt/sdcard", 
                    "module": [
                        {"storage_total": 0, "storage_left": 0, "pro_suc": 0, "index": 1}, 
                        {"storage_total": 61024, "storage_left": 46182, "pro_suc": 1, "index": 2}, 
                        {"storage_total": 0, "storage_left": 0, "pro_suc": 0, "index": 3}, 
                        {"storage_total": 61024, "storage_left": 46402, "pro_suc": 1, "index": 4}, 
                        {"storage_total": 61024, "storage_left": 46182, "pro_suc": 1, "index": 5}, 
                        {"storage_total": 61024, "storage_left": 46182, "pro_suc": 1, "index": 6}
                    ]
                }
            }
        */
        case CMD_WEB_UI_TF_NOTIFY: {    /* 查询TF卡的状态 */
            Log.d(TAG, "[%s:%d] get notify form server for TF info", __FILE__, __LINE__);

            bool bResult = false;
            char cStoragePath[64] = {0};
            char cState[32] = {0};
            int iModuleArray = 0;
            std::vector<sp<Volume>> storageList;
            
            storageList.clear();

            if ( (jsonData["state"].isNull() == false) && (jsonData["results"].isNull() == false)) {
                if (!strcmp(jsonData["state"].asCString(), "done")) {
                    if (jsonData["results"]["module"].isArray()) {
                        for (int i = 0; i < jsonData["results"]["module"].size(); i++) {
                            sp<Volume> tmpVol = (sp<Volume>)(new Volume());
                            if (jsonData["results"]["module"][i]["index"].isInt()) {
                                tmpVol->iIndex = jsonData["results"]["module"][i]["index"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["storage_total"].isInt()) {
                                tmpVol->uTotal = jsonData["results"]["module"][i]["storage_total"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["storage_left"].isInt()) {
                                tmpVol->uAvail = jsonData["results"]["module"][i]["storage_left"].asInt();
                            }

                            if (jsonData["results"]["module"][i]["pro_suc"].isInt()) {
                                tmpVol->iSpeedTest = jsonData["results"]["module"][i]["pro_suc"].asInt();
                            }

                            /* 类型为"SD"
                            * 外部TF卡的命名规则
                            * 名称: "tf-1","tf-2","tf-3"....
                            */
                            sprintf(tmpVol->cVolName, "mSD%d", tmpVol->iIndex);
                            Log.d(TAG, "[%s: %d] TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d]",
                                        __FILE__, __LINE__, tmpVol->cVolName, 
                                        tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest);

                            storageList.push_back(tmpVol);

                        }
                        bResult = true; 
                    } else {
                        Log.e(TAG, "[%s: %d] module not array, what's wrong", __FILE__, __LINE__);
                    }
                }
            } else {
                Log.e(TAG, "[%s: %d] state node not exist!", __FILE__, __LINE__);
            }

            mOLEDHandle->updateTfStorageInfo(bResult, storageList);
            break;
        }


        case CMD_WEB_UI_TF_FORMAT: {    /* 格式化结果 */
            Log.d(TAG, "[%s: %d] Get Notify(mSD Format Info)", __FILE__, __LINE__);

            sp<Volume> tmpVolume = (sp<Volume>)(new Volume());

            cJSON* pState = NULL;
            std::vector<sp<Volume>> storageList;

            if (jsonData["state"].isNull()) {
                Log.d(TAG, "[%s:%d] CMD_WEB_UI_TF_FORMAT Protocal Err, no 'state'", __FILE__, __LINE__);
                storageList.push_back(tmpVolume); 
            } else {
                
                if (!strcmp(jsonData["state"].asCString(), "done")) { /* 格式化成功 */
                    /* do nothind */
                } else {    /* 格式化失败: TODO - 传递格式化失败的设备号(需要camerad处理) */
                    storageList.push_back(tmpVolume); 
                }                
            }
            /* 直接将消息丢入UI线程的消息队列中 */
            mOLEDHandle->notifyTfcardFormatResult(storageList);
            break;
        }


        case CMD_WEB_UI_TEST_SPEED_RES: {

            Log.d(TAG, "[%s: %d] Return Speed Test Result", __FILE__, __LINE__);

            std::vector<sp<Volume>> storageList;
            sp<Volume> tmpVol = NULL;


            storageList.clear();

            if (jsonData["local"].isNull() == false) {
                tmpVol = (sp<Volume>)(new Volume());
                tmpVol->iType = VOLUME_TYPE_NV;
                tmpVol->iSpeedTest = jsonData["local"].asInt();
                Log.d(TAG, "[%s: %d] Local Device Test Speed Result: %d", __FILE__, __LINE__, tmpVol->iSpeedTest);
                storageList.push_back(tmpVol);
            }

            if (jsonData["module"].isNull() == false) {
                if (jsonData["module"].isArray()) {
                    for (int i = 0; i < jsonData["module"].size(); i++) {
                        tmpVol = (sp<Volume>)(new Volume());

                        tmpVol->iType       = VOLUME_TYPE_MODULE;
                        tmpVol->iIndex      = jsonData["module"][i]["index"].asInt();
                        tmpVol->iSpeedTest  = jsonData["module"][i]["result"].asInt();

                        /* 类型为"SD"
                        * 外部TF卡的命名规则
                        * 名称: "tf-1","tf-2","tf-3"....
                        */
                        snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpVol->iIndex);
                        Log.d(TAG, "[%s: %d] mSD card node[%s] info index[%d], speed[%d]",
                                    __FILE__, __LINE__, tmpVol->cVolName,  tmpVol->iIndex, tmpVol->iSpeedTest);

                        storageList.push_back(tmpVol);
                    }

                } else {
                    Log.e(TAG, "[%s: %d] node module not array!!", __FILE__, __LINE__);
                }
                
            }

            mOLEDHandle->sendSpeedTestResult(storageList);
            break;
        }

        default: 
            break;

    }
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
    char buf[1024] = {0};
    char result[1024] = {0}; 
    int error_times = 0;

    while (true) {

        memset(buf, 0, sizeof(buf));
        memset(result, 0, sizeof(result));

        get_read_fd();	/* 获取FIFO读端的fd */

		/* 首先读取8字节的头部 */
        int len = read(read_fd, buf, FIFO_HEAD_LEN);
        if (len != FIFO_HEAD_LEN) {	/* 头部读取错误 */
            Log.w(TAG, "ReadFifoThread: read fifo head mismatch(rec[%d] act[%d])", len, FIFO_HEAD_LEN);
            if (++error_times >= 3) {
                Log.e(TAG, ">> read fifo broken?");
                close_read_fd();
            }
        } else {
            int msg_what = bytes_to_int(buf);	/* 前4字节代表消息类型: what */
            if (msg_what == CMD_EXIT) {	/* 如果是退出消息 */
				// Log.d(TAG," rec cmd exit");
                break;
            } else {
				
				/* 头部的后4字节代表本次数据传输的长度 */
                int content_len = bytes_to_int(&buf[FIFO_DATA_LEN_OFF]);
                CHECK_NE(content_len, 0);

				/* 读取传输的数据 */
                len = read(read_fd, &buf[FIFO_HEAD_LEN], content_len);

				if (len != content_len) {	/* 读取的数据长度不一致 */
                    Log.w(TAG, "3read fifo content mismatch(%d %d)", len, content_len);
                    if (++error_times >= 3) {
                        Log.e(TAG, " 2read fifo broken? ");
                        close_read_fd();
                    }
                } else {
                
                    cJSON *root = cJSON_Parse(&buf[FIFO_HEAD_LEN]);

                    cJSON *subNode = 0;
                    if (!root) {	/* 解析出错 */
                        Log.e(TAG, "cJSON parse string error, func(%s), line(%d)", __FILE__, __LINE__);
                    }
					
                    Json::Value rootJson;
                    Json::Reader reader;
                    Json::FastWriter writer;
	                if (!reader.parse(&buf[FIFO_HEAD_LEN], rootJson, false)) {
		                Log.e(TAG, "[%s: %d] bad json format!", __FILE__, __LINE__);
		                continue;
	                }

                    string jsonstr = writer.write(rootJson);
                    Log.d(TAG, "=============>> [Recv Message]: %s", jsonstr.c_str());

#if 1
                    parseAndDispatchRecMsg(msg_what, rootJson);
#else

					/* 根据消息的类型做出处理 */
                    switch (msg_what) {

                        case CMD_OLED_DISP_TYPE: {	/* 通信UI线程显示指定UI */
							
                            #if 1
							/* 解析子节点的数据来构造DISP_TYPE 
                             * {"type":int, }
                             */
                            sp<DISP_TYPE> mDispType = sp<DISP_TYPE>(new DISP_TYPE());
                            subNode = cJSON_GetObjectItem(root, "type");
                            CHECK_NE(subNode, nullptr);
							
                            mDispType->type = subNode->valueint;
                            mDispType->mSysSetting = nullptr;
                            mDispType->mStichProgress = nullptr;
                            mDispType->mAct = nullptr;
                            mDispType->control_act = -1;
                            mDispType->tl_count  = -1;
                            mDispType->qr_type  = -1;

                            subNode = cJSON_GetObjectItem(root, "content");
                            if (subNode) {  /* {"type":int, "content":{}} */
                                // handleQrContent(mDispType, root, subNode);
                            } else {
                                subNode = cJSON_GetObjectItem(root, "req");
                                if (subNode) {
                                    handleReqFormHttp(mDispType, root, subNode);
                                } else {
                                    //{"flicker":0,"speaker":0,"led_on":0,"fan_on":0,"aud_on":0,"aud_spatial":0,"set_logo":0,}};
                                    subNode = cJSON_GetObjectItem(root, "sys_setting");
                                    if (subNode) {
                                        handleSetting(mDispType, subNode);
                                    } else {

                                        subNode = cJSON_GetObjectItem(root, "stitch_progress");
                                        if (subNode) {
                                            handleStitchProgress(mDispType, subNode);
                                        } else {
                                            subNode = cJSON_GetObjectItem(root, "tl_count");
                                            if (subNode) {
                                                mDispType->tl_count = subNode->valueint;
                                            }
                                        }
                                    }
                                }
                            }
                            // Log.d(TAG, "mDispType type (%d %d %d)",  mDispType->type, mDispType->qr_type, mDispType->control_act);
                            send_disp_str_type(mDispType);
                            #else 
                            parseAndDispatchRecMsg(CMD_OLED_DISP_TYPE, rootJson);
                            #endif
                            break;
                        }
						


                        /* 设置SN/UUID
                         * {"sn":string, "uuid":string}
                         */
                        case CMD_OLED_SET_SN: {
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

						
                        /* 同步信息:
                         * {"state":int, "a_v":string, "h_v":string, "c_v": string}
                         */
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


                        /* 显示错误类型
                         * {"type": int, "err_code": int}
                         */
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


                        /* 配置连接的WIFI
                         * {"ssid": string, "pwd": string}
                         */
                        case CMD_CONFIG_WIFI: {		/* 发送WiFi的配置信息: SSID和密码 */
							#if 0
                            char ssid[128];
                            char pwd[64];
						
                            GET_CJSON_OBJ_ITEM_STR(subNode, root, "ssid", ssid, sizeof(ssid));
                            GET_CJSON_OBJ_ITEM_STR(subNode, root, "pwd", pwd, sizeof(pwd));
                            Log.d(TAG, "ssid %s pwd %s %d", ssid, pwd, strlen(pwd));
                            send_wifi_config(ssid, pwd);
							#endif
							break;
                        }

                        /* 暂时每次只能解析一张卡的变化 */
                        case CMD_WEB_UI_TF_CHANGED: {

                            Log.d(TAG, "[%s:%d] Get Tfcard Changed....", __FILE__, __LINE__);      

                            int iModuleArray = 0;
                            std::vector<sp<Volume>> storageList;
                            
                            cJSON* tmpJsonNode = NULL;
                            cJSON* tmpJsonIndex = NULL;
                            cJSON* tmpJsonTotalSpace = NULL;
                            cJSON* tmpJsonLeftSpace = NULL;

                            storageList.clear();

                            subNode = cJSON_GetObjectItem(root, "module");
                            if (subNode) { 
                                if (subNode) {
                                    sp<Volume> tmpVol = (sp<Volume>)(new Volume());

                                    tmpJsonIndex = cJSON_GetObjectItem(subNode, "index");
                                    tmpJsonTotalSpace = cJSON_GetObjectItem(subNode, "storage_total");
                                    tmpJsonLeftSpace = cJSON_GetObjectItem(subNode, "storage_left");

                                    if (tmpJsonIndex && tmpJsonTotalSpace && tmpJsonLeftSpace) {
                                        Log.d(TAG, "[%s: %d] TF card node info index[%d], total space[%d]M, left space[%d]",
                                                    __FILE__, __LINE__, tmpJsonIndex->valueint, tmpJsonTotalSpace->valueint, tmpJsonLeftSpace->valueint);
                                        tmpVol->iIndex = tmpJsonIndex->valueint;
                                        tmpVol->uTotal  = tmpJsonTotalSpace->valueint;
                                        tmpVol->uAvail  = tmpJsonLeftSpace->valueint;

                                        /* 类型为"SD"
                                            * 外部TF卡的命名规则
                                            * 名称: "tf-1","tf-2","tf-3"....
                                            */
                                        snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpJsonIndex->valueint);
                                        storageList.push_back(tmpVol);

                                        /* 直接将消息丢入UI线程的消息队列中 */
                                        mOLEDHandle->sendTfStateChanged(storageList);
                                    } else {
                                        Log.e(TAG, "[%s: %d] Parse 'module' node failed....", __FILE__, __LINE__);
                                    }
                                }
                            } else {
                                Log.d(TAG, "[%s:%d] get module json node[module] failed", __FILE__, __LINE__);                               
                            }

                            break;
                        }


                        /* 发送通知TF的状态 
                         *  {
                         *      "name": "camera._queryStorage", 
                         *      "sequence": 512, 
                         *      "state": "done", 
                         *      "results": {
                         *          "storagePath": "/mnt/udisk1", 
                         *          "module": [
                         *                      {"storage_total": 61024, "storage_left": 52962, "index": 1, "pro_suc": 0}, 
                         *                      {"storage_total": 61024, "storage_left": 47073, "index": 2, "pro_suc": 0}, 
                         *                      {"storage_total": 61037, "storage_left": 47086, "index": 3, "pro_suc": 0}, 
                         *                      {"storage_total": 61024, "storage_left": 47073, "index": 4, "pro_suc": 0}, 
                         *                      {"storage_total": 61024, "storage_left": 47073, "index": 5, "pro_suc": 0}, 
                         *                      {"storage_total": 61024, "storage_left": 47073, "index": 6, "pro_suc": 0}
                         *                     ]
                         *      }
                         * }
                         *  
                         */
                        case CMD_WEB_UI_TF_NOTIFY: {
                            Log.d(TAG, "[%s:%d] get notify form server for TF info", __FILE__, __LINE__);

                            bool bResult = false;
                            char cStoragePath[64] = {0};
                            char cState[32] = {0};
                            int iModuleArray = 0;
                            std::vector<sp<Volume>> storageList;
                            
                            cJSON* pTmpResultNode = NULL;
                            cJSON* tmpJsonNode = NULL;
                            cJSON* tmpJsonIndex = NULL;
                            cJSON* tmpJsonTotalSpace = NULL;
                            cJSON* tmpJsonLeftSpace = NULL;
                            cJSON* tmpJsonSpeedTest = NULL;

                            storageList.clear();

                            GET_CJSON_OBJ_ITEM_STR(subNode, root, "state", cState, sizeof(cState));
                            if (!strcmp(cState, "done")) {  /* 查询成功 */

                                pTmpResultNode = cJSON_GetObjectItem(root, "results");
                                if (pTmpResultNode) {
                                    subNode = cJSON_GetObjectItem(pTmpResultNode, "module");
                                    if (subNode) {
                                        iModuleArray = cJSON_GetArraySize(subNode);
                                        for (u32 i = 0; i < iModuleArray; i++) {
                                            tmpJsonNode = cJSON_GetArrayItem(subNode, i);
                                            if (tmpJsonNode) {
                                                sp<Volume> tmpVol = (sp<Volume>)(new Volume());

                                                tmpJsonIndex        = cJSON_GetObjectItem(tmpJsonNode, "index");
                                                tmpJsonTotalSpace   = cJSON_GetObjectItem(tmpJsonNode, "storage_total");
                                                tmpJsonLeftSpace    = cJSON_GetObjectItem(tmpJsonNode, "storage_left");
                                                tmpJsonSpeedTest    = cJSON_GetObjectItem(tmpJsonNode, "pro_suc");

                                                tmpVol->iIndex      = tmpJsonIndex->valueint;
                                                tmpVol->uTotal       = tmpJsonTotalSpace->valueint;
                                                tmpVol->uAvail       = tmpJsonLeftSpace->valueint;
                                                tmpVol->iSpeedTest  = tmpJsonSpeedTest->valueint;

                                                /* 类型为"SD"
                                                * 外部TF卡的命名规则
                                                * 名称: "tf-1","tf-2","tf-3"....
                                                */
                                                sprintf(tmpVol->cVolName, "mSD%d", tmpJsonIndex->valueint);
                                                Log.d(TAG, "[%s: %d] TF card node[%s] info index[%d], total space[%d]M, left space[%d], speed[%d]",
                                                            __FILE__, __LINE__, tmpVol->cVolName, 
                                                            tmpVol->iIndex, tmpVol->uTotal, tmpVol->uAvail, tmpVol->iSpeedTest);

                                                storageList.push_back(tmpVol);
                                            }
                                        }    
                                        bResult = true;                                    
                                    } else {
                                        Log.d(TAG, "[%s:%d] get module json node[module] failed", __FILE__, __LINE__);
                                    }
                                }

                            } else {    /* 查询失败 */
                                Log.d(TAG, "[%s: %d] Query mSD Card info Failed....", __FILE__, __LINE__);
                            }

                            mOLEDHandle->updateTfStorageInfo(bResult, storageList);
                            break;
                        }



                        /*
                         * 格式化一张卡: 
                         * 成功: {"name": "camera._formatCameraMoudle", "sequence": 8, "state": "done"}
                         * 失败: {"name": "camera._formatCameraMoudle", "sequence": 9, "state": "error", "error": {"code": -1, "description": "unknow error"}}
                         * 格式化所有卡:
                         * 成功: {"name": "camera._formatCameraMoudle", "sequence": 10, "state": "done"}
                         * 失败: 
                         */
                        case CMD_WEB_UI_TF_FORMAT: {
                            Log.d(TAG, "[%s: %d] Get Notify(mSD Format Info)", __FILE__, __LINE__);

                            sp<Volume> tmpVolume = (sp<Volume>)(new Volume());

                            cJSON* pState = NULL;
                            std::vector<sp<Volume>> storageList;
                            
                            pState = cJSON_GetObjectItem(root, "state");
                            if (pState) {
                                Log.d(TAG, "[%s:%d] 'state' = %s", __FILE__, __LINE__, pState->valuestring);
                                if (!strcmp(pState->valuestring, "done")) { /* 格式化成功 */
                                    /* do nothind */
                                } else {    /* 格式化失败: TODO - 传递格式化失败的设备号(需要camerad处理) */
                                   storageList.push_back(tmpVolume); 
                                }
                            } else {    /* 通信失败也当成格式化失败处理 */
                                Log.d(TAG, "[%s:%d] CMD_WEB_UI_TF_FORMAT Protocal Err, no 'state'", __FILE__, __LINE__);
                                storageList.push_back(tmpVolume); 
                            }

                            /* 直接将消息丢入UI线程的消息队列中 */
                            mOLEDHandle->notifyTfcardFormatResult(storageList);

                            break;
                        }


                        /*
                         * {
                         *      "local": true, 
                         *      "module": [
                         *          {"index": 1, "result": true}, 
                         *          {"index": 2, "result": true}, 
                         *          {"index": 3, "result": true}, 
                         *          {"index": 4, "result": true}, 
                         *          {"index": 5, "result": true}, 
                         *          {"index": 6, "result": false}
                         *      ]
                         * }
                         */
                        case CMD_WEB_UI_TEST_SPEED_RES: {

                            Log.d(TAG, "[%s: %d] Return Speed Test Result", __FILE__, __LINE__);

                            int iModuleArray = 0;
                            std::vector<sp<Volume>> storageList;
                            sp<Volume> tmpVol = NULL;

                            cJSON* pTmpLocalResultNode = NULL;

                            cJSON* tmpJsonNode = NULL;
                            cJSON* tmpJsonIndex = NULL;
                            cJSON* tmpJsonSpeedTest = NULL;

                            storageList.clear();

                            pTmpLocalResultNode = cJSON_GetObjectItem(root, "local");
                            if (pTmpLocalResultNode) {
                                tmpVol = (sp<Volume>)(new Volume());
                                tmpVol->iType = VOLUME_TYPE_NV;
                                tmpVol->iSpeedTest = pTmpLocalResultNode->valueint;
                                Log.d(TAG, "[%s: %d] Local Device Test Speed Result: %d", __FILE__, __LINE__, tmpVol->iSpeedTest);
                                storageList.push_back(tmpVol);
                            }

                            subNode = cJSON_GetObjectItem(root, "module");
                            if (subNode) {
                                iModuleArray = cJSON_GetArraySize(subNode);
                                for (u32 i = 0; i < iModuleArray; i++) {
                                    tmpJsonNode = cJSON_GetArrayItem(subNode, i);
                                    if (tmpJsonNode) {
                                        tmpVol = (sp<Volume>)(new Volume());

                                        tmpJsonIndex        = cJSON_GetObjectItem(tmpJsonNode, "index");
                                        tmpJsonSpeedTest    = cJSON_GetObjectItem(tmpJsonNode, "result");

                                        tmpVol->iType       = VOLUME_TYPE_MODULE;
                                        tmpVol->iIndex      = tmpJsonIndex->valueint;
                                        tmpVol->iSpeedTest  = tmpJsonSpeedTest->valueint;

                                        /* 类型为"SD"
                                        * 外部TF卡的命名规则
                                        * 名称: "tf-1","tf-2","tf-3"....
                                        */
                                        snprintf(tmpVol->cVolName, sizeof(tmpVol->cVolName), "mSD%d", tmpJsonIndex->valueint);
                                        Log.d(TAG, "[%s: %d] mSD card node[%s] info index[%d], speed[%d]",
                                                    __FILE__, __LINE__, tmpVol->cVolName,  tmpVol->iIndex, tmpVol->iSpeedTest);

                                        storageList.push_back(tmpVol);
                                    }
                                }    
                            } else {
                                Log.d(TAG, "[%s:%d] get module json node[module] failed", __FILE__, __LINE__);
                            }
                            mOLEDHandle->sendSpeedTestResult(storageList);
                            break;
                        }

						/*
						 * 等待查询信息,
						 */
                        default:
                            break;
                    }

                    if (0 != root) {
                        cJSON_Delete(root);
                    }
#endif                    
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
    if (read_fd != -1) {
        close(read_fd);
        read_fd = -1;
    }
}

void fifo::close_write_fd()
{
    if (write_fd != -1) {
        close(write_fd);
        write_fd = -1;
    }
}

int fifo::get_read_fd()
{
    if (read_fd == -1) {
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
    if (write_fd == -1) {

        bWFifoStop = true;
        write_fd = open(FIFO_TO_CLIENT, O_WRONLY);
        CHECK_NE(write_fd, -1);
        bWFifoStop = false;
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
