/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: MenuUI.cpp
** 功能描述: UI核心
**
**
**
** 作     者: Skymixos
** 版     本: V2.0
** 日     期: 2016年12月1日
** 修改记录:
** V1.0			Wans			2016-12-01		创建文件
** V2.0			Skymixos		2018-06-05		添加注释
** V3.0         skymixos        2018年8月5日     修改存储逻辑及各种模式下的处理方式
******************************************************************************************************/
#include <future>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <linux/mtio.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <common/include_common.h>

#include <sys/net_manager.h>


#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <util/bytes_int_convert.h>
#include <sys/pro_cfg.h>
#include <sys/pro_uevent.h>
#include <sys/action_info.h>
#include <util/GitVersion.h>
#include <log/stlog.h>
#include <system_properties.h>
#include <prop_cfg.h>


#include <hw/battery_interface.h>
#include <hw/oled_light.h>
#include <hw/lan.h>
#include <hw/dev_manager.h>

#include <hw/MenuUI.h>

#include <hw/InputManager.h>

#include <util/icon_ascii.h>

#include <trans/fifo.h>

#include <sys/Menu.h>


#include <icon/setting_menu_icon.h>

#include <icon/pic_video_select.h>

using namespace std;

#undef TAG
#define TAG "MenuUI"


//#define USE_OLD_NET
#define ENABLE_LIGHT

#define ENABLE_SOUND

#define CAL_DELAY       (5)
#define SN_LEN          (14)

//sometimes met bat jump to 3 ,but in fact not
#define BAT_LOW_VAL     (3)

//#define BAT_LOW_PROTECT (5)
#define MAX_ADB_TIMES   (5)
#define LONG_PRESS_MSEC (2000)

#define OPEN_BAT_LOW

#define ONLY_EXFAT

//#define NEW_FORMAT


#define ERR_MENU_STATE(menu,state) \
Log.e(TAG,"line:%d err menu state (%d 0x%x)",__LINE__,menu,state);

#define INFO_MENU_STATE(menu,state) \
Log.d(TAG,"line:%d menu state (%d 0x%x)",__LINE__, menu,state);

//#define ONLY_PIC_FLOW
//#define ENABLE_ADB_OFF
//#define LIVE_ORG


enum {
    PRINT_DEVICE_ERRORS     = 1U << 0,
    PRINT_DEVICE            = 1U << 1,
    PRINT_DEVICE_NAME       = 1U << 2,
    PRINT_DEVICE_INFO       = 1U << 3,
    PRINT_VERSION           = 1U << 4,
    PRINT_POSSIBLE_EVENTS   = 1U << 5,
    PRINT_INPUT_PROPS       = 1U << 6,
    PRINT_HID_DESCRIPTOR    = 1U << 7,
    PRINT_ALL_INFO          = (1U << 8) - 1,
    PRINT_LABELS            = 1U << 16,
};

enum {
    DISP_DISK_FULL,
    DISP_NO_DISK,
    DISP_SDCARD_ATTACH,
    DISP_SDCARD_DETTACH,
    DISP_USB_ATTACH,
    DIPS_USB_DETTACH,
    DISP_WIFI_ON,
    DISP_ADB_OPEN,
    DISP_ADB_CLOSE,
    DISP_ADB_ROOT,
    DISP_ADB_UNROOT,
    DISP_ALERT_FAN_OFF,
    DISP_LIVE_REC_USB,
    DISP_VID_SEGMENT,
    //mtp
    DISP_USB_CONNECTED,
};


/*
 * UI线程使用的消息
 */
enum {
    OLED_DISP_STR_TYPE,//0
    UI_MSG_DISP_ERR_MSG,
    UI_MSG_KEY_EVENT,
    UI_MSG_LONG_KEY_EVENT,
    UI_MSG_UPDATE_IP,
    UI_DISP_BATTERY,//5
    
    //set config wifi same thread as oled wifi key action
    UI_MSG_CONFIG_WIFI,		        /* 配置WIFI参数 */

    UI_MSG_SET_SN,
    UI_MSG_SET_SYNC_INFO,
//    OLED_CHECK_LIVE_OR_REC,
    OLED_UPDATE_DEV_INFO,
    UI_UPDATE_MID, //10
    OLED_DISP_BAT_LOW,
    OLED_UPDATE_CAPTURE_LIGHT,
    OLED_UPDATE_CAL_LIGHT,
    UI_CLEAR_MSG__BOX,
    UI_READ_BAT, //15
    UI_DISP_LIGHT,
    //disp oled info at start
    UI_DISP_INIT,
    UI_UPDATE_TF,               /* TF卡状态消息 */
    UI_EXIT,                    /* 退出消息循环 */
};

enum {
    ERR_PIC,
    ERR_START_PREVIEW,
    ERR_STOP_PREVIEW,
    ERR_START_VIDEO,
    ERR_STOP_VIDEO,
    ERR_START_LIVE,
    ERR_STOP_LIVE,
};

/*
 *  org res start
 */
enum {
    UP = 0,
    DOWN = 1,
};

enum {
    SYS_KEY_SN,
    SYS_KEY_UUID,
    SYS_KEY_MAX,
};

typedef struct _ver_info_ {
    char a12_ver[128];
    char c_ver[128];
    char r_ver[128];
    char p_ver[128];
    char h_ver[128];			/*  */
    char k_ver[128];			/* 内核的版本 */
    char r_v_str[128];
} VER_INFO;


typedef struct _sys_read_ {
    const char *key;
    const char *header;
    const char *file_name;
}SYS_READ;

static const SYS_READ astSysRead[] = {
    { "sn", "sn=", "/home/nvidia/insta360/etc/sn"},
    { "uuid", "uuid=", "/home/nvidia/insta360/etc/uuid"},
};


/* Slave Addr: 0x77 Reg Addr: 0x02
 * bit[7] - USB_POWER_EN2
 * bit[6] - USB_POWER_EN1
 * bit[5] - LED_BACK_B
 * bit[4] - LED_BACK_G
 * bit[3] - LED_BACK_R
 * bit[2] - LED_FRONT_B
 * bit[1] - LED_FRONT_G
 * bit[0] - LED_FRONT_R
 */
enum {
    LIGHT_OFF 		= 0xc0,		/* 关闭所有的灯 bit[7:6] = Camera module */
    FRONT_RED 		= 0xc1,		/* 前灯亮红色,后灯全灭 */
    FRONT_GREEN 	= 0xc2,		/* 前灯亮绿色,后灯全灭 */
    FRONT_YELLOW 	= 0xc3,		/* 前灯亮黄色(G+R), 后灯全灭 */
    FRONT_DARK_BLUE = 0xc4,		/* 前灯亮蓝色, 后灯全灭 */
    FRONT_PURPLE 	= 0xc5,
    FRONT_BLUE 		= 0xc6,
    FRONT_WHITE 	= 0xc7,		/* 前灯亮白色(R+G+B),后灯全灭 */

    BACK_RED 		= 0xc8,		/* 后灯亮红色 */
    BACK_GREEN 		= 0xd0,		/* 后灯亮绿色 */
    BACK_YELLOW 	= 0xd8,		/* 后灯亮黄色 */
    BACK_DARK_BLUE 	= 0xe0,
    BACK_PURPLE 	= 0xe8,
    BACK_BLUE 		= 0xf0,
    BACK_WHITE		= 0xf8,		/* 后灯亮白色 */

    LIGHT_ALL 		= 0xff		/* 所有的灯亮白色 */
};


#define INTERVAL_1HZ 	    (1000)

//#define INTERVAL_3HZ      (333)
#define INTERVAL_5HZ 		(200)

#define FLASH_LIGHT			BACK_BLUE
#define BAT_INTERVAL		(5000)
#define AVAIL_SUBSTRACT		(1024)


/*
 * 声音索引
 */
enum {
    SND_SHUTTER,
    SND_COMPLE,
    SND_FIVE_T,
    SND_QR,
    SND_START,
    SND_STOP,
    SND_THREE_T,
    SND_ONE_T,
    SND_MAX_NUM,
};

/*
 * 声音文件
 */
static const char *sound_str[] = {
    "/home/nvidia/insta360/wav/camera_shutter.wav",
    "/home/nvidia/insta360/wav/completed.wav",
    "/home/nvidia/insta360/wav/five_s_timer.wav",
    "/home/nvidia/insta360/wav/qr_code.wav",
    "/home/nvidia/insta360/wav/start_rec.wav",
    "/home/nvidia/insta360/wav/stop_rec.wav",
    "/home/nvidia/insta360/wav/three_s_timer.wav",
    "/home/nvidia/insta360/wav/one_s_timer.wav"
};


typedef struct _area_info_ {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
} AREA_INFO;

static const AREA_INFO storage_area[] = {
	{25, 16, 103, 16},
	{25, 32, 103, 16}
};

typedef struct _rec_info_ {
    int rec_hour;
    int rec_min;
    int rec_sec;
} REC_INFO;



static int main_icons[][MAINMENU_MAX] = {
	{
		ICON_INDEX_CAMERA_WC_LIGHT128_48,
		ICON_INDEX_VIDEO_WC_LIGHT128_48,
		ICON_INDEX_LIVE_WC_LIGHT128_48,
		ICON_INDEX_WIFI_WC_LIGHT128_48,
		ICON_INDEX_STORAGE_WC_LIGHT128_48,
		ICON_INDEX_SET_WC_LIGHT128_48,
	},
	{
		ICON_INDEX_CAMERA_WP_LIGHT128_48,
		ICON_INDEX_VIDEO_WP_LIGHT128_48,
		ICON_INDEX_LIVE_WP_LIGHT128_48,
		ICON_INDEX_WIFI_WP_LIGHT128_48,
		ICON_INDEX_STORAGE_WP_LIGHT128_48,
		ICON_INDEX_SET_WP_LIGHT128_48,
	}
};



static const char *dev_type[SET_STORAGE_MAX] = {"sd", "usb"};

static int main_menu[][MAINMENU_MAX] = {
	{
		ICON_INDEX_CAMERA_NORMAL24_24,
		ICON_INDEX_VIDEO_NORMAL24_24,
		ICON_INDEX_LIVE_NORMAL24_24,
		ICON_INDEX_IC_WIFICLOSE_NORMAL24_24,
		ICON_CALIBRATION_NORMAL_24_2424_24,
		ICON_INDEX_SET_NORMAL24_24,
	},
	{
		ICON_INDEX_CAMERA_LIGHT_12_16_24_24,
		ICON_INDEX_VIDEO_LIGHT_52_16_24_24,
		ICON_INDEX_LIVE_LIGHT_92_16_24_24,
		ICON_INDEX_IC_WIFICLOSE_LIGHT24_24,
		ICON_CALIBRATION_HIGH_24_2424_24,
		ICON_INDEX_SET_LIGHT_52_40_24_24,
	}
};


typedef struct _sys_error_ {
    int type;
    const char * code;
} SYS_ERROR;

typedef struct _err_code_detail_ {
    int code;
    const char *str;
    int icon;
} ERR_CODE_DETAIL;



const char* getCurMenuStr(int iCurMenu)
{
	const char* pRet = NULL;
	
	switch (iCurMenu) {
	case MENU_TOP:
		pRet = "MENU_TOP";
		break;
	
	case MENU_PIC_INFO:
		pRet = "MENU_PIC_INFO";
		break;
		
	case MENU_VIDEO_INFO:
		pRet = "MENU_VIDEO_INFO";
		break;
	
	case MENU_LIVE_INFO:
		pRet = "MENU_LIVE_INFO";
		break;
	
	case MENU_STORAGE:
		pRet = "MENU_LIVE_INFO";
		break;
		
	case MENU_SYS_SETTING:
		pRet = "MENU_SYS_SETTING";
		break;
	
	case MENU_PIC_SET_DEF:
		pRet = "MENU_PIC_SET_DEF";
		break;
		
	case MENU_VIDEO_SET_DEF:
		pRet = "MENU_VIDEO_SET_DEF";
		break;
	
	case MENU_LIVE_SET_DEF:
		pRet = "MENU_LIVE_SET_DEF";
		break;

	case MENU_CALIBRATION:
		pRet = "MENU_CALIBRATION";
		break;
		
	case MENU_QR_SCAN:
		pRet = "MENU_QR_SCAN";
		break;
	
	case MENU_SYS_DEV_INFO:
		pRet = "MENU_SYS_DEV_INFO";
		break;

	case MENU_SYS_ERR:
		pRet = "MENU_SYS_ERR";
		break;
	
	case MENU_LOW_BAT:
		pRet = "MENU_SYS_ERR";
		break;
	
	case MENU_GYRO_START:
		pRet = "MENU_GYRO_START";
		break;

	case MENU_SPEED_TEST:
		pRet = "MENU_SPEED_TEST";
		break;
		
	case MENU_RESET_INDICATION:
		pRet = "MENU_RESET_INDICATION";
		break;

#ifdef MENU_WIFI_CONNECT
	case MENU_WIFI_CONNECT:
		pRet = "MENU_WIFI_CONNECT";
		break;
#endif

	case MENU_AGEING:
		pRet = "MENU_AGEING";
		break;

	case MENU_NOSIE_SAMPLE:
		pRet = "MENU_NOSIE_SAMPLE";
		break;
	
	case MENU_LIVE_REC_TIME:
		pRet = "MENU_LIVE_REC_TIME";
		break;

#ifdef ENABLE_MENU_STITCH_BOX
	case MENU_STITCH_BOX:
		pRet = "MENU_STITCH_BOX";
		break;
#endif		

	case MENU_FORMAT:
		pRet = "MENU_FORMAT";
		break;

	case MENU_FORMAT_INDICATION:
		pRet = "MENU_FORMAT_INDICATION";
		break;
	
	case MENU_SET_PHOTO_DEALY:
		pRet = "MENU_SET_PHOTO_DEALY";
		break;
		
	
	//messagebox keep at the end
	case MENU_DISP_MSG_BOX:
		pRet = "MENU_DISP_MSG_BOX";
		break;

	default:
		pRet = "Unkown Menu";
		break;
	}

	return pRet;
}


//str not used 0613
static ERR_CODE_DETAIL mErrDetails[] = {
    {432, "No Space", ICON_STORAGE_INSUFFICIENT128_64},
    {433, "No Storage", ICON_CARD_EMPTY128_64},
    {434, "Speed Low", ICON_SPEEDTEST06128_64},
    {414, " ", ICON_ERROR_414128_64},

    // add for live rec finish
    {390, " ", ICON_LIV_REC_INSUFFICIENT_128_64128_64},
    {391, " ", ICON_LIVE_REC_LOW_SPEED_128_64128_64},
};

static SYS_ERROR mSysErr[] = {
    {START_PREVIEW_FAIL,    "1401"},    /* 启动预览失败 */
    {CAPTURE_FAIL,          "1402"},    /* 拍照失败 */
    {START_REC_FAIL,        "1403"},    /* 录像失败 */
    {START_LIVE_FAIL,       "1404"},    /* 直播失败 */
    {START_QR_FAIL,         "1405"},    /* 启动二维码扫描失败 */
    {CALIBRATION_FAIL,      "1406"},    /* 矫正失败 */
    {QR_FINISH_ERROR,       "1407"},    /* 扫描二维码结束失败 */
    {START_GYRO_FAIL,       "1408"},
    {START_STA_WIFI_FAIL,   "1409"},
    {START_AP_WIFI_FAIL,    "1410"},
    {SPEED_TEST_FAIL,       "1411"},
    {START_NOISE_FAIL,      "1412"},
    {STOP_PREVIEW_FAIL,     "1501"},
    {STOP_REC_FAIL,         "1503"},
    {STOP_LIVE_FAIL,        "1504"},
    {STOP_QR_FAIL,          "1505"},
    {QR_FINISH_UNRECOGNIZE, "1507"},
    {STOP_STA_WIFI_FAIL,     "1509"},
    {STOP_AP_WIFI_FAIL,     "1510"},
    {RESET_ALL,             "1001"},
    {START_FORCE_IDLE,      "1002"}
};


class oled_arhandler : public ARHandler {
public:
    oled_arhandler(MenuUI *source): mHandler(source) {
    }

    virtual ~oled_arhandler() override {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override {
        mHandler->handleMessage(msg);
    }
	
private:
    MenuUI *mHandler;
};



/*************************************************************************
** 方法名称: MenuUI
** 方法功能: 构造函数,UI对象
** 入口参数: 无
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
MenuUI::MenuUI(const sp<ARMessage> &notify): mNotify(notify)
{
    Log.d(TAG, "MenuUI contructr......");


    init_handler_thread();	    /* 初始化消息处理线程 */

    Log.d(TAG, "Core UI thread created ... ");

    init();					    /* MenuUI内部成员初始化 */

    Log.d(TAG, "Send Init display Msg");

    send_init_disp();		    /* 给消息处理线程发送初始化显示消息 */

}


/*************************************************************************
** 方法名称: ~MenuUI
** 方法功能: 析构函数,UI对象
** 入口参数: 无
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
MenuUI::~MenuUI()
{
    deinit();
}


/*************************************************************************
** 方法名称: send_init_disp
** 方法功能: 发送初始化显示消息(给UI消息循环中投递消息)
** 入口参数: 无
** 返回值:   无 
** 调 用: MenuUI构造函数
**
*************************************************************************/
void MenuUI::send_init_disp()
{
    sp<ARMessage> msg = obtainMessage(UI_DISP_INIT);
    msg->post();
}



void MenuUI::set_setting_select(int type,int val)
{
    #if 0
    mSettingItems.iSelect[type] = val;
    #endif
}


void MenuUI::disp_top_info()
{
	
    if (mProCfg->get_val(KEY_WIFI_ON)) {
        disp_icon(ICON_WIFI_OPEN_0_0_16_16);
    } else {
        disp_icon(ICON_WIFI_CLOSE_0_0_16_16);
    }

	uiShowStatusbarIp();

    //disp battery icon
    oled_disp_battery();
    
    bDispTop = true;
}




/*************************************************************************
** 方法名称: init_cfg_select
** 方法功能: 根据配置初始化选择项
** 入口参数: 无
** 返回值:   无 
** 调 用:    oled_init_disp
**
*************************************************************************/
void MenuUI::init_cfg_select()
{
    
    Log.d(TAG, "[%s:%d] init_cfg_select...", __FILE__, __LINE__);

    mSetItemsList.clear();
    mPhotoDelayList.clear();
    mAebList.clear();

    mPicAllItemsList.clear();
    mVidAllItemsList.clear();
    mLiveAllItemsList.clear();

    init_menu_select();     /* 菜单项初始化 */

    sp<ARMessage> msg;
    sp<DEV_IP_INFO> tmpInfo;
    int iCmd = -1;

    tmpInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

	if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
        iCmd = NETM_STARTUP_NETDEV;
		disp_wifi(true);
	} else {
		disp_wifi(false);
        iCmd = NETM_CLOSE_NETDEV;
	}	

    msg = (sp<ARMessage>)(new ARMessage(iCmd));
    Log.d(TAG, "init_cfg_select: wifi state[%d]", mProCfg->get_val(KEY_WIFI_ON));
    msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
    NetManager::getNetManagerInstance()->postNetMessage(msg);

}


/*************************************************************************
** 方法名称: oled_init_disp
** 方法功能: 初始化显示
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::oled_init_disp()
{
    read_sn();						/* 获取系统序列号 */
    read_uuid();					/* 读取设备的UUID */
    read_ver_info();				/* 读取系统的版本信息 */ 
	
    init_cfg_select();				/* 根据配置初始化选择项 */

	/*
	 * 显示IP地址
	 */

	//disp top before check battery 170623 for met enter low power of protect at beginning
    bDispTop = true;				/* 显示顶部标志设置为true */

	check_battery_change(true);		/* 检查电池的状态 */
	// set_oled_power(1);
}


void MenuUI::start_qr_func()
{
    send_option_to_fifo(ACTION_QR);
}


void MenuUI::exit_qr_func()
{
    send_option_to_fifo(ACTION_QR);
}


void MenuUI::send_update_light(int menu, int state, int interval, bool bLight, int sound_id)
{
	Log.d(TAG, "send_update_light　(%d [%s] [%d] interval[%d] speaker[%d] sound_id %d) ", 
					bSendUpdate, 
					getCurMenuStr(menu),
					state,
					interval,
                    mProCfg->get_val(KEY_SPEAKER),
					sound_id);
	
    if (sound_id != -1 && mProCfg->get_val(KEY_SPEAKER) == 1) {
        flick_light();
        play_sound(sound_id);
	
        //force to 0 ,for play sounds cost times
        interval = 0;
    } else if (bLight) {	/* 需要闪灯 */
        flick_light();
    }

    if (!bSendUpdate) {
		
        bSendUpdate = true;
        sp<ARMessage> msg = obtainMessage(UI_DISP_LIGHT);
        msg->set<int>("menu", menu);
        msg->set<int>("state", state);
        msg->set<int>("interval", interval);
        msg->postWithDelayMs(interval);
    }
}



void MenuUI::write_p(int *p, int val)
{
    char c = (char)val;
    int  rc;
    rc = write_pipe(p, &c, 1);
    if (rc != 1) {
        Log.d("Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}



void MenuUI::play_sound(u32 type)
{

	Log.d(TAG, "play_sound type = %d", type);

    if (mProCfg->get_val(KEY_SPEAKER) == 1) {
        if (type >= 0 && type <= sizeof(sound_str) / sizeof(sound_str[0])) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "aplay -D hw:1,0 %s", sound_str[type]);
            exec_sh(cmd);
		} else {
            Log.d(TAG,"sound type %d exceed \n",type);
		}
    }
}


void MenuUI::sound_thread()
{
#ifdef ENABLE_SOUND
    while (!bExitSound) {
        if (mProCfg->get_val(KEY_SPEAKER) == 1) {

        }
        msg_util::sleep_ms(INTERVAL_1HZ);
    }
#endif
}


void MenuUI::init_sound_thread()
{
    th_sound_ = thread([this] { sound_thread(); });
}




/*************************************************************************
** 方法名称: commUpKeyProc
** 方法功能: 方向上键的通用处理
** 入口参数: 
** 返回值: 无 
** 调 用: procUpKeyEvent
**
*************************************************************************/
void MenuUI::commUpKeyProc()
{

    bool bUpdatePage = false;

#ifdef DEBUG_INPUT_KEY	
	Log.d(TAG, "addr 0x%p", &(mMenuInfos[cur_menu].mSelectInfo));
#endif

    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    CHECK_NE(mSelect, nullptr);

#ifdef DEBUG_INPUT_KEY		
	Log.d(TAG, "mSelect 0x%p", mSelect);
#endif
    mSelect->last_select = mSelect->select;

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG, "cur_menu %d commUpKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d", cur_menu,
          mSelect->select ,mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
#endif

    mSelect->select--;

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG, "select %d total %d mSelect->page_num %d",
          mSelect->select, mSelect->total, mSelect->page_num);
#endif

    if (mSelect->select < 0) {
        if (mSelect->page_num > 1) {    /* 需要上翻一页 */
            bUpdatePage = true;
            if (--mSelect->cur_page < 0) {  /* 翻完第一页到最后一页 */
                mSelect->cur_page = mSelect->page_num - 1;
                mSelect->select = (mSelect->total - 1) % mSelect->page_max; /* 选中最后一页最后一项 */
            } else {
                mSelect->select = mSelect->page_max - 1;    /* 上一页的最后一项 */
            }
        } else {    /* 整个菜单只有一页 */
            mSelect->select = mSelect->total - 1;   /* 选中最后一项 */
        }
    }

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG," commUpKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
#endif

    if (bUpdatePage) {  /* 更新菜单页 */
        if (mMenuInfos[cur_menu].privList) {
            vector<struct stSetItem*>* pSetItemLists = static_cast<vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);
            dispSettingPage(*pSetItemLists);
        } else {
            Log.e(TAG, "[%s:%d] Current Menu[%s] havn't privList ????, Please check", __FILE__, __LINE__, getCurMenuStr(cur_menu));
        }        
    } else {    /* 更新菜单(页内更新) */
        updateMenu();
    }
}



/*************************************************************************
** 方法名称: commDownKeyProc
** 方法功能: 方向下键的通用处理
** 入口参数: 
** 返回值: 无 
** 调 用: procDownKeyEvent
**
*************************************************************************/
void MenuUI::commDownKeyProc()
{
    bool bUpdatePage = false;
	
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    CHECK_NE(mSelect, nullptr);

#ifdef DEBUG_INPUT_KEY	
	Log.d(TAG," commDownKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
#endif

    mSelect->last_select = mSelect->select;
    mSelect->select++;
    if (mSelect->select + (mSelect->cur_page * mSelect->page_max) >= mSelect->total) {
        mSelect->select = 0;
        if (mSelect->page_num > 1) {
            mSelect->cur_page = 0;
            bUpdatePage = true;
        }
    } else if (mSelect->select >= mSelect->page_max) {
        mSelect->select = 0;
        if (mSelect->page_num > 1) {
            mSelect->cur_page++;
            bUpdatePage = true;
        }
    }

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG," commDownKeyProc select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over bUpdatePage %d",
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total, bUpdatePage);
#endif

    if (bUpdatePage) {
        if (mMenuInfos[cur_menu].privList) {
            vector<struct stSetItem*>* pSetItemLists = static_cast<vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);
            dispSettingPage(*pSetItemLists);
        } else {
            Log.e(TAG, "[%s:%d] Current Menu[%s] havn't privList ????, Please check", __FILE__, __LINE__, getCurMenuStr(cur_menu));
        }
    } else {
        updateMenu();
    }
}


/*************************************************************************
** 方法名称: init_handler_thread
** 方法功能: 创建事件处理线程
** 入口参数: 无
** 返 回 值: 无 
**
**
*************************************************************************/
void MenuUI::init_handler_thread()
{
    std::promise<bool> pr;
    std::future<bool> reply = pr.get_future();
    th_msg_ = thread([this, &pr]
                   {
                       mLooper = sp<ARLooper>(new ARLooper());
                       mHandler = sp<ARHandler>(new oled_arhandler(this));
                       mHandler->registerTo(mLooper);
                       pr.set_value(true);
                       mLooper->run();
                   });
    CHECK_EQ(reply.get(), true);
}



int MenuUI::getMenuLastSelectIndex(int menu)
{
    int val = mMenuInfos[menu].mSelectInfo.last_select + mMenuInfos[menu].mSelectInfo.cur_page * mMenuInfos[menu].mSelectInfo.page_max;
    return val;
}


int MenuUI::getMenuSelectIndex(int menu)
{
    int val = mMenuInfos[menu].mSelectInfo.select + mMenuInfos[menu].mSelectInfo.cur_page * mMenuInfos[menu].mSelectInfo.page_max;
    return val;
}


/*************************************************************************
** 方法名称: updateMenuCurPageAndSelect
** 方法功能: 为指定的菜单选择选择项
** 入口参数: menu - 菜单ID
**			 iSelect - 选择项ID
** 返 回 值: 无 
** 调     用: init_menu_select
**
*************************************************************************/
void MenuUI::updateMenuCurPageAndSelect(int menu, int iSelect)
{
    mMenuInfos[menu].mSelectInfo.cur_page = iSelect / mMenuInfos[menu].mSelectInfo.page_max;
    mMenuInfos[menu].mSelectInfo.select = iSelect % mMenuInfos[menu].mSelectInfo.page_max;
}



/*************************************************************************
** 方法名称: setSysMenuInit
** 方法功能: 设置子菜单下的设置子项初始化
** 入口参数: pParentMenu - 父菜单对象指针
** 返回值: 无 
** 调 用: init_menu_select
**
*************************************************************************/
void MenuUI::setSysMenuInit(MENU_INFO* pParentMenu)
{
    Log.d(TAG, "[%s:%d] Init System Setting subsyste Menu...", __FILE__, __LINE__);

    int size = pParentMenu->mSelectInfo.total;
    ICON_POS tmPos;
    int val = 0;
    SettingItem** pSetItem = static_cast<SettingItem**>(pParentMenu->priv);

    for (int i = 0; i < size; i++) {

        /*
         * 坐标的设置
         */
		int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
		switch (pos) {
			case 0:
				tmPos.yPos 		= 16;
				break;

			case 1:
				tmPos.yPos 		= 32;
				break;
			
			case 2:
				tmPos.yPos 		= 48;
				break;
		}

        tmPos.xPos 		= 32;
        tmPos.iWidth	= 96;
        tmPos.iHeight   = 16;

        pSetItem[i]->stPos = tmPos;


        /*
         * 配置值的初始化
         */
        const char* pItemName = pSetItem[i]->pItemName;

        if (!strcmp(pItemName, SET_ITEM_NAME_DHCP)) {   /* DHCP */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_DHCP);

        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "DHCP Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif 
            /* 需要开启DHCP?? */
        } else if (!strcmp(pItemName, SET_ITEM_NAME_FREQ)) {    /* FREQ -> 需要通知对方 */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_PAL_NTSC);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Flick Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif           
        } else if (!strcmp(pItemName, SET_ITEM_NAME_HDR)) {     /* HDR -> 需要通知对方 */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_HDR);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "HDR Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif             
            /* 需要启动HDR Feature */
            //send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);
        } else if (!strcmp(pItemName, SET_ITEM_NAME_RAW)) {     /* RAW */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_RAW);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Raw Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_AEB)) {     /* AEB */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_AEB);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "AEB Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif 
        } else if (!strcmp(pItemName, SET_ITEM_NAME_PHDEALY)) {     /* PHTODELAY */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_PH_DELAY);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "PhotoDelay Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif 
        } else if (!strcmp(pItemName, SET_ITEM_NAME_SPEAKER)) {     /* Speaker */
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_SPEAKER);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Speaker Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_LED)) {     /* 开机时根据配置,来决定是否开机后关闭前灯 */     
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_LIGHT_ON);
            if (val == 0) {
                setLightDirect(LIGHT_OFF);
            }
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "LedLight Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif              
        } else if (!strcmp(pItemName, SET_ITEM_NAME_AUDIO)) {       /* Audio -> 需要通知对方 */     
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_AUD_ON);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Audio Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif              
        } else if (!strcmp(pItemName, SET_ITEM_NAME_SPAUDIO)) {     /* Spatital Audio -> 需要通知对方 */          
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_AUD_SPATIAL);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "SpatitalAudio Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif              
        } else if (!strcmp(pItemName, SET_ITEM_NAME_FLOWSTATE)) {   /* FlowState -> 需要通知对方 */        
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_FLOWSTATE);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "FlowState Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif             
        } else if (!strcmp(pItemName, SET_ITEM_NAME_GYRO_ONOFF)) {        
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_GYRO_ON);
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Gyro OnOff Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif            
        } else if (!strcmp(pItemName, SET_ITEM_NAME_FAN)) {        
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_FAN);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "Fan Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif           
        } else if (!strcmp(pItemName, SET_ITEM_NAME_BOOTMLOGO)) {  /* Bottom Logo -> 需要通知对方  */      
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_SET_LOGO);
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "BottomLogo Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif              
        } else if (!strcmp(pItemName, SET_ITEM_NAME_VIDSEG)) {      /* Video Segment -> 需要通知对方  */        
            pSetItem[i]->iCurVal = mProCfg->get_val(KEY_VID_SEG);
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_VID_SEG);
        #ifdef DEBUG_SETTING_PAGE
            Log.d(TAG, "VideoSeg Init Val --> [%d]", pSetItem[i]->iCurVal);
        #endif  
        }
        mSetItemsList.push_back(pSetItem[i]);
    }
    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);

}


void MenuUI::setCommonMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists)
{
    Log.d(TAG, "[%s:%d] Init Set Photo Delay Menu...", __FILE__, __LINE__);

    if (pParentMenu) {
        int size = pParentMenu->mSelectInfo.total;
        ICON_POS tmPos;
        SettingItem** pSetItem = static_cast<SettingItem**>(pParentMenu->priv);

        for (int i = 0; i < size; i++) {

            /*
            * 坐标的设置
            */
            int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
            switch (pos) {
                case 0:
                    tmPos.yPos 		= 16;
                    break;

                case 1:
                    tmPos.yPos 		= 32;
                    break;
                
                case 2:
                    tmPos.yPos 		= 48;
                    break;
            }
            tmPos.xPos 		= 34;   /* 水平方向的起始坐标 */
            tmPos.iWidth	= 89;   /* 显示的宽 */
            tmPos.iHeight   = 16;   /* 显示的高 */
        
            pSetItem[i]->stPos = tmPos;
            pItemLists.push_back(pSetItem[i]);  
        }
    } else {
        Log.e(TAG, "[%s:%d] Invalid Pointer, please checke!!!", __FILE__, __LINE__);
    }
}

void MenuUI::setStorageMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists)
{
    Log.d(TAG, "[%s:%d] Init Set Photo Delay Menu...", __FILE__, __LINE__);

    if (pParentMenu) {
        int size = pParentMenu->mSelectInfo.total;
        ICON_POS tmPos;
        SettingItem** pSetItem = static_cast<SettingItem**>(pParentMenu->priv);

        for (int i = 0; i < size; i++) {

            /*
            * 坐标的设置
            */
            int pos = i % pParentMenu->mSelectInfo.page_max;		// 3
            switch (pos) {
                case 0:
                    tmPos.yPos 		= 16;
                    break;

                case 1:
                    tmPos.yPos 		= 32;
                    break;
                
                case 2:
                    tmPos.yPos 		= 48;
                    break;
            }
            tmPos.xPos 		= 25;   /* 水平方向的起始坐标 */
            tmPos.iWidth	= 103;   /* 显示的宽 */
            tmPos.iHeight   = 16;   /* 显示的高 */
        
            pSetItem[i]->stPos = tmPos;
            pItemLists.push_back(pSetItem[i]);  
        }
    } else {
        Log.e(TAG, "[%s:%d] Invalid Pointer, please checke!!!", __FILE__, __LINE__);
    }
}

/*
 * 设置页配置初始化
 */
void MenuUI::setMenuCfgInit()
{
    int iPageCnt = 0;

    /*
     * 设置菜单
     */
    mMenuInfos[MENU_SYS_SETTING].priv = static_cast<void*>(gSettingItems);
    mMenuInfos[MENU_SYS_SETTING].privList = static_cast<void*>(&mSetItemsList);

    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total = sizeof(gSettingItems) / sizeof(gSettingItems[0]);
    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.select = 0;
    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total % mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total / mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total / mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_num = iPageCnt;
    Log.d(TAG, "Setting Menu Info: total items [%d], page count[%d]", 
                mMenuInfos[MENU_SYS_SETTING].mSelectInfo.total,
                mMenuInfos[MENU_SYS_SETTING].mSelectInfo.page_num);

    setSysMenuInit(&mMenuInfos[MENU_SYS_SETTING]);   /* 设置系统菜单初始化 */


    /*
     * Photot delay菜单
     */
    mMenuInfos[MENU_SET_PHOTO_DEALY].priv = static_cast<void*>(gSetPhotoDelayItems);
    mMenuInfos[MENU_SET_PHOTO_DEALY].privList = static_cast<void*>(&mPhotoDelayList);

    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total = sizeof(gSetPhotoDelayItems) / sizeof(gSetPhotoDelayItems[0]);
    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.select = 0;   
    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total % mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total / mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total / mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_num = iPageCnt;

    /* 使用配置值来初始化首次显示的页面 */
    updateMenuCurPageAndSelect(MENU_SET_PHOTO_DEALY, mProCfg->get_val(KEY_PH_DELAY));

    Log.d(TAG, "Set PhotoDealy Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.total,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.page_num,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.cur_page,
                mMenuInfos[MENU_SET_PHOTO_DEALY].mSelectInfo.select
                );

    setCommonMenuInit(&mMenuInfos[MENU_SET_PHOTO_DEALY], mPhotoDelayList);   /* 设置系统菜单初始化 */


#ifdef ENABLE_MENU_AEB	

    mMenuInfos[MENU_SET_AEB].priv = static_cast<void*>(gSetAebItems);
    mMenuInfos[MENU_SET_AEB].privList = static_cast<void*>(&mAebList);

    mMenuInfos[MENU_SET_AEB].mSelectInfo.total = sizeof(gSetAebItems) / sizeof(gSetAebItems[0]);
    mMenuInfos[MENU_SET_AEB].mSelectInfo.select = 0;   
    mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total % mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total / mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_SET_AEB].mSelectInfo.total / mMenuInfos[MENU_SET_AEB].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_SET_AEB].mSelectInfo.page_num = iPageCnt;

    /* 使用配置值来初始化首次显示的页面 */
    updateMenuCurPageAndSelect(MENU_SET_AEB, mProCfg->get_val(KEY_AEB));

    Log.d(TAG, "Set AEB Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_SET_AEB].mSelectInfo.total,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.page_num,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.cur_page,
                mMenuInfos[MENU_SET_AEB].mSelectInfo.select
                );

    setCommonMenuInit(&mMenuInfos[MENU_SET_AEB], mAebList);   /* 设置系统菜单初始化 */
#endif


    mMenuInfos[MENU_STORAGE].priv = static_cast<void*>(gStorageSetItems);
    mMenuInfos[MENU_STORAGE].privList = static_cast<void*>(&mStorageList);

    mMenuInfos[MENU_STORAGE].mSelectInfo.total = sizeof(gStorageSetItems) / sizeof(gStorageSetItems[0]);
    mMenuInfos[MENU_STORAGE].mSelectInfo.select = 0;   
    mMenuInfos[MENU_STORAGE].mSelectInfo.page_max = 3;

    iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total % mMenuInfos[MENU_STORAGE].mSelectInfo.page_max;
    if (iPageCnt == 0) {
        iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total / mMenuInfos[MENU_STORAGE].mSelectInfo.page_max;
    } else {
        iPageCnt = mMenuInfos[MENU_STORAGE].mSelectInfo.total / mMenuInfos[MENU_STORAGE].mSelectInfo.page_max + 1;
    }

    mMenuInfos[MENU_STORAGE].mSelectInfo.page_num = iPageCnt;

#if 0
    /* 使用配置值来初始化首次显示的页面 */
    updateMenuCurPageAndSelect(MENU_SET_AEB, mProCfg->get_val(KEY_AEB));
#endif

    Log.d(TAG, "Set Storage Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_STORAGE].mSelectInfo.total,
                mMenuInfos[MENU_STORAGE].mSelectInfo.page_num,
                mMenuInfos[MENU_STORAGE].mSelectInfo.cur_page,
                mMenuInfos[MENU_STORAGE].mSelectInfo.select
                );

    setStorageMenuInit(&mMenuInfos[MENU_STORAGE], mStorageList);   /* 设置系统菜单初始化 */

}



void MenuUI::cfgPicModeItemCurVal(PicVideoCfg* pPicCfg)
{
    int iRawVal = 0;
    int iAebVal = 0;

    iRawVal = mProCfg->get_val(KEY_RAW);     /* 0: Off Raw; 1: Open Raw */
    iAebVal = mProCfg->get_val(KEY_AEB);  

    if (pPicCfg) {
        const char* pItemName = pPicCfg->pItemName;

        /* Customer模式: 从配置文件中读取保存的customer */
        if (!strcmp(pItemName, TAKE_PIC_MODE_CUSTOMER)) {   /* Customer DO nothing */
            pPicCfg->iCurVal = 0;
            pPicCfg->pStAction = mProCfg->get_def_info(KEY_ALL_PIC_DEF);
            if (pPicCfg->pStAction) {   /* 得到保存在配置文件中ACTION_INFO */
                Log.d(TAG, "Get user_cfg PIC ACTION_INFO ......");
                Log.d(TAG, "mode[%d] size_per_act [%d]M", pPicCfg->pStAction->mode, pPicCfg->pStAction->size_per_act);
            }
        } else if (!strcmp(pItemName, TAKE_PIC_MODE_AEB)) {
            int iIndexBase = 0;

            /* 根据RAW和AEB当前值来决定 */
            if (iRawVal) {
                iIndexBase = 4;
            }

            pPicCfg->iCurVal = iIndexBase + iAebVal;
            if (pPicCfg->iCurVal > pPicCfg->iItemMaxVal) {
                Log.e(TAG, "[%s: %d] Current val [%d], max val[%d]", __FILE__, __LINE__, pPicCfg->iCurVal, pPicCfg->iItemMaxVal);
            }
        } else {
            pPicCfg->iCurVal = iRawVal;
            Log.e(TAG, "[%s: %d] Current val [%d]", __FILE__, __LINE__, pPicCfg->iCurVal);
        }

    } else {
        Log.e(TAG, "[%s: %d] Invalid Argument passed", __FILE__, __LINE__);
    }
}


void MenuUI::cfgPicVidLiveSelectMode(MENU_INFO* pParentMenu, vector<struct stPicVideoCfg*>& pItemLists)
{
    if (pParentMenu && !pItemLists.size()) {
        
        int size = pParentMenu->mSelectInfo.total;
        ICON_POS tmPos = {0, 48, 78, 16};
        int iIndex = 0;

        PicVideoCfg** pSetItems = static_cast<PicVideoCfg**>(pParentMenu->priv);
        
        switch (pParentMenu->iMenuId) {
            case MENU_PIC_SET_DEF:       /* PIC */
                iIndex = mProCfg->get_val(KEY_ALL_PIC_DEF); /* 当前所中的项 */
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */

                /* 根据配置值来更新每个ACTION_INFO的值
                 * 根据配置是否使能RAW，来选各项显示图标的索引（显示图标用）
                 * 根据配置AEB值来选中显示图标的索引
                 * 以及传递参数的ACTION_INFO的值
                 */
                for (int i = 0; i < size; i++) {
                    pSetItems[i]->stPos = tmPos;

                    /* TODO: 在此处为各个项设置对应的ACTION_INFO
                     * 通过解析配置文件得到的ACTION_INFO列表(通过名称进行匹配)
                     */
                    cfgPicModeItemCurVal(pSetItems[i]);
                    pItemLists.push_back(pSetItems[i]);
                }
                break;

            case MENU_VIDEO_SET_DEF:
                iIndex = mProCfg->get_val(KEY_ALL_VIDEO_DEF); /* 当前所中的项 */
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */
                for (int i = 0; i < size; i++) {
                    pSetItems[i]->stPos = tmPos;
                    pSetItems[i]->iCurVal = 0;

                    /* ADD: */
                    if (!strcmp(pSetItems[i]->pItemName, TAKE_VID_MOD_CUSTOMER)) {   /* Customer DO nothing */
                        pSetItems[i]->iCurVal = 0;
                        pSetItems[i]->pStAction = mProCfg->get_def_info(KEY_ALL_VIDEO_DEF);
                        if (pSetItems[i]->pStAction) {   /* 得到保存在配置文件中ACTION_INFO */
                            Log.d(TAG, "Get user_cfg PIC ACTION_INFO ......");
                            Log.d(TAG, "mode[%d] size_per_act [%d]M", pSetItems[i]->pStAction->mode, pSetItems[i]->pStAction->size_per_act);
                        }
                    } 
                    pItemLists.push_back(pSetItems[i]);
                }                
                break;

            case MENU_LIVE_SET_DEF:
                iIndex = mProCfg->get_val(KEY_ALL_LIVE_DEF); /* 当前所中的项 */
                updateMenuCurPageAndSelect(pParentMenu->iMenuId, iIndex);   /* 根据配置来选中当前菜单默认选中的项 */
                for (int i = 0; i < size; i++) {
                    pSetItems[i]->stPos = tmPos;
                    pSetItems[i]->iCurVal = 0;

                    /* ADD: */
                    if (!strcmp(pSetItems[i]->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {   /* Customer DO nothing */
                        pSetItems[i]->iCurVal = 0;
                        pSetItems[i]->pStAction = mProCfg->get_def_info(KEY_ALL_LIVE_DEF);
                        if (pSetItems[i]->pStAction) {   /* 得到保存在配置文件中ACTION_INFO */
                            Log.d(TAG, "Get user_cfg PIC ACTION_INFO ......");
                            Log.d(TAG, "mode[%d] size_per_act [%d]M", pSetItems[i]->pStAction->mode, pSetItems[i]->pStAction->size_per_act);
                        }
                    } 
                    pItemLists.push_back(pSetItems[i]);
                }                
                break;

            default:
                Log.w(TAG, "[%s:%d] Unkown mode Please check", __FILE__, __LINE__);
                break;
        }
    } else {
        Log.e(TAG, "[%s: %d] Invalid Arguments Please check", __FILE__, __LINE__);
    }
}


void MenuUI::init_menu_select()
{

    /*
     * 设置系统的参数配置(优先于菜单初始化)
     */
    setMenuCfgInit();

    /*
     * 拍照，录像，直播参数配置
     */
    mMenuInfos[MENU_PIC_SET_DEF].priv = static_cast<void*>(gPicAllModeCfgList);
    mMenuInfos[MENU_PIC_SET_DEF].privList = static_cast<void*>(&mPicAllItemsList);

    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total = sizeof(gPicAllModeCfgList) / sizeof(gPicAllModeCfgList[0]);
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_num = 1;
    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_PIC_SET_DEF], mPicAllItemsList);
    
    Log.d(TAG, "[%s:%d] mPicAllItemsList size = %d", __FILE__, __LINE__, mPicAllItemsList.size());

    Log.d(TAG, "MENU_PIC_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.select
                );
    
    mMenuInfos[MENU_VIDEO_SET_DEF].priv = static_cast<void*>(gVidAllModeCfgList);
    mMenuInfos[MENU_VIDEO_SET_DEF].privList = static_cast<void*>(&mVidAllItemsList);

    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total = sizeof(gVidAllModeCfgList) / sizeof(gVidAllModeCfgList[0]);
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_num = 1;
    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_VIDEO_SET_DEF], mVidAllItemsList);
    
    Log.d(TAG, "[%s:%d] mVidAllItemsList size = %d", __FILE__, __LINE__, mVidAllItemsList.size());

    Log.d(TAG, "MENU_VIDEO_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.select
                );


    mMenuInfos[MENU_LIVE_SET_DEF].priv = static_cast<void*>(gLiveAllModeCfgList);
    mMenuInfos[MENU_LIVE_SET_DEF].privList = static_cast<void*>(&mLiveAllItemsList);

    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total = sizeof(gLiveAllModeCfgList) / sizeof(gLiveAllModeCfgList[0]);
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.select = 0;
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_max = mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total;
    mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_num = 1;

    cfgPicVidLiveSelectMode(&mMenuInfos[MENU_LIVE_SET_DEF], mLiveAllItemsList);
    
    Log.d(TAG, "[%s:%d] mLiveAllItemsList size = %d", __FILE__, __LINE__, mLiveAllItemsList.size());

    Log.d(TAG, "MENU_LIVE_SET_DEF Menu Info: total items [%d], page count[%d], cur page[%d], select [%d]", 
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.page_num,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.cur_page,
                mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.select
                );
}

/*************************************************************************
** 方法名称: init
** 方法功能: 初始化oled_hander对象内部成员
** 入口参数: 无
** 返 回 值: 无 
**
**
*************************************************************************/
void MenuUI::init()
{
    
    Log.d(TAG, "MenuUI init objects start ... file[%s], line[%d], date[%s], time[%s]", __FILE__, __LINE__, __DATE__, __TIME__);

    CHECK_EQ(sizeof(mMenuInfos) / sizeof(mMenuInfos[0]), MENU_MAX);
    CHECK_EQ(mControlAct, nullptr);

    CHECK_EQ(sizeof(astSysRead) / sizeof(astSysRead[0]), SYS_KEY_MAX);

    /*
     * 清空系统UI中的vector
     * - 本地存储
     * - 设置页设置项列表
     * - 设置二级页中photodelay列表
     * - 设置二级页中aeb列表
     */
    mLocalStorageList.clear();
    mRemoteStorageList.clear();


    Log.d(TAG, "init UI state: STATE_IDLE");
    cam_state = STATE_IDLE;

    Log.d(TAG, "Create OLED display Object...");
	/* OLED对象： 显示系统 */
    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);


    Log.d(TAG, "Create Dev Listener Object...");

	/* 设备管理器: 监听设备的动态插拔 */
    sp<ARMessage> dev_notify = obtainMessage(OLED_UPDATE_DEV_INFO);
    mDevManager = sp<dev_manager>(new dev_manager(dev_notify));
    CHECK_NE(mDevManager, nullptr);

    Log.d(TAG, "Create System Configure Object...");
    mProCfg = sp<pro_cfg>(new pro_cfg());

    mRemainInfo = sp<REMAIN_INFO>(new REMAIN_INFO());
    CHECK_NE(mRemainInfo, nullptr);
    memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));

    mRecInfo = sp<REC_INFO>(new REC_INFO());
    CHECK_NE(mRecInfo, nullptr);
    memset(mRecInfo.get(), 0, sizeof(REC_INFO));


    Log.d(TAG, "Create System Light Manager Object...");
 //   init_pipe(mCtrlPipe);
#ifdef ENABLE_LIGHT
    mOLEDLight = sp<oled_light>(new oled_light());
    CHECK_NE(mOLEDLight, nullptr);
#endif

    Log.d(TAG, "Create System Battery Manager Object...");
    mBatInterface = sp<battery_interface>(new battery_interface());
    CHECK_NE(mBatInterface, nullptr);

    m_bat_info_ = sp<BAT_INFO>(new BAT_INFO());
    CHECK_NE(m_bat_info_, nullptr);
    memset(m_bat_info_.get(), 0, sizeof(BAT_INFO));
    m_bat_info_->battery_level = 1000;

//    mControlAct = sp<ACTION_INFO>(new ACTION_INFO());
//    CHECK_NE(mControlAct,nullptr);

    Log.d(TAG, "Create System Info Object...");
    mReadSys = sp<SYS_INFO>(new SYS_INFO());
    CHECK_NE(mReadSys, nullptr);

    Log.d(TAG, "Create System Version Object...");
    mVerInfo = sp<VER_INFO>(new VER_INFO());
    CHECK_NE(mVerInfo, nullptr);

#ifdef ENABLE_WIFI_STA
    mWifiConfig = sp<WIFI_CONFIG>(new WIFI_CONFIG());
    CHECK_NE(mWifiConfig, nullptr);	
    memset(mWifiConfig.get(), 0, sizeof(WIFI_CONFIG));
#endif


    Log.d(TAG, "Create System NetManager Object...");
#ifdef ENABLE_PESUDO_SN

	char tmpName[32] = {0};
	if (access(WIFI_RAND_NUM_CFG, F_OK)) {
		srand(time(NULL));

		int iRandNum = rand() % 32768;
		Log.d(TAG, "rand num %d", iRandNum);

		sprintf(tmpName, "%5d", iRandNum);
		FILE* fp = fopen(WIFI_RAND_NUM_CFG, "w+");
		if (fp) {
			fprintf(fp, "%s", tmpName);
			fclose(fp);
			Log.d(TAG, "generated rand num and save[%s] ok", WIFI_RAND_NUM_CFG);
		}
	} else {
		FILE* fp = fopen(WIFI_RAND_NUM_CFG, "r");
		if (fp) {
			fgets(tmpName, 6, fp);
			Log.d(TAG, "get rand num [%s]", tmpName);
			fclose(fp);
		} else {
			Log.e(TAG, "open [%s] failed", WIFI_RAND_NUM_CFG);
			strcpy(tmpName, "Test");
		}
	}

	property_set(PROP_SYS_AP_PESUDO_SN, tmpName);
	Log.d(TAG, "get pesudo sn [%s]", property_get(PROP_SYS_AP_PESUDO_SN));
	
#endif


    memset(mLocalIpAddr, 0, sizeof(mLocalIpAddr));
    strcpy(mLocalIpAddr, "0.0.0.0");


    /* NetManager Subsystem Init
     * Eth0
     * Wlan0
     */

    mNetManager = NetManager::getNetManagerInstance();
    mNetManager->startNetManager();

    /* 注册以太网卡(eth0) */
	Log.d(TAG, "eth0 get ip mode [%s]", (mProCfg->get_val(KEY_DHCP) == 1) ? "DHCP" : "STATIC" );
    sp<EtherNetDev> eth0 = (sp<EtherNetDev>)(new EtherNetDev("eth0", mProCfg->get_val(KEY_DHCP)));
    sp<ARMessage> registerLanMsg = obtainMessage(NETM_REGISTER_NETDEV);
    registerLanMsg->set<sp<NetDev>>("netdev", eth0);
    mNetManager->postNetMessage(registerLanMsg);

    /* Register Wlan0 */
    sp<WiFiNetDev> wlan0 = (sp<WiFiNetDev>)(new WiFiNetDev(WIFI_WORK_MODE_AP, "wlan0", 0));
    sp<ARMessage> registerWlanMsg = obtainMessage(NETM_REGISTER_NETDEV);
    registerWlanMsg->set<sp<NetDev>>("netdev", wlan0);
    mNetManager->postNetMessage(registerWlanMsg);


    sp<ARMessage> looperMsg = obtainMessage(NETM_POLL_NET_STATE);
    mNetManager->postNetMessage(looperMsg);

    sp<ARMessage> listMsg = obtainMessage(NETM_LIST_NETDEV);
    mNetManager->postNetMessage(listMsg);

#ifdef  ENABLE_PESUDO_SN
	if (!mHaveConfigSSID) {

		const char* pRandSn = NULL;

		pRandSn = property_get(PROP_SYS_AP_PESUDO_SN);
		if (pRandSn == NULL) {
			pRandSn = "Test";
		}

		sp<WifiConfig> wifiConfig = (sp<WifiConfig>)(new WifiConfig());


		snprintf(wifiConfig->cApName, 32, "%s-%s", "Insta360-Pro2", pRandSn);
		strcpy(wifiConfig->cPasswd, "none");
		strcpy(wifiConfig->cInterface, WLAN0_NAME);
		wifiConfig->iApMode = WIFI_HW_MODE_G;
		wifiConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_BG;
		wifiConfig->iAuthMode = AUTH_WPA2;			/* 加密认证模式 */

		Log.d(TAG, "SSID[%s], Passwd[%s], Inter[%s], Mode[%d], Channel[%d], Auth[%d]",
								wifiConfig->cApName,
								wifiConfig->cPasswd,
								wifiConfig->cInterface,
								wifiConfig->iApMode,
								wifiConfig->iApChannel,
								wifiConfig->iAuthMode);

		handleorSetWifiConfig(wifiConfig);
		mHaveConfigSSID = true;
	}
#endif

#if 1

    Log.d(TAG, "Init Input Manager");
    sp<ARMessage> inputNotify = obtainMessage(UI_MSG_KEY_EVENT);
	mInputManager = sp<InputManager>(new InputManager(inputNotify));
    CHECK_NE(mInputManager, nullptr);

#endif

    Log.d(TAG, "Init MenUI object ok");
}


/*
 * 界面没变,IP地址发生变化
 * IP地址没变,界面发生变化
 */
void MenuUI::uiShowStatusbarIp()
{
	if (check_allow_update_top()) {
		if (!strlen(mLocalIpAddr)) {
			strcpy(mLocalIpAddr, "0.0.0.0");
		}
		mOLEDModule->disp_ip((const u8 *)mLocalIpAddr);
	}
}


sp<ARMessage> MenuUI::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}



void MenuUI::stop_bat_thread()
{
#if 0
    Log.d(TAG, "sendExit bExitBat %d", bExitBat);
    if (!bExitBat) {
        bExitBat = true;
        if (th_bat_.joinable()) {
            th_bat_.join();
        } else {
            Log.e(TAG, " th_light_ not joinable ");
        }
    }
    Log.d(TAG, "sendExit bExitBat %d over", bExitBat);
#endif
}

void MenuUI::sendExit()
{
    Log.d(TAG, "sendExit");

    if (!bExitMsg) {
        bExitMsg = true;
        if (th_msg_.joinable()) {
            obtainMessage(UI_EXIT)->post();
            th_msg_.join();
        } else {
            Log.e(TAG, " th_msg_ not joinable ");
        }
    }
    Log.d(TAG, "sendExit2");
}


/*
 * 更新TF卡存储信息
 */
void MenuUI::updateTfStorageInfo(vector<sp<Volume>>& mList)
{
#if 0    
    sp<ARMessage> msg = obtainMessage(UI_UPDATE_TF);
    msg->set<vector<sp<Volume>>>("tf_info", mStorageList);
    msg->post();

    /* Warnning: 加访问外部存储设备列表锁 
     * 此处的访问近可能快,并且不能死锁
     */
#else    
    unique_lock<mutex> lock(mRemoteDevLock);    
    mRemoteStorageList.clear();
    sp<Volume> tmpVolume = NULL;

    for (u32 i = 0; i < mList.size(); i++) {
        tmpVolume = mList.at(i);
        if (tmpVolume) {
            mRemoteStorageList.push_back(tmpVolume);
            Log.d(TAG, "Volume[%s] add to remote storage list", tmpVolume->name);
        }
    }

    /* 表示TF存储状态已经更新 */
    mRemoteStorageUpdate = true;

#endif

}

void MenuUI::send_disp_str(sp<DISP_TYPE> &sp_disp)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_STR_TYPE);
    msg->set<sp<DISP_TYPE>>("disp_type", sp_disp);
    msg->post();
}

void MenuUI::send_disp_err(sp<struct _err_type_info_> &mErrInfo)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_DISP_ERR_MSG);
    msg->set<sp<ERR_TYPE_INFO>>("err_type_info", mErrInfo);
    msg->post();
}

void MenuUI::send_sync_init_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_SET_SYNC_INFO);
    msg->set<sp<SYNC_INIT_INFO>>("sync_info", mSyncInfo);
    msg->post();
}

void MenuUI::send_get_key(int key)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_KEY_EVENT);
    msg->set<int>("oled_key", key);
    msg->post();
}

void MenuUI::send_long_press_key(int key,int64 ts)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_LONG_KEY_EVENT);
    msg->set<int>("key", key);
    msg->set<int64>("ts", ts);
    msg->postWithDelayMs(LONG_PRESS_MSEC);
}

void MenuUI::send_disp_ip(int ip, int net_type)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_UPDATE_IP);
    msg->set<int>("ip", ip);
    msg->post();
}

void MenuUI::send_disp_battery(int battery, bool charge)
{
    sp<ARMessage> msg = obtainMessage(UI_DISP_BATTERY);
    msg->set<int>("battery", battery);
    msg->set<bool>("charge", charge);
    msg->post();
}

void MenuUI::sendWifiConfig(sp<WifiConfig> &mConfig)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_CONFIG_WIFI);
    msg->set<sp<WifiConfig>>("wifi_config", mConfig);
    msg->post();
}


void MenuUI::send_sys_info(sp<SYS_INFO> &mSysInfo)
{
    sp<ARMessage> msg = obtainMessage(UI_MSG_SET_SN);
    msg->set<sp<SYS_INFO>>("sys_info",mSysInfo);
    msg->post();
}

void MenuUI::send_update_mid_msg(int interval)
{
    if (!bSendUpdateMid) {
        bSendUpdateMid = true;
        send_delay_msg(UI_UPDATE_MID, interval);
    } else {
        Log.d(TAG, "set_update_mid true (%d 0x%x)", cur_menu, cam_state);
    }
}

void MenuUI::set_update_mid()
{
    send_update_mid_msg(INTERVAL_1HZ);
	
    clear_icon(ICON_CAMERA_WAITING_2016_76X32);
	
    //just send msg from live connecting to live state
    if (!check_state_in(STATE_LIVE_CONNECTING)) {
        Log.d(TAG," reset rec cam_state 0x%x", cam_state);
        memset(mRecInfo.get(), 0, sizeof(REC_INFO));
        disp_mid();
        flick_light();
    }
}


int MenuUI::oled_reset_disp(int type)
{
    //reset to original
//    reset_last_info();
    INFO_MENU_STATE(cur_menu,cam_state)
    set_oled_power(1);
    cam_state = STATE_IDLE;
//keep sys error back to menu top
    disp_sys_err(type,MENU_TOP);
    //fix select if working by controller 0616
    mControlAct = nullptr;
    bStiching = false;

//    init_menu_select();
    //reset wifi config
    return 0;
}


void MenuUI::set_cur_menu_from_exit()
{
    int old_menu = cur_menu;
    int new_menu = get_back_menu(cur_menu);
    CHECK_NE(cur_menu, new_menu);
	
    Log.d(TAG, "cur_menu is %d new_menu %d", cur_menu, new_menu);
    if (new_menu == -1 && cur_menu == MENU_TOP) {
        Log.e(TAG,"set_cur_menu_from_exit met error ");
        cur_menu = MENU_TOP;
    }
	
    if (menuHasStatusbar(cur_menu)) {
        clear_area();
    }
	
    switch (new_menu) {
        case MENU_PIC_SET_DEF:
            setCurMenu(MENU_PIC_INFO);
            break;
		
        case MENU_VIDEO_SET_DEF:
            setCurMenu(MENU_VIDEO_INFO);
            break;
		
        case MENU_LIVE_SET_DEF:
            setCurMenu(MENU_LIVE_INFO);
            break;
		
        case MENU_SYS_ERR:
            Log.e(TAG, "func back to sys err");
            break;
		
        case MENU_CALIBRATION:
            Log.d(TAG, "func back from other to calibration");
            break;
        default:
            break;
    }
	
    setCurMenu(new_menu);
	
    if (old_menu == MENU_SYS_ERR || old_menu == MENU_LOW_BAT) {
        //force set normal light while backing from sys err
        setLight();
    }
}




/*************************************************************************
** 方法名称: setCurMenu
** 方法功能: 设置当前显示的菜单
** 入口参数: menu - 当前将要显示的菜单ID
**			 back_menu - 返回菜单ID
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void MenuUI::setCurMenu(int menu, int back_menu)
{
    bool bDispBottom = true; //add 170804 for menu_pic_info not recalculate for pic num MENU_TOP

	if (menu == cur_menu) {
        Log.d(TAG, "set cur menu same menu %d cur menu %d\n", menu, cur_menu);
        bDispBottom = false;
    } else  {
        if (menuHasStatusbar(menu))  {
            if (back_menu == -1)  {
                set_back_menu(menu, cur_menu);
            } else {
                set_back_menu(menu, back_menu);
            }
        }
        cur_menu = menu;
    }
	
    enterMenu(bDispBottom);
}



void MenuUI::restore_all()
{
    add_state(STATE_RESTORE_ALL);
}



void MenuUI::update_sys_info()
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();

    mSelect->last_select = mSelect->select;

    Log.d(TAG, " mSelect->last_select %d select %d\n", mSelect->last_select, mSelect->select);

    switch (mSelect->last_select) {
    case 0:
        disp_icon(ICON_INFO_SN_NORMAL_25_48_0_16103_16);
        break;
	
    case 1:
        disp_str((const u8 *)mVerInfo->r_v_str, 25,16,false,103);
        break;
	
    SWITCH_DEF_ERROR(mSelect->last_select)
    }

    switch (mSelect->select) {
    case 0:
        disp_icon(ICON_INFO_SN_LIGHT_25_48_0_16103_16);
        break;
	
    case 1:
        disp_str((const u8 *)mVerInfo->r_v_str, 25,16,true,103);
        break;
	
    SWITCH_DEF_ERROR(mSelect->last_select)
    }
}



void MenuUI::dispSysInfo()
{
    int col = 0; //25
    char buf[32];

    read_sn();

    clear_area(0, 16);  /* 清除状态栏之外的操作区域 */

    if (strlen(mReadSys->sn) <= SN_LEN) {
        snprintf(buf, sizeof(buf), "SN:%s", mReadSys->sn);
    } else {
        snprintf(buf, sizeof(buf), "SN:%s", (char *)&mReadSys->sn[strlen(mReadSys->sn) - SN_LEN]);
    }
	
    disp_str((const u8 *)buf, col, 16);
    disp_str((const u8 *)mVerInfo->r_v_str, col, 32);
}



/*
 * disp_msg_box - 显示消息框
 */
void MenuUI::disp_msg_box(int type)
{
    if (cur_menu == -1) {
        Log.e(TAG,"disp msg box before pro_service finish\n");
        return;
    }

    if (cur_menu != MENU_DISP_MSG_BOX) {
        //force light back to front or off 170731
        if (cur_menu == MENU_SYS_ERR || ((MENU_LOW_BAT == cur_menu) && check_state_equal(STATE_IDLE))) {
            //force set front light
            if (mProCfg->get_val(KEY_LIGHT_ON) == 1) {
                setLightDirect(front_light);
            } else {
                setLightDirect(LIGHT_OFF);
            }
        }
		
        setCurMenu(MENU_DISP_MSG_BOX);
        switch (type) {
            case DISP_LIVE_REC_USB:
            case DISP_ALERT_FAN_OFF:
            case DISP_VID_SEGMENT:
                send_clear_msg_box(2500);
                break;
			
            default:
                send_clear_msg_box();
                break;
        }
    }
	
    switch (type) {
        case DISP_DISK_FULL:
            disp_icon(ICON_STORAGE_INSUFFICIENT128_64);
            break;
		
        case DISP_NO_DISK:
            disp_icon(ICON_CARD_EMPTY128_64);
            break;
		
        case DISP_USB_ATTACH:
            disp_icon(ICON_USB_DETECTED128_64);
            break;
		
        case DIPS_USB_DETTACH:
            disp_icon(ICON_USB_REMOVED128_64);
            break;
		
        case DISP_SDCARD_ATTACH:
            disp_icon(ICON_SD_DETECTED128_64);
            break;
		
        case DISP_SDCARD_DETTACH:
            disp_icon(ICON_SD_REMOVED128_64);
            break;
		
        case DISP_WIFI_ON:
            disp_str((const u8 *)"not allowed",0,16);
            break;
		
        case DISP_ADB_OPEN:
            disp_icon(ICON_ADB_WAS_OPENED128_64);
            break;
		
        case DISP_ADB_CLOSE:
            disp_icon(ICON_ADB_WAS_CLOSED128_64);
            break;
		
        case DISP_ADB_ROOT:
            disp_icon(ICON_ADB_ROOT_128_64128_64);
            break;
		
        case DISP_ADB_UNROOT:
            disp_icon(ICON_ADB_UNROOT128_64);
            break;
		
        case DISP_ALERT_FAN_OFF:
            disp_icon(ICON_ALL_ALERT_FANOFFRECORDING128_64);
            break;
		
        case DISP_USB_CONNECTED:
            break;
			
        case DISP_VID_SEGMENT:
            disp_icon(ICON_SEGMENT_MSG_128_64128_64);
            break;
		
        case DISP_LIVE_REC_USB:
            disp_icon(ICON_LIVE_REC_USB_128_64128_64);
            break;
		
        SWITCH_DEF_ERROR(type);
    }
}




bool MenuUI::check_allow_pic()
{
    bool bRet = false;
//    Log.d(TAG,"check_allow_pic cam_state is 0x%x\n", cam_state);
    if (check_state_preview() || check_state_equal(STATE_IDLE)) {
        bRet = true;
    } else {
        Log.e(TAG, " check_allow_pic error state 0x%x ", cam_state);
    }
    return bRet;
}

bool MenuUI::start_speed_test()
{
    bool ret = false;
    if (!check_dev_speed_good(mLocalStorageList.at(mSavePathIndex)->path)) {
        setCurMenu(MENU_SPEED_TEST);
        ret = true;
    }
    return ret;
}

void MenuUI::fix_live_save_per_act(struct _action_info_ *mAct)
{
    if (mAct->stOrgInfo.save_org != SAVE_OFF) {
        mAct->size_per_act = (mAct->stOrgInfo.stOrgAct.mOrgV.org_br * 6) / 10;
    }

    if (mAct->stStiInfo.stStiAct.mStiL.file_save) {
       mAct->size_per_act += mAct->stStiInfo.stStiAct.mStiV.sti_br / 10;
    }

    if (mAct->size_per_act <= 0) {
        Log.d(TAG,"fix_live_save_per_act strange %d %d %d",
              mAct->stOrgInfo.save_org,mAct->stOrgInfo.stOrgAct.mOrgV.org_br,
              mAct->stStiInfo.stStiAct.mStiV.sti_br);
        mAct->size_per_act = 1;
    }
}



/*
 * 检查直播是否需要存文件
 */
bool MenuUI::check_live_save(ACTION_INFO* mAct)
{
    bool ret = false;

    if (mAct->stOrgInfo.save_org != SAVE_OFF || mAct->stStiInfo.stStiAct.mStiL.file_save) {
        ret = true;
    }

    Log.d(TAG, "check_live_save is %d %d ret %d",
          mAct->stOrgInfo.save_org, mAct->stStiInfo.stStiAct.mStiL.file_save, ret);
    return ret;
}



bool MenuUI::start_live_rec(const struct _action_info_ * mAct, ACTION_INFO *dest)
{
    bool bAllow = false;
	
    if (!checkHaveLocalSavePath()) {
		
		/* 去掉直播录像,必须录制在U盘的限制 */
		if (/* check_save_path_usb() */ 1) {
            if (!start_speed_test()) {
                if (cur_menu == MENU_LIVE_REC_TIME) {
                    cam_state |= STATE_START_LIVING;
                    setCurMenu(MENU_LIVE_INFO);
                    bAllow = true;
                    memcpy(dest, mAct, sizeof(ACTION_INFO));
                } else {
                    disp_icon(ICON_LIVE_REC_TIME_128_64128_64);

                    char disp[32];
                    
                    if (mAct->size_per_act > 0) {
                        if (localStorageAvail()) {
                            int size_M = (int)(mLocalStorageList.at(mSavePathIndex)->avail >> 20);
                            if (size_M > AVAIL_SUBSTRACT) {
                                size_M -= AVAIL_SUBSTRACT;

                                int rest_sec = size_M / mAct->size_per_act;
                                int min, hour, sec;


                                sec = rest_sec % 60;
                                min = rest_sec / 60;
                                hour = min / 60;

                                min = min % 60;
                                snprintf(disp, sizeof(disp), "%02d:%02d:%02d",
                                         hour, min, sec);
                            }  else {
                                snprintf(disp,sizeof(disp),"%s","00:00:00");
                            }
                        } else {
                            snprintf(disp,sizeof(disp),"%s","no access");
                            Log.e(TAG,"start_live_rec usb no access");
                        }
                    } else {
                        snprintf(disp,sizeof(disp),"%s","99:99:99");
                    }

                    Log.d(TAG, "start_live_rec pstTmp->size_per_act %d %s", mAct->size_per_act,disp);

                    disp_str((const u8 *)disp, 37, 32);
                    setCurMenu(MENU_LIVE_REC_TIME);
                }
            }
        }
    }
    return bAllow;
}



void MenuUI::setGyroCalcDelay(int iDelay)
{
	if (iDelay > 0)
		mGyroCalcDelay = iDelay;
	else 
		mGyroCalcDelay = 5;	/* Default */
}



//cmd -- for power off or set option
bool MenuUI::send_option_to_fifo(int option,int cmd,struct _cam_prop_ * pstProp)
{
    bool bAllow = true;
    sp<ARMessage> msg = mNotify->dup();
    sp<ACTION_INFO> mActionInfo = sp<ACTION_INFO>(new ACTION_INFO());
    int iIndex = 0;
    struct stPicVideoCfg* pTmpPicVidCfg = NULL;

    switch (option) {

        case ACTION_PIC:	/* 拍照动作 */

            Log.d(TAG, ">>>>>>>>>>>>>>>>> send_option_to_fifo +++ ACTION_PIC");

            /* customer和非customer */
            iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);
            pTmpPicVidCfg = mPicAllItemsList.at(iIndex);

            if (pTmpPicVidCfg) {
                
                Log.d(TAG, "[%s: %d] Take pic mode [%s]", __FILE__, __LINE__, pTmpPicVidCfg->pItemName);

                if (!strcmp(pTmpPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {

                    Log.d(TAG, "[%s: %d] Customer mode Take Pic", __FILE__, __LINE__);
                    
                    memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_ALL_PIC_DEF), sizeof(ACTION_INFO));
                
                    if (strlen(mActionInfo->stProp.len_param) > 0 || strlen(mActionInfo->stProp.mGammaData) > 0) {
                        mProCfg->get_def_info(KEY_ALL_PIC_DEF)->stProp.audio_gain = -1;
                        send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_ALL_PIC_DEF)->stProp);
                    }
                } else {    /* 非Customer模式 */

                    /* 根据是否开启RAW来设置 */
                    int iRaw = mProCfg->get_val(KEY_RAW);
                    int iAebVal = mProCfg->get_val(KEY_AEB);

                    if (iRaw) {
                        pTmpPicVidCfg->pStAction->stOrgInfo.mime = EN_JPEG_RAW;
                    } else {
                        pTmpPicVidCfg->pStAction->stOrgInfo.mime = EN_JPEG;
                    }

                    if (strcmp(pTmpPicVidCfg->pItemName, TAKE_PIC_MODE_AEB) == 0) {
                        pTmpPicVidCfg->pStAction->stOrgInfo.stOrgAct.mOrgP.hdr_count = convIndex2AebNum(iAebVal);
                    }

                    Log.d(TAG, "[%s:%d] Raw [%d] stitch mode [%d], aeb[%d]", 
                            __FILE__, __LINE__, iRaw, pTmpPicVidCfg->pStAction->mode,
                            iAebVal);
                    
                    memcpy(mActionInfo.get(), pTmpPicVidCfg->pStAction, sizeof(ACTION_INFO));
                }

                msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
            } else {
                Log.e(TAG, "[%s:%d] Invalid index[%d]", __FILE__, __LINE__);
                abort();
            }
            break;
			

        case ACTION_VIDEO:
        #if 1
            if (check_state_in(STATE_RECORD) && (!check_state_in(STATE_STOP_RECORDING))) {
                oled_disp_type(STOP_RECING);
            } else if (check_state_preview()) {
                #if 0
                if (check_dev_exist(option)) {
                    if (start_speed_test()) {
                        bAllow = false;
                    } else {
                        oled_disp_type(START_RECING);
                        item = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
//                        Log.d(TAG, " vid set is %d ", item);
                        if (item >= 0 && item < VID_CUSTOM) {
                            memcpy(mActionInfo.get(), &mVIDAction[item], sizeof(ACTION_INFO));
                        }
                        else if (VID_CUSTOM == item)
                        {
                            send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stProp);
                            memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_ALL_VIDEO_DEF), sizeof(ACTION_INFO));
                        }
                        else
                        {
                            ERR_ITEM(item);
                        }
                        msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                    }
                } else {
                    bAllow = false;
                }
                #else 
                if (check_dev_exist(option)) {
                    if (start_speed_test()) {
                        bAllow = false;
                    } else {
                        oled_disp_type(START_RECING); 
                        iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
                        pTmpPicVidCfg = mVidAllItemsList.at(iIndex);
                        if (pTmpPicVidCfg) {
                
                            Log.d(TAG, "[%s: %d] Take Live mode [%s]", __FILE__, __LINE__, pTmpPicVidCfg->pItemName);
                            
                            /* Customer模式 */
                            if (!strcmp(pTmpPicVidCfg->pItemName, TAKE_VID_MOD_CUSTOMER)) {
                                Log.d(TAG, "[%s: %d] Customer mode Take Pic", __FILE__, __LINE__);
                
                                send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stProp);
                                memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_ALL_VIDEO_DEF), sizeof(ACTION_INFO));
                            } else {    /* 非Customer模式 */
                                Log.d(TAG, "[%s:%d] stitch mode [%d]", __FILE__, __LINE__, pTmpPicVidCfg->pStAction->mode);
                                memcpy(mActionInfo.get(), pTmpPicVidCfg->pStAction, sizeof(ACTION_INFO));
                            }
                            msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                        } else {
                            Log.e(TAG, "[%s:%d] Invalid index[%d]", __FILE__, __LINE__);
                            abort();
                        }
                    }
                }
                #endif
            } else {
                Log.w(TAG, " ACTION_VIDEO bad state 0x%x", cam_state);
                bAllow = false;
            }

        #endif            
            break;
			
        case ACTION_LIVE:
            if ((check_live()) && (!check_state_in(STATE_STOP_LIVING))) {
                oled_disp_type(STOP_LIVING);
            } else if (check_state_preview()) {

                #if 0
                item = getMenuSelectIndex(MENU_LIVE_SET_DEF);
//				Log.d(TAG, "live item is %d", item);
                switch (item) {
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
                        oled_disp_type(STRAT_LIVING);
                        memcpy(mActionInfo.get(), &mLiveAction[item], sizeof(ACTION_INFO));
                        break;

                    case VID_ARIAL:
                        bAllow = start_live_rec(&mLiveAction[item],mActionInfo.get());
                        break;
                    
                    case LIVE_CUSTOM:

                        if (!check_live_save(mProCfg->get_def_info(KEY_ALL_LIVE_DEF))) {
                            oled_disp_type(STRAT_LIVING);
                            memcpy(mActionInfo.get(),
                                   mProCfg->get_def_info(KEY_ALL_LIVE_DEF),
                                   sizeof(ACTION_INFO));
//                            Log.d(TAG, "(url and format %s %s)",
//                                  mActionInfo->stStiInfo.stStiAct.mStiL.url,
//                                  mActionInfo->stStiInfo.stStiAct.mStiL.format);
                        } else {
                            bAllow = start_live_rec((const ACTION_INFO *)mProCfg->get_def_info(KEY_ALL_LIVE_DEF),mActionInfo.get());
                        }

                        if (bAllow) {
                            send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stProp);
                        }

                        break;
#ifdef LIVE_ORG
                    case LIVE_ORIGIN:
                        option = ACTION_LIVE_ORIGIN;
                        break;
#endif
                    SWITCH_DEF_ERROR(item);
                }

                #else                

                Log.d(TAG, ">>>>>>>>>>>>>>>>> send_option_to_fifo +++ ACTION_LIVE");

                /* customer和非customer */
                iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);
                pTmpPicVidCfg = mLiveAllItemsList.at(iIndex);
                if (pTmpPicVidCfg) {
                    oled_disp_type(STRAT_LIVING);

                    Log.d(TAG, "[%s: %d] Take Live mode [%s]", __FILE__, __LINE__, pTmpPicVidCfg->pItemName);

                    /* Customer模式 */
                    if (!strcmp(pTmpPicVidCfg->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {
                        Log.d(TAG, "[%s: %d] Customer mode Take Pic", __FILE__, __LINE__);
                        memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_ALL_LIVE_DEF), sizeof(ACTION_INFO));
            

                        if (!check_live_save(mProCfg->get_def_info(KEY_ALL_LIVE_DEF))) {
                            oled_disp_type(STRAT_LIVING);
                            memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_ALL_LIVE_DEF), sizeof(ACTION_INFO));
                        } else {
                            bAllow = start_live_rec((const ACTION_INFO *)mProCfg->get_def_info(KEY_ALL_LIVE_DEF),mActionInfo.get());
                        }
                    } else {    /* 非Customer模式 */

                        oled_disp_type(STRAT_LIVING);

                        Log.d(TAG, "[%s:%d] stitch mode [%d]", 
                                __FILE__, __LINE__, pTmpPicVidCfg->pStAction->mode);
                    
                        memcpy(mActionInfo.get(), pTmpPicVidCfg->pStAction, sizeof(ACTION_INFO));
                    }

                    msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                } else {
                    Log.e(TAG, "[%s:%d] Invalid index[%d]", __FILE__, __LINE__);
                    abort();
                }
                #endif
            } else {
                Log.w(TAG," act live bad state 0x%x", cam_state);
                bAllow = false;
            }


#if 0
#ifdef LIVE_ORG
                if (option != ACTION_LIVE_ORIGIN) {
                    msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                }
#else
                msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
#endif
            } else {
                Log.w(TAG," act live bad state 0x%x",cam_state);
                bAllow = false;
            }
#endif
            break;
			


        case ACTION_PREVIEW:
            if (check_state_preview() || check_state_equal(STATE_IDLE)) {
                bAllow = true;
            } else {
                ERR_MENU_STATE(cur_menu, cam_state);
                bAllow = false;
            }
            break;
			

        case ACTION_CALIBRATION:	/* 陀螺仪校正 */
			#if 0
            if ((cam_state & STATE_CALIBRATING) != STATE_CALIBRATING) {
                setGyroCalcDelay(5);
                oled_disp_type(START_CALIBRATIONING);
            } else {
                Log.e(TAG, "calibration happen cam_state 0x%x", cam_state);
                bAllow = false;
            }
			#endif
			
            break;
			
        case ACTION_QR:
//            Log.d(TAG,"action qr state 0x%x", cam_state);
            if ((check_state_equal(STATE_IDLE) || check_state_preview())) {
                oled_disp_type(START_QRING);
            } else if (check_state_in(STATE_START_QR) && !check_state_in(STATE_STOP_QRING)) {
                oled_disp_type(STOP_QRING);
            } else {
                bAllow = false;
            }
            break;
			
        case ACTION_REQ_SYNC: {     /* 请求同步ACTION */
            sp<REQ_SYNC> mReqSync = sp<REQ_SYNC>(new REQ_SYNC());
            snprintf(mReqSync->sn, sizeof(mReqSync->sn), "%s", mReadSys->sn);
            snprintf(mReqSync->r_v, sizeof(mReqSync->r_v), "%s", mVerInfo->r_ver);
            snprintf(mReqSync->p_v, sizeof(mReqSync->p_v), "%s", mVerInfo->p_ver);
            snprintf(mReqSync->k_v, sizeof(mReqSync->k_v), "%s", mVerInfo->k_ver);
            msg->set<sp<REQ_SYNC>>("req_sync", mReqSync);
			break;
        }
            
        case ACTION_LOW_BAT:
            msg->set<int>("cmd", cmd);
            break;
		
//        case ACTION_LOW_PROTECT:
//            break;


        case ACTION_SPEED_TEST:
            if (checkHaveLocalSavePath()) {
                Log.e(TAG, "action speed test mSavePathIndex -1");
                bAllow = false;
            } else {
                if (!check_state_in(STATE_SPEED_TEST)) {
                    msg->set<char *>("path", mLocalStorageList.at(mSavePathIndex)->path);
                    oled_disp_type(SPEED_START);
                } else {
                    bAllow = false;
                }
            }
            break;

        case ACTION_GYRO:
            if (check_state_in(STATE_IDLE)) {
                oled_disp_type(START_GYRO);
            } else  {
                ERR_MENU_STATE(cur_menu, cam_state);
                bAllow = false;
            }
            break;

        case ACTION_NOISE:
            if (check_state_in(STATE_IDLE)) {
                oled_disp_type(START_NOISE);
            } else {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;

        case ACTION_AGEING:
            if (check_state_in(STATE_IDLE)) {
                oled_disp_type(START_RECING);
                setCurMenu(MENU_AGEING);
            } else {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;

        case ACTION_SET_OPTION:
            msg->set<int>("type", cmd);
            switch (cmd) {
                case OPTION_FLICKER:
                    msg->set<int>("flicker", mProCfg->get_val(KEY_PAL_NTSC));
                    break;

                case OPTION_LOG_MODE:
                    msg->set<int>("mode", 1);
                    msg->set<int>("effect", 0);
                    break;

                case OPTION_SET_FAN:
                    msg->set<int>("fan", mProCfg->get_val(KEY_FAN));
                    break;

                case OPTION_SET_AUD:
                    if (mProCfg->get_val(KEY_AUD_ON) == 1) {
                        if (mProCfg->get_val(KEY_AUD_SPATIAL) == 1) {
                            msg->set<int>("aud", 2);
                        } else {
                            msg->set<int>("aud", 1);
                        }
                    } else {
                        msg->set<int>("aud", 0);
                    }
                    break;

                case OPTION_GYRO_ON:
                    msg->set<int>("gyro_on", mProCfg->get_val(KEY_GYRO_ON));
                    break;

                case OPTION_SET_LOGO:
                    msg->set<int>("logo_on", mProCfg->get_val(KEY_SET_LOGO));
                    break;

                case OPTION_SET_VID_SEG:
                    msg->set<int>("video_fragment", mProCfg->get_val(KEY_VID_SEG));
                    break;

//                case OPTION_SET_AUD_GAIN:
//                {
//                    sp<CAM_PROP> mProp = sp<CAM_PROP>(new CAM_PROP());
//                    memcpy(mProp.get(),pstProp,sizeof(CAM_PROP));
//                    msg->set<sp<CAM_PROP>>("cam_prop", mProp);
//                }
//                    break;
                SWITCH_DEF_ERROR(cmd);
            }
            break;

        case ACTION_POWER_OFF:
            if (!check_state_in(STATE_POWER_OFF)) {
                add_state(STATE_POWER_OFF);
                clear_area();
                setLightDirect(LIGHT_OFF);
                msg->set<int>("cmd", cmd);
            } else {
                ERR_MENU_STATE(cur_menu, cam_state);
                bAllow = false;
            }
            break;

        case ACTION_CUSTOM_PARAM: {
            sp<CAM_PROP> mProp = sp<CAM_PROP>(new CAM_PROP());
            memcpy(mProp.get(), pstProp, sizeof(CAM_PROP));
            msg->set<sp<CAM_PROP>>("cam_prop", mProp);
			break;
        }
            
        case ACTION_SET_STICH:
            break;
			
		case ACTION_AWB:		
			setLightDirect(FRONT_GREEN);
			break;			

		case ACTION_QUERY_STORAGE:
			break;
			
        SWITCH_DEF_ERROR(option)
    }


	if (bAllow) {
        msg->set<int>("what", OLED_KEY);
        msg->set<int>("action", option);
        msg->post();
    }
	
    return bAllow;
}



void MenuUI::postUiMessage(sp<ARMessage>& msg)
{
    msg->setHandler(mHandler);
    msg->post();
}

/*
 * send_save_path_change - 设备的存储路径发生改变
 */
void MenuUI::send_save_path_change()
{
    switch (cur_menu) {
        case MENU_PIC_INFO:
        case MENU_VIDEO_INFO:
        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
            //make update bottom before sent it to ws 20170710
            if (!check_cam_busy())  {	//not statfs while busy ,add 20170804
                updateBottomSpace(true);
            }
            break;
			
        default:
            break;
    }

    sp<ARMessage> msg = mNotify->dup();
    msg->set<int>("what", SAVE_PATH_CHANGE);
    sp<SAVE_PATH> mSavePath = sp<SAVE_PATH>(new SAVE_PATH());
	
    if (!checkHaveLocalSavePath()) {
        snprintf(mSavePath->path, sizeof(mSavePath->path), "%s", mLocalStorageList.at(mSavePathIndex)->path);
    } else {
        snprintf(mSavePath->path, sizeof(mSavePath->path), "%s", NONE_PATH);
    }
	
    msg->set<sp<SAVE_PATH>>("save_path", mSavePath);
    msg->post();
}



void MenuUI::send_delay_msg(int msg_id, int delay)
{
    sp<ARMessage> msg = obtainMessage(msg_id);
    msg->postWithDelayMs(delay);
}

void MenuUI::send_bat_low()
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_BAT_LOW);
    msg->post();
}

void MenuUI::send_read_bat()
{
    sp<ARMessage> msg = obtainMessage(UI_READ_BAT);
    msg->post();
}

void MenuUI::send_clear_msg_box(int delay)
{
    sp<ARMessage> msg = obtainMessage(UI_CLEAR_MSG__BOX);
    msg->postWithDelayMs(delay);
}


void MenuUI::send_update_dev_list(std::vector<sp<Volume>> &mList)
{
    sp<ARMessage> msg = mNotify->dup();

    msg->set<int>("what", UPDATE_DEV);
    msg->set<std::vector<sp<Volume>>>("dev_list", mList);
    msg->post();
}



/*************************************************************************
** 方法名称: read_ver_info
** 方法功能: 读取系统版本信息
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void MenuUI::read_ver_info()
{
    char file_name[64];

	/* 读取系统的版本文件:  */
    if (check_path_exist(VER_FULL_PATH))  {
        snprintf(file_name, sizeof(file_name), "%s", VER_FULL_PATH);
    } else {
        memset(file_name, 0, sizeof(file_name));
    }

    if (strlen(file_name)  > 0)  {
        int fd = open(file_name, O_RDONLY);
        CHECK_NE(fd, -1);

        char buf[1024];
        if (read_line(fd, (void *) buf, sizeof(buf)) > 0) {
            snprintf(mVerInfo->r_ver, sizeof(mVerInfo->r_ver), "%s", buf);
        } else {
            snprintf(mVerInfo->r_ver,sizeof(mVerInfo->r_ver), "%s", "999");
        }
        close(fd);
    } else {
        Log.d(TAG, "r not f");
        snprintf(mVerInfo->r_ver, sizeof(mVerInfo->r_ver), "%s", "000");
    }
	
    snprintf(mVerInfo->r_v_str, sizeof(mVerInfo->r_v_str), "V: %s", mVerInfo->r_ver);
	
    snprintf(mVerInfo->p_ver, sizeof(mVerInfo->p_ver), "%s", GIT_SHA1);

	/* 内核使用的版本 */
    snprintf(mVerInfo->k_ver, sizeof(mVerInfo->k_ver), "%s", "4.4.38");
    Log.d(TAG, "r:%s p:%s k:%s\n", mVerInfo->r_ver, mVerInfo->p_ver, mVerInfo->k_ver);
}




/*************************************************************************
** 方法名称: read_sn
** 方法功能: 读取序列号
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void MenuUI::read_sn()
{
    read_sys_info(SYS_KEY_SN);
}


/*************************************************************************
** 方法名称: read_uuid
** 方法功能: 读取设备UUID
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void MenuUI::read_uuid()
{
    read_sys_info(SYS_KEY_UUID);
}


bool MenuUI::read_sys_info(int type, const char *name)
{
    bool ret = false;
	
    if (check_path_exist(name)) {
        int fd = open(name, O_RDONLY);
        if (fd != -1) {
            char buf[1024];
            char *pStr;
            if (read_line(fd, (void *) buf, sizeof(buf)) > 0) {
                if (strlen(buf) > 0) {
//                    Log.d(TAG,"buf is %s name %s",buf,name);
                    pStr = strstr(buf, astSysRead[type].header);
                    if (pStr) {
                        pStr = pStr + strlen(astSysRead[type].header);
//                        Log.d(TAG,"pStr %s",pStr);
                        switch (type) {
                            case SYS_KEY_SN:
                                snprintf(mReadSys->sn, sizeof(mReadSys->sn), "%s",pStr);
                                ret = true;
                                break;
                            case SYS_KEY_UUID:
                                snprintf(mReadSys->uuid, sizeof(mReadSys->uuid), "%s",pStr);
                                ret = true;
                                break;
                            default:
                                break;
                        }
                    }
                } else {
                    Log.d(TAG,"no buf is in %s", name);
                }
            }
            close(fd);
        }
    }
    return ret;
}



/*************************************************************************
** 方法名称: read_sys_info
** 方法功能: 读取系统信息
** 入口参数: type - 信息类型
** 返 回 值: 是否查找到信息
** 调     用: 
**
*************************************************************************/
bool MenuUI::read_sys_info(int type)
{
    bool bFound = false;

	/*
	 * 检查/data/下是否存在sn,uuid文件
	 * 1.检查/home/nvidia/insta360/etc/下SN及UUID文件是否存在,如果不存在,查看/data/下的SN,UUID文件是否存在
	 * 2.如果/home/nvidia/insta360/etc/下SN及UUID文件存在,直接读取配置
	 * 3.如果/home/nvidia/insta360/etc/下SN,UUID不存在,而/data/下存在,拷贝过来并读取
	 * 4.都不存在,在/home/nvidia/insta360/etc/下创建SN,UUID文件并使用默认值
	 */
	
    if (!bFound) {
        bFound = read_sys_info(type, astSysRead[type].file_name);
        if (!bFound) {
            switch (type) {
                case SYS_KEY_SN:
                    snprintf(mReadSys->sn, sizeof(mReadSys->sn), "%s", "sn123456");
                    break;

                case SYS_KEY_UUID:
                    snprintf(mReadSys->uuid, sizeof(mReadSys->uuid), "%s", "uuid12345678");
                    break;

                default:
                    break;
            }
        }
    }

    return bFound;
}


/*************************************************************************
** 方法名称: set_sync_info
** 方法功能: 保存同步信息
** 入口参数: mSyncInfo - 同步初始化信息对象强指针引用
** 返回值: 无
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::set_sync_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    int state = mSyncInfo->state;

    snprintf(mVerInfo->a12_ver, sizeof(mVerInfo->a12_ver), "%s", mSyncInfo->a_v);
    snprintf(mVerInfo->c_ver, sizeof(mVerInfo->c_ver), "%s", mSyncInfo->c_v);
    snprintf(mVerInfo->h_ver, sizeof(mVerInfo->h_ver), "%s", mSyncInfo->h_v);

    Log.d(TAG, "[%s: %d] sync state 0x%x va:%s vc %s vh %s",
          __FILE__, __LINE__, mSyncInfo->state, mSyncInfo->a_v, mSyncInfo->c_v, mSyncInfo->h_v);

    INFO_MENU_STATE(cur_menu, cam_state);
    Log.d(TAG, "[%s: %d] SET SYNC INFO: get state[%d]", __FILE__, __LINE__, state);

    if (state == STATE_IDLE) {	            /* 如果相机处于Idle状态: 显示主菜单 */
		Log.d(TAG, "set_sync_info oled_reset_disp cam_state 0x%x, cur_menu %d", cam_state, cur_menu);
        cam_state = STATE_IDLE;
        setCurMenu(MENU_TOP);	/* 初始化显示为顶级菜单 */
    } else if ((state & STATE_RECORD) == STATE_RECORD) {    /* 录像状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_REC_AND_PREVIEW);
        } else {
            oled_disp_type(START_REC_SUC);
        }
    } else if ((state & STATE_LIVE) == STATE_LIVE) {        /* 直播状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_LIVE_AND_PREVIEW);
        } else {
            oled_disp_type(START_LIVE_SUC);
        }
    } else if ((state & STATE_LIVE_CONNECTING) == STATE_LIVE_CONNECTING) {  /* 直播连接状态 */
        if ((state & STATE_PREVIEW) == STATE_PREVIEW) {
            oled_disp_type(SYNC_LIVE_CONNECT_AND_PREVIEW);
        } else {
            oled_disp_type(START_LIVE_CONNECTING);
        }
    } else if ((state & STATE_CALIBRATING) == STATE_CALIBRATING) {          /* 拼接校正状态 */
        oled_disp_type(START_CALIBRATIONING);
    } else if ((state & STATE_PIC_STITCHING) == STATE_PIC_STITCHING) {      /* 图像拼接状态 */
        oled_disp_type(SYNC_PIC_STITCH_AND_PREVIEW);
    } else if ((state & STATE_TAKE_CAPTURE_IN_PROCESS) == STATE_CALIBRATING) {
        oled_disp_type(SYNC_PIC_CAPTURE_AND_PREVIEW);
    } else if ((state & STATE_PREVIEW) == STATE_PREVIEW)  {                 /* 启动预览状态 */
        oled_disp_type(START_PREVIEW_SUC);
    }
	
}

void MenuUI::write_sys_info(sp<SYS_INFO> &mSysInfo)
{

#if 0
    {
        const char *new_line = "\n";

        int fd = open(sys_file_name, O_RDWR | O_CREAT | O_TRUNC);
        CHECK_NE(fd, -1);
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        unsigned int write_len = 0;
        unsigned int len = 0;
        char *val;
        lseek(fd, 0, SEEK_SET);
        for (int i = 0; i < SYS_KEY_MAX; i++) {
            switch (i) {
                case SYS_KEY_SN:
                    val = mSysInfo->sn;
                    break;
                case SYS_KEY_UUID:
                    val = mSysInfo->uuid;
                    break;
                SWITCH_DEF_ERROR(i);
            }
            snprintf(buf, sizeof(buf), "%s%s%s", sys_key[i],val, new_line);
            Log.d(TAG, " write sys %s", buf);
            len = strlen(buf);
            write_len = write(fd, buf, len);
            CHECK_EQ(write_len,len);
        }
        close(fd);
        memcpy(mReadSys.get(),mSysInfo.get(),sizeof(SYS_INFO));
    }
#else
    Log.d(TAG, "close write sn");
#endif
}



void MenuUI::disp_wifi(bool bState, int disp_main)
{
    Log.d(TAG, "disp wifi bState %d disp_main %d",
          bState, disp_main);

    if (bState) {
        set_mainmenu_item(MAINMENU_WIFI, 1);
		
        if (check_allow_update_top()) {
            disp_icon(ICON_WIFI_OPEN_0_0_16_16);
        }
		
        if (cur_menu == MENU_TOP) {
            switch (disp_main) {
                case 0:
                    disp_icon(ICON_INDEX_IC_WIFIOPEN_NORMAL24_24);
                    break;
				
                case 1:
                    disp_icon(ICON_INDEX_IC_WIFIOPEN_LIGHT24_24);
                    break;
                default:
                    break;
            }
        }
    } else {
        set_mainmenu_item(MAINMENU_WIFI, 0);
		
        if (check_allow_update_top()) {
            disp_icon(ICON_WIFI_CLOSE_0_0_16_16);
        }
		
        if (cur_menu == MENU_TOP) {
            switch (disp_main) {
                case 0:
                    disp_icon(ICON_INDEX_IC_WIFICLOSE_NORMAL24_24);
                    break;

                case 1:
                    disp_icon(ICON_INDEX_IC_WIFICLOSE_LIGHT24_24);
                    break;

                default:
                    break;
            }
        }
    }
}



void MenuUI::handleWifiAction()
{
    Log.e(TAG, " handleWifiAction %d", mProCfg->get_val(KEY_WIFI_ON));
    
    const char* pWifiDrvProp = NULL;
    int iCmd = -1;
    bool bShowWifiIcon = false;
    int iSetVal = 0;

    sp<ARMessage> msg;
    sp<DEV_IP_INFO> tmpInfo;


    tmpInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

#if 1
    pWifiDrvProp = property_get(PROP_WIFI_DRV_EXIST);

    if (pWifiDrvProp == NULL || strcmp(pWifiDrvProp, "false") == 0) {
        Log.e(TAG, ">>>> Wifi driver not loaded, please load wifi deriver first!!!");
        return;
    } else {
#endif
        /*
         * 检查启动WIFI热点的情况（PROP_WIFI_AP_STATE） 
         */
        if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
            Log.e(TAG, "set KEY_WIFI_ON -> 0");
            iCmd = NETM_CLOSE_NETDEV;
            bShowWifiIcon = false;
            iSetVal = 0;
        } else {
            Log.e(TAG, "set KEY_WIFI_ON -> 1");
            iCmd = NETM_STARTUP_NETDEV;
            bShowWifiIcon = true;
            iSetVal = 1;
        }

        msg = (sp<ARMessage>)(new ARMessage(iCmd));
        msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
        NetManager::getNetManagerInstance()->postNetMessage(msg);

        msg_util::sleep_ms(500);

        Log.d(TAG, "Current wifi state [%s]", property_get(PROP_WIFI_AP_STATE));

        mProCfg->set_val(KEY_WIFI_ON, iSetVal);
        disp_wifi(bShowWifiIcon, 1);
    }	

}


int MenuUI::get_back_menu(int item)
{
    if (item >= 0 && (item < MENU_MAX)) {
        return mMenuInfos[item].back_menu;
    } else {
        Log.e(TAG, "get_back_menu item %d", item);
        return MENU_TOP;
    }
}

void MenuUI::set_back_menu(int item, int menu)
{
    if (menu == -1) {
        if (cur_menu == -1) {
            cur_menu = MENU_TOP;
            menu = MENU_TOP;
        } else {
            Log.e(TAG,"back menu is -1 cur_menu %d\n",cur_menu);

            #ifdef ENABLE_ABORT
			#else
            menu = cur_menu;
			#endif
        }
    }

//    Log.d(TAG, " set back menu item %d menu %d", item, menu);
    if (item > MENU_TOP && (item < MENU_MAX)) {
        {
            if (MENU_SYS_ERR == menu || menu == MENU_DISP_MSG_BOX || menu == MENU_LOW_BAT || menu == MENU_LIVE_REC_TIME) {
                 menu = get_back_menu(menu);
            }
        }
//        Log.d(TAG, "2 set back menu item %d menu %d", item, menu);
        if (item == menu) {
            Log.e(TAG,"same (%d %d)",item,menu);
            menu = get_back_menu(menu);
        }
//        Log.d(TAG, "3 set back menu item %d menu %d", item, menu);
        if (item != menu)  {
            Log.d(TAG,"set back (%d %d)",item,menu);
            mMenuInfos[item].back_menu = menu;
        }
    } else {
        Log.e(TAG, "set_back_menu item %d", item);
    }
}

void MenuUI::reset_last_info()
{
    tl_count = -1;
}




void MenuUI::procBackKeyEvent()
{
    INFO_MENU_STATE(cur_menu, cam_state)
		
    if (cur_menu == MENU_SYS_ERR ||
        cur_menu == MENU_SYS_DEV_INFO    ||
		cur_menu == MENU_DISP_MSG_BOX ||
		cur_menu == MENU_LOW_BAT ||
		cur_menu == MENU_LIVE_REC_TIME ||
		cur_menu == MENU_SET_PHOTO_DEALY /* || cur_menu == MENU_LOW_PROTECT*/)  {	/* add by skymixos */
        set_cur_menu_from_exit();

#ifdef ENABLE_MENU_STITCH_BOX        
    } else if (cur_menu == MENU_STITCH_BOX) {
        if (!bStiching) {
            set_cur_menu_from_exit();
            send_option_to_fifo(ACTION_SET_STICH);
        }
#endif        
    } else if (cur_menu == MENU_FORMAT_INDICATION) {
        rm_state(STATE_FORMAT_OVER);
        set_cur_menu_from_exit();
    } else {
		
        switch (cam_state) {
            //force back directly while state idle 17.06.08
            case STATE_IDLE:
                set_cur_menu_from_exit();
                break;
			
            case STATE_PREVIEW:     /* 预览状态下,按返回键 */
                switch (cur_menu) {
                    case MENU_PIC_INFO:
                    case MENU_VIDEO_INFO:
                    case MENU_LIVE_INFO:
                        if (send_option_to_fifo(ACTION_PREVIEW)) {
                            oled_disp_type(STOP_PREVIEWING);
                        } else {
                            Log.e(TAG, "stop preview fail");
                        }
                        break;
						
                     // preview state sent from http req while calibrating ,qr scan,gyro_start
                    case MENU_CALIBRATION:
                    case MENU_QR_SCAN:
                    case MENU_GYRO_START:
                    case MENU_NOSIE_SAMPLE:
                        setCurMenu(MENU_PIC_INFO);
                        break;
					
                    case MENU_PIC_SET_DEF:
                    case MENU_VIDEO_SET_DEF:
                    case MENU_LIVE_SET_DEF:
                    case MENU_SPEED_TEST:
#ifdef MENU_WIFI_CONNECT                    
                    case MENU_WIFI_CONNECT:
#endif
                        set_cur_menu_from_exit();
                        break;
					
                    default:
                        break;
                }
                break;
				
            default:
                switch (cur_menu) {
                    case MENU_QR_SCAN:
                        exit_qr_func();
                        break;
					
                    default:
                        Log.d(TAG, "strange enter (%d 0x%x)", cur_menu, cam_state);
                        if (check_live()) {
                            if (cur_menu != MENU_LIVE_INFO) {
                                setCurMenu(MENU_LIVE_INFO);
                            }
                        } else if (check_state_in(STATE_RECORD)) {
                            if (cur_menu != MENU_VIDEO_INFO) {
                                setCurMenu(MENU_VIDEO_INFO);
                            }
                        } else if (check_state_in(STATE_CALIBRATING)) {
                            if (cur_menu != MENU_CALIBRATION) {
                                setCurMenu(MENU_CALIBRATION);
                            }
                        } else if (check_state_in(STATE_SPEED_TEST)) {
                            if (cur_menu != MENU_SPEED_TEST) {
                                setCurMenu(MENU_SPEED_TEST);
                            }
                        } else if (check_state_in(STATE_START_GYRO)) {
                            if (cur_menu != MENU_GYRO_START) {
                                setCurMenu(MENU_GYRO_START);
                            }
                        } else if (check_state_in(STATE_NOISE_SAMPLE)) {
                            if (cur_menu != MENU_NOSIE_SAMPLE) {
                                setCurMenu(MENU_NOSIE_SAMPLE);
                            }
                        } else {
                            ERR_MENU_STATE(cur_menu, cam_state)
                        }
                        break;
                }
                break;
        }
    }
}


void MenuUI::disp_scroll()
{

}

#if 1
void MenuUI::update_menu_disp(const int *icon_light, const int *icon_normal)
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    int last_select = getCurMenuLastSelectIndex();
//    Log.d(TAG,"cur_menu %d last_select %d getCurMenuCurSelectIndex() %d "
//                  " mSelect->cur_page %d mSelect->page_max %d total %d",
//          cur_menu,last_select, getCurMenuCurSelectIndex(), mSelect->cur_page ,
//          mSelect->page_max,mSelect->total);
    if (icon_normal != nullptr) {
        //sometimes force last_select to -1 to avoid update last select
        if (last_select != -1) {
            disp_icon(icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    disp_icon(icon_light[getMenuSelectIndex(cur_menu)]);
}

void MenuUI::update_menu_disp(const ICON_INFO *icon_light,const ICON_INFO *icon_normal)
{
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    int last_select = getCurMenuLastSelectIndex();
	
    if (icon_normal != nullptr) {
        //sometimes force last_select to -1 to avoid update last select
        if (last_select != -1) {
            dispIconByLoc(&icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    dispIconByLoc(&icon_light[getMenuSelectIndex(cur_menu)]);

}
#endif



void MenuUI::set_mainmenu_item(int item,int val)
{
    switch (item) {
        case MAINMENU_WIFI:
            //off
//            Log.d(TAG,"set_mainmenu_item val %d",val);
            if (val == 0) {
                main_menu[0][item] = ICON_INDEX_IC_WIFICLOSE_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFICLOSE_LIGHT24_24;
            } else {
                main_menu[0][item] = ICON_INDEX_IC_WIFIOPEN_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFIOPEN_LIGHT24_24;
            }
            break;
    }
}



/********************************************************************************************
** 函数名称: updateInnerSetPage
** 函数功能: 设置页的页内更新
** 入口参数: 
**      setItemList - 设置项列表容器
**      bUpdateLast - 是否更新上次选择的项
** 返 回 值: 无
** 调    用: 
**
*********************************************************************************************/
void MenuUI::updateInnerSetPage(vector<struct stSetItem*>& setItemList, bool bUpdateLast)
{
    struct stSetItem* pTmpLastSetItem = NULL;
    struct stSetItem* pTmpCurSetItem = NULL;

    int iLastSelectIndex = getMenuLastSelectIndex(cur_menu);
    int iCurSelectedIndex = getMenuSelectIndex(cur_menu);
	
    Log.d(TAG, "[%s:%d] Last Selected index[%d], Current Index[%d]", __FILE__, __LINE__, iLastSelectIndex, iCurSelectedIndex);
    pTmpLastSetItem = setItemList.at(iLastSelectIndex);
    pTmpCurSetItem = setItemList.at(iCurSelectedIndex);

    if (pTmpLastSetItem && pTmpCurSetItem) {
        if (bUpdateLast) {
            dispSetItem(pTmpLastSetItem, false);
        }
        dispSetItem(pTmpCurSetItem, true);
    } else {
        Log.d(TAG, "[%s:%d] Invalid Last Selected index[%d], Current Index[%d]", iLastSelectIndex, iCurSelectedIndex);
    }
}



/********************************************************************************************
** 函数名称: updateSetItemCurVal
** 函数功能: 更新指定设置列表中指定设置项的当前值
** 入口参数: 
**      setItemList - 设置项列表容器
**      name - 需更新的设置项的名称
**      iSetVal - 更新的值
** 返 回 值: 无
** 调    用: 
**
*********************************************************************************************/
void MenuUI::updateSetItemCurVal(std::vector<struct stSetItem*>& setItemList, const char* name, int iSetVal)
{
    struct stSetItem* pTmpItem = NULL;
    bool bFound = false;

    Log.d(TAG, "[%s: %d] updateSetItemCurVal item name [%s], new val[%d]", __FILE__, __LINE__, name, iSetVal);

    for (u32 i = 0; i < setItemList.size(); i++) {
        pTmpItem = setItemList.at(i);
        if (pTmpItem && !strcmp(pTmpItem->pItemName, name)) {
            bFound = true;
            break;
        }
    }

    if (bFound) {
        if (iSetVal < 0 || iSetVal > pTmpItem->iItemMaxVal) {
            Log.e(TAG, "[%s:%d] invalid set val[%d], set itme[%s] curVal[%d], maxVal[%d]", __FILE__, __LINE__,
                    iSetVal, pTmpItem->pItemName, pTmpItem->iCurVal, pTmpItem->iItemMaxVal);
        } else {
            pTmpItem->iCurVal = iSetVal;
            Log.d(TAG, "[%s:%d] item [%s] current val[%d]", __FILE__, __LINE__, pTmpItem->pItemName, pTmpItem->iCurVal);
        }
    } else {
        Log.w(TAG, "[%s:%d] Can't find set item[%s], please check it ....", __FILE__, __LINE__, name);
    }
}


void MenuUI::update_sys_cfg(int item, int val)
{
    val = val & 0x00000001;
    Log.d(TAG," update_sys_cfg item %d val %d",item,val);
    switch(item) {
        case SET_FREQ:
            mProCfg->set_val(KEY_PAL_NTSC, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_FREQ, val);
            break;

        case SET_SPEAK_ON:
            mProCfg->set_val(KEY_SPEAKER, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_SPEAKER, val);
            break;

        case SET_BOTTOM_LOGO:
            mProCfg->set_val(KEY_SET_LOGO, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_BOOTMLOGO, val);
            break;

        case SET_LIGHT_ON:
            mProCfg->set_val(KEY_LIGHT_ON, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_LED, val);
            if (val == 1) {
                setLight();
            } else {
                setLightDirect(LIGHT_OFF);
            }
            break;

//        case SET_RESTORE:
//            if(check_state_equal(STATE_IDLE))
//            {
//                restore_all();
//            }
//            else
//            {
//                Log.e(TAG,"SET_RESTORE error cam_state 0x%x",cam_state);
//            }
//            break;

        case SET_SPATIAL_AUD:
            mProCfg->set_val(KEY_AUD_SPATIAL, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_SPAUDIO, val);
            break;

#if 0
        case SET_GYRO_ON:
            mProCfg->set_val(KEY_GYRO_ON, val);
            set_setting_select(item, val);
            break;
#endif

        case SET_FAN_ON:
            mProCfg->set_val(KEY_FAN, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_FAN, val);

            break;

        case SET_AUD_ON:
            mProCfg->set_val(KEY_AUD_ON, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_AUDIO, val);
            break;

        case SET_VIDEO_SEGMENT:
            mProCfg->set_val(KEY_VID_SEG, val);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_VIDSEG, val);
            break;

            SWITCH_DEF_ERROR(item)
    }
}


#ifdef ENABLE_MENU_STITCH_BOX
void MenuUI::disp_stitch_progress(sp<struct _stich_progress_> &mProgress)
{
    Log.d(TAG,"%s %d %d %d %d %f",
          __FUNCTION__,
          mProgress->total_cnt,
          mProgress->successful_cnt,
          mProgress->failing_cnt,
          mProgress->task_over,
          mProgress->runing_task_progress);

    if (cur_menu == MENU_STITCH_BOX) {
        char buf[32];
        int x ;
        int y;
        y = 24;
        if (mProgress->task_over) {
            bStiching = false;
            disp_icon(ICON_STITCHING_SUCCESS129_48);
            x = 17;
            if (mProgress->successful_cnt > 999) {
                x -= 6;
            }
            snprintf(buf,sizeof(buf),"%d",mProgress->successful_cnt);
            disp_str((const u8 *)buf,x,y);

            x = 24;
            y += 16;
            if (mProgress->failing_cnt > 999) {
                x -= 6;
            }
            snprintf(buf,sizeof(buf),"%d",mProgress->failing_cnt);
            disp_str((const u8 *)buf,x,y);
        } else {
            disp_icon(ICON_STITCHING_SCHEDULE129_48);
            x = 65;
            snprintf(buf,sizeof(buf),"%d/%d",(mProgress->successful_cnt + mProgress->failing_cnt + 1),mProgress->total_cnt);
            disp_str((const u8 *)buf,x,y);

            x = 16;
            y += 16;
            snprintf(buf,sizeof(buf),"(%.2f%%)",mProgress->runing_task_progress);
            disp_str((const u8 *)buf,x,y);

            bStiching = true;
        }
    } else {
        Log.e(TAG,"%s cur_menu %d %d", __FUNCTION__,cur_menu,bStiching);
    }
}
#endif



void MenuUI::set_sys_setting(sp<struct _sys_setting_> & mSysSetting)
{
    CHECK_NE(mSysSetting, nullptr);

#if 0
    Log.d(TAG, "%s %d %d %d %d %d %d %d %d %d %d", __FUNCTION__,
                                                    mSysSetting->flicker,
                                                    mSysSetting->speaker,
                                                    mSysSetting->led_on,
                                                    mSysSetting->fan_on,
                                                    mSysSetting->aud_on,
                                                    mSysSetting->aud_spatial,
                                                    mSysSetting->set_logo,
                                                    mSysSetting->gyro_on,
                                                    mSysSetting->video_fragment);

    {

        #if 1
        if (mSysSetting->flicker != -1) {
            update_sys_cfg(SET_FREQ, mSysSetting->flicker);
        }

        if (mSysSetting->speaker != -1) {
            update_sys_cfg(SET_SPEAK_ON, mSysSetting->speaker);
        }

        if (mSysSetting->aud_on != -1) {
            update_sys_cfg(SET_AUD_ON, mSysSetting->aud_on);
        }

        if (mSysSetting->aud_spatial != -1) {
            update_sys_cfg(SET_SPATIAL_AUD, mSysSetting->aud_spatial);
        }

        if (mSysSetting->fan_on != -1)  {
            update_sys_cfg(SET_FAN_ON, mSysSetting->fan_on);
        }

        if (mSysSetting->set_logo != -1) {
            update_sys_cfg(SET_BOTTOM_LOGO, mSysSetting->set_logo);
        }

        if (mSysSetting->led_on != -1) {
            update_sys_cfg(setLight_ON, mSysSetting->led_on);
        }

        if (mSysSetting->gyro_on != -1) {
            update_sys_cfg(SET_GYRO_ON, mSysSetting->gyro_on);
        }

        if (mSysSetting->video_fragment != -1)  {
            update_sys_cfg(SET_VIDEO_SEGMENT, mSysSetting->video_fragment);
        }
        #endif

        //update current menu
        if (cur_menu == MENU_SYS_SETTING) { /* 如果当前的菜单为设置菜单,重新进入设置菜单(以便更新各项) */
            setCurMenu(MENU_SYS_SETTING);
        }
    }
#endif

}



void MenuUI::add_qr_res(int type, sp<ACTION_INFO> &mAdd, int control_act)
{
    CHECK_NE(type,-1);
    CHECK_NE(mAdd, nullptr);
    int menu;
    int max;
    int key;

    sp<ACTION_INFO> mRes = sp<ACTION_INFO>(new ACTION_INFO());
    memcpy(mRes.get(), mAdd.get(), sizeof(ACTION_INFO));

    Log.d(TAG, "add_qr_res type (%d %d)", type, control_act);

    switch (control_act) {
        case ACTION_PIC:
        case ACTION_VIDEO:
        case ACTION_LIVE:
            mControlAct = mRes;     /* 得到自定义参数: ACTION_INFO */
            //force 0 to 10
            if (mControlAct->size_per_act == 0) {
                Log.d(TAG, "force control size_per_act is %d", mControlAct->size_per_act);
                mControlAct->size_per_act = 10;
            }
            break;

        /* 设置Customer模式的值 */
        case CONTROL_SET_CUSTOM:
            switch (type) {
                case ACTION_PIC:        /* 设置拍照模式的Customer */
                    key = KEY_ALL_PIC_DEF;
                    if (mRes->size_per_act == 0) {
                        Log.d(TAG, "[%s:%d] CONTROL_SET_CUSTOM pic size_per_act is %d", __FILE__, __LINE__, mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;

                case ACTION_VIDEO:      /* 设置录像模式下的Customer */
                    key = KEY_ALL_VIDEO_DEF;
                    if (mRes->size_per_act == 0) {
                        Log.d(TAG, "[%s: %d] CONTROL_SET_CUSTOM video size_per_act is %d", __FILE__, __LINE__, mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;

                case ACTION_LIVE:       /* 直播模式下的Customer */
                    key = KEY_ALL_LIVE_DEF;
                    fix_live_save_per_act(mRes.get());
                    break;

                SWITCH_DEF_ERROR(type);
            }
            mProCfg->set_def_info(key, -1, mRes);   /* 将Customer模式的参数设置保存到文件中 */
            break;

        /* 传递的是啥 ?????? */
        default: {
            /*  */
            //only save one firstly
            switch (type)  {
                case ACTION_PIC:
                    menu = MENU_PIC_SET_DEF;
                    key = KEY_ALL_PIC_DEF;
                    // max = PIC_CUSTOM;
                    if (mRes->size_per_act == 0) {
                        Log.d(TAG, "force qr pic size_per_act is %d", mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;

                case ACTION_VIDEO:
                    key = KEY_ALL_VIDEO_DEF;
                    menu = MENU_VIDEO_SET_DEF;
                    // max = VID_CUSTOM;
                    if (mRes->size_per_act == 0) {
                        Log.d(TAG,"force qr video size_per_act is %d",
                              mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;

                case ACTION_LIVE:
                    key = KEY_ALL_LIVE_DEF;
                    menu = MENU_LIVE_SET_DEF;
                    // max = LIVE_CUSTOM;
                    fix_live_save_per_act(mRes.get());
                    break;

                SWITCH_DEF_ERROR(type);
            }
            mProCfg->set_def_info(key, -1, mRes);      // max -> -1
            updateMenuCurPageAndSelect(menu, max);
        }
    }
}



void MenuUI::updateMenu()
{
    int item = getMenuSelectIndex(cur_menu);

    switch (cur_menu) {
        case MENU_TOP:
            update_menu_disp(main_menu[1], main_menu[0]);
            break;
		
        case MENU_SYS_SETTING:  /* 更新设置页的显示 */
            updateInnerSetPage(mSetItemsList, true);
            break;
		
		case MENU_SET_PHOTO_DEALY:
            updateInnerSetPage(mPhotoDelayList, true);
			break;

#ifdef ENABLE_MENU_AEB
        case MENU_SET_AEB:
            updateInnerSetPage(mAebList, true);
            break;    
#endif


        /* 更新: MENU_PIC_SET_DEF菜单的值(高亮显示) */
        case MENU_PIC_SET_DEF:
            Log.d(TAG, "[%s: %d] Update SET_PIC_DEF val[%d]", __FILE__, __LINE__, item);
            mProCfg->set_def_info(KEY_ALL_PIC_DEF, item);   /* 设置当前选中的项到mProcCfg */
            dispBottomInfo(true, true);
            break;
			

        case MENU_VIDEO_SET_DEF:   
            Log.d(TAG, "[%s: %d] Update SET_PIC_DEF val[%d]", __FILE__, __LINE__, item);
            mProCfg->set_def_info(KEY_ALL_VIDEO_DEF, item);   /* 设置当前选中的项到mProcCfg */
            dispBottomInfo(true, true);   
            break;
			

        case MENU_LIVE_SET_DEF:
            mProCfg->set_def_info(KEY_ALL_LIVE_DEF, item);

            // update_menu_disp(live_def_setting_menu[1]);

            #if 0
            if (item < LIVE_CUSTOM) {
                disp_org_rts((int)(mLiveAction[item].stOrgInfo.save_org != SAVE_OFF),0,
                             (int)(mLiveAction[item].stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            } else {
                disp_org_rts((int)(mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             /*mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stStiInfo.stStiAct.mStiL.file_save,*/0,
                             (int)(mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            }
            disp_live_ready();


            #else
            updateBottomMode(true);    /* 正常显示 */

            #endif

            break;
			
        case MENU_SYS_DEV_INFO:
//            update_sys_info();
            break;

        case MENU_STORAGE:
            updateInnerSetPage(mStorageList, true);
            break;
		
        case MENU_SHOW_SPACE:
            disp_storage_setting();
            break;

        case MENU_FORMAT:
            disp_format();
            break;
		
        default:
            Log.w(TAG, " Unkown join Update Menu [%d]", cur_menu);
            break;
    }
}

/*
 * 先按当前的
 */

void MenuUI::get_storage_info()
{
    int storage_index;

    memset(total_space, 0, sizeof(total_space));
    memset(used_space, 0, sizeof(used_space));

    Log.d(TAG, "get total mLocalStorageList.size() %d", mLocalStorageList.size());
    for (u32 i = 0; i < mLocalStorageList.size(); i++) {
        struct statfs diskInfo;
        u64 totalsize = 0;
        u64 used_size = 0;
        storage_index = get_dev_type_index(mLocalStorageList.at(i)->dev_type);

        if (access(mLocalStorageList.at(i)->path, F_OK) != -1) {
            statfs(mLocalStorageList.at(i)->path, &diskInfo);
            u64 blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
            totalsize = blocksize * diskInfo.f_blocks;   //总的字节数，f_blocks为block的数目
//            used_size = (diskInfo.f_blocks - diskInfo.f_bavail) * blocksize;   //可用空间大小
            used_size = (diskInfo.f_blocks - diskInfo.f_bfree) * blocksize;   //可用空间大小

//            Log.d(TAG,"f_blocks %lld f_bfree %lld avail %lld ",
//                  diskInfo.f_blocks,diskInfo.f_bfree,diskInfo.f_bavail);
            convert_space_to_str(totalsize,
                                 total_space[storage_index],
                                 sizeof(total_space[storage_index]));
            convert_space_to_str(used_size,
                                 used_space[storage_index],
                                 sizeof(used_space[storage_index]));

            Log.d(TAG," used %s total %s path %s",used_space[storage_index],total_space[storage_index],mLocalStorageList.at(i)->path);
        } else {
            Log.d(TAG, "%s not access", mLocalStorageList.at(i)->path);
        }
    }
}



bool MenuUI::localStorageAvail()
{
    bool ret = false;
    if (!checkHaveLocalSavePath()) {
        int times = 3;
        int i = 0;
        for (i = 0; i < times; i++) {
            struct statfs diskInfo;
            if (access(mLocalStorageList.at(mSavePathIndex)->path, F_OK) != -1) {

                statfs(mLocalStorageList.at(mSavePathIndex)->path, &diskInfo);

                /*
                 * 老化模式下为了能让时间尽可能长,虚拟出足够长的录像时间
                 */
				#ifdef ENABLE_AGEING_MODE
                mLocalStorageList.at(mSavePathIndex)->avail = 0x10000000000;
				#else
                mLocalStorageList.at(mSavePathIndex)->avail = diskInfo.f_bavail *diskInfo.f_bsize;
				#endif
				
                Log.d(TAG, "remain ( %s %llu)\n", mLocalStorageList.at(mSavePathIndex)->path, mLocalStorageList.at(mSavePathIndex)->avail);
                break;
            }  else {
                Log.d(TAG, "fail to access(%d) %s", i, mLocalStorageList.at(mSavePathIndex)->path);
                msg_util::sleep_ms(10);
            }
        }

        if (i == times) {
            Log.d(TAG, "still fail to access %s", mLocalStorageList.at(mSavePathIndex)->path);
                mLocalStorageList.at(mSavePathIndex)->avail = 0;
        }
        ret = true;
    }

    return ret;
}


void MenuUI::calcRemoteRemainSpace()
{
    u64 iTmpMinSize = ~0UL;
    for (u32 i = 0; i < mRemoteStorageList.size(); i++) {
        if (iTmpMinSize > mRemoteStorageList.at(i)->avail) {
            iTmpMinSize = mRemoteStorageList.at(i)->avail;
        }
    }
    mReoteRecLiveLeftSize = iTmpMinSize;
    Log.d(TAG, "[%s: %d] remote left space [%d]M", __FILE__, __LINE__, mReoteRecLiveLeftSize);

}


void MenuUI::calcRemainSpace()
{
    Log.d(TAG, "Calc Remian Space now ......");

    /* 计算出各种模式下的剩余容量:
     * 对于拍照: 计算出大卡的剩余容量 / 当前模式下没组照片的大小 - 将结果保存在mCanTakePicNum中
     * 对于录像: 首先计算出容量最小的小卡, 然后按照当前的码率
     */
    queryCurStorageState(2000);
    if (cur_menu == MENU_PIC_INFO || cur_menu == MENU_PIC_SET_DEF) {    /* 拍照模式下计算剩余空间: 只计算大卡的即可 */
        if (localStorageAvail()) {
            convSize2LeftNumTime(mLocalStorageList.at(mSavePathIndex)->avail);
        }
    } else {
        /* 计算出大卡和小卡的剩余空间 
         * 暂时不计算大卡的 get_cam_state()
         */
        // queryCurStorageState(2000);
        calcRemoteRemainSpace();
        convSize2LeftNumTime(mReoteRecLiveLeftSize << 20);


        #if 0
        if (cur_menu == MENU_VIDEO_SET_DEF || cur_menu == MENU_LIVE_SET_DEF) {
            if (mReoteRecLiveLeftSize > 0) {
                convSize2LeftNumTime(mReoteRecLiveLeftSize << 20);
            }
        } 
        #endif

    }
}



/********************************************************************************************
** 函数名称: convSize2LeftNumTime
** 函数功能: 计算剩余空间信息(可拍照片的张数,可录像的时长,可直播存储的时长)
** 入口参数: 存储容量的大小(单位为M)
** 返 回 值: 无(计算出的值来更新mRemainInfo对象)
** 调     用: calcRemainSpace
**
*********************************************************************************************/
void MenuUI::convSize2LeftNumTime(u64 size)
{
    INFO_MENU_STATE(cur_menu, cam_state);
    int size_M = (int)(size >> 20);
	
    if (size_M > AVAIL_SUBSTRACT) {
		
        int rest_sec = 0;
        int rest_min = 0;

        //deliberately minus 1024
        size_M -= AVAIL_SUBSTRACT;;
        int item = 0;
		
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if (mControlAct != nullptr) {
                    mCanTakePicNum = size_M / mControlAct->size_per_act;
                    Log.d(TAG, "0remain(%d %d)", size_M, mCanTakePicNum);
                } else {
					/*
                     * 使能RAW和没有使能RAW
                     */
                    item = getMenuSelectIndex(MENU_PIC_SET_DEF);
                    int iRawVal = mProCfg->get_val(KEY_RAW);

                    Log.d(TAG, "[%s: %d] Raw[%d]", __FILE__, __LINE__, iRawVal);
                    
                    struct stPicVideoCfg* pPicVidCfg = mPicAllItemsList.at(item);
                    int iUnitSize = 30;
                    if (pPicVidCfg) {
                        if (strcmp(pPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                            iUnitSize = pPicVidCfg->pStAction->size_per_act;
                            if (iRawVal) {
                                iUnitSize = iUnitSize * pPicVidCfg->iRawStorageRatio;
                            }
                            Log.d(TAG, "[%s:%d] One Group Picture size [%d]M", __FILE__, __LINE__, iUnitSize);

                        } else {
                            iUnitSize = mProCfg->get_def_info(KEY_ALL_PIC_DEF)->size_per_act;
                        }
                    } else {
                        Log.e(TAG, "[%s: %d] Invalid item[%d]", __FILE__, __LINE__, item);
                    }
                    
                    mCanTakePicNum = size_M / iUnitSize;
                }
                break;
				
            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF:
            #if 1
                //not cal while rec already started
                if (!(check_state_in(STATE_RECORD) 
                    /* && (mRecInfo->rec_hour > 0 || mRecInfo->rec_min > 0 || mRecInfo->rec_sec > 0) */)) {

                    Log.d(TAG, ">>>>>>>>>>>>>>>>>>calc video size");

                    if (mControlAct != nullptr) {
                        Log.d(TAG,"vid size_per_act %d", mControlAct->size_per_act);
                        rest_sec = size_M / mControlAct->size_per_act;
                    } else {
                        int index = getMenuSelectIndex(MENU_VIDEO_SET_DEF);

                        Log.d(TAG, "[%s: %d], >>>>>>>>>>>> item %d", __FILE__, __LINE__, item);
                        Log.d(TAG, ">>>>>>>>>>>>size_M = %d>>size = %d", size_M, mVidAllItemsList.size());
                       
                        struct stPicVideoCfg* pPicVidCfg = mVidAllItemsList.at(index);
                        if (pPicVidCfg) {

                            Log.d(TAG, "calc item name [%s], unit size [%d]", pPicVidCfg->pItemName, pPicVidCfg->pStAction->size_per_act);
                            
                            if (strcmp(pPicVidCfg->pItemName, TAKE_VID_MOD_CUSTOMER)) {
                                rest_sec = size_M / pPicVidCfg->pStAction->size_per_act;
                            } else {
                                Log.d(TAG, "3vid size_per_act %d",
                                    mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->size_per_act);
                                rest_sec = size_M / mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->size_per_act;
                            }

                            rest_min = rest_sec / 60;
                            mRemainInfo->remain_sec = rest_sec % 60;
                            mRemainInfo->remain_hour = rest_min / 60;
                            mRemainInfo->remain_min = rest_min % 60;
                            Log.d(TAG, "convSize2LeftNumTime ( %d %d %d  %d )", size_M, 
                                                        mRemainInfo->remain_hour,
                                                        mRemainInfo->remain_min, 
                                                        mRemainInfo->remain_sec);
                        } else {
                            Log.e(TAG, "[%s: %d] Invalid item[%d]", __FILE__, __LINE__, index);
                        }
                    }
                }
            #endif
                break;
        }
    } else {
        Log.d(TAG,"reset remainb");
        memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));
    }
}



void MenuUI::convert_space_to_str(u64 size, char *str, int len)
{
    double size_b = (double)size;
    double info_G = (size_b/1024/1024/1024);

    if (info_G >= 100.0) {
        snprintf(str,len,"%ldG",(int64_t)info_G);
    } else {
        snprintf(str,len,"%.1fG",info_G);
    }
}


/*
 * checkHaveLocalSavePath - 检查有本地存储设备是否存在
 * 存在返回true; 否则返回false
 */
bool MenuUI::checkHaveLocalSavePath()
{
    bool bRet = false;
    if (mSavePathIndex == -1) {    
        bRet = true;
    }
    return bRet;
}


bool MenuUI::check_save_path_usb()
{
    bool bRet = false;
	
    Log.d(TAG, "mSavePathIndex is %d", mSavePathIndex);
	
    if (mSavePathIndex != -1 && (get_dev_type_index(mLocalStorageList.at(mSavePathIndex)->dev_type) == SET_STORAGE_USB)) {
        bRet = true;
    } else {
        disp_msg_box(DISP_LIVE_REC_USB);
    }
    return bRet;
}



/*
 * dispBottomLeftSpace - 显示底部剩余空间
 * 如果本地设备或者小卡不存在，直接显示None
 */
void MenuUI::dispBottomLeftSpace()
{
    char disk_info[16] = {0};
    
    /*
     * 如果是拍照,本地存储设备在,远端的存储设备不在,仍显示剩余的张数
     * 如果是存录像和直播,必须本地和远端的设备都存在
     */
    if (cur_menu == MENU_PIC_INFO || cur_menu == MENU_PIC_SET_DEF) {
        if (checkHaveLocalSavePath()) {
            Log.d(TAG, "[%s: %d] Current menu[%s] have not local stroage device, show none", __FILE__, __LINE__, getCurMenuStr(cur_menu));
            disp_icon(ICON_LIVE_INFO_NONE_7848_50X16);
        } else {
            /* 如果系统处于启动预览状态下或者正在查询系统存储的状态下,显示"..." */
            INFO_MENU_STATE(cur_menu, cam_state)

            if (cam_state == STATE_START_PREVIEWING || cam_state == STATE_START_LIVING || cam_state == STATE_START_RECORDING) {
                /* 此时不显示剩余量 */
                Log.d(TAG, "[%s: %d] wait preview and calc left done....", __FILE__, __LINE__);
                clear_area(78, 48);
            } else {
                /* 拍照的话直接显示: mCanTakePicNum 的值 */
                snprintf(disk_info, sizeof(disk_info), "  %d", mCanTakePicNum);
                disp_str_fill((const u8 *) disk_info, 78, 48);
            }
        }
    } else {    /* 录像或拍照页面 */

        #if 1
        if (!checkHaveLocalSavePath() && 1 /* checkHaveRemoteSavePath()*/) {
            switch (cur_menu) {
			    /* 录像界面: */				
                case MENU_VIDEO_INFO:
                case MENU_VIDEO_SET_DEF:

                    if (cam_state == STATE_START_PREVIEWING || cam_state == STATE_START_RECORDING) {
                        /* 此时不显示剩余量 */
                        Log.d(TAG, "[%s: %d] wait preview and calc left done....", __FILE__, __LINE__);
                        clear_area(78, 48);
                    } else {
                        // if (tl_count == -1) {

                        //     if (check_rec_tl()) {
                        //         clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                        //     } else {
                                snprintf(disk_info, sizeof(disk_info), "%02d:%02d:%02d", mRemainInfo->remain_hour,
                                    mRemainInfo->remain_min, mRemainInfo->remain_sec);
                                disp_str_fill((const u8 *) disk_info, 78, 48);
                        //     }
                        // }
                    }
                    break;
            
                case MENU_LIVE_INFO:
                case MENU_LIVE_SET_DEF:

                    if (mControlAct != nullptr) {
                        if (mControlAct->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON) {
                            disp_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                        } else {
                            clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                        }
                    } else {

                        int iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);
                        bool bShowLefSpace = false;

                        struct stPicVideoCfg* pTmpCfg = mLiveAllItemsList.at(iIndex);
                        if (pTmpCfg) {
                            if (!strcmp(pTmpCfg->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {
                                if (check_live_save(pTmpCfg->pStAction)) {
                                    bShowLefSpace = true;
                                }
                            }
                        }

                        if (cam_state == STATE_START_PREVIEWING || cam_state == STATE_START_LIVING) {
                            /* 此时不显示剩余量 */
                            Log.d(TAG, "[%s: %d] wait preview and calc left done....", __FILE__, __LINE__);
                            clear_area(78, 48);
                        } else {
                            if (bShowLefSpace) {    /* 需要显示剩余空间 */
                                snprintf(disk_info, sizeof(disk_info), "%02d:%02d:%02d", 
                                                        mRemainInfo->remain_hour, 
                                                        mRemainInfo->remain_min, 
                                                        mRemainInfo->remain_sec);
                                disp_str_fill((const u8 *) disk_info, 78, 48);
                            } else {
                                clear_area(78, 48);
                            }
                        }
                    }
                    break;
            }
        } else {    /* 存储设备不存在 */
            
            /* 对于直播模式,需要根据当前是否需要存储来显示"None"或者不显示 */

            Log.d(TAG, "[%s:%d] Local or Remote Storage Device Not exist....", __FILE__, __LINE__);
            memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));
            
            bool bShowLefSpace = true;

            /* 对于直播模式 */
            if (cur_menu == MENU_LIVE_INFO || cur_menu == MENU_LIVE_SET_DEF) {
                int iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);

                struct stPicVideoCfg* pTmpCfg = mLiveAllItemsList.at(iIndex);
                if (pTmpCfg) {
                    if (!strcmp(pTmpCfg->pItemName, TAKE_LIVE_MODE_CUSTOMER)) {
                        if (check_live_save(pTmpCfg->pStAction)) {
                            bShowLefSpace = true;
                        } else {
                            bShowLefSpace = false;
                        }
                    } else {    /* 非Customer模式,不显示 */
                        bShowLefSpace = false;
                    }
                }             
            }
            
            if (bShowLefSpace) {
                disp_icon(ICON_LIVE_INFO_NONE_7848_50X16);
            } else {
                clear_area(78, 48);
            }
        }
        #endif
    }	
}


void MenuUI::updateBottomSpace(bool bNeedCalc)
{
    if (bNeedCalc == true) {
        calcRemainSpace();          /* 计算剩余空间 */
    }
    dispBottomLeftSpace();      /* 显示底部空间 */
}




/*************************************************************************
** 方法名称: dispBottomInfo
** 方法功能: 显示一个菜单的底部信息(包括底部的挡位及对应参数<如: RTS, Origin等>
** 入口参数: 
**		high  - 是否高亮显示底部的挡位
**      bTrueLeftSpace - 是否显示底部的剩余空间值
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::dispBottomInfo(bool high, bool bTrueLeftSpace)
{
    updateBottomMode(high);                  /* 更新底部的挡位及附加参数 */
    updateBottomSpace(bTrueLeftSpace);        /* 更新底部的剩余空间 */
}




/*************************************************************************
** 方法名称: getCurMenuSelectInfo
** 方法功能: 获取当前菜单的选择信息(当前菜单选择的是第几项等)
** 入口参数: 
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
struct _select_info_ * MenuUI::getCurMenuSelectInfo()
{
    return (SELECT_INFO *)&(mMenuInfos[cur_menu].mSelectInfo);
}



/*************************************************************************
** 方法名称: dispSettingPage
** 方法功能: 显示一个设置页面
** 入口参数: 
**		setItemsList  - 设置页元素列表
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::dispSettingPage(vector<struct stSetItem*>& setItemsList)
{
    int item = 0;
    bool iSelected = false;

    struct stSetItem* pTempSetItem = NULL;
    SELECT_INFO * mSelect = getCurMenuSelectInfo();
    const int iIndex = getMenuSelectIndex(cur_menu);    /* 选中项的索引值 */

    int start = mSelect->cur_page * mSelect->page_max;
    int end = start + mSelect->page_max;
    
    if (end > mSelect->total)
        end = mSelect->total;
    

    /* 重新显示正页时，清除整个页 */
    clear_area(32, 16);

	/* 5/3 = 1 8 (3, 6, 5) 5 = (1*3) + 2 */
    Log.d(TAG, "start %d end  %d select %d ", start, end, iIndex);

    while (start < end) {

        Log.d(TAG, "[%s:%d] dispSettingPage -> cur index [%d] in lists", __FILE__, __LINE__, start);
        pTempSetItem = setItemsList.at(start);
        
        if (pTempSetItem != NULL) {
            if (start < mSelect->total) {
                if (start == iIndex) {
                    iSelected = true;
                } else {
                    iSelected = false;
                }
                dispSetItem(pTempSetItem, iSelected);
            }
        } else {
            Log.e(TAG, "[%s:%d] dispSettingPage -> invalid index[%d]", start);
        }
        start++;
        item++;
    }

}


void MenuUI::reset_devmanager()
{
    Log.d(TAG,"reset_devmanager mLocalStorageList.size() %d",mLocalStorageList.size());
    bFirstDev = true;
    mDevManager->resetDev(0);
    exec_sh("pkill vold");
    msg_util::sleep_ms(45000);
    Log.d(TAG,"reset_devmanager suc");
}



int MenuUI::exec_sh_new(const char *buf)
{
    return exec_sh(buf);
}


//- 格式化exfat: mkexfat /dev/block/mmcblk1p1
//- 格式化ext4: mke2fs -t ext4 /dev/block/mmcblk1p1
//- Trim废弃block: fstrim /dev/block/mmcblk1p1
//mountufsd ${partition} ${path} -o uid=1023,gid=1023,fmask=0700,dmask=0700,force
void MenuUI::format(const char *src,const char *path,int trim_err_icon,int err_icon,int suc_icon)
{
    char buf[1024];
    int err_trim = 0;

    Log.d(TAG," %d src %s path %s cur_menu %d",
          getMenuSelectIndex(MENU_FORMAT),
          src,path,cur_menu);

    add_state(STATE_FORMATING);

    switch (getMenuSelectIndex(MENU_FORMAT)) {
        //exfat
        case 0:
            snprintf(buf, sizeof(buf), "umount -f %s", path);
            if (exec_sh_new((const char *)buf) != 0) {
                goto ERROR;
            }

            snprintf(buf,sizeof(buf),"mke2fs -F -t ext4 %s",src);
            if (exec_sh_new((const char *)buf) != 0) {
                err_trim = 1;
            }  else {
                snprintf(buf,sizeof(buf),"mount -t ext4 -o discard %s %s",src,path);
                //            snprintf(buf,sizeof(buf),"mountufsd %s %s -o uid=1023,gid=1023,fmask=0700,dmask=0700,force",src,path);
                if (exec_sh_new((const char *)buf) != 0) {
                    err_trim = 1;
                }  else {
                    snprintf(buf,sizeof(buf),"fstrim %s",path);
                    if (exec_sh_new((const char *)buf) != 0) {
                        err_trim = 1;
                    }

                    snprintf(buf,sizeof(buf),"umount -f %s",path);
                    if (exec_sh_new((const char *)buf) != 0) {
                        goto ERROR;
                    }
                  }
             }

            snprintf(buf,sizeof(buf),"mkexfat -f %s",src);
            if (exec_sh_new((const char *)buf) != 0) {
                goto ERROR;
            }
            Log.d(TAG,"err_trim %d",err_trim);
#ifdef NEW_FORMAT
            reset_devmanager();
#else
            snprintf(buf,sizeof(buf),"mountufsd %s %s "
                    "-o uid=1023,gid=1023,fmask=0700,dmask=0700,force",
                     src,path);
            if (exec_sh((const char *)buf) != 0) {
                goto ERROR;
            }  else {
                Log.d(TAG,"mountufsd suc");
            }
#endif

            if (err_trim) {
                goto ERROR_TRIM;
            }

            switch(getMenuSelectIndex(MENU_STORAGE)) {
                case SET_STORAGE_SD:
                    disp_icon(ICON_SDCARD_FORMATTED_SUCCESS128_48);
                    break;

                case SET_STORAGE_USB:
                    disp_icon(ICON_USBHARDDRIVE_FORMATTED_SUCCESS128_48);
                    break;
            }
            msg_util::sleep_ms(3000);
            break;

#ifndef ONLY_EXFAT
        //ext4
        case 1:
            snprintf(buf,sizeof(buf),"umount -f %s",path);
            if(exec_sh((const char *)buf) != 0)
            {
                goto ERROR;
            }

            snprintf(buf,sizeof(buf),"mke2fs -F -t ext4 %s",src);
            if(exec_sh((const char *)buf) != 0)
            {
                goto ERROR;
            }

            snprintf(buf,sizeof(buf),"mount -t ext4 -o discard %s %s",src,path);
//            snprintf(buf,sizeof(buf),"mountufsd %s %s -o uid=1023,gid=1023,fmask=0700,dmask=0700,force",src,path);
            if(exec_sh((const char *)buf) != 0)
            {
                goto ERROR;
            }

            snprintf(buf,sizeof(buf),"fstrim %s",path);
            if(exec_sh((const char *)buf) != 0)
            {
                goto ERROR_TRIM;
            }
            disp_icon(suc_icon);
            break;
#endif
        SWITCH_DEF_ERROR(getMenuSelectIndex(MENU_FORMAT));
    }

    return;
ERROR_TRIM:
    disp_icon(trim_err_icon);
    msg_util::sleep_ms(3000);
    return;
ERROR:
    disp_icon(err_icon);
    msg_util::sleep_ms(3000);
}

void MenuUI::start_format()
{
    bool bFound = false;
    switch (getMenuSelectIndex(MENU_SHOW_SPACE)) {
        case SET_STORAGE_SD:
            disp_icon(ICON_SDCARD_FORMATTING128_48);
            for(u32 i = 0; i < mLocalStorageList.size(); i++) {
                if(get_dev_type_index(mLocalStorageList.at(i)->dev_type) == SET_STORAGE_SD) {
                    format(mLocalStorageList.at(i)->src,mLocalStorageList.at(i)->path,ICON_FORMAT_REPLACE_NEW_SD_128_48128_48,ICON_SDCARD_FORMATTED_FAILED128_48,ICON_SDCARD_FORMATTED_SUCCESS128_48);
                    bFound = true;
                    break;
                }
            }
            break;

        case SET_STORAGE_USB:
            disp_icon(ICON_USBHARDDRIVE_FORMATTING128_48);
            for(u32 i = 0; i < mLocalStorageList.size(); i++)  {
                if (get_dev_type_index(mLocalStorageList.at(i)->dev_type) == SET_STORAGE_USB) {
                    bFound = true;
                    format(mLocalStorageList.at(i)->src,mLocalStorageList.at(i)->path,
                           ICON_FORMAT_REPLACE_NEW_USB_128_48128_48,
                           ICON_USBDARDDRIVE_FORMATTED_FAILED128_48,ICON_USBHARDDRIVE_FORMATTED_SUCCESS128_48);
                    break;
                }
            }
            break;
        SWITCH_DEF_ERROR(getMenuSelectIndex(MENU_STORAGE))
    }

    if (!bFound) {
        Log.e(TAG,"no format device found");
        abort();
    } else {
//        msg_util::sleep_ms(3000);
        rm_state(STATE_FORMATING);
        add_state(STATE_FORMAT_OVER);
//        setCurMenu(MENU_FORMAT);
    }
//    msg_util::sleep_ms(2000);
//    rm_state(STATE_FORMATING);
//    setCurMenu(MENU_FORMAT);
}

void MenuUI::disp_format()
{
    //force clear STATE_FORMAT_OVER
    rm_state(STATE_FORMAT_OVER);

#ifdef ONLY_EXFAT
    disp_icon(ICON_SD_FORMAT_EXFAT_LIGHT90_16);
#else
    //exfat high
    if (getMenuSelectIndex(MENU_FORMAT) == 0) {
        disp_icon(ICON_SD_FORMAT_EXFAT_LIGHT90_16);
        disp_icon(ICON_SD_FORMAT_EXT4_NORMAL90_16);
    } else {
        disp_icon(ICON_SD_FORMAT_EXFAT_NORMAL90_16);
        disp_icon(ICON_SD_FORMAT_EXT4_LIGHT90_16);
    }
#endif
}

void MenuUI::disp_storage_setting()
{
    char dev_space[128];

    rm_state(STATE_FORMAT_OVER);
	
    for (u32 i = 0; i < sizeof(used_space) / sizeof(used_space[0]); i++) {
        switch (i) {
            case SET_STORAGE_SD:
            case SET_STORAGE_USB:
                if (strlen(used_space[i]) == 0) {
                    snprintf(dev_space, sizeof(dev_space), "%s:None", dev_type[i]);
                } else {
                    snprintf(dev_space, sizeof(dev_space),"%s:%s/%s",
                             dev_type[i], used_space[i], total_space[i]);
                }

                Log.d(TAG,"disp storage[%d] select %d %s (%d %d %d)",
                      i, getMenuSelectIndex(MENU_SHOW_SPACE),
                      dev_space, storage_area[i].x, storage_area[i].y, storage_area[i].w);

                if (getMenuSelectIndex(MENU_SHOW_SPACE) == (int)i) {
                    disp_str((const u8 *)dev_space, storage_area[i].x, storage_area[i].y, true, storage_area[i].w);
                } else {
                    disp_str((const u8 *)dev_space, storage_area[i].x, storage_area[i].y, false, storage_area[i].w);
                }
                break;
            SWITCH_DEF_ERROR(i)
        }
    }
}

void MenuUI::disp_org_rts(sp<struct _action_info_> &mAct,int hdmi)
{
    disp_org_rts((int) (mAct->stOrgInfo.save_org != SAVE_OFF),
                 (int) (mAct->stStiInfo.stich_mode != STITCH_OFF), hdmi);
}


void MenuUI::disp_org_rts(int org, int rts, int hdmi)
{
    int new_org_rts = 0;

    if (org == 1) {
        if (rts == 1) {
            new_org_rts = 0;
        } else {
            new_org_rts = 1;
        }
    } else {
        if (rts == 1) {
            new_org_rts = 2;
        } else {
            new_org_rts = 3;
        }
    }

    {
        switch (new_org_rts) {
            case 0:
                disp_icon(ICON_INFO_ORIGIN32_16);
                disp_icon(ICON_INFO_RTS32_16);
                break;
            case 1:
                disp_icon(ICON_INFO_ORIGIN32_16);
                clear_icon(ICON_INFO_RTS32_16);
                break;
            case 2:
                clear_icon(ICON_INFO_ORIGIN32_16);
                disp_icon(ICON_INFO_RTS32_16);
                break;
            case 3:
                clear_icon(ICON_INFO_ORIGIN32_16);
                clear_icon(ICON_INFO_RTS32_16);
                break;
            SWITCH_DEF_ERROR(new_org_rts)
        }
    }

    if (hdmi == 0)  {
        clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    } else if (hdmi == 1) {
        disp_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    }
}


void MenuUI::disp_qr_res(bool high)
{
    if (high) {
        disp_icon(ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16);
    } else {
        disp_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
    }
}


void MenuUI::dispPicVidCfg(PicVideoCfg* pCfg, bool bLight)
{
	ICON_INFO iconInfo = {0};

    Log.d(TAG, "[%s: %d] dispPicVidCfg Current val[%d]", __FILE__, __LINE__, pCfg->iCurVal);

    iconInfo.x = pCfg->stPos.xPos;
    iconInfo.y = pCfg->stPos.yPos;
    iconInfo.w = pCfg->stPos.iWidth;
    iconInfo.h = pCfg->stPos.iHeight;

    if (pCfg->iCurVal > pCfg->iItemMaxVal) {
        Log.e(TAG, "[%s: %d] Invalid Current val[%d], Max val[%d]", __FILE__, __LINE__, pCfg->iCurVal, pCfg->iItemMaxVal);
        Log.e(TAG, "Can't show bottom Mode!!!!!");
    } else {
        if (bLight) {
            iconInfo.dat = pCfg->stLightIcon[pCfg->iCurVal];
        } else {
            iconInfo.dat = pCfg->stNorIcon[pCfg->iCurVal];
        }
        dispIconByLoc(&iconInfo);
    }

}


/*
 * 计算出最先录满卡的时间
 */

/*
 * 更新拍照，录像，直播，底部的模式(规格)
 * - 底部显示的挡位（以及对应右上角的参数<Origin/RTS>）
 * 
 */
void MenuUI::updateBottomMode(bool bLight)
{
    int iIndex;
    int icon = -1;

    struct stPicVideoCfg* pTmpCfg = NULL;
    ACTION_INFO* pTmpActionInfo = NULL;

    INFO_MENU_STATE(cur_menu, cam_state)

    switch (cur_menu) {
        case MENU_PIC_INFO:
        case MENU_PIC_SET_DEF:
            if (mControlAct != nullptr) {   /* 客户端有传递ACTION_INFO, 显示客户端传递的ACTION_INFO */
                disp_org_rts(mControlAct);
                clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);    /* 清除Customer图标 */
            } else {

                /* 根据当前选择的挡位进行显示 */
                iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);    /* 得到菜单的索引值 */

                Log.d(TAG, "[%s: %d] menu[%s] current selected index[%d]", __FILE__, __LINE__, getCurMenuStr(MENU_PIC_SET_DEF), iIndex);
                
                Log.d(TAG, "[%s: %d] menu[%s] total[%d] PIC cfg list len[%d]", __FILE__, __LINE__, 
                            getCurMenuStr(MENU_PIC_SET_DEF), mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total, mPicAllItemsList.size());

                if (iIndex >= mMenuInfos[MENU_PIC_SET_DEF].mSelectInfo.total || iIndex >= mPicAllItemsList.size()) {
                    Log.e(TAG, "[%s: %d] invalid index(%d) on current menu[%s]", __FILE__, __LINE__, iIndex, getCurMenuStr(cur_menu));
                } else {
                    pTmpCfg = mPicAllItemsList.at(iIndex);
                    if (pTmpCfg) {

                        Log.d(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);

                        cfgPicModeItemCurVal(pTmpCfg);  /* 更新拍照各项的当前值(根据设置系统的值，比如RAW, AEB) */

                        pTmpActionInfo = pTmpCfg->pStAction;
                        if (pTmpActionInfo) {
                            disp_org_rts((int) (pTmpActionInfo->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (pTmpActionInfo->stStiInfo.stich_mode != STITCH_OFF));

                        } else {
                            Log.e(TAG, "[%s:%d] Invalid pTmpActionInfo pointer", __FILE__, __LINE__);
                        }

                        dispPicVidCfg(pTmpCfg, bLight); /* 显示拍照的挡位 */
                    } else {
                        Log.e(TAG, "[%s: %d] invalid pointer pTmpCfg", __FILE__, __LINE__);
                    }
                }
            }
            break;


        /* Video/Live */
        case MENU_VIDEO_INFO:
        case MENU_VIDEO_SET_DEF:
            if (tl_count != -1) {   /* timelapse拍摄 */
                disp_org_rts(0, 0);
                clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
            } else if (mControlAct != nullptr) {   /* 来自客户端直播请求 */
                disp_org_rts(mControlAct);
                clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
            } else {

                /* 根据菜单中选择的挡位进行显示 */
                iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);

                Log.d(TAG, "[%s: %d] menu[%s] current selected index[%d]", __FILE__, __LINE__, getCurMenuStr(MENU_VIDEO_SET_DEF), iIndex);
                
                Log.d(TAG, "[%s: %d] menu[%s] total[%d] pic cfg list len[%d]", __FILE__, __LINE__, 
                            getCurMenuStr(MENU_VIDEO_SET_DEF), mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total, mVidAllItemsList.size());


                if (iIndex >= mMenuInfos[MENU_VIDEO_SET_DEF].mSelectInfo.total || iIndex >= mVidAllItemsList.size()) {
                    Log.e(TAG, "[%s: %d] invalid index(%d) on current menu[%s]", __FILE__, __LINE__, iIndex, getCurMenuStr(cur_menu));
                } else {

                    pTmpCfg = mVidAllItemsList.at(iIndex);
                    if (pTmpCfg) {

                        Log.d(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);

                        pTmpActionInfo = pTmpCfg->pStAction;
                        if (pTmpActionInfo) {
                            disp_org_rts((int) (pTmpActionInfo->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (pTmpActionInfo->stStiInfo.stich_mode != STITCH_OFF));
                        } else {
                            Log.e(TAG, "[%s:%d] Invalid pTmpActionInfo pointer", __FILE__, __LINE__);
                        }

                        dispPicVidCfg(pTmpCfg, bLight); /* 显示配置 */
                    } else {
                        Log.e(TAG, "[%s: %d] invalid pointer pTmpCfg", __FILE__, __LINE__);
                    }
                }
            }           
            break;



        case MENU_LIVE_INFO:
        case MENU_LIVE_SET_DEF:

            if (mControlAct != nullptr) {   /* 来自客户端直播请求 */

                disp_org_rts((int)(mControlAct->stOrgInfo.save_org != SAVE_OFF),
                            0 /* mControlAct->stStiInfo.stStiAct.mStiL.file_save*/,
                            (int)(mControlAct->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
                
                clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);

            } else {
                iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);

                Log.d(TAG, "[%s: %d] menu[%s] current selected index[%d]", __FILE__, __LINE__, getCurMenuStr(MENU_LIVE_SET_DEF), iIndex);
                
                Log.d(TAG, "[%s: %d] menu[%s] total[%d] pic cfg list len[%d]", __FILE__, __LINE__, 
                            getCurMenuStr(MENU_LIVE_SET_DEF), mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total, mLiveAllItemsList.size());


                if (iIndex >= mMenuInfos[MENU_LIVE_SET_DEF].mSelectInfo.total || iIndex >= mLiveAllItemsList.size()) {
                    Log.e(TAG, "[%s: %d] invalid index(%d) on current menu[%s]", __FILE__, __LINE__, iIndex, getCurMenuStr(cur_menu));
                } else {

                    pTmpCfg = mLiveAllItemsList.at(iIndex);
                    if (pTmpCfg) {

                        Log.d(TAG, "------->>> Current PicVidCfg name [%s]", pTmpCfg->pItemName);

                        pTmpActionInfo = pTmpCfg->pStAction;
                        if (pTmpActionInfo) {
                            disp_org_rts((int) (pTmpActionInfo->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (pTmpActionInfo->stStiInfo.stich_mode != STITCH_OFF));
                        } else {
                            Log.e(TAG, "[%s:%d] Invalid pTmpActionInfo pointer", __FILE__, __LINE__);
                        }

                        dispPicVidCfg(pTmpCfg, bLight); /* 显示配置 */
                    } else {
                        Log.e(TAG, "[%s: %d] invalid pointer pTmpCfg", __FILE__, __LINE__);
                    }
                }
            }
            break;
    }
}

#if 0

// highlight : 0 -- normal 1 -- high
void MenuUI::disp_cam_param(int higlight)
{
    int item;
    int icon = -1;
	ICON_INFO* pIconInfo = NULL;
	
    // Log.d(TAG,"disp_cam_param high %d cur_menu %d bDispControl %d", higlight,cur_menu);
    {
        INFO_MENU_STATE(cur_menu, cam_state)
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if (mControlAct != nullptr) {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else {
                    item = getMenuSelectIndex(MENU_PIC_SET_DEF);    /* 得到菜单的索引值 */

                    /* 找到对应的项 */

                    switch (item) {

#if 0						
//                        case PIC_8K_3D_SAVE:
                        case PIC_8K_PANO_SAVE:
                            //optical flow stich
                        case PIC_8K_3D_SAVE_OF:
                        case PIC_8K_PANO_SAVE_OF:
#ifdef OPEN_HDR_RTS
                        case PIC_HDR_RTS:
#endif
                        case PIC_HDR:
                        case PIC_BURST:
                        case PIC_RAW:
#else
						case PIC_ALL_8K_3D_OF:
						case PIC_ALL_8K_OF:
						case PIC_ALL_8K:	
					#ifdef ENABLE_SETTING_AEB	
						case PIC_ALL_AEB_3579:
					#endif
						case PIC_ALL_BURST:

#endif
                            //icon = pic_def_setting_menu[higlight][item];
							pIconInfo = &picAllCardMenuItems[higlight][item];
							
                            disp_org_rts((int) (mPICAction[item].stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mPICAction[item].stStiInfo.stich_mode != STITCH_OFF));
                            break;
							
                            //user define
                        case PIC_ALL_CUSTOM:
                            //icon = pic_def_setting_menu[higlight][item];
							pIconInfo = &picAllCardMenuItems[higlight][item];

							disp_org_rts((int) (mProCfg->get_def_info(KEY_ALL_PIC_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mProCfg->get_def_info(KEY_ALL_PIC_DEF)->stStiInfo.stich_mode !=
                                                STITCH_OFF));
                            break;
                        SWITCH_DEF_ERROR(item)
                    }
//                    Log.d(TAG, "2pic def item %d", item);
                }
                break;
				
            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF:
                Log.d(TAG,"tl_count is %d", tl_count);
                if (tl_count != -1) {
                    disp_org_rts(0, 0);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else if (mControlAct != nullptr) {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else {
                    item = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
//                    Log.d(TAG, "video def item %d", item);
                    if (item >= 0 && item < VID_CUSTOM) {
                        icon = vid_def_setting_menu[higlight][item];
                        disp_org_rts((int) (mVIDAction[item].stOrgInfo.save_org != SAVE_OFF),
                                     (int) (mVIDAction[item].stStiInfo.stich_mode != STITCH_OFF));
                    } else if (VID_CUSTOM == item) {
                        icon = vid_def_setting_menu[higlight][item];
                        disp_org_rts((int) (mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                     (int) (mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stStiInfo.stich_mode != STITCH_OFF));
                    } else {
                        ERR_ITEM(item);
                    }
                }
                break;
				
            case MENU_LIVE_INFO:
            case MENU_LIVE_SET_DEF:
                if (mControlAct != nullptr) {
                    disp_org_rts((int)(mControlAct->stOrgInfo.save_org != SAVE_OFF),
                                0/* mControlAct->stStiInfo.stStiAct.mStiL.file_save*/,(int)
                            (mControlAct->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else {
                    item = getMenuSelectIndex(MENU_LIVE_SET_DEF);
//                    Log.d(TAG, "live def item %d", item);
                    switch (item)
                    {
#ifdef LIVE_ORG
                        case LIVE_ORIGIN:
#endif
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
                        case VID_ARIAL:
                            icon = live_def_setting_menu[higlight][item];
                            disp_org_rts((int) (mLiveAction[item].stOrgInfo.save_org != SAVE_OFF), 0,
                                         (int) (mLiveAction[item].stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
                            break;
                        case LIVE_CUSTOM:
                            icon = live_def_setting_menu[higlight][item];
                            disp_org_rts((int) (mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                         0,/*mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stStiInfo.stStiAct.mStiL.file_save,*/
                                         (int) (mProCfg->get_def_info(KEY_ALL_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on ==
                                                HDMI_ON));
                            break;
                        SWITCH_DEF_ERROR(item)
                    }
                }
                break;
            SWITCH_DEF_ERROR(cur_menu)
        }
		
        if (icon != -1) {
//            Log.d(TAG, "disp icon %d ICON_CAMERA_INFO01_NORMAL_0_48_78_1678_16 %d",
//                  icon, ICON_CAMERA_INFO01_NORMAL_0_48_78_1678_16);
            disp_icon(icon);
        }

		if (pIconInfo) {
			dispIconByLoc(pIconInfo);
		}		
    }
}
#endif


void MenuUI::disp_sec(int sec,int x,int y)
{
    char buf[32];

    snprintf(buf,sizeof(buf), "%ds ", sec);
    disp_str((const u8 *)buf, x, y);
}

void MenuUI::disp_calibration_res(int type, int t)
{
    Log.d(TAG, " disp_calibration_res type %d", type);
	
    switch (type) {
        //suc
        case 0:
            clear_area();
            disp_icon(ICON_CALIBRATED_SUCCESSFULLY128_16);
            break;
			
        //fail
        case 1:
#if 0        
            cam_state |= STATE_CALIBRATE_FAIL;
            Log.d(TAG,"cal fail state 0x%x", cam_state);
            CHECK_EQ(cam_state, STATE_CALIBRATE_FAIL);
            disp_icon(ICON_CALIBRATION_FAILED128_16);
#endif
            break;

        //calibrating
        case 2:
            clear_area();
            disp_icon(ICON_CALIBRATING128_16);
            break;
			
        case 3: {
            disp_sec(t, 96, 32);
			break;
        }
            
        SWITCH_DEF_ERROR(type)
    }
}

bool MenuUI::menuHasStatusbar(int menu)
{
    bool ret= false;
    switch (menu) {
        case MENU_SYS_ERR:
        case MENU_DISP_MSG_BOX:
        case MENU_CALIBRATION:
        case MENU_QR_SCAN:
        case MENU_SPEED_TEST:

#ifdef MENU_WIFI_CONNECT
        case MENU_WIFI_CONNECT:
#endif  

        case MENU_RESET_INDICATION:
        case MENU_AGEING:
        case MENU_LOW_BAT:
        case MENU_LIVE_REC_TIME:
            ret = true;
            break;
    }
    return ret;
}

void MenuUI::disp_low_protect(bool bStart)
{
    clear_area();
    if (bStart) {
        disp_str((const u8 *)"Start Protect", 8, 16);
    } else {
        disp_str((const u8 *)"Protect...", 8, 16);
    }
}

void MenuUI::disp_low_bat()
{
    clear_area();
    disp_str((const u8 *)"Low Battery ",8,16);
    setLightDirect(BACK_RED|FRONT_RED);
}

void MenuUI::func_low_protect()
{
#if 0
    int times = 10;
    bool bSend = true;
    bool bCharge;
    int ret;

    for (int i = 0; i < times; i++) {
        disp_sec(times - i, 52, 48);
        msg_util::sleep_ms(1000);
        ret =get_battery_charging(&bCharge);
        Log.d(TAG,"prot (%d %d)",ret,bCharge);
        if ( ret == 0 && bCharge)
        {
            m_bat_info_->bCharge = bCharge;
            bSend = false;
            break;
        }

    }

    Log.d(TAG, "func_low_protect %d", bSend);
    
    if (bSend) {
        send_option_to_fifo(ACTION_LOW_PROTECT,REBOOT_SHUTDOWN);
        set_back_menu(MENU_LOW_PROTECT,MENU_TOP);
        disp_low_protect();
        cam_state = STATE_IDLE;
    } else {
//        //update bat icon
        oled_disp_battery();

        // back for battery charge
        procBackKeyEvent();
    }
    Log.d(TAG, "func_low_protect %d menu %d state 0x%x", bSend, cur_menu, cam_state);
#endif
}

bool MenuUI::isDevExist()
{
    int item = getMenuSelectIndex(MENU_SHOW_SPACE);
    bool bFound = false;

    for (u32 i = 0;i < mLocalStorageList.size(); i++) {
        if (get_dev_type_index(mLocalStorageList.at(i)->dev_type) == item) {
            bFound = true;
            break;
        }
    }
    Log.i(TAG, "isDevExist item %d bFound %d", item, bFound);
    return bFound;
}


/*************************************************************************
** 方法名称: enterMenu
** 方法功能: 进入菜单（要进入显示的菜单由cur_menu决定）
** 入口参数: 
**		dispBottom  - 是否显示底部信息
** 返 回 值: 无
** 调     用: 
** 
*************************************************************************/
void MenuUI::enterMenu(bool dispBottom)
{
	Log.d(TAG, "enterMenu is [%s]", getCurMenuStr(cur_menu));

    switch (cur_menu) {
        case MENU_TOP:      /* 主菜单 */
            disp_icon(main_icons[mProCfg->get_val(KEY_WIFI_ON)][getCurMenuCurSelectIndex()]);
            break;
		
        case MENU_PIC_INFO: /* 拍照菜单 */
			
            disp_icon(ICON_CAMERA_ICON_0_16_20_32);		/* 显示左侧'拍照'图标 */

            // if (dispBottom) {	                 /* 显示底部和右侧信息 */
                dispBottomInfo(false, false);    /* 正常显示底部规格,不更新剩余空间 */
            // }

            if (check_state_preview()) {	/* 启动预览成功,显示"Ready" */
                disp_ready_icon();
            } else if (check_state_equal(STATE_START_PREVIEWING) || check_state_in(STATE_STOP_PREVIEWING)) {
                disp_waiting();				/* 正在启动,显示"..." */
            } else if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {	/* 正在拍照 */
                if (mTakePicDelay == 0) {   /* 倒计时为0,显示"shooting" */
                    disp_shooting();
                } else {                    /* 清除就绪图标,等待下一次更新消息 */
                    clear_ready();
                }
            } else if (check_state_in(STATE_PIC_STITCHING)) {	/* 如果正在拼接,显示"processing" */
                disp_processing();
            } else {
                Log.d(TAG, "pic menu error state 0x%x", cam_state);
                if (check_state_equal(STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;
			

        case MENU_VIDEO_INFO:   /* 录像菜单 */

            disp_icon(ICON_VIDEO_ICON_0_16_20_32);

            dispBottomInfo(false, false);

            if (check_state_preview()) {    /* 此时处于预览状态,显示就绪 */
                disp_ready_icon();  /* 有存储条件显示就绪,否则返回NO_SD_CARD */
            } else if (check_state_equal(STATE_START_PREVIEWING) || (check_state_in(STATE_STOP_PREVIEWING)) || check_state_in(STATE_START_RECORDING)) {
                disp_waiting();
            } else if (check_state_in(STATE_RECORD)) {
                Log.d(TAG, "do nothing in rec cam state 0x%x", cam_state);
                if (tl_count != -1) {
                    clear_ready();
                }
            } else {
                Log.d(TAG, "vid menu error state 0x%x menu %d", cam_state, cur_menu);
                if (check_state_equal(STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;


        /* 进入直播页面 */	
        case MENU_LIVE_INFO:

            INFO_MENU_STATE(cur_menu, cam_state);

            disp_icon(ICON_LIVE_ICON_0_16_20_32);

            dispBottomInfo(false, false);   /* 正常显示规格,不显示剩余时长 */

            if (check_state_preview()) {
                disp_live_ready();
            } else if (check_state_equal(STATE_START_PREVIEWING) || check_state_in(STATE_STOP_PREVIEWING) || check_state_in(STATE_START_LIVING)) {
                disp_waiting();
            } else if (check_state_in(STATE_LIVE)) {
                Log.d(TAG, "do nothing in live cam state 0x%x", cam_state);
            } else if (check_state_in(STATE_LIVE_CONNECTING)) {
                disp_connecting();
            } else {
                Log.d(TAG, "live menu error state 0x%x", cam_state);
                if (check_state_equal(STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;


        case MENU_STORAGE:      /* 存储菜单 */
            clear_area(0, 16);
            disp_icon(ICON_STORAGE_IC_DEFAULT_0016_25X48);

            /* 进入存储页 */
            dispSettingPage(mStorageList);					/* 显示"右侧"的项 */
            break;

        case MENU_SHOW_SPACE:   /* 显示存储设备菜单 */
            clear_area(0, 16);
            disp_icon(ICON_STORAGE_IC_DEFAULT_0016_25X48);
            
            /* 发送查询卡容量的命令,查询小卡 */
            get_storage_info();

            disp_storage_setting();
            break;


		case MENU_SET_PHOTO_DEALY:  /* 显示PhotoDelay菜单 */
            Log.d(TAG, "[%s:%d] +++++>>> enter Set Photo Delay Menu now .... ", __FILE__, __LINE__);

			clear_area(0, 16);									/* 清除真个区域 */

            dispIconByLoc(&setPhotoDelayNv);

            /* 进入设置"Set Photo Delay" */
            dispSettingPage(mPhotoDelayList);					/* 显示"右侧"的项 */
			break;

#ifdef ENABLE_MENU_AEB	
        case MENU_SET_AEB:
            Log.d(TAG, "[%s:%d] +++++>>> enter aeb Menu now .... ", __FILE__, __LINE__);
            clear_area(0, 16);	
            dispIconByLoc(&setAebs);
            dispSettingPage(mAebList);
            break;
#endif

		
        case MENU_SYS_SETTING:      /* 显示"设置菜单"" */
            clear_area(0, 16);
            disp_icon(ICON_SET_IC_DEFAULT_25_48_0_1625_48);         /* 显示左侧的"设置图标" */

            /* 显示设置页 */
            dispSettingPage(mSetItemsList);
            break;
			

        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF:
            updateBottomMode(true); /* 从MENU_PIC_INFO/MENU_VIDE_INFO/MENU_LIVE_INFO进入到MENU_PIC_SET_DEF/MENU_VIDEO_SET_DEF/MENU_LIVE_SET_DEF只需高亮显示挡位即可 */
            break;
		

        case MENU_QR_SCAN:
            clear_area();
//          reset_last_info();
//          Log.d(TAG,"disp MENU_QR_SCAN state is 0x%x",cam_state);
            INFO_MENU_STATE(cur_menu,cam_state)
            if(check_state_in(STATE_START_QRING) || check_state_in(STATE_STOP_QRING)) {
                disp_waiting();
            } else if (check_state_in(STATE_START_QR)) {
                disp_icon(ICON_QR_SCAN128_64);
            } else {
                if (check_state_equal(STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;

			
        case MENU_CALIBRATION:
            Log.d(TAG, "MENU_CALIBRATION GyroCalc delay %d", mGyroCalcDelay);
            if (mGyroCalcDelay > 0) {
                disp_icon(ICON_CALIBRAT_AWAY128_16);
            } else {
                //disp_calibration_res(2);	/* 显示正在校正"gyro caclibrating..." */
            }
            break;

            //menu sys device
//            case MENU_SYS_DEV:
//                disp_sys_dev();
//                break;

        case MENU_SYS_DEV_INFO:     /* 显示设备的信息 */
            dispSysInfo();
            break;


        case MENU_SYS_ERR:          /* 显示系统错误菜单 */
            setLightDirect(BACK_RED|FRONT_RED);
            disp_icon(ICON_ERROR_128_64128_64);
            break;
			
        case MENU_DISP_MSG_BOX:
            break;
			
        case MENU_LOW_BAT:
//            disp_icon(ICON_LOW_BAT_128_64128_64);
            disp_low_bat();
            break;

#ifdef ENABLE_MENU_LOW_PROTECT	
       case MENU_LOW_PROTECT:
           disp_low_protect(true);
           break;
#endif

        case MENU_GYRO_START:
            if (check_state_equal(STATE_START_GYRO)) {
                disp_icon(ICON_GYRO_CALIBRATING128_48);
            } else {
                disp_icon(ICON_HORIZONTALLY01128_48);
            }
            break;

        case MENU_SPEED_TEST: {
            if (!checkHaveLocalSavePath()) {
                int dev_index = get_dev_type_index(mLocalStorageList.at(mSavePathIndex)->dev_type);

                if (check_state_in(STATE_SPEED_TEST)) {
                    switch (dev_index) {
                        case SET_STORAGE_SD:
                            disp_icon(ICON_SPEEDTEST03128_64);
                            break;

                        case SET_STORAGE_USB:
                            disp_icon(ICON_SPEEDTEST04128_64);
                            break;

                        default:
                            Log.d(TAG,"STATE_SPEED_TEST mSavePathIndex is -1");

                            disp_icon(ICON_SPEEDTEST03128_64);
                            break;
                    }
                } else {
                    switch (dev_index) {
                        case SET_STORAGE_SD:
                            disp_icon(ICON_SPEEDTEST01128_64);
                            break;

                        case SET_STORAGE_USB:
                            disp_icon(ICON_SPEEDTEST02128_64);
                            break;

                        default:
                            Log.d(TAG,"mSavePathIndex is -1");
                            disp_icon(ICON_SPEEDTEST01128_64);
                            break;
                    }
                }
            } else {
                Log.d(TAG,"card remove while speed test cam_state 0x%x",cam_state);
                procBackKeyEvent();
            }
        }
            break;

        case MENU_RESET_INDICATION:
            aging_times = 0;
            disp_icon(ICON_RESET_IDICATION_128_48128_48);
            break;

        case MENU_FORMAT_INDICATION:
            Log.d(TAG,"cam state 0x%x",cam_state);
            if (isDevExist()) {
                disp_icon(ICON_FORMAT_MSG_128_48128_48);
            } else {
                setCurMenu(MENU_SHOW_SPACE);
            }
            break;

#ifdef ENABE_MENU_WIFI_CONNECT
        case MENU_WIFI_CONNECT:
            disp_wifi_connect();
            break;
#endif

        case MENU_AGEING:
            disp_ageing();
            break;

        case MENU_NOSIE_SAMPLE:
            disp_icon(ICON_SAMPLING_128_48128_48);
            break;

        case MENU_LIVE_REC_TIME:
            break;

#ifdef ENABLE_MENU_STITCH_BOX
        case MENU_STITCH_BOX:
            bStiching = false;
            disp_icon(ICON_WAITING_STITCH_128_48128_48);
            break;
#endif

        case MENU_FORMAT: {
            int item = getMenuSelectIndex(MENU_STORAGE);
            if (isDevExist()) {
                switch(item) {
                    case SET_STORAGE_SD:
                        disp_icon(ICON_SD_FORMAT_IC_DEFAULT_38_48_0_1638_48);
                        break;
                    case SET_STORAGE_USB:
                        disp_icon(ICON_USB_FORMAT_IC_DEFAULT_38_48_0_1638_48);
                        break;
                    SWITCH_DEF_ERROR(select)
                }
                disp_format();
#ifdef ONLY_EXFAT
                clear_area(38,32,90,32);
#else
                clear_area(38,48,90,16);
#endif
            } else {
                rm_state(STATE_FORMAT_OVER);
                setCurMenu(MENU_STORAGE);
            }
        }
        break;

        SWITCH_DEF_ERROR(cur_menu);
    }

    if (menuHasStatusbar(cur_menu)) {
        reset_last_info();
        bDispTop = false;
    } else if (!bDispTop)  {
        disp_top_info();
    }
}


bool MenuUI::check_dev_exist(int action)
{
    bool bRet = false;

    Log.d(TAG, "check_dev_exist (%d %d)", mLocalStorageList.size(), mSavePathIndex);

    if (!checkHaveLocalSavePath()) {
        switch (action) {
//            case ACTION_CALIBRATION:
            case ACTION_PIC:
                if (mCanTakePicNum > 0) {
                    bRet = true;
                } else {
                    //disp full
                    Log.e(TAG, "pic dev full");
                    disp_msg_box(DISP_DISK_FULL);
                }
                break;
				
            case ACTION_VIDEO:
                if (mRemainInfo->remain_hour > 0  || mRemainInfo->remain_min > 0 || mRemainInfo->remain_sec > 0) {
                    bRet = true;
                } else {
                    //disp full
                    Log.e(TAG,"video dev full");
                    disp_msg_box(DISP_DISK_FULL);
                }
                break;

            case ACTION_LIVE:
            default:
                bRet = true;
                break;
        }
    }
    return bRet;
}



void MenuUI::handleGyroCalcEvent()
{
	Log.d(TAG, ">>>> handleGyroCalcEvent START");
    #if 0
	if ((cam_state & STATE_CALIBRATING) != STATE_CALIBRATING) {
		setGyroCalcDelay(5);
		oled_disp_type(START_CALIBRATIONING);
	} else {
		Log.e(TAG, "handleGyroCalcEvent: calibration happen cam_state 0x%x", cam_state);
	}
    #else 
    mCalibrateSrc = true;   /* 表示发起拼接动作来自UI */
    oled_disp_type(START_CALIBRATIONING);
    #endif
}



bool MenuUI::switchEtherIpMode(int iMode)
{
    sp<ARMessage> msg;
    sp<DEV_IP_INFO> tmpInfo;

    tmpInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    msg = (sp<ARMessage>)(new ARMessage(NETM_SET_NETDEV_IP));

    strcpy(tmpInfo->cDevName, ETH0_NAME);
    strcpy(tmpInfo->ipAddr, DEFAULT_ETH0_IP);
    tmpInfo->iDevType = DEV_LAN;

    if (iMode) {    /* DHCP */
        tmpInfo->iDhcp = 1;
    } else {    /* Static */
        tmpInfo->iDhcp = 0;
    }

    msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
    NetManager::getNetManagerInstance()->postNetMessage(msg);

    return true;

}


void MenuUI::procSetMenuKeyEvent()
{
    int iVal = 0;
    int iItemIndex = getMenuSelectIndex(cur_menu);    /* 得到选中的索引 */
    struct stSetItem* pCurItem = NULL;
    vector<struct stSetItem*>* pVectorList = static_cast<vector<struct stSetItem*>*>(mMenuInfos[cur_menu].privList);


    if ((pVectorList == NULL) && (iItemIndex < 0 || iItemIndex > mMenuInfos[cur_menu].mSelectInfo.total)) {
        Log.e(TAG, "[%s:%d] Invalid index val[%d] in menu[%s]", iItemIndex, getCurMenuStr(cur_menu));
    } else {
        /* 得到该项的当前值 */
        pCurItem = (*pVectorList).at(iItemIndex);
        iVal = pCurItem->iCurVal;
        Log.d(TAG, "Current selected item name [%s], cur val[%d]", pCurItem->pItemName, iVal);

        if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_DHCP)) {
            iVal = ((~iVal) & 0x00000001);
            if (switchEtherIpMode(iVal)) {
                mProCfg->set_val(KEY_DHCP, iVal);    /* 写回配置文件 */
                pCurItem->iCurVal = iVal;        
                dispSetItem(pCurItem, true);
            }

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FREQ)) {
            iVal = ((~iVal) & 0x00000001);
            mProCfg->set_val(KEY_PAL_NTSC, iVal);
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_HDR)) {   /* 开启HDR */
            /*
             * TODO: 开启HDR效果
             */
            iVal = ((~iVal) & 0x00000001);
            mProCfg->set_val(KEY_HDR, iVal);
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);
            //send_option_to_fifo(ACTION_SET_OPTION, OPTION_HDR);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_RAW)) {

            iVal = ((~iVal) & 0x00000001);
            mProCfg->set_val(KEY_RAW, iVal);
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);

#ifdef ENABLE_MENU_AEB            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_AEB)) {       /* AEB -> 点击确认将进入选择子菜单中 */
            setCurMenu(MENU_SET_AEB);
#endif            

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_PHDEALY)) {   /* PhotoDelay -> 进入MENU_PHOTO_DELAY子菜单 */
            setCurMenu(MENU_SET_PHOTO_DEALY);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_SPEAKER)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_SPEAKER, iVal);
            dispSetItem(pCurItem, true);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_LED)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_LIGHT_ON, iVal);
            if (iVal == 1) {
                setLight();
            } else {
                setLightDirect(LIGHT_OFF);
            }
            dispSetItem(pCurItem, true);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_AUDIO)) {
            iVal = ((~iVal) & 0x00000001);
            mProCfg->set_val(KEY_AUD_ON, iVal);
            pCurItem->iCurVal = iVal;
            dispSetItem(pCurItem, true);                    
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_SPAUDIO)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_AUD_SPATIAL, iVal);
            dispSetItem(pCurItem, true);    
            if (mProCfg->get_val(KEY_AUD_ON) == 1) {
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
            }                
            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FLOWSTATE)) { /* TODO */
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_FLOWSTATE, iVal);
            dispSetItem(pCurItem, true);    
            //send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_GYRO_CALC)) { /* TODO */
            setCurMenu(MENU_GYRO_START);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_FAN)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_FAN, iVal);
            if (iVal == 0) {
                disp_msg_box(DISP_ALERT_FAN_OFF);
            } else {
                dispSetItem(pCurItem, true);  
            }
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_FAN);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_NOISESAM)) {
            send_option_to_fifo(ACTION_NOISE);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_BOOTMLOGO)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_SET_LOGO, iVal);
            dispSetItem(pCurItem, true); 
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_VIDSEG)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_VID_SEG, iVal);
            if (iVal == 0) {
                disp_msg_box(DISP_VID_SEGMENT);
            } else {
                dispSetItem(pCurItem, true); 
            }
           send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_VID_SEG);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_STORAGE)) {   /* 选择的是Storage设置项,将进入MENU_STORAGE菜单 */
            setCurMenu(MENU_STORAGE);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_INFO)) {      /* 读取系统INFO */
            setCurMenu(MENU_SYS_DEV_INFO);

        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_RESET)) {
            setCurMenu(MENU_RESET_INDICATION);
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_GYRO_ONOFF)) {
            iVal = ((~iVal) & 0x00000001);
            pCurItem->iCurVal = iVal;
            mProCfg->set_val(KEY_GYRO_ON, iVal);
            dispSetItem(pCurItem, true); 
            send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);            
        } else if (!strcmp(pCurItem->pItemName, SET_ITEM_NAME_STITCH_BOX)) {
        #ifdef ENABLE_FEATURE_STICH_BOX
            setCurMenu(MENU_STITCH_BOX);
            send_option_to_fifo(ACTION_SET_STICH);
        #endif           
        }
        
    }
}



/*************************************************************************
** 方法名称: procPowerKeyEvent
** 方法功能: POWER/OK键按下的处理
** 入口参数: 
**		无
** 返回值: 无
** 调 用: 
** 
*************************************************************************/
void MenuUI::procPowerKeyEvent()
{
	Log.d(TAG, "cur_menu %d select %d\n", cur_menu, getCurMenuCurSelectIndex());
    int iIndex = 0;

	/* 根据当前的菜单做出不同的处理 */
    switch (cur_menu) {	/* 根据当前的菜单做出响应的处理 */
        case MENU_TOP:	/* 顶层菜单 */

            switch (getCurMenuCurSelectIndex()) {	/* 获取当前选择的菜单项 */
                case MAINMENU_PIC:	/* 选择的是"拍照"项 */
                    /* 发送预览请求 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {	/* 发送预览请求: 消息发送完成后需要接收到异步结果 */
                        oled_disp_type(START_PREVIEWING);	/* 屏幕中间会显示"..." */
                        setCurMenu(MENU_PIC_INFO);	/* 设置并显示当前菜单 */
                    } else {
                        Log.d(TAG, "pic preview fail?");
                    }
                    break;
					
                case MAINMENU_VIDEO:	/* 选择的是"录像"项 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {	/* 发送预览请求 */
                        oled_disp_type(START_PREVIEWING);
                        setCurMenu(MENU_VIDEO_INFO);
                    } else {
                        Log.d(TAG, "vid preview fail?");
                    }
                    break;
					
                case MAINMENU_LIVE:		/* 选择的是"Living"项 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {
                        oled_disp_type(START_PREVIEWING);
                        setCurMenu(MENU_LIVE_INFO);
                    } else {
                        Log.d(TAG, "live preview fail?");
                    }
                    break;


                case MAINMENU_WIFI:     		/* WiFi菜单项用于打开关闭AP */
                    handleWifiAction();
                    break;
				
                case MAINMENU_CALIBRATION:		/* 拼接校准 */
					handleGyroCalcEvent();
                    break;
				
                case MAINMENU_SETTING:          /* 设置键按下，进入“设置” */
                    setCurMenu(MENU_SYS_SETTING);
                    break;
				
                default:
                    break;
            }
            break;


        case MENU_PIC_INFO:		/* 拍照子菜单 */

            /* 检查存储设备是否存在，是否有剩余空间来拍照 */
            if (check_dev_exist(ACTION_PIC)) {
                // only happen in preview in oled panel, taking pic in live or rec only actvied by controller client
                if (check_allow_pic()) {    /* 检查当前状态是否允许拍照 */
                    oled_disp_type(CAPTURE);
                }
            } 

            //send_option_to_fifo(ACTION_PIC);	/* 发送拍照请求 */
            break;
		
        case MENU_VIDEO_INFO:	/* 录像子菜单 */
            send_option_to_fifo(ACTION_VIDEO);  /* 录像/停止录像 */
            break;
		
        case MENU_LIVE_INFO:	/* 直播子菜单 */
            send_option_to_fifo(ACTION_LIVE);
            break;

		case MENU_SET_PHOTO_DEALY: 
            /* 获取MENU_SET_PHOTO_DELAY的Select_info.select的全局索引值,用该值来更新 */
            iIndex = getMenuSelectIndex(MENU_SET_PHOTO_DEALY);

            Log.d(TAG, "[%s: %d] set photo delay index[%d]", __FILE__, __LINE__, iIndex);
            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_PHDEALY, iIndex);

            mProCfg->set_val(KEY_PH_DELAY, iIndex);
            procBackKeyEvent();
			break;

#ifdef ENABLE_MENU_AEB	

        case MENU_SET_AEB:
            /* 获取MENU_SET_PHOTO_DELAY的Select_info.select的全局索引值,用该值来更新 */
            iIndex = getMenuSelectIndex(MENU_SET_AEB);
            // int iEvArry[] = {-125, 125, -62, 62, -31, 31, -15, 15};
            Log.d(TAG, "[%s: %d] set aeb index[%d]", __FILE__, __LINE__, iIndex);

            updateSetItemCurVal(mSetItemsList, SET_ITEM_NAME_AEB, iIndex);

            /* 更新对应的EV值 */
            mProCfg->set_val(KEY_AEB, iIndex);
            
            procBackKeyEvent();
            break;
#endif 
		
        case MENU_SYS_SETTING:      /* "设置"菜单按下Power键的处理 */
            procSetMenuKeyEvent();     
            break;


//        case MENU_CALIBRATION_SETTING:
//            power_menu_cal_setting();
//            break;

        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF:
            procBackKeyEvent();
            break;

        case MENU_GYRO_START:
            if (check_state_equal(STATE_IDLE)) {
                send_option_to_fifo(ACTION_GYRO);
            } else {
                Log.d(TAG, "gyro start cam_state 0x%x", cam_state);
            }
            break;
			
        case MENU_SPEED_TEST:
            if (check_state_equal(STATE_PREVIEW)) {
                send_option_to_fifo(ACTION_SPEED_TEST);
            } else  {
                Log.d(TAG, "speed cam_state 0x%x", cam_state);
            }
            break;
			
        case MENU_RESET_INDICATION:
            if (check_state_equal(STATE_IDLE)) {
                restore_all();
            } else {
                Log.e(TAG, "menu reset indication cam_state 0x%x", cam_state);
            }
            break;
			
        case MENU_FORMAT_INDICATION:
            if (check_state_equal(STATE_IDLE)) {
                start_format();
            } else if (check_state_equal(STATE_FORMAT_OVER)) {
                rm_state(STATE_FORMAT_OVER);
                set_cur_menu_from_exit();
            } else {
                Log.w(TAG, "error state 0x%x", cam_state);
            }
            break;
			
        case MENU_LIVE_REC_TIME:
            send_option_to_fifo(ACTION_LIVE);
            break;

#ifdef ENABLE_MENU_STITCH_BOX		
        case MENU_STITCH_BOX:
            if (!bStiching) {
                set_cur_menu_from_exit();
                send_option_to_fifo(ACTION_SET_STICH);
            }
            break;
#endif
         
        case MENU_SHOW_SPACE: {
            int select = getMenuSelectIndex(MENU_SHOW_SPACE);
            Log.d(TAG, "used_space[%d] %s", select, used_space[select]);

            if (strlen(used_space[select]) != 0) {
                setCurMenu(MENU_FORMAT);
            }
            break;
        }

        /*
         * 根据选择的值进入对应的下级别菜单
         */
        case MENU_STORAGE: {    /* 存储菜单 */
            int iIndex = getMenuSelectIndex(MENU_STORAGE);
            Log.d(TAG, "[%s: %d] Menu Storage current select val[%d]", __FILE__, __LINE__, iIndex);
            SettingItem* pTmpSetItem = mStorageList.at(iIndex);
            if (pTmpSetItem) {
                if (!strcmp(pTmpSetItem->pItemName, SET_ITEM_NAME_STORAGESPACE)) {
                    setCurMenu(MENU_SHOW_SPACE);
                } else if (!strcmp(pTmpSetItem->pItemName, SET_ITEM_NAME_TESTSPEED)) {
                    Log.d(TAG, "[%s: %d] Hi Hi not support yet");
                }
            }

			break;
        }
            
        case MENU_FORMAT:
            setCurMenu(MENU_FORMAT_INDICATION);
            break;
		
        SWITCH_DEF_ERROR(cur_menu)
    }
}


int MenuUI::getCurMenuCurSelectIndex()
{
    return mMenuInfos[cur_menu].mSelectInfo.select;
}


int MenuUI::getCurMenuLastSelectIndex()
{
    return mMenuInfos[cur_menu].mSelectInfo.last_select;
}


/*************************************************************************
** 方法名称: procSettingKeyEvent
** 方法功能: 设置键的处理
** 入口参数: 
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procSettingKeyEvent()
{
    switch (cur_menu) {
        case MENU_TOP:	/* 如果当前处于主界面 */
            if (getCurMenuCurSelectIndex() == MAINMENU_WIFI) {	/* 主界面,当前选中的是WIFI项,按下设置键将启动二维码扫描功能 */
                Log.d(TAG, "wif state %d ap %d", mProCfg->get_val(KEY_WIFI_ON));
                
                #if 0
                if (/*mProCfg->get_val(KEY_WIFI_ON) == 0 &&*/ get_setting_select(SET_WIFI_AP) == 0) {
                    start_qr_func();
                }
                #endif

            } else {	/* 主界面直接按"设置"键,将跳到设置菜单 */
                setCurMenu(MENU_SYS_SETTING);
            }
            break;
			
        case MENU_PIC_INFO:
        case MENU_VIDEO_INFO:
        case MENU_LIVE_INFO:
        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF:
            if (check_state_preview()) {
                start_qr_func();
            }
            break;
			
        case MENU_SYS_DEV_INFO:
		#ifdef ENABLE_ADB_OFF
            if (++adb_root_times >= MAX_ADB_TIMES) {
                set_root();
            }
		#endif
            break;
		
        case MENU_RESET_INDICATION:
            Log.d(TAG, "aging_times %d state 0x%x", aging_times, cam_state);
            if (check_state_equal(STATE_IDLE)) {
                if (aging_times >= 5) {
                    send_option_to_fifo(ACTION_AGEING);
                }
            }
            break;
			
        default:
            break;
    }
}



/*************************************************************************
** 方法名称: procUpKeyEvent
** 方法功能: 按键事件处理
** 入口参数: 
**		
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procUpKeyEvent()
{
    switch (cur_menu) {
    case MENU_RESET_INDICATION:		/* 如果当前的菜单为MENU_RESET_INDICATION */
	    if (check_state_equal(STATE_IDLE)) {	/* 如果当前系统为空闲状态 */
            aging_times++;	/* 老化次数加1 */
        }
		
        Log.d(TAG, "aging_times %d cam_state 0x%x\n", aging_times, cam_state);
        break;
		
    default:	/* 其他的菜单 */
        commUpKeyProc();
        break;
    }
}



/*************************************************************************
** 方法名称: procDownKeyEvent
** 方法功能: 按键事件处理(方向下)
** 入口参数: 
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::procDownKeyEvent()
{
    switch (cur_menu) { /* 对于MENU_PIC_INFO/MENU_VIDEO_INFO/MENU_LIVE_INFO第一次按下将进入XXX_XXX_SET_DEF菜单 */
    
        case MENU_PIC_INFO:
	        if (check_state_preview()) {
		        setCurMenu(MENU_PIC_SET_DEF);
	        }
	        break;
		
        case MENU_VIDEO_INFO:
            if (check_state_preview()) {
                setCurMenu(MENU_VIDEO_SET_DEF);
            }
            break;
		
        case MENU_LIVE_INFO:
            if (check_state_preview()) {
                setCurMenu(MENU_LIVE_SET_DEF);
            }
            break;
		
	    default:
	        commDownKeyProc();
	        break;
        }
}



void MenuUI::exit_sys_err()
{
    if (cur_menu == MENU_SYS_ERR || ((MENU_LOW_BAT == cur_menu) && check_state_equal(STATE_IDLE))) {

        Log.d(TAG, "exit_sys_err ( %d 0x%x )", cur_menu, cam_state);

        if (mProCfg->get_val(KEY_LIGHT_ON) == 1) {
            setLightDirect(front_light);
        } else {
            setLightDirect(LIGHT_OFF);
        }
        procBackKeyEvent();
    }
}


bool MenuUI::check_cur_menu_support_key(int iKey)
{
	for (int i = 0; i < OLED_KEY_MAX; i++) {
		if (mMenuInfos[cur_menu].mSupportkeys[i] == iKey)
			return true;
	}
	return false;
}



/*************************************************************************
** 方法名称: handleKeyMsg
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返回值: 无 
** 调 用: 
**
*************************************************************************/
void MenuUI::handleKeyMsg(int iKey)
{
    if (cur_menu == -1) {
        Log.d(TAG, "[%s: %d] Menu System not Inited Yet!!!!", __FILE__, __LINE__);
        return;
    }

	/* 判断当前菜单是否支持该键值 */
	if (check_cur_menu_support_key(iKey)) {	
		
        unique_lock<mutex> lock(mutexState);
        switch (iKey) {
        case OLED_KEY_UP:		/* "UP"键的处理 */
        case 0x73:
            procUpKeyEvent();
            break;
		
        case OLED_KEY_DOWN:		/* "DOWN"键的处理 */
        case 0x72:
            procDownKeyEvent();
            break;
		
        case OLED_KEY_BACK:		/* "BACK"键的处理 */
        case 0x66:
            procBackKeyEvent();
            break;
		
        case OLED_KEY_SETTING:	/* "Setting"键的处理 */
        case 0x160:
            procSettingKeyEvent();
            break;
		
        case OLED_KEY_POWER:	/* "POWER"键的处理 */
        case 0x74:
            procPowerKeyEvent();
            break;

        default:
            break;
        }
    } else {
        Log.d(TAG, "[%s: %d] cur menu[%s] not support cur key[%d]", __FILE__, __LINE__, getCurMenuStr(cur_menu), iKey);
        exit_sys_err();
    }
}


void MenuUI::clear_area(u8 x,u8 y,u8 w,u8 h)
{
    mOLEDModule->clear_area(x,y,w,h);
}

void MenuUI::clear_area(u8 x, u8 y)
{
    mOLEDModule->clear_area(x,y);
}

bool MenuUI::check_allow_update_top()
{
    return !menuHasStatusbar(cur_menu);
}



/*************************************************************************
** 方法名称: oled_disp_battery
** 方法功能: 显示电池信息
** 入口参数: 
** 返回值: 0
** 调 用: 
**
*************************************************************************/
int MenuUI::oled_disp_battery()
{

#ifdef DEBUG_BATTERY
    Log.d(TAG, "mBatInterface->isSuc()  %d "
                  "m_bat_info_->bCharge %d "
                  "m_bat_info_->battery_level %d",
                mBatInterface->isSuc(), m_bat_info_->bCharge, m_bat_info_->battery_level);
#endif

    if (mBatInterface->isSuc()) {   /* 电池存在 */
        int icon;
        const int x = 110;
        u8 buf[16];

        if (m_bat_info_->bCharge && m_bat_info_->battery_level < 100) { /* 电池正在充电并且没有充满 */
            icon = ICON_BATTERY_IC_CHARGE_103_0_6_166_16;   
        } else {
            icon = ICON_BATTERY_IC_FULL_103_0_6_166_16;
        }
		
        if (check_allow_update_top()) { /* 允许更新电池信息到屏幕上 */
            if (m_bat_info_->battery_level == 1000) {
                m_bat_info_->battery_level = 0;
            }
			
            if ( m_bat_info_->battery_level >= 100) {
                snprintf((char *) buf, sizeof(buf), "%d", 100);
            } else {
                snprintf((char *)buf, sizeof(buf), "%d", m_bat_info_->battery_level);
            }
			
            disp_str_fill(buf, x, 0);   /* 显示电池图标及电量信息 */
            disp_icon(icon);
        }
    } else {
        //disp nothing while no bat
        clear_area(103, 0, 25, 16);
    }
	
    setLight(); /* 设置灯 */
    return 0;
}

void MenuUI::disp_waiting()
{
    disp_icon(ICON_CAMERA_WAITING_2016_76X32);
}

void MenuUI::disp_connecting()
{
    disp_icon(ICON_LIVE_RECONNECTING_76_32_20_1676_32);
}

void MenuUI::disp_saving()
{
    disp_icon(ICON_CAMERA_SAVING_2016_76X32);
}

void MenuUI::clear_ready()
{
    clear_icon(ICON_CAMERA_READY_20_16_76_32);
}

void MenuUI::disp_live_ready()
{
    bool bReady = true;
    int item = getMenuSelectIndex(MENU_LIVE_SET_DEF);

#if 0
    switch (item) {
#ifdef LIVE_ORG
        case LIVE_ORIGIN:
#endif
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
        case VID_ARIAL:
            if (mLiveAction[item].stOrgInfo.save_org != SAVE_OFF && checkHaveLocalSavePath()) {
                bReady = false;
            }
            break;
			
        case LIVE_CUSTOM:
            if (check_live_save(mProCfg->get_def_info(KEY_ALL_LIVE_DEF)) && checkHaveLocalSavePath()) {
                bReady = false;
            }
            break;
        SWITCH_DEF_ERROR(item)
    }
#endif

	Log.d(TAG, "disp_live_ready %d %d", item, bReady);
	
    if (bReady) {
        disp_icon(ICON_CAMERA_READY_20_16_76_32);
    } else {
        disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
    }
}

void MenuUI::disp_ready_icon(bool bDispReady)
{
	/* 调用存储管理器来判断显示图标 */
    if (!checkHaveLocalSavePath()) {    /* TODO: 必须要有内部还外部卡 */
        disp_icon(ICON_CAMERA_READY_20_16_76_32);
    } else {
        disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
    }
}


void MenuUI::disp_shooting()
{
    Log.d(TAG, "checkHaveLocalSavePath() is %d", checkHaveLocalSavePath());
    disp_icon(ICON_CAMERA_SHOOTING_2016_76X32);
    setLight(fli_light);
}

void MenuUI::disp_processing()
{
    disp_icon(ICON_PROCESS_76_3276_32);
    send_update_light(MENU_PIC_INFO, STATE_PIC_STITCHING, INTERVAL_5HZ);
}

bool MenuUI::check_state_preview()
{
    return check_state_equal(STATE_PREVIEW);
}

bool MenuUI::check_state_equal(int state)
{
    return (cam_state == state);
}

bool MenuUI::check_state_in(int state)
{
    bool bRet = false;
    if ((cam_state&state) == state) {
        bRet = true;
    }
    return bRet;
}

bool MenuUI::check_live()
{
    return (check_state_in(STATE_LIVE) || check_state_in(STATE_LIVE_CONNECTING));
}

void MenuUI::update_by_controller(int action)
{
//    switch(action)
//    {
//        case ACTION_PIC:
//            updateMenuCurPageAndSelect(MENU_PIC_SET_DEF,PIC_DEF_MAX);
//            break;
//        case ACTION_VIDEO:
//            updateMenuCurPageAndSelect(MENU_VIDEO_SET_DEF,VID_DEF_MAX);
//            break;
//        case ACTION_LIVE:
//            updateMenuCurPageAndSelect(MENU_LIVE_SET_DEF,LIVE_DEF_MAX);
//            break;
//        SWITCH_DEF_ERROR(action);
//    }
}

void MenuUI::add_state(int state)
{
    cam_state |= state;
	
    switch (state) {
		
        #if 0
		case STATE_QUERY_STORAGE:
            disp_waiting();		/* 屏幕中间显示"..." */
			break;
        #endif

        case STATE_STOP_RECORDING:
            disp_saving();
            break;
		
        case STATE_START_PREVIEWING:
        case STATE_STOP_PREVIEWING:
        case STATE_START_RECORDING:
        case STATE_START_LIVING:
        case STATE_STOP_LIVING:
            disp_waiting();		/* 屏幕中间显示"..." */
            break;
		
        case STATE_TAKE_CAPTURE_IN_PROCESS:
        case STATE_PIC_STITCHING:
            //force pic menu to make bottom info updated ,if req from http
            setCurMenu(MENU_PIC_INFO);
            break;
		
        case STATE_RECORD:  /* 添加录像状态(取出正在启动录像状态) */
            rm_state(STATE_START_RECORDING);
            break;
		
        case STATE_LIVE:
            if (check_state_in(STATE_LIVE_CONNECTING)) {
                //clear reconnecting
                clear_ready();
            }
			
            rm_state(STATE_START_LIVING | STATE_LIVE_CONNECTING);
            break;
			
        case STATE_LIVE_CONNECTING:
            rm_state(STATE_START_LIVING | STATE_LIVE);
            disp_connecting();
            break;

		/*
		 * 之前在进入预览状态之后就会显示就绪及容量信息
		 * 现在进入预览状态之后,将进入容量查询状态
		 */
        case STATE_PREVIEW:
            INFO_MENU_STATE(cur_menu, cam_state);
			
            rm_state(STATE_START_PREVIEWING | STATE_STOP_PREVIEWING);
            if (!(check_live() || check_state_in(STATE_RECORD))) {
                switch (cur_menu) {
                    case MENU_PIC_INFO:
                    case MENU_VIDEO_INFO:
                        disp_ready_icon();
                        break;
					
                    case MENU_LIVE_INFO:
                        disp_live_ready();
                        break;
					
                    case MENU_CALIBRATION:
                    case MENU_QR_SCAN:
                    case MENU_GYRO_START:
                    case MENU_SPEED_TEST:
                    case MENU_NOSIE_SAMPLE:
                        //do nothing while menu calibration
                        Log.e(TAG, "add preview but do nothing while menu %d", cur_menu);
                        break;
					
                        //state preview with req sync state
                    default:
                        Log.d(TAG, "STATE_PREVIEW default cur_menu %d", cur_menu);
                        setCurMenu(MENU_PIC_INFO);
                        break;
                }
            }
            break;


			
        case STATE_CALIBRATING:
//            Log.d(TAG,"state clibration cur_menu %d",cur_menu);
//            if(cur_menu != MENU_CALIBRATION)
//            {
//                setCurMenu(MENU_CALIBRATION);
//            }
            break;

        case STATE_START_QRING:
            if (cur_menu != MENU_QR_SCAN) {
                setCurMenu(MENU_QR_SCAN);
            }
            break;
			
        case STATE_START_QR:
            Log.d(TAG,"start qr cur_menu %d", cur_menu);
            rm_state(STATE_START_QRING);
            setCurMenu(MENU_QR_SCAN);
            break;
			
        case STATE_STOP_QRING:
            if (cur_menu == MENU_QR_SCAN) {
                enterMenu();
            } else {
                ERR_MENU_STATE(cur_menu,state);
            }
            break;
			
        case STATE_LOW_BAT:
            break;
			
        case STATE_START_GYRO:
            setCurMenu(MENU_GYRO_START);
            break;
		
        case STATE_NOISE_SAMPLE:
            break;
			
        case STATE_SPEED_TEST:
            setCurMenu(MENU_SPEED_TEST);
            break;
		
        case STATE_RESTORE_ALL:

            disp_icon(ICON_RESET_SUC_128_48128_48);
        
            mProCfg->reset_all();
            msg_util::sleep_ms(500);

            rm_state(STATE_RESTORE_ALL);

//            Log.d(TAG,"bDispTop %d",bDispTop);
            init_cfg_select();
            Log.d(TAG,"STATE_RESTORE_ALL cur_menu is %d cam_state 0x%x", MENU_TOP, cam_state);
            if (cur_menu == MENU_TOP) {
                setCurMenu(cur_menu);
            } else {
                procBackKeyEvent();
            }
            break;
			
        case STATE_POWER_OFF:
            Log.d(TAG,"do nothing for power off");
            break;
		
        case STATE_FORMATING:
            break;

			
        case STATE_FORMAT_OVER:
            break;
			
        SWITCH_DEF_ERROR(state)
    }
	
}

void MenuUI::rm_state(int state)
{
    cam_state &= ~state;
}

void MenuUI::set_tl_count(int count)
{
    Log.d(TAG, "set tl count %d", count);
    tl_count = count;
}

void MenuUI::disp_tl_count(int count)
{
    if (count < 0) {
        Log.e(TAG, "error tl count %d", tl_count);
    } else if (count == 0) {
        clear_ready();
        char buf[32];
        snprintf(buf,sizeof(buf),"%d",count);
        disp_str((const u8 *)buf,57,24);
        clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    } else {
        if (check_state_in(STATE_RECORD) && !check_state_in(STATE_STOP_RECORDING)) {
            char buf[32];
            snprintf(buf,sizeof(buf),"%d",count);
            disp_str((const u8 *)buf,57,24);
            setLight(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            setLight(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            setLight(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            setLight(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
        } else {
            Log.e(TAG, "tl count error state 0x%x", cam_state);
        }
    }
}

void MenuUI::minus_cam_state(int state)
{
    rm_state(state);

    // set_cap_delay(0);

//    Log.d(TAG,"after minus_cam_state cam_state 0x%x "
//                  "cur_menu %d state 0x%x",
//          cam_state,cur_menu, state);

    if (check_state_equal(STATE_IDLE)) {
        reset_last_info();
        switch (state) {
            case STATE_CALIBRATING:
            case STATE_START_GYRO:
            case STATE_START_QR:
            case STATE_NOISE_SAMPLE:
                set_cur_menu_from_exit();
                break;

            default:
                setCurMenu(MENU_TOP);
                break;
        }
    } else if (check_state_preview()) {
        Log.d(TAG, "minus_cam_state　state preview (%d 0x%x)", cur_menu, state);
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_VIDEO_INFO:
                disp_ready_icon();
                break;

            case MENU_LIVE_INFO:
                disp_live_ready();
                break;

            case MENU_QR_SCAN:
                set_cur_menu_from_exit();
                break;

            default:
                //force pic menu
                setCurMenu(MENU_PIC_INFO);
                break;
        }
    } else {
        Log.w(TAG," minus error cam_state 0x%x state 0x%x", cam_state,state);
    }
}


void MenuUI::disp_err_code(int code, int back_menu)
{
    bool bFound = false;
    char err_code[128];

    memset(err_code, 0, sizeof(err_code));

    set_back_menu(MENU_SYS_ERR, back_menu);
    for (u32 i = 0; i < sizeof(mErrDetails)/sizeof(mErrDetails[0]); i++) {
        if (mErrDetails[i].code == code) {
            if (mErrDetails[i].icon != -1) {
                setLightDirect(BACK_RED | FRONT_RED);
                Log.d(TAG, "disp error code (%d %d %d)", i, mErrDetails[i].icon, code);
                disp_icon(mErrDetails[i].icon);
                bFound = true;
            } else {
                snprintf(err_code, sizeof(err_code), "%s", mErrDetails[i].str);
            }
            break;
        }
    }
	
    if (!bFound) {
        if (strlen(err_code) == 0) {
            disp_icon(ICON_ERROR_128_64128_64);
            snprintf(err_code, sizeof(err_code), "%d", code);
            disp_str((const u8 *)err_code, 64, 16);
        } else {
            clear_area();
            disp_str((const u8 *)err_code, 16, 16);
        }
        Log.d(TAG, "disp err code %s\n", err_code);
    }
	
    if ((cam_state & STATE_LIVE) == STATE_LIVE) {
        Log.w(TAG, "no clear controll on state live cam_state 0x%x", cam_state);
    } else {
        mControlAct = nullptr;
    }

    reset_last_info();

    //force cur menu sys_err
    setLightDirect(BACK_RED|FRONT_RED);
    cur_menu = MENU_SYS_ERR;
    bDispTop = false;

    Log.d(TAG,"disp_err_code code %d "
                  "back_menu %d cur_menu %d "
                  "bFound %d cam_state 0x%x",
          code,back_menu,cur_menu,bFound,cam_state);
}

void MenuUI::disp_err_str(int type)
{
    for (u32 i = 0; i < sizeof(mSysErr)/sizeof(mSysErr[0]); i++) {
        if (type == mSysErr[i].type) {
            disp_str((const u8 *) mSysErr[i].code, 64, 16);
            break;
        }
    }
}

void MenuUI::disp_sys_err(int type, int back_menu)
{
    Log.d(TAG, "disp_sys_err cur menu %d"
                  " state 0x%x back_menu %d type %d",
          cur_menu,
          cam_state,
          back_menu,
          type);
	
    //met error at the
    if (cur_menu == -1 && check_state_equal(STATE_IDLE)) {
        Log.e(TAG, " met error at the beginning");
    }
	
    if (cur_menu != MENU_SYS_ERR) {
        setCurMenu(MENU_SYS_ERR, back_menu);
    }
	
    /* 如果是模组上电错误(可能是个别模组没起来)，调用关电 */
    if (type == 460) {
        system("power_manager power_off");
    }

    mControlAct = nullptr;
    disp_err_str(type);
}




/*************************************************************************
** 方法名称: set_flick_light
** 方法功能: 设置灯光闪烁的颜色值
** 入口参数: 无
** 返 回 值: 无 
** 调     用: setLight
**
*************************************************************************/
void MenuUI::set_flick_light()
{
    
    if (mProCfg->get_val(KEY_LIGHT_ON) == 1) {
        switch ((front_light)) {
            case FRONT_RED:
                fli_light = BACK_RED;
                break;
            case FRONT_YELLOW:
                fli_light = BACK_YELLOW;
                break;
            case FRONT_WHITE:
                fli_light = BACK_WHITE;
                break;
            SWITCH_DEF_ERROR(front_light);
        }
    }
}

bool MenuUI::check_cam_busy()
{
    bool bRet = false;
    //busy state with light flick
    const int busy_state[] = {STATE_TAKE_CAPTURE_IN_PROCESS,
                              STATE_PIC_STITCHING,
                              STATE_RECORD,
                              STATE_LIVE,
                              /*STATE_LIVE_CONNECTING,*/
                              STATE_CALIBRATING};

    for (u32 i = 0; i < sizeof(busy_state)/sizeof(busy_state[0]); i++) {
        if (check_state_in(busy_state[i])) {
            bRet = true;
            break;
        }
    }
    return bRet;
}

void MenuUI::setLight()
{
    if (mBatInterface->isSuc()) {	/* 电池存在,根据电池的电量来设置前灯的显示状态:   */
        if (m_bat_info_->battery_level < 10) {			/* 电量小于10%显示红色 */
            front_light = FRONT_RED;
        } else if (m_bat_info_->battery_level < 20) {	/* 电量小于20%显示黄色 */
            front_light = FRONT_YELLOW;
        } else {										/* 电量高于20%,显示白色 */
            front_light = FRONT_WHITE;
        }
    } else {	/* 电池不存在,显示白色 */
        front_light = FRONT_WHITE;
    }
	
    set_flick_light();	/* 根据前灯的状态来设置闪烁时的灯颜色 */

    if (!check_cam_busy() && (cur_menu != MENU_SYS_ERR) && (cur_menu != MENU_LOW_BAT)) {
        setLight(front_light);
    }
	
    return;
}

int MenuUI::get_error_back_menu(int force_menu)
{
    int back_menu = MENU_TOP;
    if (check_state_equal(STATE_IDLE)) {
//        Log.d(TAG,"set_sync_info oled_reset_disp "
//                      "cam_state 0x%x, cur_menu %d",
//              cam_state,cur_menu);
        back_menu = MENU_TOP;
    } else if (check_state_in(STATE_NOISE_SAMPLE)) {
        back_menu = MENU_NOSIE_SAMPLE;
    } else if (check_state_in(STATE_SPEED_TEST)) {
        back_menu = MENU_SPEED_TEST;
    } else if (check_state_in(STATE_START_GYRO)) {
        back_menu = MENU_GYRO_START;
    } else if(check_state_in(STATE_START_QR) || check_state_in(STATE_START_QRING) ||
             check_state_in(STATE_STOP_QRING)) {
        back_menu = MENU_QR_SCAN;
    } else if (check_state_in(STATE_RECORD) || 
		check_state_in(STATE_START_RECORDING) ||
		check_state_in(STATE_STOP_RECORDING)) {
        back_menu = MENU_VIDEO_INFO;
    } else if (check_state_in(STATE_LIVE) || 
		check_state_in(STATE_START_LIVING) || 
		check_state_in(STATE_STOP_LIVING) || 
		check_state_in(STATE_LIVE_CONNECTING)) {
        back_menu = MENU_LIVE_INFO;
    } else if (check_state_in(STATE_CALIBRATING)) {
        back_menu = MENU_CALIBRATION;
    } else if (check_state_in(STATE_PIC_STITCHING) || 
			check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || 
			check_state_in(STATE_PREVIEW) || 
			check_state_in(STATE_START_PREVIEWING) || 
			check_state_in(STATE_STOP_PREVIEWING)) {
        if (force_menu != -1) {
            back_menu = force_menu;
        } else {
            back_menu = MENU_PIC_INFO;
        }
    }
			
    Log.d(TAG, "get_error_back_menu state 0x%x back_menu %d", cam_state, back_menu);
    return back_menu;
}


int MenuUI::oled_disp_err(sp<struct _err_type_info_> &mErr)
{
    int type = mErr->type;
    int err_code = mErr->err_code;

    Log.d(TAG, "oled_disp_err type %d err_code %d cur_menu %d cam_state 0x%x",
          type, err_code, cur_menu, cam_state);

    if (err_code == -460) { /* 模组打开失败 */
        system("power_manager power_off");  /* 关闭模组，已免下次打开失败 */
    }
	//original error handle
    if (err_code == -1) {
        oled_disp_type(type);
    } else { // new error_code
        int back_menu = MENU_TOP;
        tl_count = -1;
		
        //make it good code
        err_code = abs(err_code);
        switch (type) {
            case START_PREVIEW_FAIL:
                rm_state(STATE_START_PREVIEWING);
                back_menu = get_error_back_menu();
                break;
			
            // #BUG1402
            case CAPTURE_FAIL:
                rm_state(STATE_TAKE_CAPTURE_IN_PROCESS | STATE_PIC_STITCHING);
                back_menu = get_error_back_menu(MENU_PIC_INFO); // MENU_PIC_INFO;
                break;
				
            case START_REC_FAIL:
                rm_state(STATE_START_RECORDING|STATE_RECORD);
                back_menu = get_error_back_menu(MENU_VIDEO_INFO);//MENU_VIDEO_INFO;
                break;
				
            case START_LIVE_FAIL:
                rm_state(STATE_START_LIVING|STATE_LIVE|STATE_LIVE_CONNECTING);
                back_menu = get_error_back_menu(MENU_LIVE_INFO);//MENU_LIVE_INFO;
                break;
				
            case QR_FINISH_ERROR:
                rm_state(STATE_STOP_QRING|STATE_START_QR);
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case START_QR_FAIL:
                rm_state(STATE_START_QRING);
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case STOP_QR_FAIL:
                rm_state(STATE_STOP_QRING | STATE_START_QR);
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case QR_FINISH_UNRECOGNIZE:
                rm_state(STATE_STOP_QRING|STATE_START_QR);
                back_menu = get_back_menu(MENU_QR_SCAN);
                break;
				
            case CALIBRATION_FAIL:
                rm_state(STATE_CALIBRATING);
                back_menu = get_back_menu(MENU_CALIBRATION);
                break;
				
            case START_GYRO_FAIL:
                rm_state(STATE_START_GYRO);
                back_menu = get_back_menu(MENU_GYRO_START);
                break;
				
            case START_NOISE_FAIL:
                rm_state(STATE_NOISE_SAMPLE);
                back_menu = get_back_menu(MENU_NOSIE_SAMPLE);
                break;
				
            case STOP_PREVIEW_FAIL:
                rm_state(STATE_STOP_PREVIEWING | STATE_PREVIEW);
                back_menu = get_error_back_menu();//MENU_TOP;
                break;
				
            case STOP_REC_FAIL:
                rm_state(STATE_STOP_RECORDING | STATE_RECORD);
                back_menu = get_error_back_menu(MENU_VIDEO_INFO);//MENU_VIDEO_INFO;
                break;
				
            case STOP_LIVE_FAIL:
                rm_state(STATE_STOP_LIVING | STATE_LIVE | STATE_LIVE_CONNECTING);
                back_menu = get_error_back_menu(MENU_LIVE_INFO);//MENU_LIVE_INFO;
                break;
				
            case RESET_ALL:
                oled_disp_type(RESET_ALL);
                err_code = -1;
                break;
				
            case SPEED_TEST_FAIL:
                rm_state(STATE_SPEED_TEST);
                back_menu = get_back_menu(MENU_SPEED_TEST);
                break;
				
            case LIVE_REC_OVER:
                back_menu = MENU_LIVE_INFO;
                break;
			
            default:
                Log.d(TAG,"bad type %d code %d",type,err_code);
                err_code =-1;
                break;
        }
		
        if (err_code != -1) {
            disp_err_code(err_code, back_menu);
        }
    }
    return 0;
}


void MenuUI::set_led_power(unsigned int on)
{
    if (on == 1) {
        setLight();
    } else {
        setLightDirect(LIGHT_OFF);
    }
}

void MenuUI::set_oled_power(unsigned int on)
{
#if 0    
    int GPIP_PX3 = 187;


    Log.i(TAG,"set_oled_power on %d",on);
    if (gpio_is_requested(GPIP_PX3) != 1)
    {
//        Log.d(TAG,"create gpio GPIO_V6");
        gpio_request(GPIP_PX3);
    }
//    gpio_direction_output(GPIO_V6, 1);
//    msg_util::sleep_ms(1000);
    gpio_direction_output(GPIP_PX3, on);

    set_led_power(on);
#endif
}

#define STR(cmd)	#cmd



bool MenuUI::queryCurStorageState(int iTimeout)
{
	sp<Volume> tmpVol;
	
    int  iStepTime = 50;     // 50Ms
    bool bQueryResult = false;

    send_option_to_fifo(ACTION_QUERY_STORAGE);

    while (iTimeout > 0) {
        {
            unique_lock<mutex> lock(mRemoteDevLock);
            bQueryResult = mRemoteStorageUpdate;
        }

        if (bQueryResult == true) {
            mRemoteStorageUpdate = false;
            break;    
        } else {
            msg_util::sleep_ms(iStepTime);
        }
        iTimeout -= iStepTime;
    }    
    return bQueryResult;

}


int MenuUI::convCapDelay2Index(int iDelay) 
{
    int iDelayArray[] = {3, 5, 10, 20, 30, 40, 50, 60};
    int iSize = sizeof(iDelayArray) / sizeof(iDelayArray[0]);
    int iIndex = 0;
    for (iIndex = 0; iIndex < iSize; iIndex++) {
        if (iDelay == iDelayArray[iIndex])
            break;
    }

    if (iIndex < iSize) {
        return iIndex;
    } else {
        return 1;   /* 默认返回5s */
    }
}



int MenuUI::convIndex2CapDelay(int iIndex)
{
    int iDelayArray[] = {3, 5, 10, 20, 30, 40, 50, 60};
    int iSize = sizeof(iDelayArray) / sizeof(iDelayArray[0]);

    if (iIndex < 0 || iIndex > iSize - 1) {
        Log.e(TAG, "[%s: %d] Invalid index gived [%d]", __FILE__, __LINE__, iIndex);
        return 5;   /* 默认为5s */
    } else {
        return iDelayArray[iIndex];
    }
}


int MenuUI::convAebNumber2Index(int iAebNum)
{
    int iAebNumArry[] = {3, 5, 7, 9}; 

    int iSize = sizeof(iAebNumArry) / sizeof(iAebNumArry[0]);
    int iIndex = 0;
    for (iIndex = 0; iIndex < iSize; iIndex++) {
        if (iAebNum == iAebNumArry[iIndex])
            break;
    }

    if (iIndex < iSize) {
        return iIndex;
    } else {
        return 1;   /* 默认返回5s */
    }


}

int MenuUI::convIndex2AebNum(int iIndex)
{
    int iAebNumArry[] = {3, 5, 7, 9}; 
    int iSize = sizeof(iAebNumArry) / sizeof(iAebNumArry[0]);   

    if (iIndex < 0 || iIndex > iSize - 1) {
        Log.e(TAG, "[%s: %d] Invalid index gived [%d]", __FILE__, __LINE__, iIndex);
        return 5;   /* 默认为5s */
    } else {
        return iAebNumArry[iIndex];
    }

}


//all disp is at bottom
int MenuUI::oled_disp_type(int type)
{
    Log.d(TAG, "oled_disp_type (%d %s 0x%x)\n", type, getCurMenuStr(cur_menu), cam_state);

    rm_state(STATE_FORMAT_OVER);

    switch (type) {
        case START_AGEING_FAIL:
            INFO_MENU_STATE(cur_menu,cam_state)
            minus_cam_state(STATE_RECORD);
            break;

		/*
		 * 启动老化 - 需要传递(本次老化的时间)
		 */
        case START_AGEING:
            if (check_state_equal(STATE_IDLE)) {
                setCurMenu(MENU_AGEING);
            }
            break;
			
        case START_RECING:
            add_state(STATE_START_RECORDING);
            break;

		/*
		 * 启动录像成功
		 */
        case START_REC_SUC:	/* 发送显示录像成功 */
            if (!check_state_in(STATE_RECORD)) {    /* 处于非录像状态 */
                if (check_rec_tl()) {   /* 检查是否为非timelapse */
                    tl_count = 0;
                    //disable update_light
                    disp_tl_count(tl_count);
                } else {    /* 非timelapse录像 */
                    //before send for disp_mid
                    set_update_mid();
                }
				
                INFO_MENU_STATE(cur_menu, cam_state)
                add_state(STATE_RECORD);    /* 添加正在录像状态(去除正在启动录像状态) */
                setCurMenu(MENU_VIDEO_INFO);
            } else {
                Log.e(TAG, "START_REC_SUC error state 0x%x\n", cam_state);
            }
            break;

		/* 
		 * 启动测试速度
		 */
        case SPEED_START:
            if (!check_state_in(STATE_SPEED_TEST)) {
                add_state(STATE_SPEED_TEST);
            } else {
                Log.e(TAG, "speed test happened already\n");
            }
            break;

		/*
		 * 速度测试成功
		 */
        case SPEED_TEST_SUC:
            if (check_state_in(STATE_SPEED_TEST)) {
                rm_state(STATE_SPEED_TEST);
                disp_icon(ICON_SPEEDTEST05128_64);
                msg_util::sleep_ms(1000);
                procBackKeyEvent();
            }
            break;

		/*
		 * 速度测试失败
		 */
        case SPEED_TEST_FAIL:
            if (check_state_in(STATE_SPEED_TEST)) {
                rm_state(STATE_SPEED_TEST);
                disp_icon(ICON_SPEEDTEST06128_64);
            }
            break;
			
        /*
		 * 启动录像失败
		 */
        case START_REC_FAIL:
            rm_state(STATE_START_RECORDING);
            disp_sys_err(type);
            break;

		/*
		 * 停止录像
		 */
        case STOP_RECING:
            add_state(STATE_STOP_RECORDING);    /* 添加STATE_STOP_RECORDING状态 */
            break;

		/*
		 * 停止录像成功
		 */
        case STOP_REC_SUC:
            if (check_state_in(STATE_RECORD)) {
                tl_count = -1;
                minus_cam_state(STATE_RECORD | STATE_STOP_RECORDING);
//              play_sound(SND_STOP);
                mControlAct = nullptr;
                //fix select for changed by controller or timelapse
                if (cur_menu == MENU_VIDEO_INFO)  {
                    dispBottomInfo();
                } else {
                    Log.e(TAG, "error cur_menu %d ", cur_menu);
                }

				/* 添加用于老化测试： 灯全绿 */
				#ifdef ENABLE_AGING_MODE
				setLightDirect(FRONT_GREEN | BACK_GREEN);
				#endif
            }
            break;
			
        case STOP_REC_FAIL:
            if (check_state_in(STATE_RECORD)) {
                tl_count = -1;
                rm_state(STATE_STOP_RECORDING | STATE_RECORD);
                disp_sys_err(type);
            }
            break;


/************************************ 拍照相关 START **********************************************/	
        case CAPTURE:

            if (check_allow_pic()) {
                /* mControlAct不为NULL,有两种情况 
                 * - 来自客户端的拍照请求
                 * - 识别二维码的请求
                 */
                if (mControlAct != nullptr) {   
                	Log.d(TAG, "+++++++++++++++>>>> CAPTURE mControlAct != nullptr");
                    mNeedSendAction = false;
                    
                    /** 不发送请求 */
                    setTakePicDelay(mControlAct->delay);
                } else {
                    mNeedSendAction = true;
                    int item = getMenuSelectIndex(MENU_PIC_SET_DEF);
                    int iDelay = convIndex2CapDelay(mProCfg->get_val(KEY_PH_DELAY));

                    struct stPicVideoCfg* pPicVidCfg = mPicAllItemsList.at(item);
                    if (pPicVidCfg) {
                        if (strcmp(pPicVidCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                            setTakePicDelay(iDelay);
                        } else {
                            setTakePicDelay(mProCfg->get_def_info(KEY_ALL_PIC_DEF)->delay);
                        }
                    } else {
                        Log.e(TAG, "[%s: %d] Invalid item[%d] for capture", __FILE__, __LINE__, item);
                        setTakePicDelay(iDelay);
                    }			
                }
				
                add_state(STATE_TAKE_CAPTURE_IN_PROCESS);
				setCurMenu(MENU_PIC_INFO);  /* 再次进入MENU_PIC_INFO菜单 */
				
				/* 第一次发送更新消息, 根据cap_delay的值来决定播放哪个声音 */
                send_update_light(MENU_PIC_INFO, STATE_TAKE_CAPTURE_IN_PROCESS, INTERVAL_1HZ);
            }
            break;

			
        case CAPTURE_SUC:

            if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || check_state_in(STATE_PIC_STITCHING)) {
                minus_cam_state(STATE_TAKE_CAPTURE_IN_PROCESS | STATE_PIC_STITCHING);

                if (cur_menu == MENU_PIC_INFO) {
                    if (mControlAct != nullptr) {
                        Log.d(TAG,"control cap suc");
                        mControlAct = nullptr;
                        dispBottomInfo();
                    } else {
                        Log.d(TAG, ">>> CAPTURE_SUC remain pic %d", mCanTakePicNum);
                        if (mCanTakePicNum > 0) {
                            mCanTakePicNum--;
                        }
                        dispBottomLeftSpace();
                    }
                } else {
                    mControlAct = nullptr;
                    Log.e(TAG, "error capture　suc cur_menu %d ", cur_menu);
                }
                play_sound(SND_COMPLE);
            }
            break;
			
        case CAPTURE_FAIL:  /*  */
            if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || check_state_in(STATE_PIC_STITCHING)) {
                rm_state(STATE_TAKE_CAPTURE_IN_PROCESS | STATE_PIC_STITCHING);
                disp_sys_err(type);
            }
            break;

/************************************ 拍照相关 END **********************************************/




/************************************ 直播相关 START ACTION_INFO**********************************************/	
        case STRAT_LIVING:
            add_state(STATE_START_LIVING);
            break;
		
        case START_LIVE_SUC:
            //avoid conflict with http req
            if (!check_state_in(STATE_LIVE)) {
                tl_count = -1;
                set_update_mid();
                add_state(STATE_LIVE);
                setCurMenu(MENU_LIVE_INFO);
            } else {
                Log.e(TAG,"start live suc error state 0x%x",cam_state);
            }
            break;
			
        case START_LIVE_CONNECTING:
            if (!check_state_in(STATE_LIVE_CONNECTING)) {
                add_state(STATE_LIVE_CONNECTING);
                setCurMenu(MENU_LIVE_INFO);
            }
            Log.d(TAG,"live connect 0x%x",cam_state);
            break;
			
        case START_LIVE_FAIL:
            rm_state(STATE_START_LIVING);
            disp_sys_err(type);
            break;
			
        case STOP_LIVING:
            add_state(STATE_STOP_LIVING);
            break;
		
        case STOP_LIVE_SUC:
            if (check_live()) {
                minus_cam_state(STATE_LIVE|STATE_STOP_LIVING|STATE_LIVE_CONNECTING);
                if (mControlAct != nullptr) {
                    mControlAct = nullptr;
                    if (cur_menu == MENU_LIVE_INFO) {
                        // disp_cam_param(0);
                    } else {
                        Log.e(TAG, "error capture　suc cur_menu %d ", cur_menu);
                    }
                }
            }
            break;
			
        case STOP_LIVE_FAIL:
            rm_state(STATE_STOP_LIVING | STATE_LIVE | STATE_LIVE_CONNECTING);
            disp_sys_err(type);
            break;

/************************************ 直播相关 END **********************************************/

			
        case PIC_ORG_FINISH:
            if (!check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
                Log.e(TAG, "pic org finish error state 0x%x", cam_state);
            } else {
                if (!check_state_in(STATE_PIC_STITCHING)) {
                    rm_state(STATE_TAKE_CAPTURE_IN_PROCESS);
                    play_sound(SND_SHUTTER);
                    add_state(STATE_PIC_STITCHING);
                }
            }
            break;
			
        case START_FORCE_IDLE:
            oled_reset_disp(START_FORCE_IDLE);
            break;
		
        case RESET_ALL:
            oled_reset_disp(RESET_ALL);
            break;
		
        case RESET_ALL_CFG:

            disp_icon(ICON_RESET_SUC_128_48128_48);
            msg_util::sleep_ms(500);
            mProCfg->reset_all(false);

            init_cfg_select();

            Log.d(TAG, "RESET_ALL_CFG cur_menu is %d", cur_menu);

            if (cur_menu == MENU_TOP) {
                setCurMenu(cur_menu);
            } else {
                procBackKeyEvent();
            }
            break;



/*********************************  预览相关状态 START ********************************************/
			
        case START_PREVIEWING:
            add_state(STATE_START_PREVIEWING);
            break;

        case START_PREVIEW_SUC:		/* 启动预览成功 */
            if (!check_state_in(STATE_PREVIEW)) {   /*  */

                /* TODO: 考虑先更新底部存储,添加STATE_PREVIEW状态
                 * 在进入STATE_PREVIEW之后会根据存储来显示就就绪状态还是NO SD CARD状态
                 */
				Log.d(TAG, ">>>>>>>>>>>>>>>  PREVIEW SUCCESS");                  
                add_state(STATE_PREVIEW);       /* 添加"STATE_PREVIEW",如果后会显示"READY"字样 */
                updateBottomSpace(true);        /* 计算并更新底部剩余空间(将查询空间的方法统一放入updateBottomSpace中调用) */
            } else {
                Log.e(TAG, "[%s: %d] Invalid state change", __FILE__, __LINE__);
            }
            break;

			
        case START_PREVIEW_FAIL:	/* 启动预览失败 */
            Log.d(TAG, "[%s: %d] START_PREVIEW_FAIL cur_menu [%s] cam state %d", __FILE__, __LINE__, getCurMenuStr(cur_menu), cam_state);
            rm_state(STATE_START_PREVIEWING);
            disp_sys_err(type, MENU_TOP);
            break;
				
        case STOP_PREVIEWING:		/* 停止预览 */
            add_state(STATE_STOP_PREVIEWING);	/* 显示"..." */
            break;
		
        case STOP_PREVIEW_SUC:		/* 停止预览成功 */	
            if (check_state_in(STATE_PREVIEW)) {
                minus_cam_state(STATE_PREVIEW | STATE_STOP_PREVIEWING);
            }
            break;
			
        case STOP_PREVIEW_FAIL:		/* 停止预览失败 */
            Log.d(TAG, "STOP_PREVIEW_FAIL fail cur_menu %d %d", cur_menu, cam_state);
            rm_state(STATE_STOP_PREVIEWING | STATE_PREVIEW);
            disp_sys_err(type);
            break;


		case START_QUERY_STORAGE_SUC:	/* 查询容量成功:(只能Idle状态或者预览完成完成之后进行) */
			rm_state(STATE_QUERY_STORAGE);
			Log.d(TAG, "curtent menu [%s], cam_state [%d]", STR(cur_menu), cam_state);
		
			/* 打印当前的菜单ID */
			/* 如果是在预览状态下查询的,并且当前菜单为PIC,VID,LIVE, 显示"Ready"及右下角的可拍张数 */


			/* 如果是在Idle状态下查询的,只需要存储统计信息即可 */
			break;
			

		case START_QUERY_STORAGE_FAIL:	/* 查询容量失败(显示"NO SD CARD"),左下角显示"模式", 右下角显示"None" */
			rm_state(STATE_QUERY_STORAGE);
			/* 如果是在预览状态下查询的,并且当前菜单为PIC,VID,LIVE, (显示"NO SD CARD"),左下角显示"模式", 右下角显示"None" */

			/* 如果是在Idle状态下查询的,只需要存储统计信息即可 */

		
			break;
			
/*********************************	预览相关状态 START ********************************************/



/*********************************	陀螺仪相关状态 START ********************************************/
			
        case START_CALIBRATIONING: {

            Log.d(TAG, "START_CALIBRATIONING calc delay %d cur_menu [%s]", mGyroCalcDelay, getCurMenuStr(cur_menu));
            if (mGyroCalcDelay <= 0) {
                mGyroCalcDelay = 5;
            }

            if (cur_menu != MENU_CALIBRATION) {		/* 进入陀螺仪校正菜单 */
                setCurMenu(MENU_CALIBRATION);
            }
            add_state(STATE_CALIBRATING);
            send_update_light(MENU_CALIBRATION, STATE_CALIBRATING, INTERVAL_1HZ);

            break;
        }

			
        case CALIBRATION_SUC:
            mCalibrateSrc = false;
            if (check_state_in(STATE_CALIBRATING)) {
                disp_calibration_res(0);
                msg_util::sleep_ms(1000);
                minus_cam_state(STATE_CALIBRATING);
            }
            break;
			
        case CALIBRATION_FAIL:
            mCalibrateSrc = false;
            rm_state(STATE_CALIBRATING);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;

/*********************************	陀螺仪相关状态 END ********************************************/

			
        case SYNC_REC_AND_PREVIEW:
            if (!check_state_in(STATE_RECORD)) {
                cam_state = STATE_PREVIEW;
                add_state(STATE_RECORD);
			
                //disp video menu before add state_record
                setCurMenu(MENU_VIDEO_INFO);
                Log.d(TAG," set_update_mid a");
                set_update_mid();
            }
            break;
			
        case SYNC_PIC_CAPTURE_AND_PREVIEW:
            if (!check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
                Log.d(TAG," SYNC_PIC_CAPTURE_AND_PREVIEW");
                INFO_MENU_STATE(cur_menu,cam_state);
                cam_state = STATE_PREVIEW;
                add_state(STATE_TAKE_CAPTURE_IN_PROCESS);
                //disp video menu before add state_record
                setCurMenu(MENU_PIC_INFO);
                send_update_light(MENU_PIC_INFO, STATE_TAKE_CAPTURE_IN_PROCESS, INTERVAL_1HZ);
            }
            break;
			
        case SYNC_PIC_STITCH_AND_PREVIEW:
            if (!check_state_in(STATE_PIC_STITCHING)) {
                Log.d(TAG, " SYNC_PIC_CAPTURE_AND_PREVIEW");
                INFO_MENU_STATE(cur_menu, cam_state);
                cam_state = STATE_PREVIEW;
                add_state(STATE_PIC_STITCHING);
                //disp video menu before add state_record
                setCurMenu(MENU_PIC_INFO);
                send_update_light(MENU_PIC_INFO, STATE_PIC_STITCHING, INTERVAL_5HZ);
            }
            break;
			
        case SYNC_LIVE_AND_PREVIEW:
            Log.d(TAG,"SYNC_LIVE_AND_PREVIEW for state 0x%x",cam_state);

			// not sync in state_live and state_live_connecting
            if (!check_state_in(STATE_LIVE)) {
                //must before add live_state for keeping state live_connecting avoiding recalculate time 170804
                set_update_mid();
                cam_state = STATE_PREVIEW;
                add_state(STATE_LIVE);
                setCurMenu(MENU_LIVE_INFO);
                Log.d(TAG," set_update_mid b");
            }
            break;
			
        case SYNC_LIVE_CONNECT_AND_PREVIEW:
            if (!check_state_in(STATE_LIVE_CONNECTING)) {
                cam_state = STATE_PREVIEW;
                add_state(STATE_LIVE_CONNECTING);
                setCurMenu(MENU_LIVE_INFO);
            }
            break;
			
        case START_QRING:
            add_state(STATE_START_QRING);
            break;
		
        case START_QR_SUC:
            if (!check_state_in(STATE_START_QR)) {
                add_state(STATE_START_QR);
            }
            break;
			
        case START_QR_FAIL:
            rm_state(STATE_START_QRING);
            Log.d(TAG, "cur menu %d back menu %d",
                  cur_menu, get_back_menu(cur_menu));
            disp_sys_err(type, get_back_menu(cur_menu));
            break;
			
        case STOP_QRING:
            add_state(STATE_STOP_QRING);
            break;
		
        case STOP_QR_SUC:
            rm_state(STATE_STOP_QRING);
            minus_cam_state(STATE_START_QR);
            break;
			
        case STOP_QR_FAIL:
            rm_state(STATE_STOP_QRING);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case QR_FINISH_CORRECT:
            play_sound(SND_QR);
            minus_cam_state(STATE_START_QR);
            break;
			
        case QR_FINISH_ERROR:
            rm_state(STATE_START_QR);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case QR_FINISH_UNRECOGNIZE:
            rm_state(STATE_START_QR);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case CAPTURE_ORG_SUC:
            Log.d(TAG, "rec capture org suc");
            break;
		
        case CALIBRATION_ORG_SUC:
            Log.d(TAG,"rec calibration org suc");
            break;

        /* 根据Customer参数重新修改配置: 比如：Origin, RTS, 剩余空间 */
        case SET_CUS_PARAM:
            switch (cur_menu) {
                case MENU_PIC_INFO:
                case MENU_PIC_SET_DEF:
                    if (mControlAct) {
                        /* 将该参数直接拷贝给自身的customer */
                        int iIndex = getMenuSelectIndex(MENU_PIC_SET_DEF);
                        PicVideoCfg* curCfg = mPicAllItemsList.at(iIndex);
                        if (curCfg) {
                            if (!strcmp(curCfg->pItemName, TAKE_PIC_MODE_CUSTOMER)) {
                                Log.d(TAG, "update customer arguments now...");
                                memset(curCfg->pStAction, 0, sizeof(ACTION_INFO));
                                memcpy(curCfg->pStAction, mControlAct.get(), sizeof(ACTION_INFO));
                                Log.d(TAG, "[%s:%d] new customer arg mode[%d], mime[%d]",
                                        __FILE__, __LINE__, curCfg->pStAction->mode,
                                        curCfg->pStAction->stOrgInfo.mime);

                                /* 更新底部空间及右侧 */
                            }
                        }
                    } else {
                        Log.d(TAG, "[%s: %d] can't update customer argument to bottom space", __FILE__, __LINE__);
                    }
                    Log.d(TAG, "[%s: %d]", __FILE__, __LINE__);
                    break;
                
                default:    /* TODO: 2018年8月3日 */
                    break;
            }
            break;

        case SET_SYS_SETTING:
        case STITCH_PROGRESS:
            Log.d(TAG, "do nothing for %d", type);
            break;
		
        case TIMELPASE_COUNT:
            INFO_MENU_STATE(cur_menu, cam_state)
            Log.d(TAG, "tl_count %d", tl_count);

            if (!check_state_in(STATE_RECORD)) {
                Log.e(TAG," TIMELPASE_COUNT cam_state 0x%x", cam_state);
            } else {
                disp_tl_count(tl_count);
            }
            break;

//        case POWER_OFF_SUC:
//            break;
//        case POWER_OFF_FAIL:
//            break;

        case START_GYRO:
            if (!check_state_in(STATE_START_GYRO)) {
                add_state(STATE_START_GYRO);
            }
            break;

        case START_GYRO_SUC:
            if (check_state_in(STATE_START_GYRO)) {
                minus_cam_state(STATE_START_GYRO);
            }
            break;

        case START_GYRO_FAIL:
            rm_state(STATE_START_GYRO);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;

        case START_NOISE:
            if (!check_state_in(STATE_NOISE_SAMPLE)) {
                setCurMenu(MENU_NOSIE_SAMPLE);
                add_state(STATE_NOISE_SAMPLE);
            }
            break;

        case START_LOW_BAT_SUC:
            INFO_MENU_STATE(cur_menu,cam_state)
            cam_state = STATE_IDLE;
            if (MENU_LOW_BAT != cur_menu) {
                setCurMenu(MENU_LOW_BAT);
            } else {
                setLightDirect(BACK_RED|FRONT_RED);
            }
            mControlAct = nullptr;
            break;

        case START_LOW_BAT_FAIL:
            cam_state = STATE_IDLE;
            INFO_MENU_STATE(cur_menu,cam_state)
            if (MENU_LOW_BAT != cur_menu) {
                setCurMenu(MENU_LOW_BAT);
            } else {
                setLightDirect(BACK_RED|FRONT_RED);
            }
            mControlAct = nullptr;
            break;

        case START_NOISE_SUC:
            if (check_state_in(STATE_NOISE_SAMPLE)) {
                minus_cam_state(STATE_NOISE_SAMPLE);
            }
            break;

        case START_NOISE_FAIL:
            rm_state(STATE_NOISE_SAMPLE);
            disp_sys_err(type, get_back_menu(MENU_NOSIE_SAMPLE));
            break;

        case START_BLC:
            set_oled_power(0);
            break;

        case STOP_BLC:
            set_oled_power(1);
            break;

        default:
            break;
    }
    return 0;
}



/*************************************************************************
** 方法名称: exitAll
** 方法功能: 退出消息循环
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::exitAll()
{
    mLooper->quit();
}

void MenuUI::disp_str_fill(const u8 *str, const u8 x, const u8 y, bool high)
{
    mOLEDModule->ssd1306_disp_16_str_fill(str,x,y,high);
}

void MenuUI::disp_str(const u8 *str,const u8 x,const u8 y, bool high,int width)
{
    mOLEDModule->ssd1306_disp_16_str(str,x,y,high,width);
}

void MenuUI::clear_icon(u32 type)
{
    mOLEDModule->clear_icon(type);
}

void MenuUI::disp_icon(u32 type)
{
    mOLEDModule->disp_icon(type);
}

void MenuUI::dispIconByLoc(const ICON_INFO* pInfo)
{
    mOLEDModule->disp_icon(pInfo);
}


void MenuUI::dispSetItem(struct stSetItem* pItem, bool iSelected)
{
    /* 根据设置项当前的值来选择显示的图标及位置 */
    if (pItem->iCurVal < 0 || pItem->iCurVal > pItem->iItemMaxVal) {
        Log.e(TAG, "[%s:%d] Invalid setting item [%s], curVal[%d]", __FILE__, __LINE__, pItem->pItemName, pItem->iCurVal);
    } else {

        Log.e(TAG, "[%s:%d] dispSetItem item name [%s], curVal[%d] selected[%s]", __FILE__, __LINE__, 
                        pItem->pItemName, pItem->iCurVal, (iSelected == true) ? "yes": "no");
        ICON_INFO tmpIconInfo;
        tmpIconInfo.x = pItem->stPos.xPos;
        tmpIconInfo.y = pItem->stPos.yPos;
        tmpIconInfo.w = pItem->stPos.iWidth;
        tmpIconInfo.h = pItem->stPos.iHeight;
        if (iSelected) {
           tmpIconInfo.dat = pItem->stLightIcon[pItem->iCurVal];
        } else {
           tmpIconInfo.dat = pItem->stNorIcon[pItem->iCurVal];
        }
       mOLEDModule->disp_icon(&tmpIconInfo);
    }
}


void MenuUI::disp_ageing()
{
    clear_area();
    disp_str((const u8 *)"Ageing...",8,16);
}



int MenuUI::get_dev_type_index(char *type)
{
    int storage_index;
    if (strcmp(type, dev_type[SET_STORAGE_SD]) == 0) {
        storage_index = SET_STORAGE_SD;
    } else if (strcmp(type, dev_type[SET_STORAGE_USB]) == 0) {
        storage_index = SET_STORAGE_USB;
    } else {
        Log.e(TAG, "error dev type %s ", type);
    }
    return storage_index;
}

void MenuUI::disp_dev_msg_box(int bAdd, int type, bool bChange)
{
    Log.d(TAG, "bAdd %d type %d", bAdd, type);

    if (bChange) {
        send_save_path_change();
    } 

	switch (bAdd) {
        //add
        case ADD:
            if (type == SET_STORAGE_SD) {
                disp_msg_box(DISP_SDCARD_ATTACH);
            } else {
                disp_msg_box(DISP_USB_ATTACH);
            }
            break;
			
        //delete
        case REMOVE:
            if (type == SET_STORAGE_SD) {
                disp_msg_box(DISP_SDCARD_DETTACH);
            } else {
                disp_msg_box(DIPS_USB_DETTACH);
            }
            break;
			
        default:
            Log.w(TAG, "strange bAdd %d type %d\n", bAdd, type);
            break;
    }
}


void MenuUI::set_mdev_list(std::vector <sp<Volume>> &mList)
{
    int dev_change_type;

    // 0 -- add, 1 -- remove, -1 -- do nothing
    int bAdd = CHANGE;
    bool bDispBox = false;
    bool bChange = true;
	
    if (bFirstDev) {	/* 第一次发送 */
		
        bFirstDev = false;
        mLocalStorageList.clear();
	
        switch (mList.size()) {
            case 0:
                mSavePathIndex = -1;
                break;
			
            case 1: {
                sp<Volume> dev = sp<Volume>(new Volume());
                memcpy(dev.get(), mList.at(0).get(), sizeof(Volume));
                mLocalStorageList.push_back(dev);
                mSavePathIndex = 0;
				break;
            }
                
			
            case 2:
                for (u32 i = 0; i < mList.size(); i++) {
                    sp<Volume> dev = sp<Volume>(new Volume());
                    memcpy(dev.get(), mList.at(i).get(), sizeof(Volume));
                    if (get_dev_type_index(mList.at(i)->dev_type) == SET_STORAGE_USB) {
                        mSavePathIndex = i;
                    }
                    mLocalStorageList.push_back(dev);
                }
                break;
				
            default:
                Log.d(TAG, "strange bFirstDev mList.size() is %d", mList.size());
                break;
        }
        send_save_path_change();	/* 将当前选中的存储路径发送给Camerad */
    } else {
        Log.d(TAG, " new save list is %d , org save list %d", mList.size(), mLocalStorageList.size());
        if (mList.size() == 0) {
            bAdd = REMOVE;
            mSavePathIndex = -1;
            if (mLocalStorageList.size() == 0) {
                Log.d(TAG,"strange save list size (%d %d)",mList.size(),mLocalStorageList.size());
                mLocalStorageList.clear();
                send_save_path_change();
                bChange = false;
            } else {
                dev_change_type = get_dev_type_index(mLocalStorageList.at(0)->dev_type);
                mLocalStorageList.clear();
                bDispBox = true;
            }
        } else {
            if (mList.size() < mLocalStorageList.size()) {
                //remove
                bAdd = REMOVE;
                switch (mList.size()) {
                    case 1:
                        if (get_dev_type_index(mList.at(0)->dev_type) == SET_STORAGE_SD) {
                            dev_change_type = SET_STORAGE_USB;
                            mSavePathIndex = 0;
                        } else {
                            dev_change_type = SET_STORAGE_SD;
                            bChange = false;
                        }
                        mLocalStorageList.clear();
                        mLocalStorageList.push_back(mList.at(0));
                        bDispBox = true;
                        break;

                    default:
                        Log.d(TAG,"2strange save list size (%d %d)",mList.size(),mLocalStorageList.size());
                        break;
                }
            } else if (mList.size() > mLocalStorageList.size()) {	//add
                bAdd = ADD;
                if (mLocalStorageList.size() == 0) {
                    dev_change_type = get_dev_type_index(mList.at(0)->dev_type);
                    sp<Volume> dev = sp<Volume>(new Volume());
                    memcpy(dev.get(), mList.at(0).get(), sizeof(Volume));
//                    snprintf(new_path, sizeof(new_path), "%s", mList.at(0)->path);
                    mSavePathIndex = 0;
                    mLocalStorageList.push_back(dev);
                    bDispBox = true;
                } else {
//old is usb
//                    CHECK_EQ(mList.size(), 2);
                    switch (mList.size()) {
                        case 2:
                            dev_change_type = get_dev_type_index(mLocalStorageList.at(0)->dev_type);
                            if (dev_change_type == SET_STORAGE_USB) {
                                // new insert is sd
                                dev_change_type = SET_STORAGE_SD;
                            } else {
                                // new insert is usb
                                dev_change_type = SET_STORAGE_USB;
                            }
							
                            mLocalStorageList.clear();
                            for (unsigned int i = 0; i < mList.size(); i++) {
                                sp<Volume> dev = sp<Volume>(new Volume());
                                memcpy(dev.get(), mList.at(i).get(), sizeof(Volume));
                                if (dev_change_type == SET_STORAGE_USB && get_dev_type_index(mList.at(i)->dev_type) == SET_STORAGE_USB) {
                                    mSavePathIndex = i;
                                    bChange = true;
                                }
                                mLocalStorageList.push_back(dev);
                            }
                            bDispBox = true;
                            break;

                        default:
                            Log.d(TAG, "3strange save list size (%d %d)",mList.size(),mLocalStorageList.size());
                            break;
                    }
                }
            } else {
                Log.d(TAG, "5strange save list size (%d %d)", mList.size(), mLocalStorageList.size());
            }
        }

        if (mLocalStorageList.size() == 0 ) {
            if (mSavePathIndex != -1) {
                mSavePathIndex = -1;
            }
        } else if (mSavePathIndex >= (int)mLocalStorageList.size()) {
            mSavePathIndex = mLocalStorageList.size() - 1;

            Log.e(TAG, "force path select %d ",mSavePathIndex);
        }
		
        if (bDispBox) {
            disp_dev_msg_box(bAdd, dev_change_type, bChange);
        }
    }
}


/*
 * 剩余时间减1
 */
bool MenuUI::decrease_rest_time()
{
    bool bRet = true;
    mRemainInfo->remain_sec -= 1;
    if (mRemainInfo->remain_sec < 0) {
        if (mRemainInfo->remain_min > 0) {
            mRemainInfo->remain_min -= 1;
            mRemainInfo->remain_sec = 59;
        } else if (mRemainInfo->remain_hour > 0) {
            mRemainInfo->remain_min = 59;
            mRemainInfo->remain_sec = 59;
            mRemainInfo->remain_hour -= 1;
        } else {
            Log.d(TAG, "no space to rec\n");
            memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));
            //waitting for rec no space from camerad
            bRet = false;
        }
    }
    return bRet;
}


/*
 * increase_rec_time - 录像时间增加1s
 */
void MenuUI::increase_rec_time()
{
    mRecInfo->rec_sec += 1;
    if (mRecInfo->rec_sec >= 60) {
        mRecInfo->rec_sec = 0;
        mRecInfo->rec_min += 1;
        if (mRecInfo->rec_min >= 60) {
            mRecInfo->rec_min = 0;
            mRecInfo->rec_hour += 1;
        }
    }
}

void MenuUI::flick_light()
{
	// Log.d(TAG, "last_light 0x%x fli_light 0x%x", last_light, fli_light);
	
    if ((last_light & 0xf8) != fli_light) {
        setLight(fli_light);
    } else {
        setLight(LIGHT_OFF);
    }
}

void MenuUI::flick_low_bat_lig()
{
//    if(last_back_light != BACK_RED)
//    {
//        mOLEDLight->setLight_val(BACK_RED|FRONT_RED);
//    }
//    else
//    {
//        mOLEDLight->setLight_val(LIGHT_OFF);
//    }
}


/*
 * disp_mid - 显示已经录像的时间(时分秒)
 */
void MenuUI::disp_mid()
{
    char disp[32];
    snprintf(disp, sizeof(disp), "%02d:%02d:%02d", mRecInfo->rec_hour, mRecInfo->rec_min, mRecInfo->rec_sec);

//    Log.d(TAG," disp rec mid %s tl_count %d",disp,tl_count);
    if (tl_count == -1) {   /* 非timelapse模式,显示已经录像的时间 */
        disp_str((const u8 *)disp, 37, 24);
    }
}

bool MenuUI::is_bat_low()
{
    bool ret = false;
    if (mBatInterface->isSuc() && !m_bat_info_->bCharge && m_bat_info_->battery_level <= BAT_LOW_VAL) {
        ret = true;
    }
    return ret;
}


void MenuUI::func_low_bat()
{
    send_option_to_fifo(ACTION_LOW_BAT, REBOOT_SHUTDOWN);
}

void MenuUI::setLightDirect(u8 val)
{

#ifdef ENABLE_DEBUG_LIGHT
    Log.d(TAG, "[%s:%d] last_light 0x%x val 0x%x", __FILE__, __LINE__, last_light, val);
#endif

    if (last_light != val) {
        last_light = val;
        mOLEDLight->set_light_val(val);
    }
}

void MenuUI::setLight(u8 val)
{
#ifdef ENABLE_DEBUG_LIGHT
    Log.d(TAG, "[%s:%d] setLight 0x%x  front_light 0x%x", __FILE__, __LINE__, val, front_light);
#endif

    if (mProCfg->get_val(KEY_LIGHT_ON) == 1) {
        setLightDirect(val | front_light);
    }
}



bool MenuUI::check_rec_tl()
{
    bool ret = false;
    if (mControlAct != nullptr) {
        // Log.d(TAG,"mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int is %d",
        // mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
        if (mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
            ret = true;
        }
    }  else {
        int iIndex = getMenuSelectIndex(MENU_VIDEO_SET_DEF);
        struct stPicVideoCfg* pTmpCfg = mVidAllItemsList.at(iIndex);
        if (pTmpCfg) {
            if (!strcmp(pTmpCfg->pItemName, TAKE_VID_MOD_CUSTOMER)) {
                Log.d(TAG, "mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",
                    mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);

                if (mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
                    if (mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->size_per_act == 0) {
                        mProCfg->get_def_info(KEY_ALL_VIDEO_DEF)->size_per_act = 10;
                    }
                    ret = true;
                }
            }
        }

        #if 0
        if (item >= 0 && item < VID_CUSTOM) {
        // Log.d(TAG,"mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
            // if (mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
            //     ret = true;
            // }
        } else if (VID_CUSTOM == item) {
        } else {
            ERR_ITEM(item);
        }
        #endif
    }

    if (!ret) {
        tl_count = -1;
    }
    return ret;
}



void MenuUI::procUpdateIp(const char* ipAddr)
{
	memset(mLocalIpAddr, 0, sizeof(mLocalIpAddr));
	strcpy(mLocalIpAddr, ipAddr);
	uiShowStatusbarIp();
}


void MenuUI::handleDispStrTypeMsg(sp<DISP_TYPE>& disp_type)
{
	switch (cur_menu) {
		case MENU_DISP_MSG_BOX:
		case MENU_SPEED_TEST:
			procBackKeyEvent();
			break;
		#if 1
		case MENU_LOW_BAT:
		if (!(disp_type->type == START_LOW_BAT_SUC || START_LOW_BAT_FAIL == disp_type->type || disp_type->type == RESET_ALL || disp_type->type == START_FORCE_IDLE)) {
		  Log.d(TAG,"MENU_LOW_BAT not allow (0x%x %d)",cam_state,disp_type->type);
		  return;
		}
		#endif
		
		default:
			// get http req before getting low bat protect in flask 170922
			if (check_state_in(STATE_LOW_BAT)) {				
				Log.d(TAG, "STATE_LOW_BAT not allow (0x%x %d)", cam_state, disp_type->type);
			} else if(disp_type->type != RESET_ALL) {
				exit_sys_err();
			}
			break;
	}

	// add param from controller or qr scan
	if (disp_type->qr_type != -1) {
		CHECK_NE(disp_type->mAct,nullptr);
		add_qr_res(disp_type->qr_type, disp_type->mAct, disp_type->control_act);
	} else if (disp_type->tl_count != -1) {
		set_tl_count(disp_type->tl_count);

	} else if (disp_type->mSysSetting != nullptr) { /* 系统设置不为空 */

		set_sys_setting(disp_type->mSysSetting);    /* 更新设置(来自客户端) */


#ifdef ENABLE_MENU_STITCH_BOX        
	} else if (disp_type->mStichProgress != nullptr) {
		disp_stitch_progress(disp_type->mStichProgress);
#endif
	} else {
		Log.d(TAG, "nothing");
	}

	oled_disp_type(disp_type->type);

}

void MenuUI::handleDispErrMsg(sp<ERR_TYPE_INFO>& mErrInfo)
{
	switch (cur_menu) {
		case MENU_DISP_MSG_BOX:
		case MENU_SPEED_TEST:
			procBackKeyEvent();
			break;
		
		default:
			if (mErrInfo->type != RESET_ALL) {
				exit_sys_err();
			}
			break;
	}
	oled_disp_err(mErrInfo);
}



/*************************************************************************
** 方法名称: handleLongKeyMsg
** 方法功能: 处理长按键的消息
** 入口参数: 
**      key - 被按下的键值
**      ts  - 按下的时长
** 返回值: 
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::handleLongKeyMsg(int key, int64 ts)
{
	if (ts == last_key_ts && last_down_key == key) {
		Log.d(TAG," long press key 0x%x",key);
		if (key == OLED_KEY_POWER) {
			sys_reboot();
		}
	}
}



/*************************************************************************
** 方法名称: handleDispLightMsg
** 方法功能: 处理灯闪烁消息
** 入口参数: 
**      menu - 所处的菜单ID
**      state  - 状态
**      interval - 下次发送消息的间隔
** 返回值: 
** 调 用: handleMessage
**
*************************************************************************/
void MenuUI::handleDispLightMsg(int menu, int state, int interval)
{
	// Log.d(TAG," (%d %d	%d %d)", menu, state, interval, cap_delay);
	bSendUpdate = false;

	unique_lock<mutex> lock(mutexState);
	switch (menu) {
		case MENU_PIC_INFO:
			if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
	
				if (mTakePicDelay == 0) {

                    if (mNeedSendAction) {  /* 暂时用于处理客户端发送拍照请求时，UI不发送拍照请求给camerad */
                        send_option_to_fifo(ACTION_PIC);
                    }

                    if (menu == cur_menu) {
						disp_shooting();
					}

				} else {

					if (menu == cur_menu) {
						disp_sec(mTakePicDelay, 52, 24);	/* 显示倒计时的时间 */
					}

                    Log.d(TAG, "MENU_PIC_INFO: sound id = %d", SND_ONE_T);

					/* 倒计时时根据当前cap_dela y的值,只在	CAPTURE中播放一次, fix bug1147 */
					send_update_light(menu, state, INTERVAL_1HZ, true, SND_ONE_T);

				}
                mTakePicDelay--;

			} else if (check_state_in(STATE_PIC_STITCHING)) {
				send_update_light(menu, state, INTERVAL_5HZ, true);
			} else {
				Log.d(TAG, "update pic light error state 0x%x", cam_state);
				setLight();
			}
			break;

					
		case MENU_CALIBRATION:
			
			Log.d(TAG, "GyroCal is %d, cur menu [%s]", mGyroCalcDelay, getCurMenuStr(cur_menu));
			
			if ((cam_state & STATE_CALIBRATING) == STATE_CALIBRATING) {
				if (mGyroCalcDelay <= 0) {
					send_update_light(menu, state, INTERVAL_5HZ, true);
					if (mGyroCalcDelay == 0) {
						if (cur_menu == menu) {	/* 当倒计时完成后才会给Camerad发送"校验消息" */
							disp_calibration_res(2);

                            if (mCalibrateSrc) {    /* 来自UI的拼接请求来需要发送 */
							    send_option_to_fifo(ACTION_CALIBRATION);
                            }
						}
					}
				} else {	/* 大于0时,显示倒计时 */

					send_update_light(menu, state, INTERVAL_1HZ, true, SND_ONE_T);				
					if (cur_menu == menu) {		/* 在屏幕上显示倒计时 */
						disp_calibration_res(3, mGyroCalcDelay);
					}
				}
				mGyroCalcDelay--;
			} else {
				Log.e(TAG, "update calibration light error state 0x%x", cam_state);
				setLight();
			}
			break;
					
		default:
			break;
	}

}



/*
 * 将消息丢给NetManager进行配置
 */
void MenuUI::handleorSetWifiConfig(sp<WifiConfig> &mConfig)
{
	Log.d(TAG, ">>>> handleConfigWifiMsg");
    sp<ARMessage> msg;
	const char* pWifName = NULL;
	const char* pWifPasswd = NULL;
	const char* pWifMode = NULL;
	const char* pWifChannel = NULL;
	

	/* SSID */
	if (strcmp(mConfig->cApName, "none") == 0) {	/* 没有传WIFI名称,使用默认的格式:Insta360-Pro2-XXXXX */
		pWifName = property_get(DEFAULT_WIFI_AP_SSID);
		if (NULL == pWifName) {
			pWifName = DEFAULT_WIFI_AP_SSID;
		}
		memset(mConfig->cApName, 0, sizeof(mConfig->cApName));
		strncpy(mConfig->cApName, pWifName, (strlen(pWifName) > DEFAULT_NAME_LEN)?DEFAULT_NAME_LEN: strlen(pWifName));
	}

	/* INTERFACE_NAME */
	if (strcmp(mConfig->cInterface, WLAN0_NAME)) {
		memset(mConfig->cInterface, 0, sizeof(mConfig->cInterface));
		strcpy(mConfig->cInterface, WLAN0_NAME);
	}

	/* MODE */
	if (mConfig->iApMode == WIFI_HW_MODE_AUTO) {
		mConfig->iApMode = WIFI_HW_MODE_G;
	}

	if (mConfig->iApMode < WIFI_HW_MODE_AUTO || mConfig->iApMode > WIFI_HW_MODE_N) {
		mConfig->iApMode = WIFI_HW_MODE_G;
	}

	if (mConfig->iApMode == WIFI_HW_MODE_B || mConfig->iApMode == WIFI_HW_MODE_G) {
		mConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_BG;
	} else {
		mConfig->iApChannel = DEFAULT_WIFI_AP_CHANNEL_NUM_AN;
	}	


	/* COMMON:
	 * HW_MODE, CHANNEL
	 */
	if (mConfig->iAuthMode == AUTH_OPEN) {	/* OPEN模式 */

	} else if (mConfig->iAuthMode == AUTH_WPA2) {	/* WPA2 */
		
		if (strcmp(mConfig->cPasswd, "none") == 0) {	/* 没有传WIFI密码，使用默认值 */
			pWifPasswd = property_get(PROP_SYS_AP_PASSWD);
			if (NULL == pWifPasswd) {
				pWifPasswd = "88888888";
			}
			memset(mConfig->cPasswd, 0, sizeof(mConfig->cPasswd));
			strncpy(mConfig->cPasswd, pWifPasswd, (strlen(pWifPasswd) > DEFAULT_NAME_LEN) ? DEFAULT_NAME_LEN: strlen(pWifPasswd));
		}


	} else {
		Log.d(TAG, "Not supported auth mode in current");
	}

	Log.d(TAG, "Send our configure to NetManager");

	msg = (sp<ARMessage>)(new ARMessage(NETM_CONFIG_WIFI_AP));
    msg->set<sp<WifiConfig>>("wifi_config", mConfig);
    NetManager::getNetManagerInstance()->postNetMessage(msg);

}



/*
 * 处理更新录像,直播的时间
 */
void MenuUI::handleUpdateMid()
{
    bSendUpdateMid = false;
    unique_lock<mutex> lock(mutexState);

    if (check_state_in(STATE_RECORD)) {         /* 录像状态 */

        send_update_mid_msg(INTERVAL_1HZ);  /* 1s后发送更新灯消息 */

        if (!check_state_in(STATE_STOP_RECORDING)) {    /* 非录像停止状态 */
            if (tl_count == -1) {               /* 非timelpase拍摄 */

                increase_rec_time();            /* 增加已录像的时间 +1s */
                                
                if (decrease_rest_time()) {     /* 录像剩余时间减1s */
                    // hide while stop recording
                    if (cur_menu == MENU_VIDEO_INFO) {  /* 如果是在录像界面,更新录像时间和剩余时间到UI */
                        disp_mid();
                        dispBottomLeftSpace();
                    }
                }
            }
        }
        flick_light();  /* 闪烁灯 */

    } else if (check_state_in(STATE_LIVE)) {     /* 直播状态 */
        send_update_mid_msg(INTERVAL_1HZ);
        if (!check_state_in(STATE_STOP_LIVING)) {   /* 非停止直播状态 */
            increase_rec_time();
            if (cur_menu == MENU_LIVE_INFO) {
                disp_mid();
            }
        }
        flick_light();

    } else if (check_state_in(STATE_LIVE_CONNECTING)) {
        Log.d(TAG,"cancel send msg　in state STATE_LIVE_CONNECTING");
        setLight();
    } else {
        ERR_MENU_STATE(cur_menu, cam_state);
        setLight();
    }
}


/*************************************************************************
** 方法名称: handleMessage
** 方法功能: 消息处理
** 入口参数: msg - 消息对象强指针引用
** 返回值: 无 
** 调 用: 消息处理线程 MENU_TOP
**
*************************************************************************/
void MenuUI::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();

#ifdef DEBUG_UI_MSG	
    Log.d(TAG, "UI Core get msg: what[%d]", what);
#endif

    if (UI_EXIT == what) {  /* 退出消息循环 */
        exitAll();
    } else {
        switch (what) {

            case OLED_DISP_STR_TYPE: {  /* 显示指定的页面(状态) */
                {
                    unique_lock<mutex> lock(mutexState);
                    sp<DISP_TYPE> disp_type;
                    CHECK_EQ(msg->find<sp<DISP_TYPE>>("disp_type", &disp_type), true);
					
                    Log.d(TAG, "OLED_DISP_STR_TYPE (%d %d %d %d 0x%x)",
								disp_type->qr_type,         // -1 
								disp_type->type,            // START_CALIBRATIONING
								disp_type->tl_count,        // -1
								cur_menu,                   // 1
								cam_state);                 // 0X8
					
					handleDispStrTypeMsg(disp_type);
                }
				break;
            }
					
            case UI_MSG_DISP_ERR_MSG: {     /* 显示错误消息 */
                unique_lock<mutex> lock(mutexState);
                sp<ERR_TYPE_INFO> mErrInfo;
                CHECK_EQ(msg->find<sp<ERR_TYPE_INFO>>("err_type_info", &mErrInfo),true);
				handleDispErrMsg(mErrInfo);
				break;
            }
               
			
            case UI_MSG_KEY_EVENT: {	/* 短按键消息处理 */
                int key = -1;
                CHECK_EQ(msg->find<int>("oled_key", &key), true);
                handleKeyMsg(key);
				break;
            }
                
            case UI_MSG_LONG_KEY_EVENT: {	/* 长按键消息处理 */
                int key;
                int64 ts;
                CHECK_EQ(msg->find<int>("key", &key), true);
                CHECK_EQ(msg->find<int64>("ts", &ts), true);
				handleLongKeyMsg(key, ts);
				 break;
            }

#if 0
            case UI_UPDATE_TF: {
                CHECK_EQ(msg->find<vector<sp<Volume>>>("tf_info", &key), true);
                break;
            }               
#endif
            case UI_MSG_UPDATE_IP: {	/* 更新IP */
				sp<DEV_IP_INFO> tmpIpInfo;
				CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);

#ifdef ENABLE_DEBUG_NET
				Log.d(TAG, "UI_MSG_UPDATE_IP dev[%s], ip[%s]", tmpIpInfo->cDevName, tmpIpInfo->ipAddr);
#endif
				procUpdateIp(tmpIpInfo->ipAddr);
               	break;
            }


            case UI_MSG_CONFIG_WIFI:  {	/* 配置WIFI (UI-CORE处理) */
                sp<WifiConfig> mConfig;
                CHECK_EQ(msg->find<sp<WifiConfig>>("wifi_config", &mConfig), true);
                handleorSetWifiConfig(mConfig);
                break;
            }
                

            case UI_MSG_SET_SN: {	/* 设置SN */
                sp<SYS_INFO> mSysInfo;
                CHECK_EQ(msg->find<sp<SYS_INFO>>("sys_info", &mSysInfo), true);
                write_sys_info(mSysInfo);
                break;
            }

			/*
			 * 同步初始化
			 */
            case UI_MSG_SET_SYNC_INFO: {	/* 同步初始化信息(来自control_center) */
                exit_sys_err();
                sp<SYNC_INIT_INFO> mSyncInfo;
                CHECK_EQ(msg->find<sp<SYNC_INIT_INFO>>("sync_info", &mSyncInfo), true);
                set_sync_info(mSyncInfo);	/* 根据同步系统初始化系统参数及显示 */
                break;
            }

            case UI_DISP_INIT: {	/* 1.初始化显示消息 */
                oled_init_disp();	                    /* 初始化显示 */
                send_option_to_fifo(ACTION_REQ_SYNC);	/* 发送请求同步消息 */
                break;
            }

			/*
			 * 更新存储设备列表(消息来自DevManager线程)
			 */
            case OLED_UPDATE_DEV_INFO: {
                vector<sp<Volume>> mList;
                CHECK_EQ(msg->find<vector<sp<Volume>>>("dev_list", &mList), true);

				set_mdev_list(mList);	/* 更新存储管理器的存储设备列表 */
				
                // make send dev_list after sent save for update_bottom while current is pic ,vid menu
                send_update_dev_list(mList);	/* 给Http发送更新设备列表消息 */
                break;
            }

            case UI_UPDATE_MID:     /* 更新显示时间(只有在录像,直播的UI */
                handleUpdateMid();
				break;

			/*
			 * 低电
			 */
            case OLED_DISP_BAT_LOW:
                break;


            case UI_READ_BAT: {     /* 读取电池电量消息 */
                unique_lock<mutex> lock(mutexState);
                check_battery_change();
                break;
            }

            case UI_DISP_LIGHT: {   /* 显示灯状态消息 */
                int menu;
                int interval;
                int state;

                CHECK_EQ(msg->find<int>("menu", &menu), true);
                CHECK_EQ(msg->find<int>("interval", &interval), true);
                CHECK_EQ(msg->find<int>("state", &state), true);

				handleDispLightMsg(menu, interval, state);
				break;
            }
                			
            case UI_CLEAR_MSG__BOX:        /* 清除消息框 */
                if (cur_menu == MENU_DISP_MSG_BOX) {    /* 如果当前处于消息框菜单中,执行返回 */
                    procBackKeyEvent();
                } else {
                    Log.d(TAG, "[%s: %d] Warnning Cler MsgBox cur_menu [%s]", __FILE__, __LINE__, getCurMenuStr(cur_menu));
                }
                break;
				
            SWITCH_DEF_ERROR(what)
        }
    }
}



/*************************************************************************
** 方法名称: sys_reboot
** 方法功能: 关机或重启系统
** 入口参数: cmd - 命令值
** 返回值: 无 
** 调 用: handleLongKeyMsg
**
*************************************************************************/
void MenuUI::sys_reboot(int cmd)
{
    switch (cmd) {
        case REBOOT_NORMAL:
            send_option_to_fifo(ACTION_POWER_OFF, cmd);
            break;
		
        case REBOOT_SHUTDOWN:
            send_option_to_fifo(ACTION_POWER_OFF, cmd);
            break;
		
        SWITCH_DEF_ERROR(cmd)
    }
}



/*************************************************************************
** 方法名称: setTakePicDelay
** 方法功能: 设置拍照的倒计时时间
** 入口参数: 
**      iDelay - 倒计时值
** 返回值: 无
** 调 用: 
**
*************************************************************************/
void MenuUI::setTakePicDelay(int iDelay)
{
    Log.d(TAG, ">>>>>>>>>>> setTakePicDelay %d", iDelay);
    mTakePicDelay = iDelay;
}


/*************************************************************************
** 方法名称: getPicVidCfgNameByIndex
** 方法功能: 获取指定的容器中指定项的名称
** 入口参数: 
**      mList - 容器列表引用
**      iIndex - 索引值
** 返回值: 存在返回指定项的名称; 失败返回NULL
** 调 用: 
**
*************************************************************************/
const char* MenuUI::getPicVidCfgNameByIndex(vector<struct stPicVideoCfg*>& mList, int iIndex)
{

    if (iIndex < 0 || iIndex > mList.size() - 1) {
        Log.e(TAG, "[%s: %d] Invalid Index[%d], please check", __FILE__, __LINE__, iIndex);
    } else {
        struct stPicVideoCfg* pTmpCfg = mList.at(iIndex);
        if (pTmpCfg) {
            return pTmpCfg->pItemName;
        } else {
            Log.e(TAG, "[%s: %d] Invalid Index[%d], please check", __FILE__, __LINE__, iIndex);
        }
    }
    return NULL;
}



/*************************************************************************
** 方法名称: check_battery_change
** 方法功能: 检测电池的变化
** 入口参数: bUpload - 
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
bool MenuUI::checkLiveNeedSave()
{
    bool ret = false;

    if (check_live()) {

        if (mControlAct != nullptr) {
            ret = check_live_save(mControlAct.get());
        } else {

            int iIndex = getMenuSelectIndex(MENU_LIVE_SET_DEF);
            Log.d(TAG, "[%s: %d] checkLiveNeedSave %d", __FILE__, __LINE__, iIndex);
            
            const char* pIndexName = getPicVidCfgNameByIndex(mLiveAllItemsList, iIndex);
            if (pIndexName) {

                /* UI上的其他直播模式都不需要存储(除了Customer模式外) */
                if (!strcmp(pIndexName, TAKE_LIVE_MODE_CUSTOMER)) { /* Customer模式需要检查是否需要直播并存储 */
                    ret = check_live_save(mProCfg->get_def_info(KEY_ALL_LIVE_DEF));
                }
            } else {
                Log.w(TAG, "[%s: %d] index[%d] item int List Not named", __FILE__, __LINE__, iIndex);
            }
        }
    }
    return ret;
}



/*************************************************************************
** 方法名称: check_battery_change
** 方法功能: 检测电池的变化
** 入口参数: bUpload - 
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
bool MenuUI::check_battery_change(bool bUpload)
{
    bool bUpdate = mBatInterface->read_bat_update(m_bat_info_);
		
    if (bUpdate || bUpload) {   /* 电池电量需要更新或者需要上报 */
		
        oled_disp_battery();	/* 显示电池及电量信息 */

		/* 发送更新电池电量消息 */
        sp<ARMessage> msg = mNotify->dup();
        sp<BAT_INFO> info = sp<BAT_INFO>(new BAT_INFO());

        memcpy(info.get(), m_bat_info_.get(), sizeof(BAT_INFO));

        //update battery in oled
        msg->set<int>("what", UPDATE_BAT);
        msg->set<sp<BAT_INFO>>("bat_info", info);
        msg->post();
    }

    if (is_bat_low()) { /* 电池电量低 */

#ifdef OPEN_BAT_LOW

        if (cur_menu != MENU_LOW_BAT) { /* 当前处于非电量低菜单 */

            Log.d(TAG, "[%s: % ] bat low menu[%s] %d state 0x%x bStiching %d", __FILE__, __LINE__,
                    getCurMenuStr(cur_menu), cam_state, bStiching);

            if (check_state_in(STATE_RECORD) || checkLiveNeedSave() || bStiching) {
                setCurMenu(MENU_LOW_BAT, MENU_TOP);
                add_state(STATE_LOW_BAT);
                func_low_bat();
                bStiching = false;
            }
        }

#endif
    }

    send_delay_msg(UI_READ_BAT, BAT_INTERVAL);  /* 给UI线程发送读取电池电量的延时消息 */

    return bUpdate;
}


/*************************************************************************
** 方法名称: get_battery_charging
** 方法功能: 检测电池是否正在充电
** 入口参数:  
**      bCharge - 是否充电 
** 返回值: 是返回true; 否则返回false
** 调 用: 
**
*************************************************************************/
int MenuUI::get_battery_charging(bool *bCharge)
{
    return mBatInterface->read_charge(bCharge);
}



/*************************************************************************
** 方法名称: read_tmp
** 方法功能: 读取电池的温度
** 入口参数:  
**      int_tmp - 温度值
**      tmp - 温度值
** 返回值: 成功读取返回0;否则返回1
** 调 用: 
**
*************************************************************************/
int MenuUI::read_tmp(double *int_tmp, double *tmp)
{
    return mBatInterface->read_tmp(int_tmp, tmp);
}




void MenuUI::deinit()
{
    Log.d(TAG, "deinit\n");
	
    setLightDirect(LIGHT_OFF);
    mDevManager = nullptr;

	mNetManager = nullptr;

    sendExit();

    Log.d(TAG, "deinit2");
}
