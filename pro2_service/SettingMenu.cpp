/*
 * Setting Menu
 */
#include <icon/setting_menu_icon.h>
#include <vector>

using namespace std;


static vector<sp<SettingItem>>  mSettingList = NULL;
static int gItemsIndex = 0;


static bool checkSettingItemExist(sp<SettingItem>& pItems)
{
    sp<SettingItem> tmpItem; 

    for (u32 i = 0;i < mSettingList.size(); i++) {
        tmpItem = mSettingList.at(i);
        if (strcmp(tmpItem->pItemName, pItems->pItemName) == 0) {
            return true;
        }
    }
    return false;
}

static int registerSettingItem(sp<SettingItem> & pItems) 
{
    if (pItems && pItems.get()) {
        if (checkSettingItemExist(pItems) == true) {
            Log.d(TAG, "Setting Item [%s] have exist", pItems->pItemName);
        } else {
            
        }
    } else {
        Log.e(TAG, "Invalid pointer %d", __LINE__);
    }
}



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
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, set_ethernet_direct_light_96_16},
		{0, 0, 0, 0, 0, set_ethernet_dhcp_light_96_16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, set_ethernet_direct_normal_96_16},
		{0, 0, 0, 0, 0, set_ethernet_dhcp_normal_96_16},
	}					
};


/* Frequency Light: 50/60Hz */
SettingItem setFreqItem = {
	"freq",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, set_frequency_50hz_light_96_16},
		{0, 0, 0, 0, 0, set_frequency_60hz_light_96_16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, set_frequency_50hz_normal_96_16},
		{0, 0, 0, 0, 0, set_frequency_60hz_normal_96_16},
	}					
};

/* DOL HDR Normal: On/Off */
SettingItem setHDRItem = {
	"hdr",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setHdrOffLight_96x16},
		{0, 0, 0, 0, 0, setHdrOnLight_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setHdrOffNor_96x16},
		{0, 0, 0, 0, 0, setHdrOnNor_96x16},
	}					
};


/* Raw Photo: On/Off */
SettingItem setRawPhotoItem = {
	"rawPhoto",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setRawPhotoOffLight_96x16},
		{0, 0, 0, 0, 0, setRawPhotoOnLight_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setRawPhotoOffNor_96x16},
		{0, 0, 0, 0, 0, setRawPhotoOnNor_96x16},
	}					
};


/* AEB: 3,5,7,9 */
SettingItem setAebItem = {
	"aeb",				// pItemName
	3,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setAeb3Light_96x16},
		{0, 0, 0, 0, 0, setAeb5Light_96x16},
		{0, 0, 0, 0, 0, setAeb7Light_96x16},
		{0, 0, 0, 0, 0, setAeb9Light_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setAeb3Nor_96x16},
		{0, 0, 0, 0, 0, setAeb5Nor_96x16},
		{0, 0, 0, 0, 0, setAeb7Nor_96x16},
		{0, 0, 0, 0, 0, setAeb9Nor_96x16},
	}					
};


/* Photo Delay: - 含有二级子菜单 */
SettingItem setPhotoDelayItem = {
	"photodelay",		// pItemName
	7,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setPhotoDelay3sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay5sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay10sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay20sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay30sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay40sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay50sLight_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay60sLight_96x16},

	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setPhotoDelay3sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay5sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay10sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay20sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay30sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay40sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay50sNor_96x16},
		{0, 0, 0, 0, 0, setPhotoDelay60sNor_96x16},
	}					
};


/* Speaker: On/Off */
SettingItem setSpeakerItem = {
	"speaker",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setSpeakOffLight_96x16},
		{0, 0, 0, 0, 0, setSpeakOnLight_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setSpeakOffNor_96x16},
		{0, 0, 0, 0, 0, setSpeakOnNor_96x16},
	}					
};



/* Led: On/Off */
SettingItem setLedItem = {
	"led",				// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setLedOffNor_96x16},
		{0, 0, 0, 0, 0, setLedOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setLedOffLight_96x16},
		{0, 0, 0, 0, 0, setLedOnLight_96x16},
	}					
};


/* Audio: Off/On */
SettingItem setAudioItem = {
	"audio",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setAudioOffNor_96x16},
		{0, 0, 0, 0, 0, setAudioOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setAudioOffLight_96x16},
		{0, 0, 0, 0, 0, setAudioOnLight_96x16},
	}					
};

/* Spatial Audio: Off/On */
SettingItem setSpatialAudioItem = {
	"spaudio",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setSpatialAudioOffNor_96x16},
		{0, 0, 0, 0, 0, setSpatialAudioOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setSpatialAudioOffLight_96x16},
		{0, 0, 0, 0, 0, setSpatialAudioOnLight_96x16},
	}					
};


/* FlowState: Off/On */
SettingItem setFlowStateItem = {
	"flowstate",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setFlowStateOffNor_96x16},
		{0, 0, 0, 0, 0, setFlowStateOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setFlowStateOffLight_96x16},
		{0, 0, 0, 0, 0, setFlowStateOnLight_96x16},
	}					
};


/* Gyro calc: Off/On */
SettingItem setGyroCalItem = {
	"gyrocal",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setGyrCalNor_96x16},
		{0, 0, 0, 0, 0, setGyrCalNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setGyrCalLight_96x16},
		{0, 0, 0, 0, 0, setGyrCalLight_96x16},
	}					
};


/* Fan: Off/On */
SettingItem setFanItem = {
	"fan",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setFanOffNor_96x16},
		{0, 0, 0, 0, 0, setFanOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setFanOffLight_96x16},
		{0, 0, 0, 0, 0, setFanOnLight_96x16},
	}					
};


/* Sample Fan Noise */
SettingItem setSampleNosieItem = {
	"samplenoise",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setSampNoiseNor_96x16},
		{0, 0, 0, 0, 0, setSampNoiseNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setSampNoiseLight_96x16},
		{0, 0, 0, 0, 0, setSampNoiseLight_96x16},
	}					
};


/* Bottom Logo: Off/On */
SettingItem setSampleNosieItem = {
	"bottomlogo",		// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setBottomlogoOff_Nor_96x16},
		{0, 0, 0, 0, 0, setBottomlogoOn_Nor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setBottomlogoOff_Light_96x16},
		{0, 0, 0, 0, 0, setBottomlogoOn_Light_96x16},
	}					
};


/* Video Seg: Off/On */
SettingItem setVideSegItem = {
	"vidseg",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setSegOffNor_96x16},
		{0, 0, 0, 0, 0, setSegOnNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setSegOffLight_96x16},
		{0, 0, 0, 0, 0, setSegOnLight_96x16},
	}					
};


/* Storage */
SettingItem setStorageItem = {
	"storage",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	false,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setStorageNor_96x16},
		{0, 0, 0, 0, 0, setStorageNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setStorageLight_96x16},
		{0, 0, 0, 0, 0, setStorageLight_96x16},
	}					
};


/* CamerInfo */
SettingItem setInfoItem = {
	"info",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setInfoNor_96x16},
		{0, 0, 0, 0, 0, setInfoNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setInfoLight_96x16},
		{0, 0, 0, 0, 0, setInfoLight_96x16},
	}					
};


/* Reset */
SettingItem setResetItem = {
	"info",			// pItemName
	1,					// iItemMaxVal
	0,					// iCurVal
	true,				// bHaveSubMenu
	setItemComProc,		// pSetItemProc
	
	{ 	/* 选中时的图标列表 */
		{0, 0, 0, 0, 0, setResetNor_96x16},
		{0, 0, 0, 0, 0, setResetNor_96x16},
	},					
	{	/* 未选中时的图标列表 */
		{0, 0, 0, 0, 0, setResetLight_96x16},
		{0, 0, 0, 0, 0, setResetLight_96x16},
	}					
};




void registerSettingItem (sp<SettingItem>& pItem)
{

}




#if 0

typedef struct stSetItem {
	const char* pItemName;								/* 设置项的名称 */
	int			iItemMaxVal;							/* 设置项可取的最大值 */
	int  		iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	bool		bHaveSubMenu;							/* 是否含有子菜单 */
	void 		(*pSetItemProc)(struct stSetItem*);		/* 菜单项的处理函数(当选中并按确认时被调用) */
	ICON_INFO 	stLightIcon[SETIING_ITEM_ICON_NUM];		/* 选中时的图标列表 */
	ICON_INFO 	stNorIcon[SETIING_ITEM_ICON_NUM];		/* 未选中时的图标列表 */
} SettingItem;
#endif 