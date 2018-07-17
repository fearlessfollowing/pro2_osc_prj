/*
 * pro_cfg.h
 *
 *  Created on: 2018年6月6日
 *      Author: root
 */

#ifndef PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_
#define PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_


#define ETH0_NAME       "eth0"
#define DEFAULT_ETH0_IP "192.168.1.188"


#define WLAN0_NAME       "wlan0"
#define WLAN0_DEFAULT_IP "192.168.43.1"

/*
 * Prop system
 */
#define PROP_SYS_FIRM_VER 			"sys.firm_ver"			/*  */
#define PROP_SYS_IMAGE_VER 			"sys.img_ver"
#define PROP_UC_START_UPDATE 		"sys.uc_update_app"
#define PROP_UC_START_APP 			"sys.uc_start_app"

#define PROP_SYS_UPDATE_IMG_PATH	"update_image_path"

/** update_check service version prop */
#define PROP_SYS_UC_VER "sys.uc_ver"

#define PROP_SYS_UA_VER "sys.ua_ver"


#define PROP_SYS_DISK_NUM 	"sys.disk_cnt"
#define PROP_SYS_DISK_RW	"sys.disk_rw"

#define PROP_SYS_DEV_DISK0	"sys.disk0"
#define PROP_SYS_DEV_DISK1	"sys.disk1"
#define PROP_SYS_DEV_DISK2	"sys.disk2"
#define PROP_SYS_DEV_DISK3	"sys.disk3"
#define PROP_SYS_DEV_DISK4	"sys.disk4"
#define PROP_SYS_DEV_DISK5	"sys.disk5"
#define PROP_SYS_DEV_DISK6	"sys.disk6"
#define PROP_SYS_DEV_DISK7	"sys.disk7"
#define PROP_SYS_DEV_DISK8	"sys.disk8"
#define PROP_SYS_DEV_DISK9	"sys.disk9"


#define PROP_MAX_DISK_SLOT_NUM	10


/*
 * WIFI固件路径
 */
#define BCMDHD_DRIVER_PATH 		"/home/nvidia/insta360/wifi/bcmdhd.ko"

/* 
 * FIFO路径
 */
#define  FIFO_FROM_CLIENT		"/home/nvidia/insta360/fifo/fifo_read_client"
#define  FIFO_TO_CLIENT			"/home/nvidia/insta360/fifo/fifo_write_client"



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

#endif /* PRO2_OSC_CODE_CODE_CORE_INCLUDE_PROP_CFG_H_ */
