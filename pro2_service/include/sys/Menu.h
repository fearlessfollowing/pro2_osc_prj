#ifndef _MENU_H_
#define _MENU_H_

#include <hw/dev_manager.h>
#include <sys/net_manager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>
#include <sys/StorageManager.h>
#include <sys/action_info.h>
#include <sys/pro_cfg.h>
#include <icon/pic_mode_select.h>


enum {

    OLED_KEY_UP			= 0x101,
    OLED_KEY_DOWN 		= 0x102,
    OLED_KEY_BACK 		= 0x104,
    OLED_KEY_SETTING 	= 0x103,
    OLED_KEY_POWER 		= 0x100,
    OLED_KEY_MAX,
};


enum {
    MENU_TOP ,
    MENU_PIC_INFO,
    MENU_VIDEO_INFO,
    MENU_LIVE_INFO,
    MENU_SYS_SETTING,       //5
    MENU_PIC_SET_DEF,
    MENU_VIDEO_SET_DEF ,
    MENU_LIVE_SET_DEF,
    MENU_CALIBRATION,
    MENU_QR_SCAN,//10
    
	MENU_STORAGE,		//menu storage setting
    
//    MENU_CALIBRATION_SETTING,
    MENU_SYS_DEV_INFO,
    MENU_SYS_ERR,
    MENU_LOW_BAT,
    MENU_GYRO_START,
    MENU_SPEED_TEST,//15
    MENU_RESET_INDICATION,
    MENU_WIFI_CONNECT,
    MENU_AGEING,
//    MENU_LOW_PROTECT,
    MENU_NOSIE_SAMPLE,
    MENU_LIVE_REC_TIME,//20

    MENU_STITCH_BOX,
    MENU_FORMAT,
    MENU_FORMAT_INDICATION,
    
	MENU_SET_PHOTO_DEALY,	/* add by skymixos */
	
    //messagebox keep at the end
    MENU_DISP_MSG_BOX,
    MENU_MAX,

#if 0
    MENU_CAMERA_SETTING,
    //lr_menu start_item
    MENU_PIC_SETTING,
    MENU_VIDEO_SETTING,
    MENU_LIVE_SETTING,
    //pic
    MENU_ORIGIN = 10, //default, raw, off
    MENU_PIC_OUTP_3D,
    MENU_PIC_OUTP_PANO,
    //video
    MENU_VID_EACH_LEN_3D,
    MENU_VID_EACH_LEN_PANO,
    MENU_VID_BR = 15,
    MENU_VID_OUT_3D,
    MENU_VID_OUT_PANO,
    //live
    MENU_LIVE_BR,
    MENU_LIVE_OUT_3D,
    MENU_LIVE_OUT_PANO = 20,
    //menu sys device
    MENU_SYS_DEV,
    MENU_SYS_DEV_FACTORY_DEFAULT,
#endif

};




enum {
    MAINMENU_PIC,
    MAINMENU_VIDEO,
    MAINMENU_LIVE,
    MAINMENU_WIFI,
    MAINMENU_CALIBRATION,
    MAINMENU_SETTING,
    MAINMENU_MAX,
};


typedef struct _select_info_ {
    int last_select;		/* 上次选中的项 */
	
    int select;				/* 当前选中的项（在当前页中的索引） */
	
    int cur_page;			/* 选项所在的页 */

    int total;				/* 真个含有的项数 */

    int page_max;			/* 一页含有的项数 */

    int page_num;			/* 含有的页数 */
    
} SELECT_INFO;



typedef struct _menu_info_ {
	
    int 		back_menu;
	
    SELECT_INFO mSelectInfo;
    const int 	mSupportkeys[OLED_KEY_MAX];

	int 		iMenuId;	/* 菜单的ID */
    
	void*		priv;		/* 菜单的私有数据 */
    void*       privList; 
} MENU_INFO;





#define PAGE_MAX (3)




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


static MENU_INFO mMenuInfos[] = {
    {	
    	-1,					/* back_menu */
		{	
			-1,				/* last_select */
			0,				/* select */
			0,				/* cur_page */
			MAINMENU_MAX,	/* total */
			MAINMENU_MAX,	/* page_max */
			1				/* page_num */
		}, 
		{OLED_KEY_UP, OLED_KEY_DOWN,  0, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_TOP,           /* Menu ID: MENU_TOP */
		NULL,
        NULL,
	},	
	
    {	
    	MENU_TOP,
		{
			-1,
			0,
			0,
			0,
			0,
			0
		}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_PIC_INFO,      /* Menu ID: MENU_PIC_INFO */
		NULL,
        NULL,        
	},
	
    {	
    	MENU_TOP,
		{
            -1, 
            0,
            0, 
            0, 
            0, 
            0
        }, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_VIDEO_INFO,    /* Menu ID: MENU_VIDEO_INFO */
		NULL,
        NULL,        
	},

    {	/* MENU_LIVE_INFO */
    	MENU_TOP,
		{
            -1, 
            0, 
            0, 
            0, 
            0, 
            0
        }, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* DOWN, BACK, SETTING, POWER */
		MENU_LIVE_INFO,     /* Menu ID: MENU_LIVE_INFO */
		NULL,
        NULL,        
	},
	

	{	
    	MENU_TOP,
		{-1, 0, 0, 0, PAGE_MAX, 5}, /* 项数设置为0，初始化菜单时根据设置项vector的size来决定 */
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},		/* UP, DOWN, BACK, POWER */
		MENU_SYS_SETTING,    /* Menu ID: MENU_SYS_SETTING */
		NULL,                /* 设置页菜单的私有数据为一个设置项列表 */
        NULL,        
	}, 
	
    {	
    	MENU_PIC_INFO,
		{
            -1,
            0, 
            0, 
            PIC_ALLCARD_MAX, 
            PIC_ALLCARD_MAX, 
            1
        },
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},  /* UP, DOWN, BACK, SETTING, POWER */
        MENU_PIC_SET_DEF,      /* Menu ID: MENU_PIC_SET_DEF */
        NULL,
        NULL,        
	},

    {	
    	MENU_VIDEO_INFO,
		{
            -1, 
            0,
            0, 
            VID_DEF_MAX, 
            VID_DEF_MAX, 
            1
        },
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
        MENU_VIDEO_SET_DEF,     /* Menu ID: MENU_VIDEO_SET_DEF */
        NULL,                   /* TODO */
        NULL,        
    },
    
    {	/* MENU_LIVE_SET_DEF */
    	MENU_LIVE_INFO,
		{
            -1, 
            0, 
            0, 
            LIVE_DEF_MAX, 
            LIVE_DEF_MAX, 
            1
        },
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* UP, DOWN, BACK, SETTING, POWER */
        MENU_LIVE_SET_DEF,      /* Menu ID: MENU_LIVE_SET_DEF */
        NULL,
        NULL,        
    },
	
    {	
    	MENU_TOP,
		{0},
		{0},
        MENU_CALIBRATION,       /* Menu ID: MENU_CALIBRATION */
        NULL,
        NULL,
	},
	
    {	
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, 0},			/* BACK */
        MENU_QR_SCAN,           /* Menu ID: MENU_QR_SCAN */
        NULL,
        NULL,        
    }, //10
	
    //menu calibartion setting
#if 0    
   {
   		MENU_TOP,
		{-1, 0, 0, MODE_MAX, PAGE_MAX, 1}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER}
	},
#endif

    {	/* MENU_STORAGE */
    	MENU_SYS_SETTING,
		{
            -1, 
            0, 
            0, 
            SET_STORAGE_MAX, 
            SET_STORAGE_MAX, 
            1
        }, 
		{0, 0, OLED_KEY_BACK, 0, 0}	,	/* BACK */
		MENU_STORAGE,           /* Menu ID: MENU_STORAGE */
		NULL,
        NULL,        
	},


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
		{
            -1, 
            0, 
            0, 
            1, 
            PAGE_MAX, 
            1
        }, 
		{0, 0, OLED_KEY_BACK, 0, 0},
        MENU_SYS_DEV_INFO,      /* Menu ID: MENU_SYS_DEV_INFO */
        NULL,
        NULL,        
	},
#endif

    {	/* MENU_SYS_ERR */
    	MENU_TOP,
		{0},
		{0},
        MENU_SYS_ERR,
        NULL,
        NULL,        
	},

    {	/* MENU_LOW_BAT */
    	MENU_TOP,
    	{0},
    	{0},
        MENU_LOW_BAT,
        NULL,
        NULL,        
	},

    {	/* MENU_GYRO_START */
    	MENU_SYS_SETTING,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_GYRO_START,
        NULL,
        NULL,        
	},
	
    {	/* MENU_SPEED_TEST */
    	MENU_PIC_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},
        MENU_SPEED_TEST,
        NULL,
        NULL,        
	},
	
    {	/* MENU_RESET_INDICATION */
    	MENU_SYS_SETTING,
		{0},
		{OLED_KEY_UP, 0, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
        MENU_RESET_INDICATION,
        NULL,
        NULL,        
	},
	
    {	/* MENU_WIFI_CONNECT */
    	MENU_SYS_SETTING,
		{0},
		{0},
        MENU_WIFI_CONNECT,
        NULL,
        NULL,        
	},

		
    {	/* MENU_AGEING */
    	MENU_TOP,
		{0},
		{0},
        MENU_AGEING,
        NULL,
        NULL,        		
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
		{0},
        MENU_NOSIE_SAMPLE,
        NULL,
        NULL,        
	},
	
    {	/* MENU_LIVE_REC_TIME */
    	MENU_LIVE_INFO,
		{0},
		{0, 0, OLED_KEY_BACK, 0, OLED_KEY_POWER},			/* BACK, POWER */
        MENU_LIVE_REC_TIME,
        NULL,
        NULL,        

	},
	
	{	/* MENU_DISP_MSG_BOX */
    	MENU_TOP,
		{0},
		{0},
        MENU_DISP_MSG_BOX,
        NULL,
        NULL,        
	},
};



#endif /* _MENU_H_ */
