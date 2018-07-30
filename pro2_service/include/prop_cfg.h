/*
 * pro_cfg.h
 *
 *  Created on: 2018年6月6日
 *      Author: root
 */

#ifndef PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_
#define PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_


#define WIFI_TMP_AP_CONFIG_FILE				"home/nvidia/insta360/etc/.wifi_ap.conf"


#define ETH0_NAME       					"eth0"
#define DEFAULT_ETH0_IP 					"192.168.1.188"


#define WLAN0_NAME      					"wlan0"
#define WLAN0_DEFAULT_IP 					"192.168.43.1"
#define DEFAULT_WIFI_AP_SSID				"Insta360-Pro2-Test"
#define DEFAULT_WIFI_AP_MODE				"g"
#define DEFAULT_WIFI_AP_CHANNEL				"6"
#define DEFAULT_WIFI_AP_CHANNEL_NUM_BG		6
#define DEFAULT_WIFI_AP_CHANNEL_NUM_AN		13

#define OFF_IP								"0.0.0.0"

/*
 * Prop system
 */
#define PROP_SYS_AP_SSID			"sys.wifi_ssid"
#define PROP_SYS_AP_PESUDO_SN		"sys.wifi_pesu_sn"
#define PROP_SYS_AP_PASSWD			"sys.wifi_passwd"
#define PROP_SYS_AP_MODE			"sys.wifi_mode"
#define PROP_SYS_AP_CHANNEL			"sys.wifi_channel"
#define PROP_WIFI_DRV_EXIST         "sys.wifi_driver"
#define PROP_WIFI_AP_STATE          "sys.wifi_ap_state"

#define PROP_SYS_FIRM_VER 			"sys.firm_ver"			/*  */
#define PROP_SYS_IMAGE_VER 			"sys.img_ver"
#define PROP_UC_START_UPDATE 		"sys.uc_update_app"
#define PROP_UC_START_APP 			"sys.uc_start_app"


#define PROP_SYS_UPDATE_IMG_PATH	"update_image_path"

/** update_check service version prop */
#define PROP_SYS_UC_VER 			"sys.uc_ver"

#define PROP_SYS_UA_VER 			"sys.ua_ver"

#define PROP_PWR_FIRST      	    "sys.hub_reset_first"

/*
 * 启动动画属性
 */
#define PROP_BOOTAN_NAME			"sys.bootan"


#define PROP_HUB_RESET_INTERVAL     "sys.hub_reset"
#define PROP_CAM_POWER_INTERVAL     "sys.cam_pinterval"

#define PROP_SYS_DISK_NUM 			"sys.disk_cnt"
#define PROP_SYS_DISK_RW			"sys.disk_rw"

#define PROP_SYS_DEV_DISK0			"sys.disk0"
#define PROP_SYS_DEV_DISK1			"sys.disk1"
#define PROP_SYS_DEV_DISK2			"sys.disk2"
#define PROP_SYS_DEV_DISK3			"sys.disk3"
#define PROP_SYS_DEV_DISK4			"sys.disk4"
#define PROP_SYS_DEV_DISK5			"sys.disk5"
#define PROP_SYS_DEV_DISK6			"sys.disk6"
#define PROP_SYS_DEV_DISK7			"sys.disk7"
#define PROP_SYS_DEV_DISK8			"sys.disk8"
#define PROP_SYS_DEV_DISK9			"sys.disk9"


#define PROP_MAX_DISK_SLOT_NUM	    10


/*
 * WIFI固件路径
 */
#define BCMDHD_DRIVER_PATH 		"/home/nvidia/insta360/wifi/bcmdhd.ko"
#define WIFI_RAND_NUM_CFG 		"/home/nvidia/insta360/etc/.wifi_rand_sn"
#define SYS_TMP 				"/home/nvidia/insta360/etc/sys_tmp"



#if 0
const char *rom_ver_file = "/home/nvidia/insta360/etc/pro_version";
const char *build_ver_file = "/home/nvidia/insta360/etc/pro_build_version";
#endif

/* 
 * FIFO路径
 */
#define  FIFO_FROM_CLIENT		"/home/nvidia/insta360/fifo/fifo_read_client"
#define  FIFO_TO_CLIENT			"/home/nvidia/insta360/fifo/fifo_write_client"
#define  ETC_RESOLV_PATH         "/etc/resolv.conf"


/*
 * 路径
 */
#define VER_FULL_PATH 			"/home/nvidia/insta360/etc/.sys_ver"
#define VER_FULL_TMP_PATH		"/home/nvidia/insta360/etc/.sys_tmp_ver"
#define VER_BUILD_PATH 			"/home/nvidia/insta360/etc/pro_build_version"

/*
 * 配置参数路径
 */
#define USER_CFG_PARAM_PATH     "/home/nvidia/insta360/etc/user_cfg"
#define DEF_CFG_PARAM_PATH      "/home/nvidia/insta360/etc/def_cfg"
#define WIFI_CFG_PARAM_PATH     "/home/nvidia/insta360/etc/wifi_cfg"


/*
 * 日志存放路径名
 */
#define PRO2_SERVICE_LOG_PATH   "/home/nvidia/insta360/log/p_log"


#define UPDATE_APP_LOG_PATH     "/home/nvidia/insta360/log/ua_log" 

/*
 * 拍照模式名称
 */
#define TAKE_PIC_MODE_8K_3D_OF      "8k_3d_of"
#define TAKE_PIC_MODE_8K_3D         "8k_3d"
#define TAKE_PIC_MODE_8K            "8k"
#define TAKE_PIC_MODE_AEB           "aeb"
#define TAKE_PIC_MODE_BURST         "burst"
#define TAKE_PIC_MODE_CUSTOMER      "pic_customer"





/*
 * 各设置项的名称
 */
#define SET_ITEM_NAME_DHCP          "dhcp"
#define SET_ITEM_NAME_FREQ          "freq"
#define SET_ITEM_NAME_HDR           "hdr"
#define SET_ITEM_NAME_RAW           "raw"
#define SET_ITEM_NAME_AEB           "aeb"
#define SET_ITEM_NAME_PHDEALY       "photodelay"
#define SET_ITEM_NAME_SPEAKER       "speaker"
#define SET_ITEM_NAME_LED           "led"
#define SET_ITEM_NAME_AUDIO         "audio"
#define SET_ITEM_NAME_SPAUDIO       "spaudio"
#define SET_ITEM_NAME_FLOWSTATE     "flowstate"
#define SET_ITEM_NAME_GYRO_ONOFF    "gyro_onoff"
#define SET_ITEM_NAME_GYRO_CALC     "gyrocal"
#define SET_ITEM_NAME_FAN           "fan"
#define SET_ITEM_NAME_NOISESAM      "samplenoise"
#define SET_ITEM_NAME_BOOTMLOGO     "bottomlogo"
#define SET_ITEM_NAME_VIDSEG        "vidseg"
#define SET_ITEM_NAME_STORAGE       "storage"
#define SET_ITEM_NAME_INFO          "info"
#define SET_ITEM_NAME_RESET         "reset"



#define SET_ITEM_NAME_3S            "3S"
#define SET_ITEM_NAME_5S            "5S"
#define SET_ITEM_NAME_10S           "10S"
#define SET_ITEM_NAME_20S           "20S"
#define SET_ITEM_NAME_30S           "30S"
#define SET_ITEM_NAME_40S           "40S"
#define SET_ITEM_NAME_50S           "50S"
#define SET_ITEM_NAME_60S           "60S"


#define SET_ITEM_NAME_AEB3          "aeb3"
#define SET_ITEM_NAME_AEB5          "aeb5"
#define SET_ITEM_NAME_AEB7          "aeb7"
#define SET_ITEM_NAME_AEB9          "aeb9"

#endif /* PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_ */
