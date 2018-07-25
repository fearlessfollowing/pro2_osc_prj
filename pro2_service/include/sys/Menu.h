#ifndef _MENU_H_
#define _MENU_H_

typedef struct _select_info_ {
    int last_select;		/* 上次选中的项 */
	
    int select;				/* 当前选中的项 */
	
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
	
} MENU_INFO;



enum {
    MENU_TOP ,
    MENU_PIC_INFO,
    MENU_VIDEO_INFO,
    MENU_LIVE_INFO,
    MENU_SYS_SETTING, //5
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
		{OLED_KEY_UP, OLED_KEY_DOWN,  0, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_TOP,
		NULL,
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
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_PIC_INFO,
		NULL,
	},
	
    {	/* MENU_VIDEO_INFO */
    	MENU_TOP,
		{-1,0,0,0,0,0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},
		MENU_VIDEO_INFO,
		NULL,
	},
    {	/* MENU_LIVE_INFO */
    	MENU_TOP,
		{-1, 0, 0, 0, 0, 0}, 
		{0, OLED_KEY_DOWN, OLED_KEY_BACK, OLED_KEY_SETTING, OLED_KEY_POWER},		/* DOWN, BACK, SETTING, POWER */
		MENU_LIVE_INFO,
		NULL,
	},
	

	{	/* MENU_SYS_SETTING */
    	MENU_TOP,
		{-1, 0, 0, SETTING_MAX, PAGE_MAX, 5}, 
		{OLED_KEY_UP, OLED_KEY_DOWN, OLED_KEY_BACK, 0, OLED_KEY_POWER},		/* UP, DOWN, BACK, POWER */
		MENU_SYS_SETTING,
		NULL,
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

    {	/* MENU_STORAGE */
    	MENU_SYS_SETTING,
		{-1, 0, 0, SET_STORAGE_MAX, SET_STORAGE_MAX, 1}, 
		{0, 0, OLED_KEY_BACK, 0, 0}	,	/* BACK */
		MENU_STORAGE,
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



#endif /* _MENU_H_ */
