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

#include <util/icon_ascii.h>

#include <trans/fifo.h>

#include <icon/pic_mode_select.h>


using namespace std;


#define TAG "MenuUI"


//#define USE_OLD_NET
#define ENABLE_LIGHT

#define ENABLE_SOUND

#define CAL_DELAY (5)
#define SN_LEN (14)
//sometimes met bat jump to 3 ,but in fact not
#define BAT_LOW_VAL (3)
//#define BAT_LOW_PROTECT (5)
#define MAX_ADB_TIMES (5)
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
enum
{
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

enum
{
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

enum
{
    OLED_DISP_STR_TYPE,//0
    OLED_DISP_ERR_TYPE,
    OLED_GET_KEY,
    OLED_GET_LONG_PRESS_KEY,
    OLED_DISP_IP,
    OLED_DISP_BATTERY,//5
    //set config wifi same thread as oled wifi key action
    OLED_CONFIG_WIFI,
    OLED_SET_SN,
    OLED_SYNC_INIT_INFO,
//    OLED_CHECK_LIVE_OR_REC,
    OLED_UPDATE_DEV_INFO,
    OLED_UPDATE_MID, //10
    OLED_DISP_BAT_LOW,
    OLED_UPDATE_CAPTURE_LIGHT,
    OLED_UPDATE_CAL_LIGHT,
    OLED_CLEAR_MSG_BOX,
    OLED_READ_BAT, //15
    OLED_DISP_LIGHT,
    //disp oled info at start
    OLED_DISP_INIT,
    OLED_EXIT,
};

enum
{
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



#define SYS_TMP "/home/nvidia/insta360/etc/sys_tmp"

#define GOOGLE_8K_5F


const char *rom_ver_file = "/home/nvidia/insta360/etc/pro_version";
const char *build_ver_file = "/home/nvidia/insta360/etc/pro_build_version";

//ssid conf
const char *wlan_wifi_conf = "/home/nvidia/insta360/etc/wpa_supplicant.conf";

//for ap
const char *host_apd_conf = "/home/nvidia/insta360/etc/hostapd.conf";
//const char *sys_file_def = "/data/etc/sys_info_def";

typedef struct _sys_read_ {
    const char *key;
    const char *header;
    const char *file_name;
}SYS_READ;

static const SYS_READ astSysRead[] = {
    { "sn", "sn=", "/home/nvidia/insta360/etc/sn"},
    { "uuid", "uuid=", "/home/nvidia/insta360/etc/uuid"},
};

//pic ,vid, live def
enum
{
    //normal stich
//#ifndef ONLY_PIC_FLOW
//    PIC_8K_3D_SAVE,
//#endif
    //optical flow stich
    PIC_8K_3D_SAVE_OF,
    PIC_8K_PANO_SAVE_OF,
    PIC_8K_PANO_SAVE,
#ifdef OPEN_HDR_RTS
    PIC_HDR_RTS,
#endif
    PIC_HDR,
    PIC_BURST,
    PIC_RAW,
    PIC_CUSTOM,
    PIC_DEF_MAX,
};



/* 6+1
 * 录像 - 
 */
enum {
	VID_8K_30FPS_3D,
	VID_8K_60FPS,
	VID_8K_30FPS,
	VID_8K_5FPS_STREETVIEW,
	VID_6K_60FPS_3D,
	VID_6K_30FPS_3D,
	VID_4K_120FPS_3D,
	VID_4K_60FPS_3D,
	VID_4K_30FPS_3D,
	VID_4K_60FPS_RTS,
	VID_4K_30FPS_RTS,
	VID_4K_30FPS_3D_RTS,
	VID_2_5K_60FPS_3D_RTS,
	VID_AERIAL,
};


/*
 * 6+1
 * 直播
 */
enum {
	LIVE_4K_30FPS,
	LIVE_4K_30FPS_3D,
	LIVE_4K_30FPS_HDMI,
	LIVE_4K_30FPS_3D_HDMI
};


enum {
    //video
    VID_8K_50M_30FPS_PANO_RTS_OFF,
#ifdef GOOGLE_8K_5F
//    VID_8K_5FPS_RTS,//for google 170921
    VID_8K_5FPS,
#endif
    VID_6K_50M_30FPS_3D_RTS_OFF,
    VID_4K_50M_120FPS_PANO_RTS_OFF,
    VID_4K_20M_60FPS_3D_RTS_OFF,
    VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON,
    VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON,
    VID_CUSTOM,
    VID_DEF_MAX,
};

enum {
    //live
#ifdef LIVE_ORG
    LIVE_ORIGIN,
#endif
    VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON,
    VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON,
    //hdmi on
    VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI,
    VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI,
    VID_ARIAL,
    LIVE_CUSTOM,
    LIVE_DEF_MAX,
};

enum
{
    MAINMENU_PIC,
    MAINMENU_VIDEO,
    MAINMENU_LIVE,
    MAINMENU_WIFI,
    MAINMENU_CALIBRATION,
    MAINMENU_SETTING,
    MAINMENU_MAX,
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

#define PAGE_MAX (3)

#define INTERVAL_1HZ 	(1000)

//#define INTERVAL_3HZ (333)
#define INTERVAL_5HZ 		(200)

#define FLASH_LIGHT			BACK_BLUE
#define BAT_INTERVAL		(5000)
#define AVAIL_SUBSTRACT		(1024)

enum {
    SND_SHUTTER,
    SND_COMPLE,
    SND_FIVE_T,
    SND_QR,
    SND_START,
    SND_STOP,
    SND_THREE_T,
    SND_ONE_T,
	SND_1S_T,
	SND_3S_T,
	SND_5S_T,
	SND_10S_T,
	SND_20S_T,
	SND_30S_T,
	SND_40S_T,
	SND_50S_T,
	SND_60S_T,
    SND_MAX_NUM,
};

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


static int get_cap_to_sound_index(int delay)
{
	int sound_index = SND_1S_T;
	switch (delay) {
	case 1:
		sound_index = SND_1S_T;
		break;

	case 3:
		sound_index = SND_3S_T;
		break;

	case 5:
		sound_index = SND_5S_T;
		break;

	case 10:
		sound_index = SND_10S_T;
		break;	

	case 20:
		sound_index = SND_20S_T;
		break;

	case 30:
		sound_index = SND_30S_T;
		break;

	case 40:
		sound_index = SND_40S_T;
		break;	
	case 50:
		sound_index = SND_50S_T;
		break;
	case 60:
		sound_index = SND_60S_T;
		break;
	default:
		sound_index = -1;
	}
	return sound_index;
}

typedef struct _area_info_ {
    u8 x;
    u8 y;
    u8 w;
    u8 h;
} AREA_INFO;

static const AREA_INFO storage_area[] =
{
	{25,16,103,16},
	{25,32,103,16}
};

typedef struct _rec_info_
{
    int rec_hour;
    int rec_min;
    int rec_sec;
} REC_INFO;


/*
 * 在进入设置菜单时,以Photo delay: Xs的索引来初始化MENU_SET_PHOTO_DELAY菜单的SELECT_INFO对象
 */

typedef struct _select_info_
{
    int last_select;		/* 上次选中的项 */
    int select;				/* 当前选中的项 */
    int cur_page;			/* 选项所在的页 */

    //how many items
    int total;				/* 真个含有的项数 */

    //how many item in one page
    int page_max;			/* 一页含有的项数 */

    //how many pages
    int page_num;			/* 含有的页数 */
} SELECT_INFO;


typedef struct _menu_info_ {
    int back_menu;
    SELECT_INFO mSelectInfo;
    const int mSupportkeys[OLED_KEY_MAX];
} MENU_INFO;

//typedef struct _cp_info_
//{
//    const char *src;
//    const char *dest;
//}CP_INFO;

static int photo_delay[SET_PHOTO_DELAY_MAX] = {3, 5, 10, 20, 30, 40, 50, 60};





static int setting_icon_normal[SETTING_MAX][2] =
{
	/* page 1 */
	{ICON_SET_WIFI_WIFI_NORMAL_25_16_96_16, ICON_SET_WIFI_AP_NORMAL_25_16_96_16},
	{ICON_SET_ETHERNET_DIRECT_NORMAL_25_32_96_16,ICON_SET_ETHERNET_DHCP_NORMAL_25_32_96_16},
	{ICON_SET_FREQUENCY_50HZ_NORMAL_25_48_96_16,ICON_SET_FREQUENCY_60HZ_NORMAL_25_48_96_16},

	/* page 2 */
	{ICON_SET_PHOTO_DELAY_5S_NORMAL_25_16_96_16, ICON_SET_PHOTO_DELAY_5S_NORMAL_25_16_96_16},
	/* 第5项改成: Speaker ON/OFF */
    {ICON_SET_SPEAKER_OFF_NORMAL_25_32_96_16, ICON_SET_SPEAKER_ON_NORMAL_25_32_96_16},
    {ICON_SET_BOTTOMLOGO_OFF_NORMAL_25_48_96_16,ICON_SET_BOTTOMLOGO_ON_NORMAL_25_48_96_16},

	/* page 3 */
    {ICON_SET_LED_OFF_NORMAL_25_16_96_16, ICON_SET_LED_ON_NORMAL_25_16_96_16},
    {ICON_SET_AUDIO_OFF_NORMAL_25_32_96_16,ICON_SET_AUDIO_ON_NORMAL_25_32_96_16},
    {ICON_SET_SPATIALAUDIO_OFF_NORMAL_25_48_96_16,ICON_SET_SPATIALAUDIO_ON_NORMAL_25_48_96_16},

	/* page 4 */
    {ICON_SET_GYRO_OFF_NORMAL_25_16_96_16,ICON_SET_GYRO_ON_NORMAL_25_16_96_16},
	//gyro calibration
    {ICON_SET_GYRO_CALC_NORMAL_25_32_96_16,ICON_SET_GYRO_CALC_NORMAL_25_32_96_16},	// need change
    {ICON_SET_FAN_OFF_NORMAL_25_48_96_16,ICON_SET_FAN_ON_NORMAL_25_48_96_16},

	/* page 5 */
    {ICON_SET_NOISE_NORMAL_25_16_96_16,    ICON_SET_NOISE_NORMAL_25_16_96_16},
    {ICON_SAVE_SEG_OFF_NORMAL_25_32_96_16,    ICON_SAVE_SEG_ON_NORMAL_25_32_96_16},
    {ICON_STICHING_BOX_NORMAL_25_48_96_16,    ICON_STICHING_BOX_NORMAL_25_48_96_16},

	/* page 6 */
    {ICON_SET_STORAGE_NORMAL_25_16_96_16, ICON_SET_STORAGE_NORMAL_25_16_96_16},
    {ICON_SET_INFO_NORMAL_25_32_96_16,    ICON_SET_INFO_NORMAL_25_32_96_16},
    {ICON_SET_RESET_NORMAL_25_48_96_16,   ICON_SET_RESET_NORMAL_25_48_96_16},
};

static int setting_icon_lights[SETTING_MAX][2] =
{
	/* page 1 */
	{ICON_SET_WIFI_WIFI_LIGHT_25_16_96_16, ICON_SET_WIFI_AP_LIGHT_25_16_96_16},
	{ICON_SET_ETHERNET_DIRECT_LIGHT_25_32_96_16,ICON_SET_ETHERNET_DHCP_LIGHT_25_32_96_16},
	{ICON_SET_FREQUENCY_50HZ_LIGHT_25_48_96_16,ICON_SET_FREQUENCY_60HZ_LIGHT_25_48_96_16},

	/* page 2 */
	/* 第4项: SetPhotoDelay */
	{ICON_SET_PHOTO_DELAY_5S_LIGHT_25_16_96_16, ICON_SET_PHOTO_DELAY_5S_LIGHT_25_16_96_16},
	/* 第5项改成: Speaker ON/OFF */
	{ICON_SET_SPEAKER_OFF_LIGHT_25_32_96_16, ICON_SET_SPEAKER_ON_LIGHT_25_32_96_16},
	{ICON_SET_BOTTOMLOGO_OFF_LIGHT_25_48_96_16,ICON_SET_BOTTOMLOGO_ON_LIGHT_25_48_96_16},

	/* page 3 */
	{ICON_SET_LED_OFF_LIGHT_25_16_96_16, ICON_SET_LED_ON_LIGHT_25_16_96_16},
	{ICON_SET_AUDIO_OFF_LIGHT_25_32_96_16,ICON_SET_AUDIO_ON_LIGHT_25_32_96_16},
	{ICON_SET_SPATIALAUDIO_OFF_LIGHT_25_48_96_16,ICON_SET_SPATIALAUDIO_ON_LIGHT_25_48_96_16},

	/* page 4 */
	{ICON_SET_GYRO_OFF_LIGHT_25_16_96_16,ICON_SET_GYRO_ON_LIGHT_25_16_96_16},
	//gyro calibration
	{ICON_SET_GYRO_CALC_LIGHT_25_32_96_16,ICON_SET_GYRO_CALC_LIGHT_25_32_96_16},
	{ICON_SET_FAN_OFF_LIGHT_25_48_96_16,ICON_SET_FAN_ON_LIGHT_25_48_96_16},


	/* page 5 */
	//sample fan noise
	{ICON_SET_NOISE_HIGH_25_16_96_16,    ICON_SET_NOISE_HIGH_25_16_96_16},
	{ICON_SAVE_SEG_OFF_HIGH_25_32_96_16,    ICON_SAVE_SEG_ON_HIGH_25_32_96_16},
	{ICON_STITCHING_BOX_HIGH_25_48_96_16,    ICON_STITCHING_BOX_HIGH_25_48_96_16},


	/* page 6 */
	{ICON_SET_STORAGE_LIGHT_25_16_96_16,ICON_SET_STORAGE_LIGHT_25_16_96_16},
	{ICON_SET_INFO_LIGHT_25_32_96_16,    ICON_SET_INFO_LIGHT_25_32_96_16},
	{ICON_SET_RESET_LIGHT_25_48_96_16,   ICON_SET_RESET_LIGHT_25_48_96_16},
};


static MENU_INFO mMenuInfos[] = {
    {	/* MENU_TOP: Top Menu */
    	-1,					/* back_menu */
		{	
			-1,				/* last_select */
			0,				/* select */
			0,				/* cur_page */
			MAINMENU_MAX,	/* total */
			MAINMENU_MAX,	/* page_max */
			1				/* page_num */
		}, 
		{OLED_KEY_UP, OLED_KEY_DOWN,  0, OLED_KEY_SETTING, OLED_KEY_POWER}		
	},	
    {	/* MENU_PIC_INFO */
    	MENU_TOP,
		{
			-1,
			0,
			0,
			0,
			0,
			0
		}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}		
	},
    {	/* MENU_VIDEO_INFO */
    	MENU_TOP,
		{-1,0,0,0,0,0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}
	},
    {	/* MENU_LIVE_INFO */
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}		/* DOWN, BACK, SETTING, POWER */
	},
    {	/* MENU_STORAGE */
    	MENU_SYS_SETTING,
		{-1, 0, 0, SET_STORAGE_MAX, SET_STORAGE_MAX, 1}, 
		{0, 0, OLED_KEY_BACK, 0, 0}		/* BACK */
	},

	{	/* MENU_SYS_SETTING */
    	MENU_TOP,
		{-1, 0, 0, SETTING_MAX, PAGE_MAX, 5}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER}		/* UP, DOWN, BACK, POWER */
	}, //5
	
    {	/* MENU_PIC_SET_DEF */
    	MENU_PIC_INFO,
		{-1, 0, 0, PIC_ALLCARD_MAX, PIC_ALLCARD_MAX, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}		/* UP, DOWN, BACK, SETTING, POWER */
	},
    {	/* MENU_VIDEO_SET_DEF */
    	MENU_VIDEO_INFO,
		{-1, 0, 0, VID_DEF_MAX, VID_DEF_MAX, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}		/* UP, DOWN, BACK, SETTING, POWER */
	},
    {	/* MENU_LIVE_SET_DEF */
    	MENU_LIVE_INFO,
		{-1, 0, 0, LIVE_DEF_MAX, LIVE_DEF_MAX, 1},
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}		/* UP, DOWN, BACK, SETTING, POWER */
	},
    {	/* MENU_CALIBRATION */
    	MENU_TOP,
		{0},
		{0}
	},
    {	/* MENU_QR_SCAN */
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, 0}			/* BACK */
	}, //10
    //menu calibartion setting
#if 0    
   {
   		MENU_TOP,
		{-1, 0, 0, MODE_MAX, PAGE_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER}
	},
#endif
    //sys info
#ifdef ENABLE_ADB_OFF
    {
    	MENU_SYS_SETTING,
		{-1, 0, 0, 1, PAGE_MAX, 1}, 
		{0, 0, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}
	},
#else
    {	/* MENU_SYS_DEV_INFO */
    	MENU_SYS_SETTING,
		{-1, 0, 0, 1, PAGE_MAX, 1}, 
		{0, 0, OLED_KEY_BACK, 0, 0}
	},
#endif

    {	/* MENU_SYS_ERR */
    	MENU_TOP,
		{0},
		{0}
	},
    {	/* MENU_LOW_BAT */
    	MENU_TOP,
    	{0},
    	{0}
	},
    {	/* MENU_GYRO_START */
    	MENU_SYS_SETTING,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER}
	},
    {	/* MENU_SPEED_TEST */
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER}
	},
    {	/* MENU_RESET_INDICATION */
    	MENU_SYS_SETTING,
		{0},
		{OLED_KEY_UP, 0, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER}
	},
    {	/* MENU_WIFI_CONNECT */
    	MENU_SYS_SETTING,
		{0},
		{0}
	},

		
    {	/* MENU_AGEING */
    	MENU_TOP,
		{0},
		{0},		
	},
#if 0	
    //low bat protect
	{
		MENU_TOP,
		{0},
		{0}
	},
#endif
    {	/* MENU_NOSIE_SAMPLE */
    	MENU_SYS_SETTING,
		{0},
		{0}
	},
    {	/* MENU_LIVE_REC_TIME */
    	MENU_LIVE_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER}			/* BACK, POWER */
	},
	    {	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
		{0}
	},
};

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


static SETTING_ITEMS mSettingItems = {
    {ICON_SET_ETHERNET_DHCP_NORMAL_25_32_96_16,
     ICON_SET_FREQUENCY_50HZ_NORMAL_25_48_96_16},
    {0},
    setting_icon_normal,
    setting_icon_lights
};


static int photodelay_icon_lights[SET_PHOTO_DELAY_MAX][2] =
{
	/* page 1 */
	{ICON_PHOTO_DELAY_3S_LIGHT, ICON_PHOTO_DELAY_3S_LIGHT},
	{ICON_PHOTO_DELAY_5S_LIGHT,ICON_PHOTO_DELAY_5S_LIGHT},
	{ICON_PHOTO_DELAY_10S_LIGHT,ICON_PHOTO_DELAY_10S_LIGHT},

	/* page 2 */
	{ICON_PHOTO_DELAY_20S_LIGHT, ICON_PHOTO_DELAY_20S_LIGHT},
	/* 第5项改成: Speaker ON/OFF */
    {ICON_PHOTO_DELAY_30S_LIGHT, ICON_PHOTO_DELAY_30S_LIGHT},
    {ICON_PHOTO_DELAY_40S_LIGHT,ICON_PHOTO_DELAY_40S_LIGHT},

	/* page 3 */
    {ICON_PHOTO_DELAY_50S_LIGHT, ICON_PHOTO_DELAY_50S_LIGHT},
    {ICON_PHOTO_DELAY_60S_LIGHT,ICON_PHOTO_DELAY_60S_LIGHT},
};

static int photodelay_icon_normal[SET_PHOTO_DELAY_MAX][2] =
{
	/* page 1 */
	{ICON_PHOTO_DELAY_3S_NORMAL, ICON_PHOTO_DELAY_3S_NORMAL},
	{ICON_PHOTO_DELAY_5S_NORMAL,ICON_PHOTO_DELAY_5S_NORMAL},
	{ICON_PHOTO_DELAY_10S_NORMAL,ICON_PHOTO_DELAY_10S_NORMAL},

	/* page 2 */
	{ICON_PHOTO_DELAY_20S_NORMAL, ICON_PHOTO_DELAY_20S_NORMAL},
	/* 第5项改成: Speaker ON/OFF */
    {ICON_PHOTO_DELAY_30S_NORMAL, ICON_PHOTO_DELAY_30S_NORMAL},
    {ICON_PHOTO_DELAY_40S_NORMAL,ICON_PHOTO_DELAY_40S_NORMAL},

	/* page 3 */
    {ICON_PHOTO_DELAY_50S_NORMAL, ICON_PHOTO_DELAY_50S_NORMAL},
    {ICON_PHOTO_DELAY_60S_NORMAL,ICON_PHOTO_DELAY_60S_NORMAL},

};



static SETTING_ITEMS mSetPhotoDelays = {
    {ICON_SET_ETHERNET_DHCP_NORMAL_25_32_96_16,
     ICON_SET_FREQUENCY_50HZ_NORMAL_25_48_96_16},
    {0},
    photodelay_icon_normal,
    photodelay_icon_lights
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


static const int vid_def_setting_menu[][VID_DEF_MAX] =
{
        {
                ICON_VIDEO_INFO02_NORMAL_0_48_78_1678_16,
#ifdef GOOGLE_8K_5F
//                ICON_8K_5FS_78_1678_16,
                ICON_8K_5FS_78_1678_16,
#endif
                ICON_VIDEO_INFO01_NORMAL_0_48_78_1678_16,
//                ICON_VIDEO_INFO03_NORMAL_0_48_78_1678_16,//100FPS
                ICON_VIDEO_INFO06_NORMAL_0_48_78_1678_16,
                ICON_VIDEO_INFO07_NORMAL_0_48_78_1678_16,
                ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
                ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
                ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16,
        },
        {
                ICON_VIDEO_INFO02_LIGHT_0_48_78_1678_16,
#ifdef GOOGLE_8K_5F
//                ICON_8K_5FPS_HIGH_78_1678_16,
                ICON_8K_5FPS_HIGH_78_1678_16,
#endif
                ICON_VIDEO_INFO01_LIGHT_0_48_78_1678_16,
//                ICON_VIDEO_INFO03_LIGHT_0_48_78_1678_16,
                ICON_VIDEO_INFO06_LIGHT_0_48_78_1678_16,
                ICON_VIDEO_INFO07_LIGHT_0_48_78_1678_16,
                ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
                ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
                ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16,
        }
};



static const int live_def_setting_menu[][LIVE_DEF_MAX] =
        {
                {
#ifdef LIVE_ORG
                        ICON_ORIGIN_NORMAL_78_1678_16,
#endif
                        ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
                        ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
                        ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
                        ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
                        ICON_VIDEO_INFO08_NORMAL_0_48_78_1678_16,
                        ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16,

                },
                {
#ifdef LIVE_ORG
                        ICON_ORIGIN_HIGH_78_1678_16,
#endif
                        ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
                        ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
                        ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
                        ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
                        ICON_VIDEO_INFO08_LIGHT_0_48_78_1678_16,
                        ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16,
                }
        };


static const ACTION_INFO mVIDAction[] = {
	//8k/30F
	{
		MODE_PANO,
		24,
		0,
		
		{	/* for test version: ORG_INFO */
			EN_H264,
			SAVE_DEF,
			3840,		/* 3840 */
			2880,		/* 2160 -> 2880 */
#if 1
			1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
			{ALL_FR_30, 80}	/* bitrate: 40 -> 80 */
        },	
        {	/* STI_INFO */
	        EN_H264,
	        STITCH_OFF,
	        7680,
	        3840,
	        {}
        },
        {},
        {}
    },

		
    //6k/30F/3D
    {
	    MODE_3D,
	    25,
	    0,
	    {
	        EN_H264,
	        SAVE_DEF,
	        3200,			/* 3840x2880 -> 3200x2400 */
	        2400,
#if 1
            1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
	        
	        {ALL_FR_30,40}
        },
		{
			EN_H264,
			STITCH_OFF,
			5760,
			5760,
			{}
		},
        {},
        {}
    },
    
	//4K/120F
	{
	    MODE_PANO,25,0,		// 
		{
			EN_H264,
			SAVE_DEF,
			1920,
			1440, 		// 1080 -> 1440
#if 1
            1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
			
            { ALL_FR_120, 80}	// 40 -> 80
        },
        {
	        EN_H264,
	        STITCH_OFF,
	        3840,
	        1920,
	        {}
        },
        {},
        {}
    },
    //4K/60F/3D
    {
		MODE_3D,25,0,
		{
			EN_H264,
			SAVE_DEF,
			1920,
			1440,
#if 1
			1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
			
			{
		        ALL_FR_60,
		        40
			}
		},
		{
			EN_H264,
			STITCH_OFF,
			3840,
			3840,
			{}
		},
		{},
		{}
	},
	
    //4K/24F/3D(org &rts)
    {
		MODE_3D,
		20,
		0,
		{
	        EN_H264,
	        SAVE_DEF,
	        1920,
	        1440,
#if 1
            1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
	        
	        {ALL_FR_30, 25}		/* ALL_FR_24 -> ALL_FR_30 2018-06-01 */
        },
		{
			EN_H264,
			STITCH_NORMAL,
			3840,
			3840,
			{ALL_FR_30, 50}
		},
		{},
		{}
    },
    
	//4K/30F(org &rts)
	{
		MODE_PANO,
		20,
		0,
		{
			EN_H264,
			SAVE_DEF,
			2560,
			1440,
#if 1
            1,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
			
			{ALL_FR_30, 25}
        },
		{
			EN_H264,
			STITCH_NORMAL,
			3840,
			1920,
			{ALL_FR_30, 50}
		},
	    {},
	    {}
    },
    
#ifndef ARIAL_LIVE
	{
	},
#endif
};

static const ACTION_INFO mLiveAction[] = {

	//live
	{
	    MODE_3D,
	    0,
	    0,
		{
			EN_H264,
			SAVE_OFF,
			1920,
			1440,
				0,		// test
			
			{ALL_FR_30, 20}
		},
		{
	        EN_H264,
	        STITCH_NORMAL,
	        3840,
	        3840,
			{
				ALL_FR_30,
				30,
				HDMI_OFF,
				0,
				{0},
				{0},
			}
        },
        {},
        {}
    },

    {
		MODE_PANO,
		0,
		0,
		{
			EN_H264,
			SAVE_OFF,
			2560,
			1440,
				0,		// test
			
			{ALL_FR_30, 20}
		},
		{
			EN_H264,
			STITCH_NORMAL,
			3840,
			1920,
			{
				ALL_FR_30,
				20,
				HDMI_OFF,
				0,
				{0},
				{0},
			}
		},
		{},
		{}
	},
	{
		MODE_3D,
		0, 0,
		{
			EN_H264,
			SAVE_OFF,
			1920,
			1440,
				0,		// test
			
			{ALL_FR_30, 20}
		},
		{
			EN_H264,
			STITCH_NORMAL,
			3840,
			3840,
			{
				{
					ALL_FR_30,
				    30,
					HDMI_ON,
					0,
			 		{0},
			  		{0}
				}
			}
        },
		{},
		{}
    },
	{
		MODE_PANO,
		0,
		0,
		{
			EN_H264,
			SAVE_OFF,
			2560,
			1440,
				0,		// test
			
			{ALL_FR_30, 20}
        },
        {
	        EN_H264,
	        STITCH_NORMAL,
	        3840,
	        1920,
	        {
	        	{
	        		ALL_FR_30,
			        20,
		            HDMI_ON,
					0,
					{0},
					{0}},
                 }
            },
            {},
            {}
        },
        //arial
//        "origin":
//        {"mime":"h264","width":2560,"height":1440,"framerate":30,"bitrate":20000, "saveOrigin":true},
//        "stiching":{"mode":"pano","mime":"h264","width":1920,"height":960, "liveOnHdmi":true}}}
#ifdef ARIAL_LIVE
        {
                    MODE_PANO,
                    20,
                    0,
                    {
                            EN_H264,
                            SAVE_DEF,
                            2560,
                            1440,
                            
							0,		// test
                            {ALL_FR_30,
                             25}
                    },
                    {
                            EN_H264,
                            STITCH_NORMAL,
                            1920,
                            960,
                            {ALL_FR_30,
                             20,
                             HDMI_ON,
                             0,
                             {0}}
                    },
                    {},
                    {}
        }
#endif
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

	case MENU_WIFI_CONNECT:
		pRet = "MENU_WIFI_CONNECT";
		break;
	
	case MENU_AGEING:
		pRet = "MENU_AGEING";
		break;

	case MENU_NOSIE_SAMPLE:
		pRet = "MENU_NOSIE_SAMPLE";
		break;
	
	case MENU_LIVE_REC_TIME:
		pRet = "MENU_LIVE_REC_TIME";
		break;

	case MENU_STITCH_BOX:
		pRet = "MENU_STITCH_BOX";
		break;
		
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

static SYS_ERROR mSysErr[] =
{
        {START_PREVIEW_FAIL,"1401"},
        {CAPTURE_FAIL,"1402"},
        {START_REC_FAIL,"1403"},
        {START_LIVE_FAIL,"1404"},
        {START_QR_FAIL,"1405"},
        {CALIBRATION_FAIL,"1406"},
        {QR_FINISH_ERROR,"1407"},
        {START_GYRO_FAIL,"1408"},
        {START_STA_WIFI_FAIL,"1409"},
        {START_AP_WIFI_FAIL,"1410"},
        {SPEED_TEST_FAIL,"1411"},
        {START_NOISE_FAIL,"1412"},
        {STOP_PREVIEW_FAIL,"1501"},
        {STOP_REC_FAIL,"1503"},
        {STOP_LIVE_FAIL,"1504"},
        {STOP_QR_FAIL,"1505"},
        {QR_FINISH_UNRECOGNIZE,"1507"},
        {STOP_STA_WIFI_FAIL,"1509"},
        {STOP_AP_WIFI_FAIL,"1510"},
        {RESET_ALL,"1001"},
        {START_FORCE_IDLE,"1002"}
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



static int get_phdelay_index()
{
	char index;
	//char index_buf[32] = {0};

	index = SET_PHOTO_DELAY_5S;
	
	/* /data/etc/ph_delay */
	if (access(PH_DELAY_PATH, F_OK) != 0) 	/* 文件不存在 */
	{
		Log.d(TAG, "%s file not exist, use default val", PH_DELAY_PATH);
	}
	else 
	{
		int fd = open(PH_DELAY_PATH, O_RDONLY);
		if (fd < 0) 
		{
			Log.e(TAG, "Open [%s] failed", PH_DELAY_PATH);
		}
		else 
		{
			int rd = read(fd, &index, sizeof(index));
			if (rd != sizeof(index))
			{
				Log.e(TAG, "<<<<<<<<<< read %s error", PH_DELAY_PATH);
			}
			else
			{
				Log.d(TAG, "<<<<<<<<<<<read index = %d", index);
			}
		}

		close(fd);
	}

	return index;
}


static void set_phdelay_index(int index)
{
	char val = index;
	
	if (index < 0 || index > SET_PHOTO_DELAY_MAX)
	{
		Log.e(TAG, "invalid index = %d", index);
		index = SET_PHOTO_DELAY_5S;
	}
	else
	{
		if (access(PH_DELAY_PATH, F_OK) != 0)
		{
			unlink(PH_DELAY_PATH);
		}
		
		int fd = open(PH_DELAY_PATH, O_CREAT | O_RDWR, 0644);
		if (fd < 0) 
		{
			Log.e(TAG, "+++++ open %s failed", PH_DELAY_PATH);
		}
		else 
		{
			int wcnt = write(fd, &val, sizeof(val));
			if (wcnt != sizeof(val))
			{
				Log.e(TAG, "write %s failed ...");
			}
			else 
			{
				Log.d(TAG, "write %s success", PH_DELAY_PATH);
			}
		}
	}
}


static void update_takepic_delay(int index)
{
	for (u32 i = 0; i < sizeof(mPICAction) / sizeof(mPICAction[0]); i++)
	{
		Log.d(TAG, "++++++ update_takepic_delay = %d", photo_delay[index]);
		mPICAction[i].delay = photo_delay[index];
	}
}


MenuUI::MenuUI(const sp<ARMessage> &notify):mNotify(notify)
{
    init_handler_thread();	/* 初始化消息处理线程 */
	
    init();					/* MenuUI内部成员初始化 */

    send_init_disp();		/* 给消息处理线程发送初始化显示消息 */
}


MenuUI::~MenuUI()
{
    deinit();
}


/*************************************************************************
** 方法名称: send_init_disp
** 方法功能: 发送初始化显示消息
** 入口参数: 无
** 返 回 值: 无 
** 调     用: MenuUI
**
*************************************************************************/
void MenuUI::send_init_disp()
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_INIT);
    msg->post();
}


int MenuUI::get_setting_select(int type)
{
    return mSettingItems.iSelect[type];
}

void MenuUI::set_setting_select(int type,int val)
{
    mSettingItems.iSelect[type] = val;
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
** 返 回 值: 无 START_REC_SUC
** 调     用: oled_init_disp
**
*************************************************************************/
void MenuUI::init_cfg_select()
{
    init_menu_select();

    sp<ARMessage> msg;
    sp<DEV_IP_INFO> tmpInfo;

    tmpInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

	if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
        msg = (sp<ARMessage>)(new ARMessage(NETM_STARTUP_NETDEV));
		disp_wifi(true);
	} else {
		disp_wifi(false);
        msg = (sp<ARMessage>)(new ARMessage(NETM_CLOSE_NETDEV));
	}	

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
	set_oled_power(1);
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
					get_setting_select(SET_SPEAK_ON),
					sound_id);
	
    if (sound_id != -1 && get_setting_select(SET_SPEAK_ON) == 1) {
        flick_light();
        play_sound(sound_id);
	
        //force to 0 ,for play sounds cost times
        //interval = 0;
    } else if (bLight) {	/* 需要闪灯 */
        flick_light();
    }

    if (!bSendUpdate) {
		
        bSendUpdate = true;
        sp<ARMessage> msg = obtainMessage(OLED_DISP_LIGHT);
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

    if (get_setting_select(SET_SPEAK_ON) == 1) {
        if (type >= 0 && type <= sizeof(sound_str) / sizeof(sound_str[0])) {
            char cmd[1024];
            snprintf(cmd,sizeof(cmd),"tinyplay %s -D 1 &", sound_str[type]);
            //Log.d(TAG,"cmd is %s", cmd);
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
        if (get_setting_select(SET_SPEAK_ON) == 1) {

        }
        msg_util::sleep_ms(INTERVAL_1HZ);
    }
#endif
}


void MenuUI::init_sound_thread()
{
    th_sound_ = thread([this] { sound_thread(); });
}


void MenuUI::select_up()
{
    bool bUpdatePage = false;

#ifdef DEBUG_INPUT_KEY	
	Log.d(TAG, "addr 0x%p", &(mMenuInfos[cur_menu].mSelectInfo));
#endif

    SELECT_INFO * mSelect = get_select_info();
    CHECK_NE(mSelect, nullptr);

#ifdef DEBUG_INPUT_KEY		
	Log.d(TAG, "mSelect 0x%p", mSelect);
#endif
    mSelect->last_select = mSelect->select;

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG, "cur_menu %d select_up select %d mSelect->last_select %d "
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
        if (mSelect->page_num > 1) {
            bUpdatePage = true;
            if (--mSelect->cur_page < 0) {
                mSelect->cur_page = mSelect->page_num - 1;
                mSelect->select = (mSelect->total - 1)%mSelect->page_max;
            } else {
                mSelect->select = mSelect->page_max - 1;
            }
        } else {
            mSelect->select = mSelect->total - 1;
        }
    }

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG," select_up select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
#endif

    if (bUpdatePage) {
        update_menu_page();
    } else {
        update_menu();
    }
}



void MenuUI::select_down()
{
    bool bUpdatePage = false;
	
    SELECT_INFO * mSelect = get_select_info();
    CHECK_NE(mSelect, nullptr);

#ifdef DEBUG_INPUT_KEY	
	Log.d(TAG," select_down select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d",
          mSelect->select, mSelect->last_select, mSelect->page_num,
          mSelect->cur_page, mSelect->page_max, mSelect->total);
#endif

    mSelect->last_select = mSelect->select;
    mSelect->select++;
    if (mSelect->select + (mSelect->cur_page * mSelect->page_max ) >= mSelect->total) {
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
    Log.d(TAG," select_down select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d over bUpdatePage %d",
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total, bUpdatePage);
#endif

    if (bUpdatePage) {
        update_menu_page();
    } else {
        update_menu();
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


void MenuUI::set_lan(int lan)
{
    mProCfg->set_val(KEY_LAN, lan);
    update_disp_func(lan);
}

void MenuUI::update_disp_func(int lan)
{
}

int MenuUI::get_menu_select_by_power(int menu)
{
    int val = mMenuInfos[menu].mSelectInfo.select +
            mMenuInfos[menu].mSelectInfo.cur_page * mMenuInfos[menu].mSelectInfo.page_max;
    return val;
}


/*************************************************************************
** 方法名称: set_menu_select_info
** 方法功能: 为指定的菜单选择选择项
** 入口参数: menu - 菜单ID
**			 iSelect - 选择项ID
** 返 回 值: 无 
** 调     用: init_menu_select
**
*************************************************************************/
void MenuUI::set_menu_select_info(int menu, int iSelect)
{
    mMenuInfos[menu].mSelectInfo.cur_page = iSelect / mMenuInfos[menu].mSelectInfo.page_max;
    mMenuInfos[menu].mSelectInfo.select = iSelect % mMenuInfos[menu].mSelectInfo.page_max;
}


/*
 * add by skymixos
 */
void set_photodelay_select(int index)
{

	/* 3(0), 5(1), 10(2), 20(3), 30(4), 40(5), 50(6), 60(7) */
	switch (index) 
	{
		case SET_PHOTO_DELAY_3S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_3S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_3S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_3S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_3S_LIGHT_25_16_96_16;
			break;

		case SET_PHOTO_DELAY_5S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_5S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_5S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_5S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_5S_LIGHT_25_16_96_16;
			break;

		case SET_PHOTO_DELAY_10S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_10S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_10S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_10S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_10S_LIGHT_25_16_96_16;
			break;

		case SET_PHOTO_DELAY_20S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_20S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_20S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_20S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_20S_LIGHT_25_16_96_16;
			break;


		case SET_PHOTO_DELAY_30S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_30S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_30S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_30S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_30S_LIGHT_25_16_96_16;
			break;

		case SET_PHOTO_DELAY_40S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_40S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_40S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_40S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_40S_LIGHT_25_16_96_16;
			break;

		case SET_PHOTO_DELAY_50S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_50S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_50S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_50S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_50S_LIGHT_25_16_96_16;
			break;


		case SET_PHOTO_DELAY_60S:
			setting_icon_normal[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_60S_NORMAL_25_16_96_16;
			setting_icon_normal[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_60S_NORMAL_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][0] = ICON_SET_PHOTO_DELAY_60S_LIGHT_25_16_96_16;
			setting_icon_lights[SET_PHOTO_DELAY][1] = ICON_SET_PHOTO_DELAY_60S_LIGHT_25_16_96_16;
			break;

		default:
			break;
	}
}



void MenuUI::init_menu_select()
{
    int val;

    for (int i = MENU_PIC_SET_DEF; i <= MENU_LIVE_SET_DEF; i++) {
        switch (i) {
            case MENU_PIC_SET_DEF:	/* 为菜单MENU_PIC_SET_DEF设置拍照的默认配置 */
                val = mProCfg->get_val(KEY_PIC_DEF);
//                Log.d(TAG,"pic set def %d", val);
                switch (val) {
                    case PIC_ALL_CUSTOM:
                    case PIC_ALL_8K_3D_OF:
                    case PIC_ALL_8K_OF:
                    case PIC_ALL_8K:
                    case PIC_ALL_BURST:
                        set_menu_select_info(i,val);
                        break;
                        //user define
                    SWITCH_DEF_ERROR(val)
                }
                break;
				
            case MENU_VIDEO_SET_DEF:
                val = mProCfg->get_val(KEY_VIDEO_DEF);
                if (val >= 0 && val < VID_DEF_MAX) {
                    set_menu_select_info(i,val);
                } else {
                    ERR_ITEM(val)
                }
                break;

            case MENU_LIVE_SET_DEF:
                val = mProCfg->get_val(KEY_LIVE_DEF);
                switch (val)  {
#ifdef LIVE_ORG
                    case LIVE_ORIGIN:
#endif
                    case LIVE_CUSTOM:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
                    case VID_ARIAL:
                        set_menu_select_info(i,val);
                        break;
                        //user define
                    SWITCH_DEF_ERROR(val)
                }
                break;
            default:
                break;
        }
    }


	/* 初始化菜单时,读取系统配置值 */
    for (int i = 0; i < SETTING_MAX; i++) {
        switch (i) {
            case SET_WIFI_AP:
                val = mProCfg->get_val(KEY_WIFI_AP);
                set_setting_select(i,val);
                break;
				
            case SET_DHCP_MODE:
                val = mProCfg->get_val(KEY_DHCP);
                set_setting_select(i,val);
                break;
				
            case SET_FREQ:
                val = mProCfg->get_val(KEY_PAL_NTSC);
                set_setting_select(i,val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);
                break;

			/* skymixos 
			 * 初始化设置菜单的"Photo Delay"项时，通过读取配置文件保存的索引值来显示
			 */
			case SET_PHOTO_DELAY:
				#if 0
				val = mProCfg->get_val(KEY_PHOTO_DELAY);	/* 根据拍照延时值来选中对应的菜单项 */
				Log.d(TAG, "KEY_PHOTO_DELAY val = %d", val);
				#endif
				
				set_photo_delay_index = get_phdelay_index();
				set_photodelay_select(set_photo_delay_index);		/* 根据索引值来选中合适的图标 */
				update_takepic_delay(set_photo_delay_index);		/* 更新拍照时的延时值 */
				Log.d(TAG, "init_menu_select: init set_photo_delay_index [%d]", set_photo_delay_index);				
				break;

				
            case SET_SPEAK_ON:
                val = mProCfg->get_val(KEY_SPEAKER);
                set_setting_select(i,val);
                break;
				
            case SET_BOTTOM_LOGO:
                val = mProCfg->get_val(KEY_SET_LOGO);
                set_setting_select(i,val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);
                break;
				
            case SET_LIGHT_ON:	/* 开机时根据配置,来决定是否开机后关闭前灯 */
                val = mProCfg->get_val(KEY_LIGHT_ON);
                set_setting_select(i,val);
                //off the light 170802
                if (val == 0) {
                    set_light_direct(LIGHT_OFF);
                }
                break;
				
            case SET_AUD_ON:
                val = mProCfg->get_val(KEY_AUD_ON);
                set_setting_select(i,val);
                break;
				
            case SET_SPATIAL_AUD:
                val = mProCfg->get_val(KEY_AUD_SPATIAL);
                set_setting_select(i,val);
                break;
				
            case SET_GYRO_ON:
                val = mProCfg->get_val(KEY_GYRO_ON);
                set_setting_select(i,val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);
                break;
				
            case SET_FAN_ON:
                val = mProCfg->get_val(KEY_FAN);
                set_setting_select(i,val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_FAN);
                break;
				
            case SET_VIDEO_SEGMENT:
                val = mProCfg->get_val(KEY_VID_SEG);
                set_setting_select(i,val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_VID_SEG);
                break;
				
            case SET_STORAGE:
            case SET_RESTORE:
            case SET_INFO:
            case SET_START_GYRO:
            case SET_NOISE:
            case SET_STITCH_BOX:
                break;
			
            SWITCH_DEF_ERROR(i);
        }
    }
    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
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

	
    CHECK_EQ(sizeof(mMenuInfos) / sizeof(mMenuInfos[0]), MENU_MAX);
    CHECK_EQ(mControlAct, nullptr);
    CHECK_EQ(sizeof(mPICAction) / sizeof(mPICAction[0]), PIC_ALL_CUSTOM);
    CHECK_EQ(sizeof(mVIDAction) / sizeof(mVIDAction[0]), VID_CUSTOM);
    CHECK_EQ(sizeof(mLiveAction) / sizeof(mLiveAction[0]), LIVE_CUSTOM);
    CHECK_EQ(sizeof(astSysRead) / sizeof(astSysRead[0]), SYS_KEY_MAX);

    CHECK_EQ(sizeof(setting_icon_normal) / sizeof(setting_icon_normal[0]), SETTING_MAX);
    CHECK_EQ(sizeof(setting_icon_lights) / sizeof(setting_icon_lights[0]), SETTING_MAX);

    mLocalStorageList.clear();

    cam_state = STATE_IDLE;

	/* OLED对象： 显示系统 */
    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);

	/* 设备管理器: 监听设备的动态插拔 */
    sp<ARMessage> dev_notify = obtainMessage(OLED_UPDATE_DEV_INFO);
    mDevManager = sp<dev_manager>(new dev_manager(dev_notify));
    CHECK_NE(mDevManager, nullptr);

    mProCfg = sp<pro_cfg>(new pro_cfg());

    mRemainInfo = sp<REMAIN_INFO>(new REMAIN_INFO());
    CHECK_NE(mRemainInfo, nullptr);
    memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));

    mRecInfo = sp<REC_INFO>(new REC_INFO());
    CHECK_NE(mRecInfo, nullptr);
    memset(mRecInfo.get(), 0, sizeof(REC_INFO));

 //   init_pipe(mCtrlPipe);
#ifdef ENABLE_LIGHT
    mOLEDLight = sp<oled_light>(new oled_light());
    CHECK_NE(mOLEDLight, nullptr);
#endif

    mBatInterface = sp<battery_interface>(new battery_interface());
    CHECK_NE(mBatInterface, nullptr);

    m_bat_info_ = sp<BAT_INFO>(new BAT_INFO());
    CHECK_NE(m_bat_info_, nullptr);
    memset(m_bat_info_.get(), 0, sizeof(BAT_INFO));
    m_bat_info_->battery_level = 1000;

//    mControlAct = sp<ACTION_INFO>(new ACTION_INFO());
//    CHECK_NE(mControlAct,nullptr);

    mReadSys = sp<SYS_INFO>(new SYS_INFO());
    CHECK_NE(mReadSys, nullptr);

    mVerInfo = sp<VER_INFO>(new VER_INFO());
    CHECK_NE(mVerInfo, nullptr);

#ifdef ENABLE_WIFI_STA
    mWifiConfig = sp<WIFI_CONFIG>(new WIFI_CONFIG());
    CHECK_NE(mWifiConfig, nullptr);	
    memset(mWifiConfig.get(), 0, sizeof(WIFI_CONFIG));
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


}


/*
 * 界面没变,IP地址发生变化
 * IP地址没变,界面发生变化
 */
void MenuUI::uiShowStatusbarIp()
{
	Log.e(TAG, "uiShowStatusbarIp --->");
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
            obtainMessage(OLED_EXIT)->post();
            th_msg_.join();
        } else {
            Log.e(TAG, " th_msg_ not joinable ");
        }
    }
    Log.d(TAG, "sendExit2");
}


void MenuUI::send_disp_str(sp<DISP_TYPE> &sp_disp)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_STR_TYPE);
    msg->set<sp<DISP_TYPE>>("disp_type", sp_disp);
    msg->post();
}

void MenuUI::send_disp_err(sp<struct _err_type_info_> &mErrInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_ERR_TYPE);
    msg->set<sp<ERR_TYPE_INFO>>("err_type_info", mErrInfo);
    msg->post();
}

void MenuUI::send_sync_init_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_SYNC_INIT_INFO);
    msg->set<sp<SYNC_INIT_INFO>>("sync_info", mSyncInfo);
    msg->post();
}

void MenuUI::send_get_key(int key)
{
    sp<ARMessage> msg = obtainMessage(OLED_GET_KEY);
    msg->set<int>("oled_key", key);
    msg->post();
}

void MenuUI::send_long_press_key(int key,int64 ts)
{
    sp<ARMessage> msg = obtainMessage(OLED_GET_LONG_PRESS_KEY);
    msg->set<int>("key", key);
    msg->set<int64>("ts", ts);
    msg->postWithDelayMs(LONG_PRESS_MSEC);
}

void MenuUI::send_disp_ip(int ip, int net_type)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_IP);
    msg->set<int>("ip", ip);
    msg->post();
}

void MenuUI::send_disp_battery(int battery, bool charge)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_BATTERY);
    msg->set<int>("battery", battery);
    msg->set<bool>("charge", charge);
    msg->post();
}

void MenuUI::send_wifi_config(sp<WIFI_CONFIG> &mConfig)
{
    sp<ARMessage> msg = obtainMessage(OLED_CONFIG_WIFI);
    msg->set<sp<WIFI_CONFIG>>("wifi_config",mConfig);
    msg->post();
}

void MenuUI::send_sys_info(sp<SYS_INFO> &mSysInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_SET_SN);
    msg->set<sp<SYS_INFO>>("sys_info",mSysInfo);
    msg->post();
}

void MenuUI::send_update_mid_msg(int interval)
{
    if (!bSendUpdateMid) {
        bSendUpdateMid = true;
        send_delay_msg(OLED_UPDATE_MID, interval);
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
        Log.d(TAG," reset rec cam_state 0x%x",cam_state);
        memset(mRecInfo.get(),0,sizeof(REC_INFO));
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
	
    if (is_top_clear(cur_menu)) {
        clear_area();
    }
	
    switch (new_menu) {
        case MENU_PIC_SET_DEF:
            set_cur_menu(MENU_PIC_INFO);
            break;
		
        case MENU_VIDEO_SET_DEF:
            set_cur_menu(MENU_VIDEO_INFO);
            break;
		
        case MENU_LIVE_SET_DEF:
            set_cur_menu(MENU_LIVE_INFO);
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
	
    set_cur_menu(new_menu);
	
    if (old_menu == MENU_SYS_ERR || old_menu == MENU_LOW_BAT) {
        //force set normal light while backing from sys err
        set_light();
    }
}




/*************************************************************************
** 方法名称: set_cur_menu
** 方法功能: 设置当前显示的菜单
** 入口参数: menu - 当前将要显示的菜单ID
**			 back_menu - 返回菜单ID
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void MenuUI::set_cur_menu(int menu, int back_menu)
{
    bool bDispBottom = true; //add 170804 for menu_pic_info not recalculate for pic num MENU_TOP

	if (menu == cur_menu) {
        Log.d(TAG, "set cur menu same menu %d cur menu %d\n", menu, cur_menu);
        bDispBottom = false;
    } else  {
        if (is_top_clear(menu))  {
            if (back_menu == -1)  {
                set_back_menu(menu, cur_menu);
            } else {
                set_back_menu(menu, back_menu);
            }
        }
        cur_menu = menu;
    }
	
    disp_menu(bDispBottom);
}



void MenuUI::restore_all()
{
    add_state(STATE_RESTORE_ALL);
}

void MenuUI::update_sys_info()
{
    SELECT_INFO * mSelect = get_select_info();

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



void MenuUI::disp_sys_info()
{
    int col = 0; //25

    //force read every time
    read_sn();
    clear_area(0, 16);
    char buf[32];

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
            if (get_setting_select(SET_LIGHT_ON) == 1) {
                set_light_direct(front_light);
            } else {
                set_light_direct(LIGHT_OFF);
            }
        }
		
        set_cur_menu(MENU_DISP_MSG_BOX);
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
        set_cur_menu(MENU_SPEED_TEST);
        ret = true;
    }
    return ret;
}

void MenuUI::fix_live_save_per_act(struct _action_info_ *mAct)
{
    if(mAct->stOrgInfo.save_org != SAVE_OFF)
    {
        mAct->size_per_act = (mAct->stOrgInfo.stOrgAct.mOrgV.org_br * 6) / 10;
    }
    if(mAct->stStiInfo.stStiAct.mStiL.file_save)
    {
       mAct->size_per_act +=
               mAct->stStiInfo.stStiAct.mStiV.sti_br / 10;
    }
    if(mAct->size_per_act <= 0)
    {
        Log.d(TAG,"fix_live_save_per_act strange %d %d %d",
              mAct->stOrgInfo.save_org,mAct->stOrgInfo.stOrgAct.mOrgV.org_br,
              mAct->stStiInfo.stStiAct.mStiV.sti_br);
        mAct->size_per_act = 1;
    }
}

bool MenuUI::check_live_save(ACTION_INFO *mAct)
{
    bool ret = false;
    if(mAct->stOrgInfo.save_org != SAVE_OFF ||
            mAct->stStiInfo.stStiAct.mStiL.file_save )
    {
        ret = true;
    }
    Log.d(TAG, "check_live_save is %d %d ret %d",
          mAct->stOrgInfo.save_org ,
          mAct->stStiInfo.stStiAct.mStiL.file_save,
          ret);
    return ret;
}

bool MenuUI::start_live_rec(const struct _action_info_ *mAct,ACTION_INFO *dest)
{
    bool bAllow = false;
	
    if (!check_save_path_none()) {
		
		/* 去掉直播录像,必须录制在U盘的限制 */
		if (/* check_save_path_usb() */ 1) {
            if (!start_speed_test()) {
                if (cur_menu == MENU_LIVE_REC_TIME) {
                    cam_state |= STATE_START_LIVING;
                    set_cur_menu(MENU_LIVE_INFO);
                    bAllow = true;
                    memcpy(dest,mAct,sizeof(ACTION_INFO));
                } else {
                    disp_icon(ICON_LIVE_REC_TIME_128_64128_64);
                    char disp[32];
                    if (mAct->size_per_act > 0) {
                        if (get_save_path_avail()) {
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
                            }
                            else
                            {
                                snprintf(disp,sizeof(disp),"%s","00:00:00");
                            }
                        }
                        else
                        {
                            snprintf(disp,sizeof(disp),"%s","no access");
                            Log.e(TAG,"start_live_rec usb no access");
                        }
                    }
                    else
                    {
                        snprintf(disp,sizeof(disp),"%s","99:99:99");
                    }
                    Log.d(TAG,"start_live_rec "
                                  "pstTmp->size_per_act %d %s",
                          mAct->size_per_act,disp);
                    disp_str((const u8 *)disp,37,32);
                    set_cur_menu(MENU_LIVE_REC_TIME);
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
    int item = 0;

    switch (option) {
        case ACTION_PIC:	/* 拍照动作 */
            if (check_dev_exist(option)) {

				Log.d(TAG, ">>>>>>>>>>>>>>>>> send_option_to_fifo +++ ACTION_PIC");
				
                // only happen in preview in oled panel, taking pic in live or rec only actvied by controller client
                if (check_allow_pic()) {
                    oled_disp_type(CAPTURE);
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
//                    Log.d(TAG," pic action item is %d",item);

                    switch (item) {
//                        case PIC_8K_3D_SAVE:
                        case PIC_8K_PANO_SAVE:
                        case PIC_8K_3D_SAVE_OF:
                            //optical flow stich
                        case PIC_8K_PANO_SAVE_OF:
#ifdef OPEN_HDR_RTS
                        case PIC_HDR_RTS:
#endif
                        case PIC_HDR:
                        case PIC_BURST:
                        case PIC_RAW:							
                            memcpy(mActionInfo.get(), &mPICAction[item], sizeof(ACTION_INFO));
                            break;
							
                        case PIC_CUSTOM:
                            memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_PIC_DEF), sizeof(ACTION_INFO));
                            if (strlen(mActionInfo->stProp.len_param) > 0 || strlen(mActionInfo->stProp.mGammaData) > 0) {
                                mProCfg->get_def_info(KEY_PIC_DEF)->stProp.audio_gain = -1;
                                send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_PIC_DEF)->stProp);
                            }
                            break;
							
                        SWITCH_DEF_ERROR(item);
                    }
                    msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                } else {
                    bAllow = false;
                }
            } else {
                bAllow = false;
            }
            break;
			
        case ACTION_VIDEO:
            if ( check_state_in(STATE_RECORD) && (!check_state_in(STATE_STOP_RECORDING))) {
//                Log.d(TAG,"stop rec ");
                oled_disp_type(STOP_RECING);
            } else if (check_state_preview()) {
                if (check_dev_exist(option)) {
                    if (start_speed_test()) {
                        bAllow = false;
                    } else {
                        oled_disp_type(START_RECING);
                        item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
//                        Log.d(TAG, " vid set is %d ", item);
                        if (item >= 0 && item < VID_CUSTOM)
                        {
                            memcpy(mActionInfo.get(), &mVIDAction[item], sizeof(ACTION_INFO));
                        }
                        else if (VID_CUSTOM == item)
                        {
                            send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_VIDEO_DEF)->stProp);
                            memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_VIDEO_DEF), sizeof(ACTION_INFO));
                        }
                        else
                        {
                            ERR_ITEM(item);
                        }
                        msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                    }
                }
                else
                {
                    bAllow = false;
                }
            }
            else
            {
                Log.w(TAG," ACTION_VIDEO bad state 0x%x",cam_state);
                bAllow = false;
            }
            break;
			
        case ACTION_LIVE:
            if ((check_live()) && (!check_state_in(STATE_STOP_LIVING))) {
                oled_disp_type(STOP_LIVING);
            } else if (check_state_preview()) {
                item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
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
                        if (!check_live_save(mProCfg->get_def_info(KEY_LIVE_DEF)))
                        {
                            oled_disp_type(STRAT_LIVING);
                            memcpy(mActionInfo.get(),
                                   mProCfg->get_def_info(KEY_LIVE_DEF),
                                   sizeof(ACTION_INFO));
//                            Log.d(TAG, "(url and format %s %s)",
//                                  mActionInfo->stStiInfo.stStiAct.mStiL.url,
//                                  mActionInfo->stStiInfo.stStiAct.mStiL.format);
                        }
                        else
                        {
                            bAllow = start_live_rec((const ACTION_INFO *)mProCfg->get_def_info(KEY_LIVE_DEF),mActionInfo.get());
                        }
                        if(bAllow)
                        {
                            send_option_to_fifo(ACTION_CUSTOM_PARAM, 0, &mProCfg->get_def_info(KEY_LIVE_DEF)->stProp);
                        }
                        break;
#ifdef LIVE_ORG
                    case LIVE_ORIGIN:
                        option = ACTION_LIVE_ORIGIN;
                        break;
#endif
                    SWITCH_DEF_ERROR(item);
                }
#ifdef LIVE_ORG
                if(option != ACTION_LIVE_ORIGIN)
                {
                    msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                }
#else
                msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
#endif
            }
            else
            {
                Log.w(TAG," act live bad state 0x%x",cam_state);
                bAllow = false;
            }
            break;
			
        case ACTION_PREVIEW:
            if (check_state_preview() || check_state_equal(STATE_IDLE)) {
//                bAllow = true;
            } else {
                ERR_MENU_STATE(cur_menu,cam_state);
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
			
        case ACTION_REQ_SYNC: {
            sp<REQ_SYNC> mReqSync = sp<REQ_SYNC>(new REQ_SYNC());
            snprintf(mReqSync->sn,sizeof(mReqSync->sn),"%s",mReadSys->sn);
            snprintf(mReqSync->r_v,sizeof(mReqSync->r_v),"%s",mVerInfo->r_ver);
            snprintf(mReqSync->p_v,sizeof(mReqSync->p_v),"%s",mVerInfo->p_ver);
            snprintf(mReqSync->k_v,sizeof(mReqSync->k_v),"%s",mVerInfo->k_ver);
            msg->set<sp<REQ_SYNC>>("req_sync", mReqSync);
			break;
        }
            
        case ACTION_LOW_BAT:
            msg->set<int>("cmd", cmd);
            break;
		
//        case ACTION_LOW_PROTECT:
//            break;
        case ACTION_SPEED_TEST:
            if (check_save_path_none()) {
                Log.e(TAG, "action speed test mSavePathIndex -1");
                bAllow = false;
            } else {
                if(!check_state_in(STATE_SPEED_TEST))
                {
                    msg->set<char *>("path", mLocalStorageList.at(mSavePathIndex)->path);
                    oled_disp_type(SPEED_START);
                }
                else
                {
                    bAllow = false;
                }
            }
            break;
        case ACTION_GYRO:
            if(check_state_in(STATE_IDLE))
            {
                oled_disp_type(START_GYRO);
            }
            else
            {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;
        case ACTION_NOISE:
            if(check_state_in(STATE_IDLE))
            {
                oled_disp_type(START_NOISE);
            }
            else
            {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;
        case ACTION_AGEING:
            if(check_state_in(STATE_IDLE))
            {
                oled_disp_type(START_RECING);
                set_cur_menu(MENU_AGEING);
            }
            else
            {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;
        case ACTION_SET_OPTION:
            msg->set<int>("type", cmd);
            switch(cmd)
            {
                case OPTION_FLICKER:
                    msg->set<int>("flicker", get_setting_select(SET_FREQ));
                    break;
                case OPTION_LOG_MODE:
                    msg->set<int>("mode", 1);
                    msg->set<int>("effect", 0);
                    break;
                case OPTION_SET_FAN:
                    msg->set<int>("fan", get_setting_select(SET_FAN_ON));
                    break;
                case OPTION_SET_AUD:
                    if(get_setting_select(SET_AUD_ON) == 1)
                    {
                        if(get_setting_select(SET_SPATIAL_AUD) == 1)
                        {
                            msg->set<int>("aud", 2);
                        }
                        else
                        {
                            msg->set<int>("aud", 1);
                        }
                    }
                    else
                    {
                        msg->set<int>("aud", 0);
                    }
                    break;
                case OPTION_GYRO_ON:
                    msg->set<int>("gyro_on", get_setting_select(SET_GYRO_ON));
                    break;
                case OPTION_SET_LOGO:
                    msg->set<int>("logo_on", get_setting_select(SET_BOTTOM_LOGO));
                    break;
                case OPTION_SET_VID_SEG:
                    msg->set<int>("video_fragment", get_setting_select(SET_VIDEO_SEGMENT));
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
            if(!check_state_in(STATE_POWER_OFF))
            {
                add_state(STATE_POWER_OFF);
                clear_area();
                set_light_direct(LIGHT_OFF);
                msg->set<int>("cmd", cmd);
            }
            else
            {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;

        case ACTION_CUSTOM_PARAM: {
            sp<CAM_PROP> mProp = sp<CAM_PROP>(new CAM_PROP());
            memcpy(mProp.get(),pstProp,sizeof(CAM_PROP));
            msg->set<sp<CAM_PROP>>("cam_prop", mProp);
			break;
        }
            
        case ACTION_SET_STICH:
            break;
			
		case ACTION_AWB:		
			set_light_direct(FRONT_GREEN);
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
                update_bottom_space();
            }
            break;
			
        default:
            break;
    }

    sp<ARMessage> msg = mNotify->dup();
    msg->set<int>("what", SAVE_PATH_CHANGE);
    sp<SAVE_PATH> mSavePath = sp<SAVE_PATH>(new SAVE_PATH());
	
    if (!check_save_path_none()) {
        snprintf(mSavePath->path,sizeof(mSavePath->path), "%s", mLocalStorageList.at(mSavePathIndex)->path);
    } else {
        snprintf(mSavePath->path,sizeof(mSavePath->path), "%s", NONE_PATH);
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
    sp<ARMessage> msg = obtainMessage(OLED_READ_BAT);
    msg->post();
}

void MenuUI::send_clear_msg_box(int delay)
{
    sp<ARMessage> msg = obtainMessage(OLED_CLEAR_MSG_BOX);
    msg->postWithDelayMs(delay);
}


void MenuUI::send_update_dev_list(std::vector<sp<Volume>> &mList)
{
    sp<ARMessage> msg = mNotify->dup();

    msg->set<int>("what", UPDATE_DEV);
    msg->set<std::vector<sp<Volume>>>("dev_list", mList);
    msg->post();
}

const u8* MenuUI::get_disp_str(int str_index)
{
    const u8 *str = mProCfg->get_str(str_index);
    Log.d(TAG,"333 get disp str[%d] %s\n",str_index,str);
    return str;
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
    char cmd[1024];
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
** 返 回 值: 无
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::set_sync_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    int state = mSyncInfo->state;

    snprintf(mVerInfo->a12_ver,sizeof(mVerInfo->a12_ver),"%s",mSyncInfo->a_v);
    snprintf(mVerInfo->c_ver,sizeof(mVerInfo->c_ver),"%s",mSyncInfo->c_v);
    snprintf(mVerInfo->h_ver,sizeof(mVerInfo->h_ver),"%s",mSyncInfo->h_v);

    Log.d(TAG,"sync state 0x%x va:%s vc %s vh %s",
          mSyncInfo->state,mSyncInfo->a_v,
          mSyncInfo->c_v,mSyncInfo->h_v);
    INFO_MENU_STATE(cur_menu,cam_state);

    if (state == STATE_IDLE)
	{	/* 如果相机处于Idle状态 */
		Log.d(TAG,"set_sync_info oled_reset_disp cam_state 0x%x, cur_menu %d", cam_state, cur_menu);
        cam_state = STATE_IDLE;
        set_cur_menu(MENU_TOP);	/* 初始化显示为顶级菜单 */
    }
    else if ((state & STATE_RECORD) == STATE_RECORD)
    {
        if ((state & STATE_PREVIEW )== STATE_PREVIEW)
        {
            oled_disp_type(SYNC_REC_AND_PREVIEW);
        }
        else
        {
            oled_disp_type(START_REC_SUC);
        }
    }
    else if((state& STATE_LIVE) == STATE_LIVE)
    {
        if ((state& STATE_PREVIEW) == STATE_PREVIEW)
        {
            oled_disp_type(SYNC_LIVE_AND_PREVIEW);
        }
        else
        {
            oled_disp_type(START_LIVE_SUC);
        }
    }
    else if((state& STATE_LIVE_CONNECTING) == STATE_LIVE_CONNECTING)
    {
        if ((state& STATE_PREVIEW) == STATE_PREVIEW)
        {
            oled_disp_type(SYNC_LIVE_CONNECT_AND_PREVIEW);
        }
        else
        {
            oled_disp_type(START_LIVE_CONNECTING);
        }
    }
    else if((state& STATE_CALIBRATING) == STATE_CALIBRATING)
    {
        oled_disp_type(START_CALIBRATIONING);
    }
    else if((state& STATE_PIC_STITCHING) == STATE_PIC_STITCHING)
    {
        oled_disp_type(SYNC_PIC_STITCH_AND_PREVIEW);
    }
    else if((state& STATE_TAKE_CAPTURE_IN_PROCESS) == STATE_CALIBRATING)
    {
        oled_disp_type(SYNC_PIC_CAPTURE_AND_PREVIEW);
    }
    else if( (state& STATE_PREVIEW) == STATE_PREVIEW)
    {
        oled_disp_type(START_PREVIEW_SUC);
    }
	
}

void MenuUI::write_sys_info(sp<SYS_INFO> &mSysInfo)
{
    //open to allow write once in future
//    if(check_path_exist(sys_file_name))
//    {
//        Log.d(TAG,"%s already exists",sys_file_name);
//    }
//    else
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
        for (int i = 0; i < SYS_KEY_MAX; i++)
        {
            switch(i)
            {
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
    Log.d(TAG,"close write sn");
#endif
}



#ifdef ENABLE_WIFI_STA
bool MenuUI::start_wifi_sta(int disp_main)
{
    bool ret = false;

    Log.d(TAG,"start_wifi_sta (%d %d)",
          mProCfg->get_val(KEY_WIFI_ON),
          mProCfg->get_val(KEY_WIFI_AP));

    if (mProCfg->get_val(KEY_WIFI_ON) == 1 && mProCfg->get_val(KEY_WIFI_AP) == 0) {
        Log.d(TAG, "wifi ssid %s pwd %s "
                      "disp_main %d cur_menu %d",
              mWifiConfig->ssid,
              mWifiConfig->pwd,
              disp_main,
              cur_menu);
        if (strlen(mWifiConfig->ssid) > 0 /*&& strlen(mWifiConfig->pwd) > 0*/) {
            wifi_stop();
            if (disp_main != -1) {
                set_cur_menu(MENU_WIFI_CONNECT);
                if (strlen(mWifiConfig->pwd) == 0) {
                    Log.d(TAG,"connect no pwd %s", mWifiConfig->ssid);
                    //tx_softsta_start(mWifiConfig->ssid,0, 7);
                } else {
                    //tx_softsta_start(mWifiConfig->ssid, mWifiConfig->pwd, 7);
                }
//            Log.d(TAG,"tx_softsta_start is %d", iRet);
                procBackKeyEvent();
            }
            else
            {
                if(strlen(mWifiConfig->pwd) == 0)
                {
                    Log.d(TAG,"connect no pwd2 %s", mWifiConfig->ssid);
                    //tx_softsta_start(mWifiConfig->ssid,0, 7);
                }
                else
                {
                    //tx_softsta_start(mWifiConfig->ssid, mWifiConfig->pwd, 7);
                }
//            Log.d(TAG,"2tx_softsta_start is %d", iRet);
            }
            disp_wifi(true, disp_main);
            ret = true;
        }
    }
    return ret;
}
#endif



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




void MenuUI::wifi_action()
{
    Log.e(TAG, " wifi_action %d", mProCfg->get_val(KEY_WIFI_ON));

    sp<ARMessage> msg;
    sp<DEV_IP_INFO> tmpInfo;

    tmpInfo = (sp<DEV_IP_INFO>)(new DEV_IP_INFO());
    strcpy(tmpInfo->cDevName, WLAN0_NAME);
    strcpy(tmpInfo->ipAddr, WLAN0_DEFAULT_IP);
    tmpInfo->iDevType = DEV_WLAN;

    if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
        Log.e(TAG, "set KEY_WIFI_ON -> 0");
        mProCfg->set_val(KEY_WIFI_ON, 0);
        msg = (sp<ARMessage>)(new ARMessage(NETM_CLOSE_NETDEV));
        disp_wifi(false, 1);
    } else {
        msg = (sp<ARMessage>)(new ARMessage(NETM_STARTUP_NETDEV));
        Log.e(TAG, "set KEY_WIFI_ON -> 1");
        mProCfg->set_val(KEY_WIFI_ON, 1);
        disp_wifi(true, 1);
    }

    msg->set<sp<DEV_IP_INFO>>("info", tmpInfo);
    NetManager::getNetManagerInstance()->postNetMessage(msg);
}




int MenuUI::get_back_menu(int item)
{
    if (item >= 0 && (item < MENU_MAX))
    {
        return mMenuInfos[item].back_menu;
    }
    else
    {
        Log.e(TAG, "get_back_menu item %d", item);
#ifdef ENABLE_ABORT
        abort();
#else
        return MENU_TOP;
#endif
    }
}

void MenuUI::set_back_menu(int item,int menu)
{
    if(menu == -1)
    {
        if(cur_menu == -1)
        {
            cur_menu = MENU_TOP;
            menu = MENU_TOP;
        }
        else
        {
            Log.e(TAG,"back menu is -1 cur_menu %d\n",cur_menu);

            #ifdef ENABLE_ABORT
			#else
            menu = cur_menu;
			#endif
        }
    }

//    Log.d(TAG, " set back menu item %d menu %d", item, menu);
    if (item > MENU_TOP && (item < MENU_MAX))
    {
        {
            if (MENU_SYS_ERR == menu || menu == MENU_DISP_MSG_BOX || menu == MENU_LOW_BAT || menu == MENU_LIVE_REC_TIME)
            {
                 menu = get_back_menu(menu);
            }
        }
//        Log.d(TAG, "2 set back menu item %d menu %d", item, menu);
        if (item == menu)
        {
            Log.e(TAG,"same (%d %d)",item,menu);
            menu = get_back_menu(menu);
        }
//        Log.d(TAG, "3 set back menu item %d menu %d", item, menu);
        if (item != menu)
        {
            Log.d(TAG,"set back (%d %d)",item,menu);
            mMenuInfos[item].back_menu = menu;
        }
    }
    else
    {
        Log.e(TAG, "set_back_menu item %d", item);
#ifdef ENABLE_ABORT
        abort();
#endif
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
		cur_menu == MENU_DISP_MSG_BOX ||
		cur_menu == MENU_LOW_BAT ||
		cur_menu == MENU_LIVE_REC_TIME ||
		cur_menu == MENU_SET_PHOTO_DEALY /* || cur_menu == MENU_LOW_PROTECT*/)  {	/* add by skymixos */
        set_cur_menu_from_exit();
    } else if (cur_menu == MENU_STITCH_BOX) {
        if (!bStiching) {
            set_cur_menu_from_exit();
            send_option_to_fifo(ACTION_SET_STICH);
        }
    } else if (cur_menu == MENU_FORMAT_INDICATION) {
        rm_state(STATE_FORMAT_OVER);
        set_cur_menu_from_exit();
    } else {
		
        switch (cam_state) {
            //force back directly while state idle 17.06.08
            case STATE_IDLE:
                set_cur_menu_from_exit();
                break;
			
            case STATE_PREVIEW:
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
                        set_cur_menu(MENU_PIC_INFO);
                        break;
					
                    case MENU_PIC_SET_DEF:
                    case MENU_VIDEO_SET_DEF:
                    case MENU_LIVE_SET_DEF:
                    case MENU_SPEED_TEST:
                    case MENU_WIFI_CONNECT:
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
                                set_cur_menu(MENU_LIVE_INFO);
                            }
                        } else if (check_state_in(STATE_RECORD)) {
                            if (cur_menu != MENU_VIDEO_INFO) {
                                set_cur_menu(MENU_VIDEO_INFO);
                            }
                        } else if (check_state_in(STATE_CALIBRATING)) {
                            if (cur_menu != MENU_CALIBRATION) {
                                set_cur_menu(MENU_CALIBRATION);
                            }
                        } else if (check_state_in(STATE_SPEED_TEST)) {
                            if (cur_menu != MENU_SPEED_TEST) {
                                set_cur_menu(MENU_SPEED_TEST);
                            }
                        } else if (check_state_in(STATE_START_GYRO)) {
                            if (cur_menu != MENU_GYRO_START) {
                                set_cur_menu(MENU_GYRO_START);
                            }
                        } else if (check_state_in(STATE_NOISE_SAMPLE)) {
                            if (cur_menu != MENU_NOSIE_SAMPLE) {
                                set_cur_menu(MENU_NOSIE_SAMPLE);
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

//void MenuUI::clear_bottom()
//{
//    Log.d(TAG,"clear_bottom");
//    mOLEDModule->clear_area(0,48,128,16);
//    Log.d(TAG,"clear_bottom over");
//}

void MenuUI::update_menu_disp(const int *icon_light,const int *icon_normal)
{
    SELECT_INFO * mSelect = get_select_info();
    int last_select = get_last_select();
//    Log.d(TAG,"cur_menu %d last_select %d get_select() %d "
//                  " mSelect->cur_page %d mSelect->page_max %d total %d",
//          cur_menu,last_select, get_select(), mSelect->cur_page ,
//          mSelect->page_max,mSelect->total);
    if (icon_normal != nullptr) {
        //sometimes force last_select to -1 to avoid update last select
        if(last_select != -1)
        {
            disp_icon(icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    disp_icon(icon_light[get_menu_select_by_power(cur_menu)]);
}

void MenuUI::update_menu_disp(const ICON_INFO *icon_light,const ICON_INFO *icon_normal)
{
    SELECT_INFO * mSelect = get_select_info();
    int last_select = get_last_select();
	
    if (icon_normal != nullptr) {
        //sometimes force last_select to -1 to avoid update last select
        if (last_select != -1) {
            dispIconByLoc(&icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    dispIconByLoc(&icon_light[get_menu_select_by_power(cur_menu)]);

}



void MenuUI::update_menu_page()
{
    // CHECK_EQ(cur_menu,MENU_SYS_SETTING);
    if (cur_menu == MENU_SYS_SETTING)
	    disp_sys_setting(&mSettingItems);
	else if (cur_menu == MENU_SET_PHOTO_DEALY) {
		//clear_area(0, 16);
		//disp_icon(ICON_SET_PHOTO_DELAY_NV_0_16_32_48);
		clear_area(32, 16);
		disp_sys_setting(&mSetPhotoDelays);
	}
}

void MenuUI::set_mainmenu_item(int item,int val)
{
    switch(item)
    {
        case MAINMENU_WIFI:
            //off
//            Log.d(TAG,"set_mainmenu_item val %d",val);
            if(val == 0)
            {
                main_menu[0][item] = ICON_INDEX_IC_WIFICLOSE_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFICLOSE_LIGHT24_24;
            }
            else
            {
                main_menu[0][item] = ICON_INDEX_IC_WIFIOPEN_NORMAL24_24;
                main_menu[1][item] = ICON_INDEX_IC_WIFIOPEN_LIGHT24_24;
            }
            break;
    }
}

void MenuUI::update_menu_sys_setting(SETTING_ITEMS* pSetting, bool bUpdateLast)
{
    int last_select = get_last_select();
    int cur= get_menu_select_by_power(cur_menu);
	
    //sometimes force last_select to -1 to avoid update last select
//    Log.d(TAG,"cur %d last %d",cur,last_select);
    if (bUpdateLast)
    {
        if(last_select != -1)
        {
            SELECT_INFO * mSelect = get_select_info();
            int last = last_select + mSelect->cur_page * mSelect->page_max;
//            Log.d(TAG,"last %d",last);
            disp_icon(pSetting->icon_normal[last][get_setting_select(last)]);
        }
    }
    disp_icon(pSetting->icon_light[cur][get_setting_select(cur)]);
}

void MenuUI::update_sys_cfg(int item, int val)
{
    val = val&0x00000001;
    Log.d(TAG," update_sys_cfg item %d val %d",item,val);
    switch(item)
    {
        case SET_FREQ:
            mProCfg->set_val(KEY_PAL_NTSC, val);
            set_setting_select(item, val);
            break;
        case SET_SPEAK_ON:
            mProCfg->set_val(KEY_SPEAKER, val);
            set_setting_select(item, val);
            break;
        case SET_BOTTOM_LOGO:
            mProCfg->set_val(KEY_SET_LOGO, val);
            set_setting_select(item, val);
            break;
        case SET_LIGHT_ON:
            mProCfg->set_val(KEY_LIGHT_ON, val);
            set_setting_select(item, val);
            if (val == 1)
            {
                set_light();
            }
            else
            {
                set_light_direct(LIGHT_OFF);
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
            set_setting_select(item, val);
            break;
        case SET_GYRO_ON:
            mProCfg->set_val(KEY_GYRO_ON, val);
            set_setting_select(item, val);
            break;
        case SET_FAN_ON:
            mProCfg->set_val(KEY_FAN, val);
            set_setting_select(item, val);
            break;
        case SET_AUD_ON:
            mProCfg->set_val(KEY_AUD_ON, val);
            set_setting_select(item, val);
            break;
        case SET_VIDEO_SEGMENT:
            mProCfg->set_val(KEY_VID_SEG, val);
            set_setting_select(item, val);
            break;
            SWITCH_DEF_ERROR(item)
    }
}

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
    }
    else
    {
        Log.e(TAG,"%s cur_menu %d %d",
              __FUNCTION__,cur_menu,bStiching);
    }
}


void MenuUI::set_sys_setting(sp<struct _sys_setting_> &mSysSetting)
{
    CHECK_NE(mSysSetting,nullptr);

    Log.d(TAG,"%s %d %d %d %d %d %d %d %d %d %d",__FUNCTION__,
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
        if(mSysSetting->flicker != -1)
        {
            update_sys_cfg(SET_FREQ, mSysSetting->flicker);
        }
        if(mSysSetting->speaker != -1)
        {
            update_sys_cfg(SET_SPEAK_ON,mSysSetting->speaker);
        }
        if(mSysSetting->aud_on != -1)
        {
            update_sys_cfg(SET_AUD_ON,mSysSetting->aud_on);
        }
        if(mSysSetting->aud_spatial != -1)
        {
            update_sys_cfg(SET_SPATIAL_AUD,mSysSetting->aud_spatial);
        }
        if(mSysSetting->fan_on != -1)
        {
            update_sys_cfg(SET_FAN_ON,mSysSetting->fan_on);
        }
        if(mSysSetting->set_logo != -1)
        {
            update_sys_cfg(SET_BOTTOM_LOGO,mSysSetting->set_logo);
        }
        if(mSysSetting->led_on != -1)
        {
            update_sys_cfg(SET_LIGHT_ON,mSysSetting->led_on);
        }
        if(mSysSetting->gyro_on != -1)
        {
            update_sys_cfg(SET_GYRO_ON,mSysSetting->gyro_on);
        }
        if(mSysSetting->video_fragment != -1)
        {
            update_sys_cfg(SET_VIDEO_SEGMENT,mSysSetting->video_fragment);
        }
        //update current menu
        if(cur_menu == MENU_SYS_SETTING)
        {
            set_cur_menu(MENU_SYS_SETTING);
        }
    }
}



void MenuUI::add_qr_res(int type,sp<ACTION_INFO> &mAdd,int control_act)
{
    CHECK_NE(type,-1);
    CHECK_NE(mAdd,nullptr);
    int menu;
    int max;
    int key;
    sp<ACTION_INFO> mRes = sp<ACTION_INFO>(new ACTION_INFO());
    memcpy(mRes.get(), mAdd.get(),sizeof(ACTION_INFO));

    Log.d(TAG,"add_qr_res type (%d %d)",
          type,control_act );
    switch(control_act)
    {
        case ACTION_PIC:
        case ACTION_VIDEO:
        case ACTION_LIVE:
            mControlAct = mRes;
            //force 0 to 10
            if (mControlAct->size_per_act == 0)
            {
                Log.d(TAG,"force control size_per_act is %d",
                      mControlAct->size_per_act);
                mControlAct->size_per_act = 10;
            }
            break;
        case CONTROL_SET_CUSTOM:
            switch (type)
            {
                case ACTION_PIC:
                    key = KEY_PIC_DEF;
                    if(mRes->size_per_act == 0)
                    {
                        Log.d(TAG,"force CONTROL_SET_CUSTOM pic size_per_act is %d",
                              mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;
                case ACTION_VIDEO:
                    key = KEY_VIDEO_DEF;
                    if(mRes->size_per_act == 0)
                    {
                        Log.d(TAG,"force CONTROL_SET_CUSTOM video size_per_act is %d",
                              mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;
                case ACTION_LIVE:
                    key = KEY_LIVE_DEF;
                    fix_live_save_per_act(mRes.get());
                    break;
                SWITCH_DEF_ERROR(type);
            }
            mProCfg->set_def_info(key, -1, mRes);
            break;
        default:
        {
            //only save one firstly
            switch (type)
            {
                case ACTION_PIC:
                    menu = MENU_PIC_SET_DEF;
                    key = KEY_PIC_DEF;
                    max = PIC_CUSTOM;
                    if(mRes->size_per_act == 0)
                    {
                        Log.d(TAG,"force qr pic size_per_act is %d",
                              mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;
                case ACTION_VIDEO:
                    key = KEY_VIDEO_DEF;
                    menu = MENU_VIDEO_SET_DEF;
                    max = VID_CUSTOM;
                    if(mRes->size_per_act == 0)
                    {
                        Log.d(TAG,"force qr video size_per_act is %d",
                              mRes->size_per_act);
                        mRes->size_per_act = 10;
                    }
                    break;
                case ACTION_LIVE:
                    key = KEY_LIVE_DEF;
                    menu = MENU_LIVE_SET_DEF;
                    max = LIVE_CUSTOM;
                    fix_live_save_per_act(mRes.get());
                    break;
                SWITCH_DEF_ERROR(type);
            }
            mProCfg->set_def_info(key, max, mRes);
            set_menu_select_info(menu, max);
        }
    }
}

void MenuUI::update_menu()
{
    int item = get_menu_select_by_power(cur_menu);
    switch (cur_menu) {
        case MENU_TOP:
            update_menu_disp(main_menu[1], main_menu[0]);
            break;
		
        case MENU_SYS_SETTING:
            update_menu_sys_setting(&mSettingItems, true);
            break;
		
		case MENU_SET_PHOTO_DEALY:
            update_menu_sys_setting(&mSetPhotoDelays, true);
			break;
			
        case MENU_PIC_SET_DEF:
//            Log.d(TAG," pic set def item is %d", item);
            mProCfg->set_def_info(KEY_PIC_DEF, item);

			//update_menu_disp(pic_def_setting_menu[1]);

			update_menu_disp(picAllCardMenuItems[1]);

			
            if (item < PIC_ALL_CUSTOM) {
                disp_org_rts((int)(mPICAction[item].stOrgInfo.save_org != SAVE_OFF),
                             (int)(mPICAction[item].stStiInfo.stich_mode != STITCH_OFF));
            } else {
                disp_qr_res();
                disp_org_rts((int)(mProCfg->get_def_info(KEY_PIC_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             (int)(mProCfg->get_def_info(KEY_PIC_DEF)->stStiInfo.stich_mode != STITCH_OFF));
            }
            update_bottom_space();
            break;
			
        case MENU_VIDEO_SET_DEF:
//            Log.d(TAG," vid set def item is %d", item);
            mProCfg->set_def_info(KEY_VIDEO_DEF,item);
            update_menu_disp(vid_def_setting_menu[1]);
            if (item < VID_CUSTOM) {
                disp_org_rts((int)(mVIDAction[item].stOrgInfo.save_org != SAVE_OFF),
                             (int)(mVIDAction[item].stStiInfo.stich_mode != STITCH_OFF));
            } else {
                disp_org_rts((int)(mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             (int)(mProCfg->get_def_info(KEY_VIDEO_DEF)->stStiInfo.stich_mode != STITCH_OFF));
            }
           	update_bottom_space();
            break;
			
        case MENU_LIVE_SET_DEF:
            mProCfg->set_def_info(KEY_LIVE_DEF,item);
            update_menu_disp(live_def_setting_menu[1]);
            if (item < LIVE_CUSTOM) {
                disp_org_rts((int)(mLiveAction[item].stOrgInfo.save_org != SAVE_OFF),0,
                             (int)(mLiveAction[item].stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            } else {
                disp_org_rts((int)(mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             /*mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.file_save,*/0,
                             (int)(mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            }
            disp_live_ready();
            break;
			
        case MENU_SYS_DEV_INFO:
//            update_sys_info();
            break;

        case MENU_STORAGE:
            disp_storage_setting();
            break;
		
        case MENU_FORMAT:
            disp_format();
            break;
		
        SWITCH_DEF_ERROR(cur_menu)
    }
}

void MenuUI::get_storage_info()
{
    int storage_index;

    memset(total_space,0,sizeof(total_space));
    memset(used_space,0,sizeof(used_space));

//    Log.d(TAG,"get total mLocalStorageList.size() %d", mLocalStorageList.size());
    for(u32 i = 0; i < mLocalStorageList.size(); i++)
    {
        struct statfs diskInfo;
        u64 totalsize = 0;
        u64 used_size = 0;
        storage_index = get_dev_type_index(mLocalStorageList.at(i)->dev_type);

        if (access(mLocalStorageList.at(i)->path, F_OK) != -1)
        {
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
        }
        else
        {
            Log.d(TAG, "%s not access", mLocalStorageList.at(i)->path);
        }
    }
}

bool MenuUI::get_save_path_avail()
{
    bool ret = false;
    if (!check_save_path_none()) {
        int times = 3;
        int i = 0;
        for (i = 0; i < times; i++) {
            struct statfs diskInfo;
            if (access(mLocalStorageList.at(mSavePathIndex)->path, F_OK) != -1)
            {
                statfs(mLocalStorageList.at(mSavePathIndex)->path, &diskInfo);

                /*
                 * 老化模式下为了能让时间尽可能长,虚拟出足够长的录像时间
                 */
				#ifdef ENABLE_AGEING_MODE
                mLocalStorageList.at(mSavePathIndex)->avail = 0x10000000000;
				#else
                mLocalStorageList.at(mSavePathIndex)->avail = diskInfo.f_bavail *diskInfo.f_bsize;
				#endif
				
                Log.d(TAG, "remain ( %s %llu)\n",
                      mLocalStorageList.at(mSavePathIndex)->path,
                      mLocalStorageList.at(mSavePathIndex)->avail);
                break;
            }
            else
            {
                Log.d(TAG,"fail to access(%d) %s",i,
                      mLocalStorageList.at(mSavePathIndex)->path);
                msg_util::sleep_ms(10);
            }
        }
        if( i == times )
        {
            Log.d(TAG,"still fail to access %s",
                  mLocalStorageList.at(mSavePathIndex)->path);
            mLocalStorageList.at(mSavePathIndex)->avail = 0;
        }
        ret = true;
    }

    return ret;
}

void MenuUI::get_save_path_remain()
{
    if (get_save_path_avail()) {
        caculate_rest_info(mLocalStorageList.at(mSavePathIndex)->avail);
    }
}



/********************************************************************************************
** 函数名称: caculate_rest_info
** 函数功能: 计算剩余空间信息(可拍照片的张数,可录像的时长,可直播存储的时长)
** 入口参数: 存储容量的大小(单位为M)
** 返 回 值: 无(计算出的值来更新mRemainInfo对象)
** 调     用: get_save_path_remain
**
*********************************************************************************************/
void MenuUI::caculate_rest_info(u64 size)
{
    INFO_MENU_STATE(cur_menu, cam_state);
    int size_M = (int)(size >> 20);
	
//        Log.d(TAG,"size M %d %d",size_M);
    if (size_M >  AVAIL_SUBSTRACT) {
		
        int rest_sec = 0;
        int rest_min = 0;

        //deliberately minus 1024
        size_M -= AVAIL_SUBSTRACT;;
        int item = 0;
		
        Log.d(TAG, " curmenu %d item %d", cur_menu, item);
		
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if (mControlAct != nullptr) {
                    mRemainInfo->remain_pic_num = size_M/mControlAct->size_per_act;
                    Log.d(TAG, "0remain(%d %d)", size_M, mRemainInfo->remain_pic_num);
                } else {
					
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
                    switch (item) {
//                          case PIC_8K_3D_SAVE:
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
                            mRemainInfo->remain_pic_num = size_M/mPICAction[item].size_per_act;
                            Log.d(TAG, "1remain(%d %d %d)",
                                  size_M, mPICAction[item].size_per_act, mRemainInfo->remain_pic_num);
                            break;
							
                        case PIC_CUSTOM:
                            mRemainInfo->remain_pic_num = size_M/mProCfg->get_def_info(KEY_PIC_DEF)->size_per_act;
                            Log.d(TAG, "3remain(%d %d)", size_M, mRemainInfo->remain_pic_num);
                            break;
                        SWITCH_DEF_ERROR(item)
                    }
                }
                break;
				
            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF:
                //not cal while rec already started
                if (!(check_state_in(STATE_RECORD) && (mRecInfo->rec_hour > 0 || mRecInfo->rec_min > 0 || mRecInfo->rec_sec > 0))) {
                    if (mControlAct != nullptr) {
                        Log.d(TAG,"vid size_per_act %d",mControlAct->size_per_act);
                        rest_sec = size_M/mControlAct->size_per_act;
                    } else {
                        item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
                        if (item >= 0 && item < VID_CUSTOM) {
                            Log.d(TAG,"2vid size_per_act %d",
                                  mVIDAction[item].size_per_act);
                            rest_sec = (size_M/mVIDAction[item].size_per_act);
                        } else if (VID_CUSTOM == item) {
                            Log.d(TAG,"3vid size_per_act %d",
                                  mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act);
                            rest_sec = (size_M/mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act);
                        } else {
                            ERR_ITEM(item);
                        }
                    }
                    rest_min = rest_sec/60;
                    mRemainInfo->remain_sec = rest_sec%60;
                    mRemainInfo->remain_hour = rest_min/60;
                    mRemainInfo->remain_min = rest_min%60;
                    Log.d(TAG,"remain( %d %d "
                                  " %d  %d )",
                          size_M,
                          mRemainInfo->remain_hour,
                          mRemainInfo->remain_min,
                          mRemainInfo->remain_sec);
                }
                break;
            SWITCH_DEF_ERROR(cur_menu)
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
//    Log.d(TAG,"info_G is %.1f ",info_G);
    if (info_G >= 100.0) {
        snprintf(str,len,"%ldG",(int64_t)info_G);
    } else {
        snprintf(str,len,"%.1fG",info_G);
    }
}

bool MenuUI::check_save_path_none()
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
	
    Log.d(TAG,"mSavePathIndex is %d",mSavePathIndex);
	
    if (mSavePathIndex != -1 && (get_dev_type_index(mLocalStorageList.at(mSavePathIndex)->dev_type) == SET_STORAGE_USB)) {
        bRet = true;
    } else {
        disp_msg_box(DISP_LIVE_REC_USB);
    }
    return bRet;
}



void MenuUI::disp_bottom_space()
{
    if (!check_save_path_none()) {	/* 存储设备存在 */
        char disk_info[16];
        switch (cur_menu) {
            case MENU_PIC_INFO:		/* 拍照界面: 显示剩余可用的空间(张数) */
            case MENU_PIC_SET_DEF:
				
                //keep show in middle,easily to add two blanks
                snprintf(disk_info, sizeof(disk_info), "  %d", mRemainInfo->remain_pic_num);
                disp_str_fill((const u8 *) disk_info, 78, 48);
                break;

			/* 录像界面: */				
            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF:
//                Log.d(TAG,"tl_count %d",tl_count);
                if (tl_count == -1) {
                    if (check_rec_tl()) {
                        clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                    } else {
                        snprintf(disk_info, sizeof(disk_info), "%02d:%02d:%02d", mRemainInfo->remain_hour,
                                 mRemainInfo->remain_min, mRemainInfo->remain_sec);
                        disp_str_fill((const u8 *) disk_info, 78, 48);
                    }
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
                    int item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
//                    Log.d(TAG, "live def item %d", item);
                    switch (item) {
#ifdef LIVE_ORG
                        case LIVE_ORIGIN:
#endif
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
                        case VID_ARIAL:
                            if (mLiveAction[item].stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON) {
                                disp_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                            } else {
                                clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                            }
                        case LIVE_CUSTOM:
                            if (mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON) {
                                disp_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                            } else {
                                clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
                            }
                            break;
							
                        SWITCH_DEF_ERROR(item)
                    }
                }
                break;
            SWITCH_DEF_ERROR(cur_menu)
        }
    } else {	/* 存储设备不存在, 显示"None" */
        Log.d(TAG,"reset remain a");
        memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));
        disp_icon(ICON_LIVE_INFO_NONE_7848_50X16);
    }
	
}

void MenuUI::update_bottom_space()
{
    get_save_path_remain();
    disp_bottom_space();
}

void MenuUI::disp_bottom_info(bool high)
{
    disp_cam_param(0);
    update_bottom_space();
}

struct _select_info_ * MenuUI::get_select_info()
{
    return (SELECT_INFO *)&(mMenuInfos[cur_menu].mSelectInfo);
}


void MenuUI::disp_sys_setting(SETTING_ITEMS* pSetting)
{
    SELECT_INFO * mSelect = get_select_info();
    int start = mSelect->cur_page * mSelect->page_max;
    const int end = start + mSelect->page_max;
    const int select = get_menu_select_by_power(cur_menu);

    int item = 0;
	/* 5/3 = 1 8 (3, 6, 5) 5 = (1*3) + 2 */
    Log.d(TAG, "start %d end  %d select %d ", start,end,select);


    while (start < end) {
        if (start < mSelect->total) {
            int val = get_setting_select(start);
            if (start == select) {
//                Log.d(TAG,"select %d high icon %d",select, mSettingItems.icon_light[start][val]);
                disp_icon(pSetting->icon_light[start][val]);
            } else {
//                Log.d(TAG,"start %d normal icon %d",start, mSettingItems.icon_normal[start][val]);
                disp_icon(pSetting->icon_normal[start][val]);
            }
        }
		/*
        else
        {
//            Log.d(TAG,"item is %d", item);
            clear_icon(pSetting->clear_icons[(item - 1)]);
        }
        */
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
          get_menu_select_by_power(MENU_FORMAT),
          src,path,cur_menu);

    add_state(STATE_FORMATING);
    switch(get_menu_select_by_power(MENU_FORMAT))
    {
        //exfat
        case 0:
            snprintf(buf,sizeof(buf),"umount -f %s",path);
            if(exec_sh_new((const char *)buf) != 0)
            {
                goto ERROR;
            }

            snprintf(buf,sizeof(buf),"mke2fs -F -t ext4 %s",src);
            if(exec_sh_new((const char *)buf) != 0)
            {
                err_trim = 1;
            } 
            else
            {
                snprintf(buf,sizeof(buf),"mount -t ext4 -o discard %s %s",src,path);
                //            snprintf(buf,sizeof(buf),"mountufsd %s %s -o uid=1023,gid=1023,fmask=0700,dmask=0700,force",src,path);
                if(exec_sh_new((const char *)buf) != 0)
                {
                    err_trim = 1;
                }
                else
                {
                    snprintf(buf,sizeof(buf),"fstrim %s",path);
                    if(exec_sh_new((const char *)buf) != 0)
                    {
                        err_trim = 1;
                    }

                    snprintf(buf,sizeof(buf),"umount -f %s",path);
                    if(exec_sh_new((const char *)buf) != 0)
                    {
                        goto ERROR;
                    }
                  }
             }

            snprintf(buf,sizeof(buf),"mkexfat -f %s",src);
            if(exec_sh_new((const char *)buf) != 0)
            {
                goto ERROR;
            }
            Log.d(TAG,"err_trim %d",err_trim);
#ifdef NEW_FORMAT
            reset_devmanager();
#else
            snprintf(buf,sizeof(buf),"mountufsd %s %s "
                    "-o uid=1023,gid=1023,fmask=0700,dmask=0700,force",
                     src,path);
            if(exec_sh((const char *)buf) != 0)
            {
                goto ERROR;
            }
            else
            {
                Log.d(TAG,"mountufsd suc");
            }
#endif

            if (err_trim)
            {
                goto ERROR_TRIM;
            }

            switch(get_menu_select_by_power(MENU_STORAGE))
            {
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
        SWITCH_DEF_ERROR(get_menu_select_by_power(MENU_FORMAT));
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
    switch(get_menu_select_by_power(MENU_STORAGE))
    {
        case SET_STORAGE_SD:
            disp_icon(ICON_SDCARD_FORMATTING128_48);
            for(u32 i = 0; i < mLocalStorageList.size(); i++)
            {
                if(get_dev_type_index(mLocalStorageList.at(i)->dev_type) == SET_STORAGE_SD)
                {
                    format(mLocalStorageList.at(i)->src,mLocalStorageList.at(i)->path,ICON_FORMAT_REPLACE_NEW_SD_128_48128_48,ICON_SDCARD_FORMATTED_FAILED128_48,ICON_SDCARD_FORMATTED_SUCCESS128_48);
                    bFound = true;
                    break;
                }
            }
            break;
        case SET_STORAGE_USB:
            disp_icon(ICON_USBHARDDRIVE_FORMATTING128_48);
            for(u32 i = 0; i < mLocalStorageList.size(); i++)
            {
                if(get_dev_type_index(mLocalStorageList.at(i)->dev_type) == SET_STORAGE_USB)
                {
                    bFound = true;
                    format(mLocalStorageList.at(i)->src,mLocalStorageList.at(i)->path,
                           ICON_FORMAT_REPLACE_NEW_USB_128_48128_48,
                           ICON_USBDARDDRIVE_FORMATTED_FAILED128_48,ICON_USBHARDDRIVE_FORMATTED_SUCCESS128_48);
                    break;
                }
            }
            break;
        SWITCH_DEF_ERROR(get_menu_select_by_power(MENU_STORAGE))
    }
    if(!bFound)
    {
        Log.e(TAG,"no format device found");
        abort();
    }
    else
    {
//        msg_util::sleep_ms(3000);
        rm_state(STATE_FORMATING);
        add_state(STATE_FORMAT_OVER);
//        set_cur_menu(MENU_FORMAT);
    }
//    msg_util::sleep_ms(2000);
//    rm_state(STATE_FORMATING);
//    set_cur_menu(MENU_FORMAT);
}

void MenuUI::disp_format()
{
    //force clear STATE_FORMAT_OVER
    rm_state(STATE_FORMAT_OVER);
#ifdef ONLY_EXFAT
    disp_icon(ICON_SD_FORMAT_EXFAT_LIGHT90_16);
#else
    //exfat high
    if (get_menu_select_by_power(MENU_FORMAT) == 0) {
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
	
    for (u32 i = 0; i < sizeof(used_space)/sizeof(used_space[0]); i++) {
        switch (i) {
            case SET_STORAGE_SD:
            case SET_STORAGE_USB:
                if (strlen(used_space[i]) == 0) {
                    snprintf(dev_space,sizeof(dev_space), "%s:None", dev_type[i]);
                } else {
                    snprintf(dev_space,sizeof(dev_space),"%s:%s/%s",
                             dev_type[i],used_space[i],total_space[i]);
                }

                Log.d(TAG,"disp storage[%d] select %d %s (%d %d %d)",
                      i,get_menu_select_by_power(MENU_STORAGE),
                      dev_space,storage_area[i].x,storage_area[i].y,storage_area[i].w);

                if (get_menu_select_by_power(MENU_STORAGE) == (int)i) {
                    disp_str((const u8 *)dev_space,storage_area[i].x,storage_area[i].y,true,storage_area[i].w);
                } else {
                    disp_str((const u8 *)dev_space,storage_area[i].x,storage_area[i].y,false,storage_area[i].w);
                }
                break;
            SWITCH_DEF_ERROR(i)
        }
    }
}

void MenuUI::disp_org_rts(sp<struct _action_info_> &mAct,int hdmi)
{
    disp_org_rts((int) (mAct->stOrgInfo.save_org != SAVE_OFF),
                 (int) (mAct->stStiInfo.stich_mode != STITCH_OFF),hdmi);
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
    //force update even not change 0623
//    if(last_org_rts != new_org_rts)
    {
//        last_org_rts = new_org_rts;
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
    if(hdmi == 0)
    {
        clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    }
    else if(hdmi == 1)
    {
//        disp_str_fill((const u8 *)"  HDMI",bottom_info_start,48,0);
        disp_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    }
//    Log.d(TAG,"disp_org_rts org %d rts %d hdmi %d",org,rts,hdmi);
}

void MenuUI::disp_qr_res(bool high)
{
    if (high) {
        disp_icon(ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16);
    } else {
        disp_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
    }
}


// highlight : 0 -- normal 1 -- high
void MenuUI::disp_cam_param(int higlight)
{
    int item;
    int icon = -1;
	ICON_INFO* pIconInfo = NULL;
	
//    Log.d(TAG,"disp_cam_param high %d cur_menu %d bDispControl %d",
//          higlight,cur_menu);
    {
        INFO_MENU_STATE(cur_menu,cam_state)
        switch (cur_menu) {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if (mControlAct != nullptr) {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else {
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
//                    Log.d(TAG, "pic def item %d", item);
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

							disp_org_rts((int) (mProCfg->get_def_info(KEY_PIC_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mProCfg->get_def_info(KEY_PIC_DEF)->stStiInfo.stich_mode !=
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
                    disp_org_rts(0,0);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else if (mControlAct != nullptr) {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                } else {
                    item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
//                    Log.d(TAG, "video def item %d", item);
                    if(item >= 0 && item < VID_CUSTOM)
                    {
                        icon = vid_def_setting_menu[higlight][item];
                        disp_org_rts((int) (mVIDAction[item].stOrgInfo.save_org != SAVE_OFF),
                                     (int) (mVIDAction[item].stStiInfo.stich_mode != STITCH_OFF));
                    }
                    else if(VID_CUSTOM == item)
                    {
                        icon = vid_def_setting_menu[higlight][item];
                        disp_org_rts((int) (mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                     (int) (mProCfg->get_def_info(KEY_VIDEO_DEF)->stStiInfo.stich_mode != STITCH_OFF));
                    }
                    else
                    {
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
                    item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
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
                            disp_org_rts((int) (mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                         0,/*mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.file_save,*/
                                         (int) (mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on ==
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
//            cam_state |= STATE_CALIBRATE_FAIL;
//            Log.d(TAG,"cal fail state 0x%x",cam_state);
//            CHECK_EQ(cam_state,STATE_CALIBRATE_FAIL);
//            disp_icon(ICON_CALIBRATION_FAILED128_16);
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

bool MenuUI::is_top_clear(int menu)
{
    bool ret= false;
    switch (menu) {
        case MENU_SYS_ERR:
        case MENU_DISP_MSG_BOX:
        case MENU_CALIBRATION:
        case MENU_QR_SCAN:
        case MENU_SPEED_TEST:
        case MENU_WIFI_CONNECT:
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
    set_light_direct(BACK_RED|FRONT_RED);
}

void MenuUI::func_low_protect()
{
#if 0
    int times = 10;
    bool bSend = true;
    bool bCharge;
    int ret;
    for(int i = 0; i < times; i++)
    {
        disp_sec(times - i,52,48);
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
    Log.d(TAG,"func_low_protect %d",bSend);
    if(bSend)
    {
        send_option_to_fifo(ACTION_LOW_PROTECT,REBOOT_SHUTDOWN);
        set_back_menu(MENU_LOW_PROTECT,MENU_TOP);
        disp_low_protect();
        cam_state = STATE_IDLE;
    }
    else
    {
//        //update bat icon
        oled_disp_battery();
        // back for battery charge
        procBackKeyEvent();
    }
    Log.d(TAG,"func_low_protect %d menu %d state 0x%x",
          bSend,cur_menu,cam_state);
#endif
}

bool MenuUI::isDevExist()
{
    int item = get_menu_select_by_power(MENU_STORAGE);
    bool bFound = false;
    for(u32 i = 0;i < mLocalStorageList.size(); i++)
    {
        if(get_dev_type_index(mLocalStorageList.at(i)->dev_type) == item)
        {
            bFound = true;
            break;
        }
    }
    Log.i(TAG,"isDevExist item %d bFound %d",item,bFound);
    return bFound;
}

void MenuUI::disp_menu(bool dispBottom)
{
	Log.d(TAG, "disp_menu is %d", cur_menu);
    switch (cur_menu) {
        case MENU_TOP:
            disp_icon(main_icons[mProCfg->get_val(KEY_WIFI_ON)][get_select()]);
            break;
		
        case MENU_PIC_INFO:
			
            disp_icon(ICON_CAMERA_ICON_0_16_20_32);		/* 显示左侧'拍照'图标 */

            Log.d(TAG, "disp_bottom_info %d", dispBottom);

            if (dispBottom) {	/* 显示底部和右侧信息 */
                disp_bottom_info();
            }
			
            if (check_state_preview()) {	/* 启动预览成功,显示"Ready" */
                disp_ready_icon();
            } else if (check_state_equal(STATE_START_PREVIEWING) || check_state_in(STATE_STOP_PREVIEWING)) {
                disp_waiting();				/* 正在启动,显示"..." */
            } else if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {	/* 正在拍照 */
                if (cap_delay == 0) {	/* 倒计时为0,显示"shooting" */
                    disp_shooting();
                } else {				/* 清除就绪图标,等待下一次更新消息 */
                    clear_ready();
                }
            } else if (check_state_in(STATE_PIC_STITCHING)) {	/* 如果正在拼接,显示"processing" */
                disp_processing();
            } else {
                Log.d(TAG,"pic menu error state 0x%x",cam_state);
                if (check_state_equal(STATE_IDLE)) {
                    procBackKeyEvent();
                }
            }
            break;
			
        case MENU_VIDEO_INFO:
            disp_icon(ICON_VIDEO_ICON_0_16_20_32);
            disp_bottom_info();
            if (check_state_preview()) {
                disp_ready_icon();
            } else if (check_state_equal(STATE_START_PREVIEWING) ||
                    (check_state_in(STATE_STOP_PREVIEWING)) || check_state_in(STATE_START_RECORDING))
            {
                disp_waiting();
            }
            else if (check_state_in(STATE_RECORD))
            {
                Log.d(TAG,"do nothing in rec cam state 0x%x",
                      cam_state);
                if(tl_count != -1)
                {
                    clear_ready();
                }
            }
            else
            {
                Log.d(TAG,"vid menu error state 0x%x menu %d",cam_state,cur_menu);
                if(check_state_equal(STATE_IDLE))
                {
                    procBackKeyEvent();
                }
            }
            break;
        case MENU_LIVE_INFO:
            INFO_MENU_STATE(cur_menu,cam_state);
            disp_icon(ICON_LIVE_ICON_0_16_20_32);
            disp_cam_param(0);
            if(check_state_preview())
            {
                disp_live_ready();
            }
            else if(check_state_equal(STATE_START_PREVIEWING) ||
                    check_state_in(STATE_STOP_PREVIEWING) || check_state_in(STATE_START_LIVING))
            {
                disp_waiting();
            }
            else if (check_state_in(STATE_LIVE))
            {
                Log.d(TAG,"do nothing in live cam state 0x%x",cam_state);
            }
            else if (check_state_in(STATE_LIVE_CONNECTING))
            {
                disp_connecting();
            }
            else
            {
                Log.d(TAG,"live menu error state 0x%x",cam_state);
                if(check_state_equal(STATE_IDLE))
                {
                    procBackKeyEvent();
                }
            }
            break;
			
        case MENU_STORAGE:
            clear_area(0,16);
            disp_icon(ICON_STORAGE_IC_DEFAULT_0016_25X48);
            get_storage_info();
            disp_storage_setting();
            break;

		/* 进入MENU_SET_PHOTO_DELAY菜单 */
		case MENU_SET_PHOTO_DEALY:
			{
			clear_area(0, 16);									/* 清除真个区域 */
			disp_icon(ICON_SET_PHOTO_DELAY_NV_0_16_32_48);		/* 绘制左侧"Photo Delay"图标 */

			Log.d(TAG, "enter MENU_SET_PHOTO_DELAY menu, select index [%d]", set_photo_delay_index);

			/* #BUG1147
			 * 需要根据当前"Photo Delay"条目的选中的值来设置进入MENU_SET_PHOTO_DELAY菜单需要
			 * 显示的页（set_photo_delay_index）
			 */
			SELECT_INFO * mSelect = get_select_info();

			/* 需要设置mSelect->cur_page,   mSelect->select */
			mSelect->cur_page = (set_photo_delay_index / mSelect->page_max);
			mSelect->select = set_photo_delay_index - (mSelect->cur_page * mSelect->page_max);

			Log.d(TAG, "cur page [%d], select [%d]", mSelect->cur_page, mSelect->select);
            disp_sys_setting(&mSetPhotoDelays);					/* 显示"右侧"的项 */
			break;
			}
		
        case MENU_SYS_SETTING:
            clear_area(0,16);
            disp_icon(ICON_SET_IC_DEFAULT_25_48_0_1625_48);
            disp_sys_setting(&mSettingItems);
            break;
			
        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
        case MENU_LIVE_SET_DEF:
            disp_cam_param(1);
            break;
		
        case MENU_QR_SCAN:
            clear_area();
//                reset_last_info();
//                Log.d(TAG,"disp MENU_QR_SCAN state is 0x%x",cam_state);
            INFO_MENU_STATE(cur_menu,cam_state)
            if(check_state_in(STATE_START_QRING) || check_state_in(STATE_STOP_QRING))
            {
                disp_waiting();
            }
            else if (check_state_in(STATE_START_QR))
            {
                disp_icon(ICON_QR_SCAN128_64);
            }
            else
            {
                if(check_state_equal(STATE_IDLE))
                {
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
#if 0
        case MENU_PIC_SETTING:
//            disp_pic_setting();
            disp_left_right_menu((LR_MENU *) &lr_menu_items[cur_menu - MENU_PIC_SETTING]);
            break;
        case MENU_VIDEO_SETTING:
            disp_video_setting();
//            disp_icon(ICON_VIDEO_LEFT);
//            disp_icon(ICON_LINE_Y);
            break;
        case MENU_LIVE_SETTING:
            disp_live_setting();
//            disp_icon(ICON_LIVE_LEFT);
//            disp_icon(ICON_LINE_Y);
            break;
        case MENU_VID_EACH_LEN:
//            disp_vid_each_len();
            break;
        case MENU_VID_BR:
//            disp_vid_br();
            break;
        case MENU_VID_OUT:
//            disp_vid_outp();
            break;
            //video
        case MENU_LIVE_BR:
//            disp_live_br();
            break;
        case MENU_LIVE_OUT:
//            disp_live_outp();
            break;
#endif
            //menu sys device
//            case MENU_SYS_DEV:
//                disp_sys_dev();
//                break;
        case MENU_SYS_DEV_INFO:
            disp_sys_info();
            break;
//            case MENU_SYS_DEV_FACTORY_DEFAULT:
//                disp_sys_dev_fac_default();
//                break;
//            case MENU_CALIBRATION_SETTING:
//                disp_cal_setting();
//                break;
        case MENU_SYS_ERR:
            set_light_direct(BACK_RED|FRONT_RED);
            disp_icon(ICON_ERROR_128_64128_64);
            break;
			
        case MENU_DISP_MSG_BOX:
            break;
			
        case MENU_LOW_BAT:
//            disp_icon(ICON_LOW_BAT_128_64128_64);
            disp_low_bat();
            break;
		
//        case MENU_LOW_PROTECT:
//            disp_low_protect(true);
//            break;
        case MENU_GYRO_START:
//                CHECK_EQ(cam_state,STATE_IDLE);
            if(check_state_equal(STATE_START_GYRO))
            {
                disp_icon(ICON_GYRO_CALIBRATING128_48);
            }
            else
            {
                disp_icon(ICON_HORIZONTALLY01128_48);
            }
            break;
        case MENU_SPEED_TEST:
        {
//                Log.d(TAG,"mSavePathIndex is %d",mSavePathIndex);
            if(!check_save_path_none())
            {
                int dev_index = get_dev_type_index(mLocalStorageList.at(mSavePathIndex)->dev_type);
                if(check_state_in(STATE_SPEED_TEST))
                {
                    switch(dev_index)
                    {
                        case SET_STORAGE_SD:
                            disp_icon(ICON_SPEEDTEST03128_64);
                            break;
                        case SET_STORAGE_USB:
                            disp_icon(ICON_SPEEDTEST04128_64);
                            break;
                        default:
                            Log.d(TAG,"STATE_SPEED_TEST mSavePathIndex is -1");
#ifdef ENABLE_ABORT
                            abort();
#else
                            disp_icon(ICON_SPEEDTEST03128_64);
#endif
                            break;
                    }
                }
                else
                {
                    switch(dev_index)
                    {
                        case SET_STORAGE_SD:
                            disp_icon(ICON_SPEEDTEST01128_64);
                            break;
                        case SET_STORAGE_USB:
                            disp_icon(ICON_SPEEDTEST02128_64);
                            break;
                        default:
                            Log.d(TAG,"mSavePathIndex is -1");
//                        abort();
                            disp_icon(ICON_SPEEDTEST01128_64);
                            break;
                    }
                }
            }
            else
            {
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
            if(isDevExist())
            {
                disp_icon(ICON_FORMAT_MSG_128_48128_48);
            }
            else
            {
                set_cur_menu(MENU_STORAGE);
            }
            break;
        case MENU_WIFI_CONNECT:
            disp_wifi_connect();
            break;
        case MENU_AGEING:
            disp_ageing();
            break;
        case MENU_NOSIE_SAMPLE:
            disp_icon(ICON_SAMPLING_128_48128_48);
            break;
        case MENU_LIVE_REC_TIME:
            break;
        case MENU_STITCH_BOX:
            bStiching = false;
            disp_icon(ICON_WAITING_STITCH_128_48128_48);
            break;
        case MENU_FORMAT:
        {
            int item = get_menu_select_by_power(MENU_STORAGE);
            if(isDevExist())
            {
                switch(item)
                {
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
            }
            else
            {
                rm_state(STATE_FORMAT_OVER);
                set_cur_menu(MENU_STORAGE);
            }
        }
        break;
        SWITCH_DEF_ERROR(cur_menu);
    }
//    Log.d(TAG,"cur menu %d disptop %d",cur_menu,bDispTop);
    if(is_top_clear(cur_menu))
    {
        reset_last_info();
        bDispTop = false;
    }
    else if(!bDispTop)
    {
        disp_top_info();
    }
}

void MenuUI::disp_wifi_connect()
{
    disp_icon(ICON_WIF_CONNECT_128_64128_64);
}


bool MenuUI::check_dev_exist(int action)
{
    bool bRet = false;
    Log.d(TAG,"check_dev_exist (%d %d)",mLocalStorageList.size(),mSavePathIndex);

    if (!check_save_path_none()) {
        switch (action) {
//            case ACTION_CALIBRATION:
            case ACTION_PIC:
                if (mRemainInfo->remain_pic_num > 0) {
                    bRet = true;
                } else {
                    //disp full
                    Log.e(TAG,"pic dev full");
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
	if ((cam_state & STATE_CALIBRATING) != STATE_CALIBRATING) {
		setGyroCalcDelay(5);
		oled_disp_type(START_CALIBRATIONING);
	} else {
		Log.e(TAG, "handleGyroCalcEvent: calibration happen cam_state 0x%x", cam_state);
	}
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




/*************************************************************************
** 方法名称: func_power
** 方法功能: POWER/OK键按下的处理
** 入口参数: 
**		无
** 返 回 值: 无
** 调     用: 
** 
{"name":"camera._queryStorage"}
*************************************************************************/
void MenuUI::procPowerKeyEvent()
{
	Log.d(TAG, "cur_menu %d select %d\n", cur_menu, get_select());

	/* 根据当前的菜单做出不同的处理 */
    switch (cur_menu) {	/* 根据当前的菜单做出响应的处理 */
        case MENU_TOP:	/* 顶层菜单 */
            switch (get_select()) {	/* 获取当前选择的菜单项 */
                case MAINMENU_PIC:	/* 选择的是"拍照"项 */

					/* 对于新版版: 发送"启动预览"后,需要查询当前卡的状态
			 		 * 6 + 1, 6, 1
			 		 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {	/* 发送预览请求: 消息发送完成后需要接收到异步结果 */
						
                        oled_disp_type(START_PREVIEWING);	/* 屏幕中间会显示"..." */

						/* 通过查询系统存储系统来决定存储策略 */

						/* 
						 * TODO: update storage status(default 6+1)
						 */
                        set_cur_menu(MENU_PIC_INFO);	/* 设置并显示当前菜单 */
                    } else {
                        Log.d(TAG, "pic preview fail?");
                    }
                    break;
					
                case MAINMENU_VIDEO:	/* 选择的是"录像"项 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {	/* 发送预览请求 */
                        oled_disp_type(START_PREVIEWING);
                        set_cur_menu(MENU_VIDEO_INFO);
                    } else {
                        Log.d(TAG, "vid preview fail?");
                    }
                    break;
					
                case MAINMENU_LIVE:		/* 选择的是"Living"项 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {
                        oled_disp_type(START_PREVIEWING);
                        set_cur_menu(MENU_LIVE_INFO);
                    } else {
                        Log.d(TAG, "live preview fail?");
                    }
                    break;
					
                case MAINMENU_WIFI:     		/* WiFi菜单项用于打开关闭AP */
                    wifi_action();
                    break;
				
                case MAINMENU_CALIBRATION:		/* 陀螺仪校正 */
                    //power_menu_cal_setting();
					handleGyroCalcEvent();
                    break;
				
                case MAINMENU_SETTING:
                    set_cur_menu(MENU_SYS_SETTING);
                    break;
				
                default:
                    break;
            }
            break;
			
        case MENU_PIC_INFO:		/* 拍照子菜单 */
            send_option_to_fifo(ACTION_PIC);	/* 发送拍照请求 */
            break;
		
        case MENU_VIDEO_INFO:	/* 录像子菜单 */
            send_option_to_fifo(ACTION_VIDEO);
            break;
		
        case MENU_LIVE_INFO:	/* 直播子菜单 */
            send_option_to_fifo(ACTION_LIVE);
            break;

		/* add by skymixos */
		case MENU_SET_PHOTO_DEALY: {
            set_photo_delay_index = get_menu_select_by_power(cur_menu);
			
			Log.d(TAG, ">>>>>> get photo delay index [%d]", set_photo_delay_index);
			#if 0
			mProCfg->set_val(KEY_PHOTO_DELAY, item);
			#endif
			set_phdelay_index(set_photo_delay_index);
			
			/* 更新Setting菜单中delay值 */
			set_photodelay_select(set_photo_delay_index);

			/* 更新所有拍照选项的delay值 */
			update_takepic_delay(set_photo_delay_index);
			
			procBackKeyEvent();
			
			break;

		}
			
        case MENU_SYS_SETTING: {
            int item = get_menu_select_by_power(cur_menu);
            int val = get_setting_select(item);
            switch (item) {
                case SET_WIFI_AP:
                    val = ((~val) & 0x00000001);
                    // wifi is on need switch
                    if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
                        //mProCfg->set_val(KEY_WIFI_AP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting(&mSettingItems);
                        //start_wifi(1);
                    } else {
                        //mProCfg->set_val(KEY_WIFI_AP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting(&mSettingItems);
                    }

                    break;
					
                case SET_DHCP_MODE: /* 用于设置eth0的IP地址 */
                    val = ((~val) & 0x00000001);
                    if (switchEtherIpMode(val)) {
                        mProCfg->set_val(KEY_DHCP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting(&mSettingItems);
                    }
                    break;
					
                case SET_FREQ:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_PAL_NTSC, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);
                    break;

				/* set photodelay  add by skymixos 2018-06-13 
				 * 进入MENU_SET_PHOTO_DELAY菜单
				 */
				case SET_PHOTO_DELAY:
                    set_cur_menu(MENU_SET_PHOTO_DEALY);
					break;
				
                case SET_SPEAK_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_SPEAKER, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    break;
                case SET_BOTTOM_LOGO:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_SET_LOGO, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);
                    break;
                case SET_LIGHT_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_LIGHT_ON, val);
                    set_setting_select(item, val);
                    if (val == 1)
                    {
                        set_light();
                    }
                    else
                    {
                        set_light_direct(LIGHT_OFF);
                    }
                    update_menu_sys_setting(&mSettingItems);
                    break;
                case SET_STORAGE:
                    set_cur_menu(MENU_STORAGE);
                    break;
				
                case SET_RESTORE:
                    set_cur_menu(MENU_RESET_INDICATION);
                    break;
                case SET_SPATIAL_AUD:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_AUD_SPATIAL, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    if(get_setting_select(SET_AUD_ON) == 1)
                    {
                        send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
                    }
                    break;
                case SET_GYRO_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_GYRO_ON, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);
                    break;
                case SET_FAN_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_FAN, val);
                    set_setting_select(item, val);
                    if (val == 0)
                    {
                        disp_msg_box(DISP_ALERT_FAN_OFF);
                    }
                    else
                    {
                        update_menu_sys_setting(&mSettingItems);
                    }
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_FAN);
                    break;
                case SET_AUD_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_AUD_ON, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting(&mSettingItems);
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
                    break;
                case SET_INFO:
                    set_cur_menu(MENU_SYS_DEV_INFO);
                    break;
                case SET_START_GYRO:
                    set_cur_menu(MENU_GYRO_START);
                    break;
                case SET_NOISE:
                    send_option_to_fifo(ACTION_NOISE);
                    break;
                case SET_VIDEO_SEGMENT:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_VID_SEG, val);
                    set_setting_select(item, val);
                    if (val == 0)
                    {
                        disp_msg_box(DISP_VID_SEGMENT);
                    }
                    else
                    {
                        update_menu_sys_setting(&mSettingItems);
                    }
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_VID_SEG);
                    break;
                case SET_STITCH_BOX:
                    set_cur_menu(MENU_STITCH_BOX);
                    send_option_to_fifo(ACTION_SET_STICH);
                    break;
                SWITCH_DEF_ERROR(get_menu_select_by_power(cur_menu))
            }
        }
            break;
//        case MENU_CALIBRATION_SETTING:
//            power_menu_cal_setting();
//            break;
        case MENU_PIC_SET_DEF:
            procBackKeyEvent();
            break;
		
        case MENU_VIDEO_SET_DEF:
            procBackKeyEvent();
            break;
		
        case MENU_LIVE_SET_DEF:
            procBackKeyEvent();
            break;

        case MENU_GYRO_START:
            if(check_state_equal(STATE_IDLE))
            {
                send_option_to_fifo(ACTION_GYRO);
            }
            else
            {
                Log.d(TAG,"gyro start cam_state 0x%x",cam_state);
            }
            break;
			
        case MENU_SPEED_TEST:
            if (check_state_equal(STATE_PREVIEW)) {
                send_option_to_fifo(ACTION_SPEED_TEST);
            } else
            {
                Log.d(TAG,"speed cam_state 0x%x",cam_state);
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
		
        case MENU_STITCH_BOX:
            if (!bStiching) {
                set_cur_menu_from_exit();
                send_option_to_fifo(ACTION_SET_STICH);
            }
            break;

        case MENU_STORAGE: {
            int select = get_menu_select_by_power(MENU_STORAGE);
            Log.d(TAG,"used_space[%d] %s",select,used_space[select]);
            if (strlen(used_space[select]) != 0) {
                set_cur_menu(MENU_FORMAT);
            }
			break;
        }
            
        case MENU_FORMAT:
            set_cur_menu(MENU_FORMAT_INDICATION);
            break;
		
        SWITCH_DEF_ERROR(cur_menu)
    }
}


int MenuUI::get_select()
{
    return mMenuInfos[cur_menu].mSelectInfo.select;
}

int MenuUI::get_last_select()
{
    return mMenuInfos[cur_menu].mSelectInfo.last_select;
}



/*************************************************************************
** 方法名称: func_up
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
void MenuUI::procSettingKeyEvent()
{
//    Log.d(TAG," func_setting cur_menu %d cam_state %d", cur_menu, cam_state);

    switch (cur_menu) {
        case MENU_TOP:	/* 如果当前处于主界面 */
            if (get_select() == MAINMENU_WIFI) {	/* 主界面,当前选中的是WIFI项,按下设置键将启动二维码扫描功能 */
                Log.d(TAG, "wif state %d ap %d", mProCfg->get_val(KEY_WIFI_ON), get_setting_select(SET_WIFI_AP));
                if (/*mProCfg->get_val(KEY_WIFI_ON) == 0 &&*/ get_setting_select(SET_WIFI_AP) == 0) {
                    start_qr_func();
                }
            } else {	/* 主界面直接按"设置"键,将跳到设置菜单 */
                set_cur_menu(MENU_SYS_SETTING);
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
** 方法名称: func_up
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返 回 值: 无 
** 调     用: 
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
        select_up();
        break;
    }
}



/*************************************************************************
** 方法名称: func_down
** 方法功能: 按键事件处理(方向下)
** 入口参数: 
** 返 回 值: 无 
** 调     用: handle_oled_key
**
*************************************************************************/
void MenuUI::procDownKeyEvent()
{
    switch (cur_menu) {	/* 对于MENU_PIC_INFO/MENU_VIDEO_INFO/MENU_LIVE_INFO第一次按下将进入XXX_XXX_SET_DEF菜单 */
    case MENU_PIC_INFO:
	    if (check_state_preview()) {
		    set_cur_menu(MENU_PIC_SET_DEF);
	    }
	    break;
		
    case MENU_VIDEO_INFO:
        if (check_state_preview()) {
            set_cur_menu(MENU_VIDEO_SET_DEF);
        }
        break;
		
    case MENU_LIVE_INFO:
        if (check_state_preview()) {
            set_cur_menu(MENU_LIVE_SET_DEF);
        }
        break;
		
	default:
	    select_down();
	    break;
    }
}



void MenuUI::exit_sys_err()
{
    if (cur_menu == MENU_SYS_ERR ||
            ((MENU_LOW_BAT == cur_menu) && check_state_equal(STATE_IDLE))) {

        Log.d(TAG, "exit_sys_err ( %d 0x%x )", cur_menu, cam_state);

        //force set front light
        if (get_setting_select(SET_LIGHT_ON) == 1) {
            set_light_direct(front_light);
        } else {
            set_light_direct(LIGHT_OFF);
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
** 方法名称: handle_oled_key
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
void MenuUI::handleKeyMsg(int iKey)
{
    if (cur_menu == -1) {
        Log.d(TAG,"pro_service not finish yet");
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
    return !is_top_clear(cur_menu);
}



int MenuUI::oled_disp_battery()
{

#ifdef DEBUG_BATTERY
    Log.d(TAG,"mBatInterface->isSuc()  %d "
                  "m_bat_info_->bCharge %d "
                  "m_bat_info_->battery_level %d",
          mBatInterface->isSuc(),
          m_bat_info_->bCharge,
          m_bat_info_->battery_level);
#endif

    if (mBatInterface->isSuc()) {
        int icon;
        const int x = 110;
        u8 buf[16];
        if (m_bat_info_->bCharge && m_bat_info_->battery_level < 100) {
            icon = ICON_BATTERY_IC_CHARGE_103_0_6_166_16;
        } else {
            icon = ICON_BATTERY_IC_FULL_103_0_6_166_16;
        }
		
        if (check_allow_update_top()) {
            if (m_bat_info_->battery_level == 1000) {
                m_bat_info_->battery_level = 0;
            }
			
            if ( m_bat_info_->battery_level >= 100) {
                snprintf((char *) buf, sizeof(buf), "%d", 100);
            } else {
                snprintf((char *)buf, sizeof(buf), "%d", m_bat_info_->battery_level);
            }
			
            disp_str_fill(buf, x, 0);
            disp_icon(icon);
        }
    } else {
        //disp nothing while no bat
        clear_area(103,0,25,16);
    }
	
    set_light();
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
    int item = get_menu_select_by_power(MENU_LIVE_SET_DEF);

    switch (item) {
#ifdef LIVE_ORG
        case LIVE_ORIGIN:
#endif
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
        case VID_ARIAL:
            if (mLiveAction[item].stOrgInfo.save_org != SAVE_OFF && check_save_path_none()) {
                bReady = false;
            }
            break;
			
        case LIVE_CUSTOM:
            if (check_live_save(mProCfg->get_def_info(KEY_LIVE_DEF)) && check_save_path_none()) {
                bReady = false;
            }
            break;
        SWITCH_DEF_ERROR(item)
    }

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
    if (!check_save_path_none()) {
        disp_icon(ICON_CAMERA_READY_20_16_76_32);
    } else {
        disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
    }
}


void MenuUI::disp_shooting()
{
//    if(!check_save_path_none())
//    {
    Log.d(TAG,"check_save_path_none() is %d",check_save_path_none());
    disp_icon(ICON_CAMERA_SHOOTING_2016_76X32);
//    }
    set_light(fli_light);
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
//            set_menu_select_info(MENU_PIC_SET_DEF,PIC_DEF_MAX);
//            break;
//        case ACTION_VIDEO:
//            set_menu_select_info(MENU_VIDEO_SET_DEF,VID_DEF_MAX);
//            break;
//        case ACTION_LIVE:
//            set_menu_select_info(MENU_LIVE_SET_DEF,LIVE_DEF_MAX);
//            break;
//        SWITCH_DEF_ERROR(action);
//    }
}

void MenuUI::add_state(int state)
{
    cam_state |= state;
	
    switch (state) {
		
		case STATE_QUERY_STORAGE:
			break;
	
        case STATE_STOP_RECORDING:
            disp_saving();
            break;
		
        case STATE_START_PREVIEWING:
        case STATE_STOP_PREVIEWING:
        case STATE_START_RECORDING:
//        case STATE_STOP_RECORDING:
        case STATE_START_LIVING:
        case STATE_STOP_LIVING:
            disp_waiting();		/* 屏幕中间显示"..." */
            break;
		
        case STATE_TAKE_CAPTURE_IN_PROCESS:
        case STATE_PIC_STITCHING:
            //force pic menu to make bottom info updated ,if req from http
            set_cur_menu(MENU_PIC_INFO);
            break;
		
        case STATE_RECORD:
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
                        set_cur_menu(MENU_PIC_INFO);
                        break;
                }
            }
            break;


			
        case STATE_CALIBRATING:
//            Log.d(TAG,"state clibration cur_menu %d",cur_menu);
//            if(cur_menu != MENU_CALIBRATION)
//            {
//                set_cur_menu(MENU_CALIBRATION);
//            }
            break;
        case STATE_START_QRING:
            if (cur_menu != MENU_QR_SCAN) {
                set_cur_menu(MENU_QR_SCAN);
            }
            break;
			
        case STATE_START_QR:
            Log.d(TAG,"start qr cur_menu %d", cur_menu);
            rm_state(STATE_START_QRING);
            set_cur_menu(MENU_QR_SCAN);
            break;
			
        case STATE_STOP_QRING:
            if (cur_menu == MENU_QR_SCAN) {
                disp_menu();
            } else {
                ERR_MENU_STATE(cur_menu,state);
            }
            break;
			
        case STATE_LOW_BAT:
            break;
			
        case STATE_START_GYRO:
            set_cur_menu(MENU_GYRO_START);
            break;
		
        case STATE_NOISE_SAMPLE:
            break;
			
        case STATE_SPEED_TEST:
            set_cur_menu(MENU_SPEED_TEST);
            break;
		
        case STATE_RESTORE_ALL:
            disp_icon(ICON_RESET_SUC_128_48128_48);
            mProCfg->reset_all();
            msg_util::sleep_ms(500);
            rm_state(STATE_RESTORE_ALL);
//            Log.d(TAG,"bDispTop %d",bDispTop);
            init_cfg_select();
            Log.d(TAG,"STATE_RESTORE_ALL cur_menu is %d cam_state 0x%x",MENU_TOP,cam_state);
            if (cur_menu == MENU_TOP) {
                set_cur_menu(cur_menu);
            } else {
                procBackKeyEvent();
            }
            break;
			
        case STATE_POWER_OFF:
            Log.d(TAG,"do nothing for power off");
            break;
		
        case STATE_FORMATING:
            break;

		case STATE_PLAY_SOUND:
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
//    Log.d(TAG,"set tl count %d",count);
    tl_count = count;
}

void MenuUI::disp_tl_count(int count)
{
    if (count < 0) {
        Log.e(TAG,"error tl count %d", tl_count);
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
            set_light(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
        } else {
            Log.e(TAG, "tl count error state 0x%x", cam_state);
        }
    }
}

void MenuUI::minus_cam_state(int state)
{
    rm_state(state);
    set_cap_delay(0);
//    Log.d(TAG,"after minus_cam_state cam_state 0x%x "
//                  "cur_menu %d state 0x%x",
//          cam_state,cur_menu, state);
    if(check_state_equal(STATE_IDLE))
    {
        reset_last_info();
        switch(state)
        {
            case STATE_CALIBRATING:
            case STATE_START_GYRO:
            case STATE_START_QR:
            case STATE_NOISE_SAMPLE:
                set_cur_menu_from_exit();
                break;
            default:
                set_cur_menu(MENU_TOP);
                break;
        }
    }
    else if (check_state_preview())
    {
        Log.d(TAG, "minus_cam_state　state preview (%d 0x%x)", cur_menu, state);
        switch(cur_menu)
        {
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
                set_cur_menu(MENU_PIC_INFO);
                break;
        }
    }
    else
    {
        Log.w(TAG," minus error cam_state 0x%x state 0x%x", cam_state,state);
    }
}



void MenuUI::disp_err_code(int code,int back_menu)
{
    bool bFound = false;
    char err_code[128];

    memset(err_code, 0, sizeof(err_code));

    set_back_menu(MENU_SYS_ERR, back_menu);
    for (u32 i = 0; i < sizeof(mErrDetails)/sizeof(mErrDetails[0]); i++) {
        if (mErrDetails[i].code == code) {
            if (mErrDetails[i].icon != -1) {
                set_light_direct(BACK_RED | FRONT_RED);
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
    set_light_direct(BACK_RED|FRONT_RED);
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
        set_cur_menu(MENU_SYS_ERR, back_menu);
    }
	
    mControlAct = nullptr;
    disp_err_str(type);
}




/*************************************************************************
** 方法名称: set_flick_light
** 方法功能: 设置灯光闪烁的颜色值
** 入口参数: 无
** 返 回 值: 无 
** 调     用: set_light
**
*************************************************************************/
void MenuUI::set_flick_light()
{
#ifdef ENABLE_LIGHT
    if (get_setting_select(SET_LIGHT_ON) == 1) {
        switch ((front_light)) {
#if 0
            case FRONT_RED:
                fli_light = FRONT_RED | BACK_RED;
                break;
            case FRONT_YELLOW:
                fli_light = FRONT_YELLOW | BACK_YELLOW;
                break;
            case FRONT_WHITE:
                fli_light = FRONT_WHITE | BACK_WHITE;
                break;
#else
            case FRONT_RED:
                fli_light = BACK_RED;
                break;
            case FRONT_YELLOW:
                fli_light = BACK_YELLOW;
                break;
            case FRONT_WHITE:
                fli_light = BACK_WHITE;
                break;
#endif
            SWITCH_DEF_ERROR(front_light);
        }
    }
//    Log.d(TAG,"set_flick_light (%d %d)",fli_light,
// get_setting_select(SET_LIGHT_ON));
#endif
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

void MenuUI::set_light()
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
        set_light(front_light);
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

	//original error handle
    if (err_code == -1) {
        oled_disp_type(type);
    } else { //new error_code
        int back_menu = MENU_TOP;
        tl_count = -1;
		
        //make it good code
        err_code = abs(err_code);
        switch (type) {
            case START_PREVIEW_FAIL:
                rm_state(STATE_START_PREVIEWING);
                back_menu = get_error_back_menu();
                break;
				
            case CAPTURE_FAIL:
                rm_state(STATE_TAKE_CAPTURE_IN_PROCESS|STATE_PIC_STITCHING);
                back_menu = get_error_back_menu(MENU_PIC_INFO);//MENU_PIC_INFO;
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
        set_light();
    } else {
        set_light_direct(LIGHT_OFF);
    }
}

void MenuUI::set_oled_power(unsigned int on)
{
    int GPIP_PX3 = 187;

#if 0
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



bool MenuUI::queryCurStorageState()
{
	sp<Volume> tmpVol;
	
	if (mLocalStorageList.size() == 0) {
		return false;
	} else {	
		for (u32 i = 0; i < mLocalStorageList.size(); i++) {
			tmpVol = mLocalStorageList.at(i);
			Log.d(TAG, "Local device [%s], remain size [%ld]M", tmpVol->name, tmpVol->avail);
			if (tmpVol->avail < mMinStorageSpce) {
				mMinStorageSpce = tmpVol->avail;
			}
		}
		
		add_state(STATE_QUERY_STORAGE);

		send_option_to_fifo(ACTION_QUERY_STORAGE);
		return true;		
	}
}


//all disp is at bottom
int MenuUI::oled_disp_type(int type)
{
    Log.d(TAG, "oled_disp_type (%d %s 0x%x)\n", type, STR(cur_menu), cam_state);

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
                set_cur_menu(MENU_AGEING);
            }
            break;
			
        case START_RECING:
            add_state(STATE_START_RECORDING);
            break;

		/*
		 * 启动录像成功
		 */
        case START_REC_SUC:	/* 发送显示录像成功 */
            if (!check_state_in(STATE_RECORD)) {
                if (check_rec_tl()) {
                    tl_count = 0;
                    //disable update_light
                    disp_tl_count(tl_count);
                } else {
                    //before send for disp_mid
                    set_update_mid();
                }
				
                INFO_MENU_STATE(cur_menu, cam_state)
                add_state(STATE_RECORD);
                set_cur_menu(MENU_VIDEO_INFO);
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
            add_state(STATE_STOP_RECORDING);
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
                    disp_bottom_info();
                } else {
                    Log.e(TAG, "error cur_menu %d ", cur_menu);
                }

				/* 添加用于老化测试： 灯全绿 */
				#ifdef ENABLE_AGING_MODE
				set_light_direct(FRONT_GREEN | BACK_GREEN);
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

			
        case CAPTURE:
            if (check_allow_pic()) {
                if (mControlAct != nullptr) {
                	Log.d(TAG, "+++++++++++++++>>>> CAPTURE mControlAct != nullptr");
                    set_cap_delay(mControlAct->delay);
                } else {
                    int item = get_menu_select_by_power(MENU_PIC_SET_DEF);
                  	Log.d(TAG, "+++++++++++++++ CAPTURE def item %d", item);
                    switch (item) {
						

						case PIC_ALL_8K_3D_OF:
						case PIC_ALL_8K_OF:
						case PIC_ALL_8K:
					#ifdef ENABLE_SETTING_AEB	
						case PIC_ALL_AEB_3579:
					#endif
						case PIC_ALL_BURST:							

							Log.d(TAG, ">>>>>>>>>>> delay val = %d", mPICAction[item].delay);						
                            set_cap_delay(mPICAction[item].delay);
                            break;
						
                            //user define
                        case PIC_ALL_CUSTOM:
                            set_cap_delay(mProCfg->get_def_info(KEY_PIC_DEF)->delay);
                            break;
						
                        SWITCH_DEF_ERROR(item)
                    }
                }
				
                add_state(STATE_TAKE_CAPTURE_IN_PROCESS);
				add_state(STATE_PLAY_SOUND);	/* 添加播放声音状态 */

				set_cur_menu(MENU_PIC_INFO);

				Log.d(TAG, "CAPTURE::: set_photo_delay_index = %d, sound_id = %d", set_photo_delay_index, SND_1S_T + set_photo_delay_index);
				
				/* 第一次发送更新消息, 根据cap_delay的值来决定播放哪个声音 */
                send_update_light(MENU_PIC_INFO, STATE_TAKE_CAPTURE_IN_PROCESS, INTERVAL_1HZ, SND_1S_T + set_photo_delay_index);
            }
            break;

			
        case CAPTURE_SUC:
            if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || check_state_in(STATE_PIC_STITCHING)) {
                //disp capture suc
                minus_cam_state(STATE_TAKE_CAPTURE_IN_PROCESS | STATE_PIC_STITCHING);
//                Log.w(TAG,"CAPTURE_SUC cur_menu %d", cur_menu);
                //waiting
                if (cur_menu == MENU_PIC_INFO) {
                    if (mControlAct != nullptr) {
                        Log.d(TAG,"control cap suc");
                        mControlAct = nullptr;
                        disp_bottom_info();
                    } else {
                        Log.d(TAG,"org remain pic %d",mRemainInfo->remain_pic_num);
                        if (mRemainInfo->remain_pic_num > 0) {
                            mRemainInfo->remain_pic_num--;
                        }
                        disp_bottom_space();
                    }
                } else {
                    mControlAct = nullptr;
                    Log.e(TAG, "error capture　suc cur_menu %d ", cur_menu);
                }
                play_sound(SND_COMPLE);
            }
            break;
			
        case CAPTURE_FAIL:
            if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || check_state_in(STATE_PIC_STITCHING)) {
                rm_state(STATE_TAKE_CAPTURE_IN_PROCESS|STATE_PIC_STITCHING);
                disp_sys_err(type);
            }
            break;
			
        case STRAT_LIVING:
            add_state(STATE_START_LIVING);
            break;
		
        case START_LIVE_SUC:
            //avoid conflict with http req
            if (!check_state_in(STATE_LIVE)) {
                tl_count = -1;
                set_update_mid();
                add_state(STATE_LIVE);
                set_cur_menu(MENU_LIVE_INFO);
            } else {
                Log.e(TAG,"start live suc error state 0x%x",cam_state);
            }
            break;
			
        case START_LIVE_CONNECTING:
            if (!check_state_in(STATE_LIVE_CONNECTING)) {
                add_state(STATE_LIVE_CONNECTING);
                set_cur_menu(MENU_LIVE_INFO);
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
                        disp_cam_param(0);
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
			
        case PIC_ORG_FINISH:
            if (!check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
                Log.e(TAG,"pic org finish error state 0x%x",cam_state);
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
            Log.d(TAG,"RESET_ALL_CFG cur_menu is %d",cur_menu);
            if (cur_menu == MENU_TOP) {
                set_cur_menu(cur_menu);
            } else {
                procBackKeyEvent();
            }
            break;



		/*********************************  预览相关状态 START ********************************************/
			
        case START_PREVIEWING:
            add_state(STATE_START_PREVIEWING);
            break;

        case START_PREVIEW_SUC:		/* 启动预览成功 */
            if (!check_state_in(STATE_PREVIEW)) {

				Log.d(TAG, ">>>>>>>>>>>>>>>  PREVIEW SUCCESS, ENTER QUERY STORAGE STATE");

#if 0
				/* 查询状态 */							
				bool bStore = queryCurStorageState();
				if (bStore) {		
					Log.d(TAG, ">>>> queryCurStorageState return true, we can take pic, vido, live now...");
					add_state(STATE_PREVIEW); /* 添加"STATE_PREVIEW"后会显示"READY"字样 */
				} else {
					/* 显示无存储设备 */
					Log.d(TAG, ">>>> queryCurStorageState return false, baddly...");					

					rm_state(STATE_START_PREVIEWING | STATE_STOP_PREVIEWING);

					/* 底部和右下角显示"No SD Card"和"None" */					
					disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
				}
#else
				
				add_state(STATE_PREVIEW); /* 添加"STATE_PREVIEW"后会显示"READY"字样 */
#endif

            }
            break;

			
        case START_PREVIEW_FAIL:	/* 启动预览失败 */
            Log.d(TAG, "START_PREVIEW_FAIL cur_menu %d cam state %d", cur_menu, cam_state);
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
			
        case START_CALIBRATIONING:
            Log.d(TAG, "gyro calc delay %d cur_menu [%s]", mGyroCalcDelay, getCurMenuStr(cur_menu));
            send_update_light(MENU_CALIBRATION, STATE_CALIBRATING, INTERVAL_1HZ);
            if (cur_menu != MENU_CALIBRATION) {		/* 进入陀螺仪校正菜单 */
                set_cur_menu(MENU_CALIBRATION);
            }
            add_state(STATE_CALIBRATING);
            break;

			
        case CALIBRATION_SUC:
            if (check_state_in(STATE_CALIBRATING)) {
                disp_calibration_res(0);
                msg_util::sleep_ms(1000);
                minus_cam_state(STATE_CALIBRATING);
            }
            break;
			
        case CALIBRATION_FAIL:
            rm_state(STATE_CALIBRATING);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;

		/*********************************	陀螺仪相关状态 END ********************************************/

			
        case SYNC_REC_AND_PREVIEW:
            if (!check_state_in(STATE_RECORD)) {
                cam_state = STATE_PREVIEW;
                add_state(STATE_RECORD);
			
                //disp video menu before add state_record
                set_cur_menu(MENU_VIDEO_INFO);
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
                set_cur_menu(MENU_PIC_INFO);
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
                set_cur_menu(MENU_PIC_INFO);
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
                set_cur_menu(MENU_LIVE_INFO);
                Log.d(TAG," set_update_mid b");
            }
            break;
			
        case SYNC_LIVE_CONNECT_AND_PREVIEW:
            if (!check_state_in(STATE_LIVE_CONNECTING)) {
                cam_state = STATE_PREVIEW;
                add_state(STATE_LIVE_CONNECTING);
                set_cur_menu(MENU_LIVE_INFO);
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
            Log.d(TAG,"rec capture org suc");
            break;
		
        case CALIBRATION_ORG_SUC:
            Log.d(TAG,"rec calibration org suc");
            break;

        case SET_SYS_SETTING:
        case SET_CUS_PARAM:
        case STITCH_PROGRESS:
            Log.d(TAG,"do nothing for %d",type);
            break;
		
        case TIMELPASE_COUNT:
            INFO_MENU_STATE(cur_menu, cam_state)
            Log.d(TAG, "tl_count %d", tl_count);
            if (!check_state_in(STATE_RECORD)) {
                Log.e(TAG," TIMELPASE_COUNT cam_state 0x%x",cam_state);
            } else {
                disp_tl_count(tl_count);
            }
            break;
//        case POWER_OFF_SUC:
//            break;
//        case POWER_OFF_FAIL:
//            break;
        case START_GYRO:
            if(!check_state_in(STATE_START_GYRO))
            {
                add_state(STATE_START_GYRO);
            }
            break;
        case START_GYRO_SUC:
            if(check_state_in(STATE_START_GYRO))
            {
                minus_cam_state(STATE_START_GYRO);
            }
            break;
        case START_GYRO_FAIL:
            rm_state(STATE_START_GYRO);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
        case START_NOISE:
            if(!check_state_in(STATE_NOISE_SAMPLE))
            {
                set_cur_menu(MENU_NOSIE_SAMPLE);
                add_state(STATE_NOISE_SAMPLE);
            }
            break;
        case START_LOW_BAT_SUC:
            INFO_MENU_STATE(cur_menu,cam_state)
            cam_state = STATE_IDLE;
            if(MENU_LOW_BAT != cur_menu)
            {
                set_cur_menu(MENU_LOW_BAT);
            }
            else
            {
                set_light_direct(BACK_RED|FRONT_RED);
            }
            mControlAct = nullptr;
            break;
        case START_LOW_BAT_FAIL:
            cam_state = STATE_IDLE;
            INFO_MENU_STATE(cur_menu,cam_state)
            if(MENU_LOW_BAT != cur_menu)
            {
                set_cur_menu(MENU_LOW_BAT);
            }
            else
            {
                set_light_direct(BACK_RED|FRONT_RED);
            }
            mControlAct = nullptr;
            break;
        case START_NOISE_SUC:
            if(check_state_in(STATE_NOISE_SAMPLE))
            {
                minus_cam_state(STATE_NOISE_SAMPLE);
            }
            break;
        case START_NOISE_FAIL:
            rm_state(STATE_NOISE_SAMPLE);
            disp_sys_err(type,get_back_menu(MENU_NOSIE_SAMPLE));
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
            Log.e(TAG,"force path select %d ",mSavePathIndex);
        }
		
        if (bDispBox) {
            disp_dev_msg_box(bAdd, dev_change_type, bChange);
        }
    }
}


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
            memset(mRemainInfo.get(),0, sizeof(REMAIN_INFO));
            //waitting for rec no space from camerad
            bRet = false;
        }
    }
    return bRet;
}

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
	Log.d(TAG, "last_light 0x%x fli_light 0x%x", last_light, fli_light);
	
    if ((last_light & 0xf8) != fli_light) {
        set_light(fli_light);
    } else {
        set_light(LIGHT_OFF);
    }
}

void MenuUI::flick_low_bat_lig()
{
//    if(last_back_light != BACK_RED)
//    {
//        mOLEDLight->set_light_val(BACK_RED|FRONT_RED);
//    }
//    else
//    {
//        mOLEDLight->set_light_val(LIGHT_OFF);
//    }
}


/*
 * disp_mid - 显示已经录像的时间(时分秒)
 */
void MenuUI::disp_mid()
{
    char disp[32];
    snprintf(disp, sizeof(disp), "%02d:%02d:%02d",
             mRecInfo->rec_hour, mRecInfo->rec_min, mRecInfo->rec_sec);
//    Log.d(TAG," disp rec mid %s tl_count %d",disp,tl_count);
    if (tl_count == -1) {
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
    send_option_to_fifo(ACTION_LOW_BAT,REBOOT_SHUTDOWN);
}

void MenuUI::set_light_direct(u8 val)
{
//    Log.d(TAG,"last_light 0x%x val 0x%x",last_light,val);
    if(last_light != val)
    {
        last_light = val;
        mOLEDLight->set_light_val(val);
    }
}

void MenuUI::set_light(u8 val)
{
#ifdef ENABLE_LIGHT
//    Log.d(TAG,"set_light (0x%x  0x%x  0x%x 0x%x %d)",
//          last_light,val, front_light,fli_light,
//          get_setting_select(SET_LIGHT_ON));
    if(get_setting_select(SET_LIGHT_ON) == 1)
    {
        set_light_direct(val|front_light);
    }
#endif
}

bool MenuUI::check_rec_tl()
{
    bool ret = false;
    if(mControlAct != nullptr)
    {
//        Log.d(TAG,"mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int is %d",
//              mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
        if(mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0)
        {
            ret = true;
        }
    }
    else
    {
        int item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
        if(item >= 0 && item < VID_CUSTOM)
        {
//            Log.d(TAG,"mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
            if(mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0)
            {
                ret = true;
            }
        }
        else if(VID_CUSTOM == item)
        {
            Log.d(TAG,"mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",
                  mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
            if(mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0)
            {
                if(mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act == 0)
                {
                    mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act = 10;
                }
                ret = true;
            }
        }
        else
        {
            ERR_ITEM(item);
        }
    }
    if(!ret)
    {
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
		#if 0
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

	//add param from controller or qr scan
	if (disp_type->qr_type != -1) {
		CHECK_NE(disp_type->mAct,nullptr);
		add_qr_res(disp_type->qr_type, disp_type->mAct,
				   disp_type->control_act);
	} else if (disp_type->tl_count != -1) {
		set_tl_count(disp_type->tl_count);
	} else if (disp_type->mSysSetting != nullptr) {
		set_sys_setting(disp_type->mSysSetting);
	} else if (disp_type->mStichProgress != nullptr) {
		disp_stitch_progress(disp_type->mStichProgress);
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


void MenuUI::handleLongKeyMsg(int key, int64 ts)
{
	if (ts == last_key_ts && last_down_key == key) {
		Log.d(TAG," long press key 0x%x",key);
		if (key == OLED_KEY_POWER) {
			sys_reboot();
		}
	}
}


void MenuUI::handleDispLightMsg(int menu, int state, int interval)
{
	// Log.d(TAG," (%d %d	%d %d)", menu, state, interval, cap_delay);
	bSendUpdate = false;

	unique_lock<mutex> lock(mutexState);
	switch (menu) {
		case MENU_PIC_INFO:
			if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
				if (check_state_in(STATE_PLAY_SOUND)) {
					/* 播放声音,去除STATE_PLAY_SOUND状态 */
					rm_state(STATE_PLAY_SOUND);
					Log.d(TAG, "MENU_PIC_INFO remove STATE_PLAY_SOUND state, play index: %d", SND_3S_T + set_photo_delay_index);
					//play_sound(SND_3S_T + set_photo_delay_index);
					play_sound(get_cap_to_sound_index(cap_delay));
				}
						
				if (cap_delay == 0) {
					if (menu == cur_menu) {
						disp_shooting();
					}
				} else {
					if (menu == cur_menu) {
						disp_sec(cap_delay, 52, 24);	/* 显示倒计时的时间 */
					}

				#ifdef ENABLE_SOUND
					/* 倒计时时根据当前cap_dela y的值,只在	CAPTURE中播放一次, fix bug1147 */
					send_update_light(menu, state, INTERVAL_1HZ, true);
				#endif

				}
				cap_delay--;
			} else if (check_state_in(STATE_PIC_STITCHING)) {
				send_update_light(menu, state, INTERVAL_5HZ, true);
			} else {
				Log.d(TAG, "update pic light error state 0x%x", cam_state);
				set_light();
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
							send_option_to_fifo(ACTION_CALIBRATION);
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
				set_light();
			}
			break;
					
		default:
			break;
	}

}



/*************************************************************************
** 方法名称: handleMessage
** 方法功能: 消息处理
** 入口参数: msg - 消息对象强指针引用
** 返 回 值: 无 
** 调     用: 消息处理线程
**
*************************************************************************/
void MenuUI::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	

    if (OLED_EXIT == what) {
        exitAll();
    } else {
        switch (what) {
            case OLED_DISP_STR_TYPE: {
                {
                    unique_lock<mutex> lock(mutexState);
                    sp<DISP_TYPE> disp_type;
                    CHECK_EQ(msg->find<sp<DISP_TYPE>>("disp_type", &disp_type), true);
					
                    Log.d(TAG, "OLED_DISP_STR_TYPE (%d %d %d %d 0x%x)",
								disp_type->qr_type, 
								disp_type->type, 
								disp_type->tl_count, 
								cur_menu, 
								cam_state);
					
					handleDispStrTypeMsg(disp_type);
                }
				break;
            }
					
            case OLED_DISP_ERR_TYPE: {
                unique_lock<mutex> lock(mutexState);
                sp<ERR_TYPE_INFO> mErrInfo;
                CHECK_EQ(msg->find<sp<ERR_TYPE_INFO>>("err_type_info", &mErrInfo),true);
				handleDispErrMsg(mErrInfo);
				break;
            }
               
			
            case OLED_GET_KEY: {	/* 短按键消息处理 */
                int key = -1;
                CHECK_EQ(msg->find<int>("oled_key", &key), true);
                handleKeyMsg(key);
				break;
            }
                
            case OLED_GET_LONG_PRESS_KEY: {	/* 长按键消息处理 */
                int key;
                int64 ts;
                CHECK_EQ(msg->find<int>("key", &key), true);
                CHECK_EQ(msg->find<int64>("ts", &ts), true);
				handleLongKeyMsg(key, ts);
				 break;
            }
               

            case OLED_DISP_IP: {	/* 更新IP */
				sp<DEV_IP_INFO> tmpIpInfo;
				CHECK_EQ(msg->find<sp<DEV_IP_INFO>>("info", &tmpIpInfo), true);
				Log.d(TAG, "OLED_DISP_IP dev[%s], ip[%s]", tmpIpInfo->cDevName, tmpIpInfo->ipAddr);

				procUpdateIp(tmpIpInfo->ipAddr);
			
               	break;
            }

			/*
			 * 配置WIFI (UI-CORE处理)
			 */
            case OLED_CONFIG_WIFI:  {	/* On/Off Wifi AP */
                sp<WIFI_CONFIG> mConfig;
                CHECK_EQ(msg->find<sp<WIFI_CONFIG>>("wifi_config", &mConfig), true);
                //wifi_config(mConfig);
            }
                break;

			/*
			 * 写SN (UI-CORE处理)
			 */
            case OLED_SET_SN: {	/* 设置SN */
                sp<SYS_INFO> mSysInfo;
                CHECK_EQ(msg->find<sp<SYS_INFO>>("sys_info", &mSysInfo), true);
                write_sys_info(mSysInfo);
                break;
            }

			/*
			 * 同步初始化
			 */
            case OLED_SYNC_INIT_INFO: {	/* 同步初始化信息(来自control_center) */
                //exit if needed
                exit_sys_err();
                sp<SYNC_INIT_INFO> mSyncInfo;
                CHECK_EQ(msg->find<sp<SYNC_INIT_INFO>>("sync_info", &mSyncInfo), true);
                set_sync_info(mSyncInfo);	/* 根据同步系统初始化系统参数及显示 */
                break;
            }

			/*
			 * 显示初始化
			 */
            case OLED_DISP_INIT: {	/* 1.初始化显示消息 */
                oled_init_disp();	/* 初始化显示 */
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


			/*
			 * 更新显示时间(只有在录像,直播的UI)
			 */
            case OLED_UPDATE_MID: {
                bSendUpdateMid = false;
                unique_lock<mutex> lock(mutexState);
				
//                Log.d(TAG,"OLED_UPDATE_MID %d 0x%x tl_count %d",cur_menu,cam_state,tl_count);
                if (check_state_in(STATE_RECORD)) {
                    send_update_mid_msg(INTERVAL_1HZ);
                    if (!check_state_in(STATE_STOP_RECORDING)) {
                        if (tl_count == -1) {
                            increase_rec_time();
							
                            if (decrease_rest_time()) {
                                //hide while stop recording
                                if (cur_menu == MENU_VIDEO_INFO) {
                                    disp_mid();
                                    disp_bottom_space();
                                }
                            }
                        }
                    }
                    flick_light();
                } else if (check_state_in(STATE_LIVE)) {
//                    send_delay_msg(OLED_UPDATE_MID,INTERVAL_1HZ);
                    send_update_mid_msg(INTERVAL_1HZ);
                    if (!check_state_in(STATE_STOP_LIVING)) {
                        increase_rec_time();
                        if (cur_menu == MENU_LIVE_INFO) {
                            disp_mid();
                        }
                    }
                    flick_light();
                } else if(check_state_in(STATE_LIVE_CONNECTING)) {
                    Log.d(TAG,"cancel send msg　in state STATE_LIVE_CONNECTING");
                    set_light();
                } else {
                    ERR_MENU_STATE(cur_menu, cam_state);
                    set_light();
                }
            }
                break;

			/*
			 * 低电
			 */
            case OLED_DISP_BAT_LOW:
                break;


			/*
			 * 读取电池电量
			 */
            case OLED_READ_BAT: {
                unique_lock<mutex> lock(mutexState);
                check_battery_change();
                break;
            }

			/*
			 * 灯显示
			 */
            case OLED_DISP_LIGHT: {
                int menu;
                int interval;
                int state;

                CHECK_EQ(msg->find<int>("menu", &menu), true);
                CHECK_EQ(msg->find<int>("interval", &interval), true);
                CHECK_EQ(msg->find<int>("state", &state), true);
				handleDispLightMsg(menu, interval, state);
				break;
            }
                
			
            case OLED_CLEAR_MSG_BOX: /* 清除消息框 */
                if (cur_menu == MENU_DISP_MSG_BOX) {
                    procBackKeyEvent();
                } else {
                    Log.d(TAG, "other cur_menu %d", cur_menu);
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
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void MenuUI::sys_reboot(int cmd)
{
    switch (cmd) {
        case REBOOT_NORMAL:
            send_option_to_fifo(ACTION_POWER_OFF,cmd);
            break;
		
        case REBOOT_SHUTDOWN:
            send_option_to_fifo(ACTION_POWER_OFF,cmd);
            break;
		
        SWITCH_DEF_ERROR(cmd)
    }
}


void MenuUI::set_cap_delay(int delay)
{
    Log.d(TAG, ">>>>>>>>>>> set_cap_delay %d", delay);
    cap_delay = delay;
}

bool MenuUI::check_live_save_org()
{
    bool ret = false;
    if (check_live()) {
        if (mControlAct != nullptr) {
//            Log.d(TAG, "check_live_save_org mControlAct->stOrgInfo.save_org is %d",
//                  mControlAct->stOrgInfo.save_org);
//            if (mControlAct->stOrgInfo.save_org != SAVE_OFF)
//            {
//                ret = true;
//            }
            ret = check_live_save(mControlAct.get());
        } else {
            int item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
            Log.d(TAG, "check_live_save_org %d", item);
            switch (item) {
                case VID_ARIAL:
                    ret = true;
                    break;
				
                case LIVE_CUSTOM:
//                    Log.d(TAG, "check_live_save_org mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org  is %d",
//                          mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org);
//                    if (mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF)
//                    {
//                        ret = true;
//                    }
                    ret = check_live_save(mProCfg->get_def_info(KEY_LIVE_DEF));
                    break;
				
                default:
                    break;
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
		
    if (bUpdate || bUpload) {
		
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

    if (is_bat_low()) {
#ifdef OPEN_BAT_LOW
        if (cur_menu != MENU_LOW_BAT) {
            Log.d(TAG, "bat low menu %d state 0x%x bStiching %d", cur_menu, cam_state, bStiching);
            if (check_state_in(STATE_RECORD) || check_live_save_org() || bStiching) {
                set_cur_menu(MENU_LOW_BAT,MENU_TOP);
                add_state(STATE_LOW_BAT);
                func_low_bat();
                bStiching = false;
            }
        }
#endif
    }
    send_delay_msg(OLED_READ_BAT, BAT_INTERVAL);

    return bUpdate;
}

int MenuUI::get_battery_charging(bool *bCharge)
{
    return mBatInterface->read_charge(bCharge);
}

int MenuUI::read_tmp(double *int_tmp, double *tmp)
{
    return mBatInterface->read_tmp(int_tmp,tmp);
}

void MenuUI::deinit()
{
    Log.d(TAG, "deinit\n");
	
    set_light_direct(LIGHT_OFF);
    mDevManager = nullptr;

	mNetManager = nullptr;

    sendExit();

    Log.d(TAG, "deinit2");
}


