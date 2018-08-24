#ifndef _VOLUMEMANAGER_H
#define _VOLUMEMANAGER_H

#include <pthread.h>
#include <sys/ins_types.h>
#include <vector>

enum {
    VOLUME_MANAGER_LISTENER_MODE_NETLINK,
    VOLUME_MANAGER_LISTENER_MODE_INOTIFY,
    VOLUME_MANAGER_LISTENER_MODE_MAX,
};


#define VOLUME_NAME_MAX 32




/* 卷:
 * 子系统:      USB/SD
 *             所属的总线（BUS）
 * 容量相关:    总线/剩余/可用/索引
 * 测速相关：   是否已经测速
 * 挂载相关:    该设备的挂载点(由所属的bus-port-subport决定)
 * 状态相关:    未插入,已挂载,正在格式化,格式化完成
 */


/*
 * 卷所属的子系统 - SD/USB两类
 */
enum {
    VOLUME_SUBSYS_SD,
    VOLUME_SUBSYS_USB,
    VOLUME_SUBSYS_MAX,
};


enum {
	VOLUME_TYPE_NV = 0,
	VOLUME_TYPE_MODULE = 1,
	VOLUME_TYPE_MAX
};


enum {
	VOLUME_SPEED_TEST_SUC = 0,
	VOLUME_SPEED_TEST_FAIL = -1,
};


enum {
    VOLUME_STATE_INIT       = -1,
    VOLUME_STATE_NOMEDIA    = 0,
    VOLUME_STATE_IDLE       = 1,
    VOLUME_STATE_PENDING    = 2,
    VOLUME_STATE_CHECKNG    = 3,
    VOLUME_STATE_MOUNTED    = 4,
    VOLUME_STATE_UNMOUNTING = 5,
    VOLUME_STATE_FORMATTING = 6,
};


#define COM_NAME_MAX_LEN       64

typedef struct stVol {
    int             iVolSubsys;         /* 卷的子系统： USB/SD */
    const char*     pBusAddr;           /* 总线地址: USB - "1-2.3" */
    const char*     pMountPath;         /* 挂载点：挂载点与总线地址是一一对应的 */

    char            cVolName[COM_NAME_MAX_LEN];       /* 卷的名称 */
    char            cDevNode[COM_NAME_MAX_LEN];      /* 设备节点名: 如'/dev/sdX' */
    char            cVolFsType[COM_NAME_MAX_LEN];     /* 卷使用的文件系统类型 */

	int		        iType;              /* 用于表示是内部设备还是外部设备 */
	int		        iIndex;			    /* 索引号（对于模组上的小卡有用） */

    int             iVolState;          /* 卷所处的状态: No_Media/Init/Mounted/Formatting */

    u64             uTotal;			    /* 总容量 */
    u64             uAvail;			    /* 剩余容量 */

	int 	        iSpeedTest;		    /* 1: 已经测速通过; 0: 没有进行测速或测速未通过 */
} Volume;


/* add/remove block  /sda
 * block
 */

#if 0


08-23 21:50:24.407 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.3/1-2.3:1.0/host38/target38:0:0/38:0:0:0/block/sde
08-23 21:50:24.409 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.3/1-2.3:1.0/host38/target38:0:0/38:0:0:0/block/sde/sde1
08-23 21:50:54.615 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.2/1-2.2:1.0/host36/target36:0:0/36:0:0:0/block/sdc
08-23 21:50:54.617 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.2/1-2.2:1.0/host36/target36:0:0/36:0:0:0/block/sdc/sdc1
08-23 21:50:57.175 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.3/1-3.3:1.0/host39/target39:0:0/39:0:0:0/block/sdf
08-23 21:50:57.177 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-3/1-3.3/1-3.3:1.0/host39/target39:0:0/39:0:0:0/block/sdf/sdf1
08-23 21:50:59.223 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.2/1-3.2:1.0/host37/target37:0:0/37:0:0:0/block/sdd
08-23 21:50:59.225 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-3/1-3.2/1-3.2:1.0/host37/target37:0:0/37:0:0:0/block/sdd/sdd1










08-23 21:51:12.567 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1
08-23 21:51:12.570 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0
08-23 21:51:12.583 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40
08-23 21:51:12.583 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/scsi_host/host40
08-23 21:51:13.591 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0
08-23 21:51:13.591 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0
08-23 21:51:13.592 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/scsi_disk/40:0:0:0
08-23 21:51:13.592 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/scsi_device/40:0:0:0
08-23 21:51:13.930 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/virtual/bdi/8:96
08-23 21:51:13.939 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
08-23 21:51:13.939 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1



08-23 21:51:33.526 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.1/1-3.1:1.0/host35/target35:0:0/35:0:0:0/block/sdb
08-23 21:51:33.545 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.1/1-3.1:1.0/host35/target35:0:0/35:0:0:0/block/sdb
08-23 21:51:33.546 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-3/1-3.1/1-3.1:1.0/host35/target35:0:0/35:0:0:0/block/sdb/sdb1


08-23 21:51:52.470 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.3/1-2.3:1.0/host38/target38:0:0/38:0:0:0/block/sde
08-23 21:51:52.490 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.3/1-2.3:1.0/host38/target38:0:0/38:0:0:0/block/sde
08-23 21:51:52.490 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.3/1-2.3:1.0/host38/target38:0:0/38:0:0:0/block/sde/sde1


08-23 21:52:21.142 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.2/1-3.2:1.0/host37/target37:0:0/37:0:0:0/block/sdd
08-23 21:52:21.162 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.2/1-3.2:1.0/host37/target37:0:0/37:0:0:0/block/sdd
08-23 21:52:21.162 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-3/1-3.2/1-3.2:1.0/host37/target37:0:0/37:0:0:0/block/sdd/sdd1
08-23 21:52:33.430 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.3/1-3.3:1.0/host39/target39:0:0/39:0:0:0/block/sdf
08-23 21:52:33.450 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-3/1-3.3/1-3.3:1.0/host39/target39:0:0/39:0:0:0/block/sdf
08-23 21:52:33.450 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-3/1-3.3/1-3.3:1.0/host39/target39:0:0/39:0:0:0/block/sdf/sdf1





08-23 21:52:49.302 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.4/1-2.4:1.0/host34/target34:0:0/34:0:0:0/block/sda
08-23 21:52:49.322 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.4/1-2.4:1.0/host34/target34:0:0/34:0:0:0/block/sda
08-23 21:52:49.322 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.4/1-2.4:1.0/host34/target34:0:0/34:0:0:0/block/sda/sda1



08-23 21:53:07.734 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.2/1-2.2:1.0/host36/target36:0:0/36:0:0:0/block/sdc
08-23 21:53:07.753 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: change@/devices/3530000.xhci/usb1/1-2/1-2.2/1-2.2:1.0/host36/target36:0:0/36:0:0:0/block/sdc
08-23 21:53:07.753 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb1/1-2/1-2.2/1-2.2:1.0/host36/target36:0:0/36:0:0:0/block/sdc/sdc1



08-23 21:53:35.613 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/scsi_device/40:0:0:0
08-23 21:53:35.613 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/scsi_disk/40:0:0:0
08-23 21:53:35.613 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1
08-23 21:53:35.613 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
08-23 21:53:35.613 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0
08-23 21:53:35.626 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/virtual/bdi/8:96
08-23 21:53:35.626 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0
08-23 21:53:35.670 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/scsi_host/host40
08-23 21:53:35.670 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40
08-23 21:53:35.671 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0
08-23 21:53:35.671 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb1/1-2/1-2.1



08-23 21:54:05.708 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2
08-23 21:54:05.718 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0
08-23 21:54:05.729 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41
08-23 21:54:05.730 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/scsi_host/host41
08-23 21:54:06.736 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0
08-23 21:54:06.736 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0
08-23 21:54:06.738 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/scsi_disk/41:0:0:0
08-23 21:54:06.738 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/scsi_device/41:0:0:0
08-23 21:54:07.160 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/virtual/bdi/8:96
08-23 21:54:07.170 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/block/sdg
08-23 21:54:07.170 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/block/sdg/sdg1



08-23 21:54:37.261 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/scsi_device/41:0:0:0
08-23 21:54:37.261 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/scsi_disk/41:0:0:0
08-23 21:54:37.261 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/block/sdg/sdg1
08-23 21:54:37.262 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0/block/sdg
08-23 21:54:37.262 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0/41:0:0:0
08-23 21:54:37.274 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/virtual/bdi/8:96
08-23 21:54:37.274 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/target41:0:0
08-23 21:54:37.314 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41/scsi_host/host41
08-23 21:54:37.314 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host41
08-23 21:54:37.315 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0
08-23 21:54:37.323 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: remove@/devices/3530000.xhci/usb2/2-2/2-2.2





08-23 21:54:44.004 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2
08-23 21:54:44.015 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0
08-23 21:54:44.026 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42
08-23 21:54:44.026 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/scsi_host/host42
08-23 21:54:45.032 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0
08-23 21:54:45.032 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0/42:0:0:0
08-23 21:54:45.032 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0/42:0:0:0/scsi_disk/42:0:0:0
08-23 21:54:45.034 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0/42:0:0:0/scsi_device/42:0:0:0
08-23 21:54:45.453 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/virtual/bdi/8:96
08-23 21:54:45.464 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0/42:0:0:0/block/sdg
08-23 21:54:45.464 D/NetlinkEvent( 1625): >>> parseAsciiNetlinkMessage: add@/devices/3530000.xhci/usb2/2-2/2-2.2/2-2.2:1.0/host42/target42:0:0/42:0:0:0/block/sdg/sdg1


#endif


static Volume gSysVols[] = {
    {   /* SD卡 - 3.0 */
        VOLUME_SUBSYS_SD,
        "2-2",
        "/mnt/sdcard",
        {0},             /* 动态生成 */
        {0},
        {0},
        VOLUME_TYPE_NV,
        0,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* Udisk1 - 2.0/3.0 */
        VOLUME_SUBSYS_USB,
        "2-1,1-2.1",            /* 接3.0设备时的总线地址 */
        "/mnt/udisk1",
        {0},                    /* 动态生成 */
        {0}, 
        {0},

        VOLUME_TYPE_NV,
        0,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },
    {   /* Udisk2 - 2.0/3.0 */
        VOLUME_SUBSYS_USB,
        "2-3",           /* 3.0 */
        "/mnt/udisk2",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_NV,
        0,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD1 */
        VOLUME_SUBSYS_SD,
        "usb1-2.3",                     /* usb1-3.2 usb1-2.3 */
        "/mnt/mSD1",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        1,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD2 */
        VOLUME_SUBSYS_SD,
        "usb1-2.2",
        "/mnt/mSD2",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        2,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD3 */
        VOLUME_SUBSYS_SD,
        "usb1-3.3",
        "/mnt/mSD3",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        3,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD4 */
        VOLUME_SUBSYS_SD,
        "usb1-3.2",
        "/mnt/mSD4",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        4,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD5 */
        VOLUME_SUBSYS_SD,
        "usb1-3.1",
        "/mnt/mSD5",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        5,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },

    {   /* mSD6 */
        VOLUME_SUBSYS_SD,
        "usb1-2.4",
        "/mnt/mSD6",
        {0},             /* 动态生成 */
        {0},
        {0},

        VOLUME_TYPE_MODULE,
        6,
        VOLUME_STATE_INIT,
        0,
        0,
        VOLUME_SPEED_TEST_FAIL,
    },          

};


class NetlinkEvent;


/*
 * 底层: 接收Netlink消息模式, 监听设备文件模式
 * - 挂载/卸载/格式化
 * - 列出所有卷
 * - 获取指定名称的卷
 */
class VolumeManager {
private:
    static VolumeManager*   sInstance;

private:

    std::vector<Volume*>    mVolumes;       /* 管理系统中所有的卷 */

    std::vector<Volume*>    mLocalVols;     /* 管理系统中所有的卷 */
    std::vector<Volume*>    mModuleVols;    /* 模组卷 */

    bool                    mDebug;

    int                     mVolManagerDisabled;

public:
    virtual ~VolumeManager();

    /*
     * 启动/停止卷管理器
     */
    bool        start();
    bool        stop();

    /*
     * 处理块设备事件的到来
     */
    void        handleBlockEvent(NetlinkEvent *evt);


    int         listVolumes();

    int         mountVolume(Volume* pVol, NetlinkEvent* pEvt);

    int         formatVolume(const char *label, bool wipe);

    int         unmountVolume(Volume* pVol, bool force);

    void        disableVolumeManager(void) { mVolManagerDisabled = 1; }

    void        setDebug(bool enable);

    Volume*     isSupportedDev(const char* busAddr);
    bool        extractMetadata(const char* devicePath, char* volFsType, int iLen);
    bool        clearMountPath(const char* mountPath);
    bool        isValidFs(const char* devName, Volume* pVol);

    static VolumeManager *Instance();

    Volume *lookupVolume(const char *label);

private:
    int         iListenerMode;          /* 监听模式 */
    VolumeManager();

};




#endif