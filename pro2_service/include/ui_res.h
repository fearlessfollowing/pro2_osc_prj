#ifndef _UI_RES_H_
#define _UI_RES_H_


#define ENABLE_LIGHT
#define ENABLE_SOUND
#define CAL_DELAY 		(5)
#define SN_LEN 			(14)

//sometimes met bat jump to 3 ,but in fact not
#define BAT_LOW_VAL 	(3)

//#define BAT_LOW_PROTECT (5)
#define MAX_ADB_TIMES 	(5)

#define LONG_PRESS_MSEC (2000)

#define ARIAL_LIVE
#define OPEN_BAT_LOW

#define ERR_MENU_STATE(menu,state) \
Log.e(TAG, "line:%d err menu state (%d 0x%x)", __LINE__, menu, state);

#define INFO_MENU_STATE(menu,state) \
Log.d(TAG, "line:%d menu state (%d 0x%x)", __LINE__, menu, state);

//#define ONLY_PIC_FLOW
//#define ENABLE_ADB_OFF
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
    //mtp
    DISP_USB_CONNECTED,
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
    char a12_ver[128];			/* 模组的版本 */
    char c_ver[128];			/* camerad */
    char r_ver[128];
    char p_ver[128];
    char h_ver[128];			/*  */
    char k_ver[128];			/* 内核的版本 */
    char r_v_str[128];
} VER_INFO;



#define SYS_TMP "/home/nvidia/insta360/etc/sys_tmp"
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
} SYS_READ;

static const SYS_READ astSysRead[] = {
	{ "sn", "sn=", "/home/nvidia/insta360/etc/sn"},
	{ "uuid", "uuid=", "/home/nvidia/insta360/etc/uuid"},
};

//pic ,vid, live def
enum {
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

enum
{
    //video
    VID_8K_50M_30FPS_PANO_RTS_OFF,
    VID_6K_50M_30FPS_3D_RTS_OFF,
    VID_4K_50M_120FPS_PANO_RTS_OFF,
    VID_4K_20M_60FPS_3D_RTS_OFF,
    VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON,
    VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON,
#ifndef ARIAL_LIVE
    VID_ARIAL,
#endif
    VID_CUSTOM,
    VID_DEF_MAX,
};

enum
{
    //live
    VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON,
    VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON,
    //hdmi on
    VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI,
    VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI,
#ifdef ARIAL_LIVE
    VID_ARIAL,
#endif
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

enum
{
    LIGHT_OFF = 0xc0,
    FRONT_RED = 0xc1,
    FRONT_GREEN =0xc2,
    FRONT_YELLOW = 0xc3,
    FRONT_DARK_BLUE = 0xc4,
    FRONT_PURPLE = 0xc5,
    FRONT_BLUE = 0xc6,
    FRONT_WHITE = 0xc7,

    BACK_RED = 0xc8,
    BACK_GREEN = 0xd0,
    BACK_YELLOW = 0xd8,
    BACK_DARK_BLUE = 0xe0,
    BACK_PURPLE = 0xe8,
    BACK_BLUE = 0xf0,
    BACK_WHITE= 0xf8,
    LIGHT_ALL = 0xff
};

#define PAGE_MAX (3)

#define INTERVAL_1HZ (1000)
//#define INTERVAL_3HZ (333)
#define INTERVAL_5HZ (200)

#define FLASH_LIGHT BACK_BLUE
#define BAT_INTERVAL (5000)
#define AVAIL_SUBSTRACT (1024)

enum
{
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
	{25,16,103,16},
	{25,32,103,16}
};

typedef struct _rec_info_ {
    int rec_hour;
    int rec_min;
    int rec_sec;
} REC_INFO;

typedef struct _select_info_ {
    int last_select;	
    int select;			
    int cur_page;		
    int total;			
    int page_max;		
    int page_num;		
} SELECT_INFO;

typedef struct _menu_info_ {
    int back_menu;							/* 鑿滃崟閫�鍑烘椂鐨勮繑鍥炶彍鍗旾D */
    SELECT_INFO mSelectInfo;				/* 鑿滃崟鐨勯�夋嫨椤逛俊鎭� */
	const int mSupportkeys[OLED_KEY_MAX];
} MENU_INFO;



typedef struct _cp_info_ {
    const char *src;	
    const char *dest;	
} CP_INFO;

enum {
    SET_WIFI_AP,
    SET_DHCP_MODE,
    SET_FREQ,//50 or 60

    SET_SPEAK_ON,
    SET_BOTTOM_LOGO, //5
    SET_LIGHT_ON,

    SET_AUD_ON,//before spatial aud 170727
    SET_SPATIAL_AUD,

    SET_GYRO_ON,
    //gyro calibration
    SET_START_GYRO,

    SET_FAN_ON, //10
    //sample fan nosie
    SET_NOISE,

    //keep at end
    SET_STORAGE,

    SET_INFO,
    SET_RESTORE,
    SETTING_MAX
};

static int setting_icon_normal[SETTING_MAX][2] =
{
	{ICON_SET_WIFI_WIFI_NORMAL_25_16_96_16, ICON_SET_WIFI_AP_NORMAL_25_16_96_16},
	{ICON_SET_ETHERNET_DIRECT_NORMAL_25_32_96_16,ICON_SET_ETHERNET_DHCP_NORMAL_25_32_96_16},
	{ICON_SET_FREQUENCY_50HZ_NORMAL_25_48_96_16,ICON_SET_FREQUENCY_60HZ_NORMAL_25_48_96_16},

	{ICON_SET_SPEAKER_OFF_NORMAL_25_16_96_16, ICON_SET_SPEAKER_ON_NORMAL_25_16_96_16},
	{ICON_SET_BOTTOMLOGO_OFF_NORMAL96_16,ICON_SET_BOTTOMLOGO_ON_NORMAL96_16},
	{ICON_SET_LED_OFF_NORMAL_25_48_96_16, ICON_SET_LED_ON_NORMAL_25_48_96_16},

	{ICON_SET_AUDIO_OFF_NORMAL96_16,ICON_SET_AUDIO_ON_NORMAL96_16},
	{ICON_SET_SPATIALAUDIO_OFF_NORMAL96_16,ICON_SET_SPATIALAUDIO_ON_NORMAL96_16},

	{ICON_SET_GYRO_OFF_NORMAL96_16,ICON_SET_GYRO_ON_NORMAL96_16},
	//gyro calibration
	{ICON_SET_GYRO_OFF_NORMAL_25_16_96_16,ICON_SET_GYRO_OFF_NORMAL_25_16_96_16},

	{ICON_SET_FAN_OFF_NORMAL96_16,ICON_SET_FAN_ON_NORMAL96_16},
	{ICON_SET_NOISE_NORMAL96_16,    ICON_SET_NOISE_NORMAL96_16},

	{ICON_SET_STORAGE_NORMAL_25_32_96_16,ICON_SET_STORAGE_NORMAL_25_32_96_16},
	{ICON_SET_INFO_NORMAL_25_32_96_16,    ICON_SET_INFO_NORMAL_25_32_96_16},
	{ICON_SET_RESET_NORMAL_25_48_96_16,   ICON_SET_RESET_NORMAL_25_48_96_16},
};

static int setting_icon_lights[SETTING_MAX][2] = {
	{ICON_SET_WIFI_WIFI_LIGHT_25_16_96_16, ICON_SET_WIFI_AP_LIGHT_25_16_96_16},
	{ICON_SET_ETHERNET_DIRECT_LIGHT_25_32_96_16,ICON_SET_ETHERNET_DHCP_LIGHT_25_32_96_16},
	{ICON_SET_FREQUENCY_50HZ_LIGHT_25_48_96_16,ICON_SET_FREQUENCY_60HZ_LIGHT_25_48_96_16},

	{ICON_SET_SPEAKER_OFF_LIGHT_25_16_96_16, ICON_SET_SPEAKER_ON_LIGHT_25_16_96_16},
	{ICON_SET_BOTTOMLOGO_OFF_LIGHT96_16,ICON_SET_BOTTOMLOGO_ON_LIGHT96_16},
	{ICON_SET_LED_OFF_LIGHT_25_48_96_16, ICON_SET_LED_ON_LIGHT_25_48_96_16},

	{ICON_SET_AUDIO_OFF_LIGHT96_16,ICON_SET_AUDIO_ON_LIGHT96_16},
	{ICON_SET_SPATIALAUDIO_OFF_LIGHT96_16,ICON_SET_SPATIALAUDIO_ON_LIGHT96_16},

	{ICON_SET_GYRO_OFF_LIGHT96_16,ICON_SET_GYRO_ON_LIGHT96_16},
	//gyro calibration
	{ICON_SET_GYRO_OFF_LIGHT_25_16_96_16,ICON_SET_GYRO_OFF_LIGHT_25_16_96_16},

	{ICON_SET_FAN_OFF_LIGHT96_16,ICON_SET_FAN_ON_LIGHT96_16},
	//sample fan noise
	{ICON_SET_NOISE_HIGH96_16,    ICON_SET_NOISE_HIGH96_16},

	{ICON_SET_STORAGE_LIGHT_25_32_96_16,ICON_SET_STORAGE_LIGHT_25_32_96_16},
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
		{-1, 0, 0, PIC_DEF_MAX, PIC_DEF_MAX, 1},
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

typedef struct _setting_items_ {
    int clear_icons[2];
    int iSelect[SETTING_MAX];
    int (*icon_normal)[2];
    int (*icon_light)[2];
} SETTING_ITEMS;

static SETTING_ITEMS mSettingItems = {
	{ICON_SET_ETHERNET_DHCP_NORMAL_25_32_96_16, ICON_SET_FREQUENCY_50HZ_NORMAL_25_48_96_16},
	{0},
	setting_icon_normal,
	setting_icon_lights
};

static const char *dev_type[SET_STORAGE_MAX] = {"sd", "usb"};
//static const int storage_none_icon[][SET_STORAGE_MAX] =
//{
//        {
//                STORAGE_SD_NONE_NORMAL_2516_103X16,
//                STORAGE_USB_NONE_NORMAL_2532_103X16,
//        },
//        {
//                STORAGE_USB_NONE_NORMAL_2532_103X16,
//                STORAGE_USB_NONE_LIGHT_2532_103X16
//        }
//};

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

static const int pic_def_setting_menu[][PIC_DEF_MAX] = {
	{
		//#ifndef ONLY_PIC_FLOW
		// ICON_CAMERA_INFO01_NORMAL_0_48_78_1678_16,
		//#endif
		ICON_CAMERA_INFO02_NORMAL_0_48_78_1678_16,
		ICON_CAMERA_INFO04_NORMAL_0_48_78_1678_16,
		ICON_CAMERA_INFO03_NORMAL_0_48_78_1678_16,
		ICON_CAMERA_INFO05_NORMAL_0_48_78_1678_16,
#ifdef OPEN_HDR_RTS
		ICON_CAMERA_INFO05_NORMAL_0_48_78_1678_16,
#endif
		ICON_CAMERA_INFO06_NORMAL_0_48_78_1678_16,
		ICON_CINFO_RAW_NORMAL_0_48_78_1678_16,
		ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16,
    },
    {
		//#ifndef ONLY_PIC_FLOW
		//                ICON_CAMERA_INFO01_LIGHT_0_48_78_1678_16,
		//#endif
		ICON_CAMERA_INFO02_LIGHT_0_48_78_1678_16,
		ICON_CAMERA_INFO04_LIGHT_0_48_78_1678_16,
		ICON_CAMERA_INFO03_LIGHT_0_48_78_1678_16,
		ICON_CAMERA_INFO05_LIGHT_0_48_78_1678_16,
#ifdef OPEN_HDR_RTS
		ICON_CAMERA_INFO05_LIGHT_0_48_78_1678_16,
#endif
		ICON_CAMERA_INFO06_LIGHT_0_48_78_1678_16,
		ICON_CINFO_RAW_LIGHT_0_48_78_1678_16,
		ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16
    }
};

static const int vid_def_setting_menu[][VID_DEF_MAX] = {
	{
		ICON_VIDEO_INFO02_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO01_NORMAL_0_48_78_1678_16,
		// ICON_VIDEO_INFO03_NORMAL_0_48_78_1678_16,//100FPS
		ICON_VIDEO_INFO06_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO07_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
#ifndef ARIAL_LIVE
		ICON_VIDEO_INFO08_NORMAL_0_48_78_1678_16,
#endif
		ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16,
    },
    {
		ICON_VIDEO_INFO02_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO01_LIGHT_0_48_78_1678_16,
		// ICON_VIDEO_INFO03_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO06_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO07_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
#ifndef ARIAL_LIVE
		ICON_VIDEO_INFO08_LIGHT_0_48_78_1678_16,
#endif
		ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16,
    }
};

static const int live_def_setting_menu[][LIVE_DEF_MAX] = {
	{
		ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO04_NORMAL_0_48_78_1678_16,
		ICON_VIDEO_INFO05_NORMAL_0_48_78_1678_16,
#ifdef ARIAL_LIVE
		ICON_VIDEO_INFO08_NORMAL_0_48_78_1678_16,
#endif
		ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16,
    },
    {
		ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO04_LIGHT_0_48_78_1678_16,
		ICON_VIDEO_INFO05_LIGHT_0_48_78_1678_16,
#ifdef ARIAL_LIVE
		ICON_VIDEO_INFO08_LIGHT_0_48_78_1678_16,
#endif
		ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16
	}
        };


static const ACTION_INFO mPICAction[] = {
	{
		MODE_3D,
		60,
		5,
		{
	        EN_JPEG,
	        SAVE_DEF,
	        4000,
	        3000,
			0,		// test
	        {}
        },
        {
			EN_JPEG,
			STITCH_OPTICAL_FLOW,
			7680,
			7680,
			{},
        }
    },
    {
		MODE_PANO,
		30,
		5,
        {
	        EN_JPEG,
	        SAVE_DEF,
	        4000,
	        3000,
			0,		// test
	        
	        {}
        },
        {
	        EN_JPEG,
	        STITCH_OPTICAL_FLOW,
	        7680,
	        3840,
	        {}
        }
    },
    {
	    MODE_PANO,
	    30,
	    5,
	    {
	        EN_JPEG,
	        SAVE_DEF,
	        4000,
	        3000,
			0,		// test
	        
	        {}

        },
        {
			EN_JPEG,
			// STITCH_NORMAL,
			STITCH_OFF,
			7680,
			3840,
			{}
        }
    },
    
#ifdef OPEN_HDR_RTS
	//HDR&rts
	{
	    MODE_PANO,
	    80,//pano
	    5,
	    {
	        EN_JPEG,
	        SAVE_DEF,
	        4000,
	        3000,
			0,		// test
	        
	        {1,3,-64,64,0}
        },
        {
	        EN_JPEG,
	        STITCH_NORMAL,
	        7680,
	        3840,
	        {}
        }
    },
#endif
	//HDR
	{
	    MODE_PANO,
	    70,//pano
	    5,
	    {
	        EN_JPEG,
	        SAVE_DEF,
	        4000,
	        3000,
			0,		// test
	        
	        {3,-32,32,0}
	    },
        {
			EN_JPEG,
			STITCH_OFF,
			7680,
			3840,
			{}
        }
    },
	//BURST
	{
		MODE_PANO,
		165,
		5,
		{
			EN_JPEG,
			SAVE_DEF,
			4000,
			3000,
			0,		// test
			
			{0,0,0,10}
		},
		{
			EN_JPEG,
			STITCH_OFF,
			7680,
			3840,
			{}
		}
    },
	//RAW
	{
		MODE_3D,
		144,
		5,
		{
			EN_RAW,
			SAVE_RAW,
			4000,
			3000,
			0,		// test
			
			{},
		},
		{
			EN_RAW,
			STITCH_OFF,
			7680,
			7680,
			{}
		}
    },
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
			0,			/* 0 -> nvidia; 1 -> module; 2 -> both */
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
	    MODE_PANO,25,0,
		{
			EN_H264,
			SAVE_DEF,
			1920,
			1080,
#if 1
			0,			/* 0 -> nvidia; 1 -> module; 2 -> both */
#endif
			
            { ALL_FR_120, 40}
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
			0,			/* 0 -> nvidia; 1 -> module; 2 -> both */
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
			0,			/* 0 -> nvidia; 1 -> module; 2 -> both */
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
#if 0	
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
#endif

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



//str not used 0613
static ERR_CODE_DETAIL mErrDetails[] =
{
    {432, "No Space", ICON_STORAGE_INSUFFICIENT128_64},
    {433, "No Storage", ICON_CARD_EMPTY128_64},
    {434, "Speed Low", ICON_SPEEDTEST06128_64},
    {414, " ", ICON_ERROR_414128_64},

    // add for live rec finish
    {390, " ", ICON_LIV_REC_INSUFFICIENT_128_64128_64},
    {391, " ", ICON_LIVE_REC_LOW_SPEED_128_64128_64},
#if 0
    {410,"410",ICON_ERROR_410128_64},
    {411,"411",ICON_ERROR_411128_64},
    {412," ",ICON_ERROR_412128_64},
    {413," ",ICON_ERROR_413128_64},
    {430," ",ICON_ERROR_430128_64},
    {431," ",ICON_ERROR_431128_64},
    {435," ",ICON_ERROR_435128_64},
    {436," ",ICON_ERROR_436128_64},
    {460," ",ICON_ERROR_460128_64},
    {461," ",ICON_ERROR_461128_64},
    {462," ",ICON_ERROR_462128_64},
    {463," ",ICON_ERROR_463128_64},
    {464," ",ICON_ERROR_464128_64},
    {465," ",ICON_ERROR_465128_64},
    {466," ",ICON_ERROR_466128_64},
    {467," ",ICON_ERROR_467128_64},
#endif
};

static SYS_ERROR mSysErr[] = {
	{START_PREVIEW_FAIL, 	"1401"},
	{CAPTURE_FAIL,	 		"1402"},
	{START_REC_FAIL, 		"1403"},
	{START_LIVE_FAIL, 		"1404"},
	{START_QR_FAIL, 		"1405"},
	{CALIBRATION_FAIL, 		"1406"},
	{QR_FINISH_ERROR, 		"1407"},
	{START_GYRO_FAIL, 		"1408"},
	{START_STA_WIFI_FAIL, 	"1409"},
	{START_AP_WIFI_FAIL, 	"1410"},
	{SPEED_TEST_FAIL, 		"1411"},
	{START_NOISE_FAIL, 		"1412"},
	{STOP_PREVIEW_FAIL, 	"1501"},
	{STOP_REC_FAIL, 		"1503"},
	{STOP_LIVE_FAIL, 		"1504"},
	{STOP_QR_FAIL, 			"1505"},
	{QR_FINISH_UNRECOGNIZE, "1507"},
	{STOP_STA_WIFI_FAIL, 	"1509"},
	{STOP_AP_WIFI_FAIL, 	"1510"},
	{RESET_ALL, 			"1001"},
	{START_FORCE_IDLE, 		"1002"}
};

#endif /* _UI_RES_H_ */
