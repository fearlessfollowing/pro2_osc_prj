/*
 * Setting Menu
 */
#include <icon/setting_menu_icon.h>
#include <vector>

using namespace std;


static vector<sp<SettingItem>>  mSettingList = NULL;
static int gItemsIndex = 0;




typedef struct  {
    u8 	xPos;
    u8 	yPos;
    u8 	iWidth;
    u8 	iHeight;
} ICON_POS;


typedef struct stSetItem {
	const char* 	pItemName;								/* 设置项的名称 */
	int				iItemMaxVal;							/* 设置项可取的最大值 */
	int  			iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	bool			bHaveSubMenu;							/* 是否含有子菜单 */
	void 			(*pSetItemProc)(struct stSetItem*);		/* 菜单项的处理函数(当选中并按确认时被调用) */
	ICON_POS		stPos;
	const u8 * 		stLightIcon[SETIING_ITEM_ICON_NUM];		/* 选中时的图标列表 */
	const u8 * 		stNorIcon[SETIING_ITEM_ICON_NUM];		/* 未选中时的图标列表 */
} SettingItem;


/* 菜单项的处理函数(当选中并按确认时被调用) */
void setItemComProc(struct stSetItem* pSetItem)
{

}



/* Ethernet Normal: DHCP/Direct  */
SettingItem setDhcpItem = {
	"dhcp",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		set_ethernet_direct_light_96_16,
		set_ethernet_dhcp_light_96_16,
	},					
	{	/* 未选中时的图标列表 */
		set_ethernet_direct_normal_96_16,
		set_ethernet_dhcp_normal_96_16,
	}					
};


/* Frequency Light: 50/60Hz */
SettingItem setFreqItem = {
	"freq",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		set_frequency_50hz_light_96_16,
		set_frequency_60hz_light_96_16,
	},					
	{	/* 未选中时的图标列表 */
		set_frequency_50hz_normal_96_16,
		set_frequency_60hz_normal_96_16,
	}					
};

/* DOL HDR Normal: On/Off */
SettingItem setHDRItem = {
	"hdr",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setHdrOffLight_96x16,
		setHdrOnLight_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setHdrOffNor_96x16,
		setHdrOnNor_96x16,
	}					
};


/* Raw Photo: On/Off */
SettingItem setRawPhotoItem = {
	"rawPhoto",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setRawPhotoOffLight_96x16,
		setRawPhotoOnLight_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setRawPhotoOffNor_96x16,
		setRawPhotoOnNor_96x16,
	}					
};


/* AEB: 3,5,7,9 */
SettingItem setAebItem = {
	"aeb",				// pItemName
	3,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setAeb3Light_96x16,
		setAeb5Light_96x16,
		setAeb7Light_96x16,
		setAeb9Light_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setAeb3Nor_96x16,
		setAeb5Nor_96x16,
		setAeb7Nor_96x16,
		setAeb9Nor_96x16,
	}					
};


/* Photo Delay: - 含有二级子菜单 */
SettingItem setPhotoDelayItem = {
	"photodelay",		// pItemName
	7,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setPhotoDelay3sLight_96x16,
		setPhotoDelay5sLight_96x16,
		setPhotoDelay10sLight_96x16,
		setPhotoDelay20sLight_96x16,
		setPhotoDelay30sLight_96x16,
		setPhotoDelay40sLight_96x16,
		setPhotoDelay50sLight_96x16,
		setPhotoDelay60sLight_96x16,

	},					
	{	/* 未选中时的图标列表 */
		setPhotoDelay3sNor_96x16,
		setPhotoDelay5sNor_96x16,
		setPhotoDelay10sNor_96x16,
		setPhotoDelay20sNor_96x16,
		setPhotoDelay30sNor_96x16,
		setPhotoDelay40sNor_96x16,
		setPhotoDelay50sNor_96x16,
		setPhotoDelay60sNor_96x16,
	}					
};


/* Speaker: On/Off */
SettingItem setSpeakerItem = {
	"speaker",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setSpeakOffLight_96x16,
		setSpeakOnLight_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setSpeakOffNor_96x16,
		setSpeakOnNor_96x16,
	}					
};



/* Led: On/Off */
SettingItem setLedItem = {
	"led",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setLedOffNor_96x16,
		setLedOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setLedOffLight_96x16,
		setLedOnLight_96x16,
	}					
};


/* Audio: Off/On */
SettingItem setAudioItem = {
	"audio",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setAudioOffNor_96x16,
		setAudioOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setAudioOffLight_96x16,
		setAudioOnLight_96x16,
	}					
};

/* Spatial Audio: Off/On */
SettingItem setSpatialAudioItem = {
	"spaudio",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setSpatialAudioOffNor_96x16,
		setSpatialAudioOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setSpatialAudioOffLight_96x16,
		setSpatialAudioOnLight_96x16,
	}					
};


/* FlowState: Off/On */
SettingItem setFlowStateItem = {
	"flowstate",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setFlowStateOffNor_96x16,
		setFlowStateOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setFlowStateOffLight_96x16,
		setFlowStateOnLight_96x16,
	}					
};


/* Gyro calc: Off/On */
SettingItem setGyroCalItem = {
	"gyrocal",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setGyrCalNor_96x16,
		setGyrCalNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setGyrCalLight_96x16,
		setGyrCalLight_96x16,
	}					
};


/* Fan: Off/On */
SettingItem setFanItem = {
	"fan",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setFanOffNor_96x16,
		setFanOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setFanOffLight_96x16,
		setFanOnLight_96x16,
	}					
};


/* Sample Fan Noise */
SettingItem setSampleNosieItem = {
	"samplenoise",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setSampNoiseNor_96x16,
		setSampNoiseNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setSampNoiseLight_96x16,
		setSampNoiseLight_96x16,
	}					
};


/* Bottom Logo: Off/On */
SettingItem setBottomLogoItem = {
	"bottomlogo",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setBottomlogoOff_Nor_96x16,
		setBottomlogoOn_Nor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setBottomlogoOff_Light_96x16,
		setBottomlogoOn_Light_96x16,
	}					
};


/* Video Seg: Off/On */
SettingItem setVideSegItem = {
	"vidseg",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setSegOffNor_96x16,
		setSegOnNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setSegOffLight_96x16,
		setSegOnLight_96x16,
	}					
};


/* Storage */
SettingItem setStorageItem = {
	"storage",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setStorageNor_96x16,
		setStorageNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setStorageLight_96x16,
		setStorageLight_96x16,
	}					
};


/* CamerInfo */
SettingItem setInfoItem = {
	"info",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setInfoNor_96x16,
		setInfoNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setInfoLight_96x16,
		setInfoLight_96x16,
	}					
};


/* Reset */
SettingItem setResetItem = {
	"info",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	{},
	{ 	/* 选中时的图标列表 */
		setResetNor_96x16,
		setResetNor_96x16,
	},					
	{	/* 未选中时的图标列表 */
		setResetLight_96x16,
		setResetLight_96x16,
	}					
};


enum {
    SET_DHCP_MODE,		
    SET_FREQ,			
	SET_HDR,

	SET_RAWPHOTO,
	SET_AEB,
    SET_PHOTO_DELAY,

    SET_SPEAK_ON,
	SET_LIGHT_ON,
	SET_AUD_ON,	

	SET_SPATIAL_AUD,
	SET_FLOWSTATE,
	SET_START_GYRO,

	SET_FAN_ON,
	SET_NOISE,
    SET_BOTTOM_LOGO, 

	SET_VIDEO_SEGMENT,
	SET_STORAGE,
	SET_INFO,

	SET_RESTORE,
    SETTING_MAX
}; 


void MenuUI::updateSettingItemVal(int iIndex, int iCurVal)
{
	if (iIndex > SETTING_MAX || iIndex < 0 )
		return;

	if (iCurVal > gSettingItems[iIndex]->iItemMaxVal)
		return;

	gSettingItems[iIndex]->iCurVal = iCurVal;
}


SettingItem* gSettingItems[] = {
	&setDhcpItem,
	&setFreqItem,
	&setHDRItem,

	&setRawPhotoItem,
	&setAebItem,
	&setPhotoDelayItem,

	&setSpeakerItem,
	&setLedItem,
	&setAudioItem,

	&setSpatialAudioItem,
	&setFlowStateItem,
	&setGyroCalItem,

	&setFanItem,
	&setSampleNosieItem,
	&setBottomLogoItem,

	&setVideSegItem,
	&setStorageItem,
	&setInfoItem,

	&setResetItem,
};


static bool checkSettingItemExist(SettingItem* pItems)
{
    SettingItem* tmpItem; 

    for (u32 i = 0;i < mSettingList.size(); i++) {
        tmpItem = mSettingList.at(i);
        if (strcmp(tmpItem->pItemName, pItems->pItemName) == 0) {
            return true;
        }
    }
    return false;
}

static void registerSettingItem(SettingItem* pItems, ICON_POS* pPos) 
{
    if (pItems && pPos) {
        if (checkSettingItemExist(pItems) == true) {
            Log.d(TAG, "Setting Item [%s] have exist", pItems->pItemName);
        } else {	/* 注册项，更加注册的顺序来决定该项的显示位置 */
			Log.d(TAG, "Setting Item [%s] not exist", pItems->pItemName);
			pItems->stPos = *pPos;
			mSettingList.push_back(pItems);
        }
    } else {
        Log.e(TAG, "Invalid pointer %d", __LINE__);
    }
}


#if 0

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
#endif






/*
 * 为指定的菜单注册设置项
 */
static void registerSettingItems(MENU_INFO* pMenu, SettingItem** pItems)
{

	ICON_POS tmPos;
	int iCnt = sizeof(pItems) / sizeof(pItems[0]);

	if (pMenu == NULL || iCnt== 0) {
		Log.w(TAG, "Invalid arguments, please check");
		return;
	}

	/* 依次注册各个设置项
	 * （32，16, 96, 16）
	 */
	for (u32 i = 0; i < iCnt; i++) {
		int pos = i % pMenu->mSelectInfo.page_max;		// 3
		switch (pos) {
			case 0:
				tmPos.xPos 		= 32;
				tmPos.yPos 		= 16;
				tmPos.iWidth	= 96;
				tmPos.iHeight   = 16;
				break;

			case 1:
				tmPos.xPos 		= 32;
				tmPos.yPos 		= 32;
				tmPos.iWidth	= 96;
				tmPos.iHeight   = 16;
				break;
			
			case 2:
				tmPos.xPos 		= 32;
				tmPos.yPos 		= 48;
				tmPos.iWidth	= 96;
				tmPos.iHeight   = 16;
				break;
		}
		registerSettingItem(pItems[i], &tmPos);
	}

	pMenu->priv = pItems;
}