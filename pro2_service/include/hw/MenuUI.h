#ifndef PROJECT_OLED_WRAPPER_H
#define PROJECT_OLED_WRAPPER_H

#include <thread>
#include <vector>
#include <common/sp.h>
#include <hw/battery_interface.h>
#include <sys/net_manager.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <hw/oled_module.h>
#include <sys/StorageManager.h>
#include <sys/action_info.h>

#include <sys/Menu.h>


typedef enum _type_ {
    START_RECING ,              // 0,
    START_REC_SUC,
    START_REC_FAIL,
    STOP_RECING,
    STOP_REC_SUC,
    STOP_REC_FAIL ,             // 5,
    CAPTURE,
    CAPTURE_SUC,
    CAPTURE_FAIL,
    COMPOSE_PIC,
    COMPOSE_PIC_FAIL ,          // 10,
    COMPOSE_PIC_SUC,
    COMPOSE_VIDEO,
    COMPOSE_VIDEO_FAIL,
    COMPOSE_VIDEO_SUC,
    STRAT_LIVING ,              // 15,
    START_LIVE_SUC,
    START_LIVE_FAIL,
    STOP_LIVING,
    STOP_LIVE_SUC,
    STOP_LIVE_FAIL ,            // 20,
    PIC_ORG_FINISH ,            // 21,
    START_LIVE_CONNECTING ,     // 22,


    START_CALIBRATIONING = 27 ,// 27,
    CALIBRATION_SUC,
    CALIBRATION_FAIL,
    START_PREVIEWING ,  // 30,
    START_PREVIEW_SUC,  
    START_PREVIEW_FAIL,
    STOP_PREVIEWING,
    STOP_PREVIEW_SUC,
    STOP_PREVIEW_FAIL ,// 35,
    START_QRING,
    START_QR_SUC,
    START_QR_FAIL,
    STOP_QRING,
    STOP_QR_SUC ,// 40,
    STOP_QR_FAIL,
    QR_FINISH_CORRECT,
    QR_FINISH_ERROR,
    CAPTURE_ORG_SUC,
    CALIBRATION_ORG_SUC ,// 45,
    SET_CUS_PARAM,
    QR_FINISH_UNRECOGNIZE,
    TIMELPASE_COUNT,
    START_NOISE_SUC,
    START_NOISE_FAIL, //50
    START_NOISE,
    START_LOW_BAT_SUC,
    START_LOW_BAT_FAIL,
    LIVE_REC_OVER,
    SET_SYS_SETTING, //55

    STITCH_PROGRESS,

    START_BLC = 70,
    STOP_BLC,

    START_GYRO = 73,
    START_GYRO_SUC,
    START_GYRO_FAIL ,
    SPEED_TEST_SUC,
    SPEED_TEST_FAIL,
    SPEED_START = 78,// 78,

    SYNC_REC_AND_PREVIEW = 80,
    SYNC_PIC_CAPTURE_AND_PREVIEW,
    SYNC_PIC_STITCH_AND_PREVIEW,
    SYNC_LIVE_AND_PREVIEW = 90,
    SYNC_LIVE_CONNECT_AND_PREVIEW,// 91,
    //used internal
    START_STA_WIFI_FAIL,// 92,
    STOP_STA_WIFI_FAIL ,// 93,
    START_AP_WIFI_FAIL ,// 94,
    STOP_AP_WIFI_FAIL = 95 ,// 95,



    START_AGEING_FAIL = 97,
    START_AGEING = 98,
    START_FORCE_IDLE = 99,// 99,
    RESET_ALL = 100,// 100,


	START_QUERY_STORAGE = 110,	/*  */
	START_QUERY_STORAGE_SUC,
	START_QUERY_STORAGE_FAIL,

//    WRITE_FOR_BROKEN = 101,
    RESET_ALL_CFG = 102,
    MAX_TYPE,
} TYPE;


typedef struct _sys_info_ {
    char sn[128];				/* 序列号 */
    char uuid[128];				/* UUID */
    char ssid[128];				/* SSID */
    char sn_str[128];			/* SN字符串 */
} SYS_INFO;

typedef struct oled_coordinate {
    int x;
    int y;
    int w;
    int h;
} OLED_COORD;

typedef struct _disp_str_ {
    int x;
    int y;
    u8 str[128];
    int state;
} DISP_STR;

typedef struct _disp_bitmap_ {
    int x;
    int y;
    u8 bitmap[128];
    int state;
} DISP_BITMAP;

typedef struct _disp_ext_ {
    int ext_id;
    int state;
} DISP_EXT;

typedef struct _remain_info_ {
    int remain_min;
    int remain_sec;
    int remain_hour;
    int remain_pic_num;
} REMAIN_INFO;

typedef struct _save_path_ {
    char path[128];
//    sp<REMAIN_INFO> mRemain;
} SAVE_PATH;


//state logical coding is different from ws for no preview in oled key menu
enum {
    STATE_IDLE = 0x00,								/* 空间状态 */
    STATE_RECORD = 0x01,							/* 录像状态 */
    STATE_TAKE_CAPTURE_IN_PROCESS = 0x02,			/* 拍照正在处理状态 */
    STATE_COMPOSE_IN_PROCESS = 0x04,
    
    //nothing to matter with preview state in oled action
    STATE_PREVIEW = 0x08,							/* 预览状态 */
    STATE_LIVE = 0x10,								/* 直播状态 */
    STATE_PIC_STITCHING = 0x20,						/* 图片拼接状态 */
    
   //state just for camera
    STATE_START_RECORDING = 0x40,					/* 正在启动录像状态 */
    STATE_STOP_RECORDING = 0x80,					/* 正在停止录像状态 */
    STATE_START_LIVING = 0x100,						/* 正在启动直播状态 */
    STATE_STOP_LIVING = 0x200,						/* 正在停止直播状态 */

	STATE_QUERY_STORAGE = 0x400,					/* 查询容量状态 */
	
    STATE_CALIBRATING = 0x1000,						/* 正在校验状态 */
    STATE_START_PREVIEWING = 0x2000,				/* 正在启动预览状态 */
    STATE_STOP_PREVIEWING = 0x4000,					/* 正在停止预览状态 */
    STATE_START_QR = 0x8000,						/* 启动QR */
    STATE_RESTORE_ALL = 0x10000,
    STATE_STOP_QRING = 0x20000,
    STATE_START_QRING = 0x40000,
    STATE_LIVE_CONNECTING = 0x80000,
    STATE_LOW_BAT = 0x100000,
    STATE_POWER_OFF = 0x200000,
    STATE_SPEED_TEST = 0x400000,
    STATE_START_GYRO = 0x800000,
    STATE_NOISE_SAMPLE = 0x1000000,
    STATE_FORMATING = 0x2000000,
    STATE_FORMAT_OVER = 0x4000000,

    STATE_BLC_CALIBRATE = 0x10000000,
//    STATE_CAP_FINISHING = 0x1000000,
//    STATE_LIVE_FINISHING = 0x2000000,
//    STATE_REC_FINISHING = 0x4000000,
//    STATE_CAL_FINISHING = 0x8000000,
//    STATE_SYS_ERR = 0x8000002
	STATE_PLAY_SOUND = 0x20000000,


};




enum {
    REBOOT_NORMAL,
    REBOOT_SHUTDOWN,
};


class ARLooper;
class ARHandler;
class ARMessage;
class pro_cfg;
class oled_module;
class oled_light;
class net_manager;
class dev_manager;

enum {
    OPTION_FLICKER,
    OPTION_LOG_MODE,
    OPTION_SET_FAN,
    OPTION_SET_AUD,
    OPTION_GYRO_ON,
    OPTION_SET_LOGO,
    OPTION_SET_VID_SEG,
//    OPTION_SET_AUD_GAIN,
};

enum {
    ACTION_REQ_SYNC,
    ACTION_PIC,
    ACTION_VIDEO,
    ACTION_LIVE,
    ACTION_PREVIEW,
    ACTION_CALIBRATION,//5,
    ACTION_QR,
    ACTION_SET_OPTION,
    ACTION_LOW_BAT,
    ACTION_SPEED_TEST,
    ACTION_POWER_OFF,//10,
    ACTION_GYRO,
    ACTION_NOISE,

    //ACTION_LOW_PROTECT = 19,
    ACTION_CUSTOM_PARAM = 18,
    ACTION_LIVE_ORIGIN = 19,
    //force at end 0620
    ACTION_AGEING = 20,
    ACTION_AWB,

    ACTION_SET_STICH = 50,
	ACTION_QUERY_STORAGE = 200,
	
};



typedef struct _sync_init_info_ {
    char a_v[128];
    char c_v[128];
    char h_v[128];
    int state;
} SYNC_INIT_INFO;

typedef struct _req_sync_ {
    char sn[64];
    char r_v[128];
    char p_v[128];
    char k_v[128];
}REQ_SYNC;

enum {
    STI_7680x7680,//= 0,
    STI_5760x5760,
    STI_4096x4096,
    STI_3840x3840,
    STI_2880x2880,
    STI_1920x1920,//= 5,
    STI_1440x1440 ,
    ORG_4000x3000,
    ORG_3840x2160,
    ORG_2560x1440,
    ORG_1920x1080,//= 10,
    //best for 3d
    ORG_3200x2400,
    ORG_2160x1620,
    ORG_1920x1440,
    ORG_1280x960,
    STI_2160x1080,//15 ,for cube 1620*1080
    RES_MAX,
};

enum {
    ALL_FR_24,
    ALL_FR_25,
    ALL_FR_30,
    ALL_FR_60,
    ALL_FR_120,
    ALL_FR_5,
    ALL_FR_MAX,
};



struct _icon_info_;
struct _lr_menu_;
struct _r_menu_;
struct _remain_info_;
struct _select_info_;
struct _qr_res_;
struct _disp_type;
struct _action_info_;
struct _wifi_config_;
struct _err_type_info_;
struct _cam_prop_;
struct stSetItem;




class MenuUI {
public:

    enum {
        OLED_KEY,
        SAVE_PATH_CHANGE,
        UPDATE_BAT,
        UPDATE_DEV,
        FACTORY_AGEING,
		UPDATE_STORAGE,
    };

    void postUiMessage(sp<ARMessage>& msg);

    explicit MenuUI(const sp<ARMessage> &notify);
    ~MenuUI();
    void handleMessage(const sp<ARMessage> &msg);
	
    void send_disp_str(sp<struct _disp_type_> &sp_disp);
    void send_disp_err(sp<struct _err_type_info_> &sp_disp);


    //net type 0 -- default to wlan
    void send_disp_ip(int ip, int net_type = 0);
    void send_disp_battery(int battery, bool charge);

    void send_sys_info(sp<SYS_INFO> &mSysInfo);
    void send_get_key(int key);
    void send_long_press_key(int key,int64 ts);
    void send_init_disp();
    void send_update_dev_list(std::vector<sp<Volume>> &mList);
    void send_sync_init_info(sp<SYNC_INIT_INFO> &mSyncInfo);

    sp<ARMessage> obtainMessage(uint32_t what);

    //void postUiMessage(sp<ARMessage>& msg, int interval = 0);

    void updateTfStorageInfo(std::vector<sp<Volume>>& mStorageList);

private:

    MenuUI();

    bool start_speed_test();
    bool check_rec_tl();
    void disp_dev_msg_box(int bAdd,int type,bool bChange = false);
    void start_qr_func();
    void exit_qr_func();

    /*
     * 获取当前菜单的SECLECT_INFO
     */
    struct _select_info_ * getCurMenuSelectInfo();

	bool check_cur_menu_support_key(int iKey);

	void set_mainmenu_item(int item,int icon);
    void disp_calibration_res(int type,int t = -1);
    void disp_sec(int sec,int x,int y);
    bool isDevExist();
    
    /*
     * enterMenu - 进入菜单（会根据菜单的当前状态进行绘制）
     * dispBottom - 是否更新底部区域
     */
    void enterMenu(bool dispBottom = true);         //add dispBottom for menu_pic_info 170804

    void disp_low_protect(bool bStart = false);
    void disp_low_bat();
    void func_low_protect();

    bool menuHasStatusbar(int menu);

    void disp_waiting();
    void disp_connecting();
    void disp_saving();
    void disp_ready_icon(bool bDispReady = true);
    void disp_live_ready();
    void clear_ready();
    void disp_shooting();
    void disp_processing();
    bool check_state_preview();
    bool check_state_equal(int state);
    bool check_state_in(int state);
    bool check_live();

    void update_menu_page();

    //void update_menu_sys_setting(bool bUpdateLast = false);
    void update_menu_disp(const int *icon_light,const int *icon_normal = nullptr);
    void disp_scroll();
    void set_back_menu(int item,int menu);
    int get_back_menu(int item);

    int get_select();


#if 0
    void disp_wifi_connect();
#endif


    
    int get_last_select();

    void disp_org_rts(int org,int rts,int hdmi = -1);
    void disp_org_rts(sp<struct _action_info_> &mAct,int hdmi = -1);
    void send_save_path_change();
    void oled_init_disp();
    void disable_sys_wifi();
    void init_cfg_select();

    void disp_msg_box(int type);
    const u8 *get_disp_str(int lan_index);
    bool send_option_to_fifo(int option,int cmd = -1,struct _cam_prop_ * pstProp = nullptr);
    void fix_live_save_per_act(struct _action_info_ *mAct);
    bool check_live_save(struct _action_info_ *mAct);
    bool start_live_rec(const struct _action_info_ *mAct,struct _action_info_  *dest);
    bool check_allow_pic();

    //reset camera state and disp as begging
    int oled_reset_disp(int type);
    void disp_video_setting();

//    void disp_pic_setting();
    void disp_live_setting();
    void disp_storage_setting();
    void disp_format();
    void start_format();
    void reset_devmanager();
    void format(const char *src,const char *path,int trim_err_icon,int err_icon,int suc_icon);
    int exec_sh_new(const char *buf);
    #if 0
    void disp_sys_setting(SETTING_ITEMS* pSetting);
    #endif

    void disp_str(const u8 *str,const u8 x,const u8 y, bool high = 0,int width = 0);
    void disp_str_fill(const u8 *str,const u8 x,const u8 y, bool high = false);
    void clear_icon(u32 type);
    void disp_icon(u32 type);
    void disp_ageing();
    void set_lan(int lan);
    int oled_disp_err(sp<struct _err_type_info_> &mErr);
    int get_error_back_menu(int force_menu = -1);
    void set_oled_power(unsigned int on);
    void set_led_power(unsigned int on);
    int oled_disp_type(int type);
    void disp_sys_err(int type,int back_menu = -1);
    void disp_err_str(int type);
    void disp_err_code(int code,int back_menu);
    void disp_top_info();

    void set_setting_select(int type,int val);

    int oled_disp_battery();
    void clear_area(u8 x,u8 y, u8 w,u8 h);
    void clear_area(u8 x = 0,u8 y = 0);
    bool check_allow_update_top();
    void handleWifiAction();
    void disp_wifi(bool bState, int disp_main = -1);
    int wifi_stop();

    int start_wifi_ap(int disp_main = -1);
    void start_wifi(int disp_main = -1);
    bool start_wifi_sta(int disp_main = -1);
    void wifi_config(sp<struct _wifi_config_> &config);
    void set_sync_info(sp<SYNC_INIT_INFO> &mSyncInfo);
    void write_sys_info(sp<SYS_INFO> &mSysInfo);

    void read_sn();
    void read_uuid();

    bool read_sys_info(int type);
    bool read_sys_info(int type, const char *name);
    void read_ver_info();

    void init();
    void check_net_status();
    void init_menu_select();
    void deinit();
    void init_handler_thread();
    void init_readkey_thread();
    void init_sound_thread();
    void sound_thread();
    void play_sound(u32 type);
    void send_update_light(int menu, int state,int interval,bool bLight = false,int sound_id = -1);
    void write_p(int *p, int val);
    void stop_readkey_thread();
    void stop_update_bottom_thread();
    void stop_bat_thread();
    void readkey_thread();
    void update_bottom_thread();
    void add_state(int state);
    void update_by_controller(int action);
    void minus_cam_state(int state);
    void disp_tl_count(int count);
    void set_tl_count(int count);
    void rm_state(int state);
    void update_disp_func(int lan);

    void update_sys_info();
    void restore_all();

    void set_cur_menu_from_exit();
    void handle_top_menu_power_key();
    void send_oled_power_action();
	
    void exit_sys_err();
    void handle_wifi_action(int key);
    void handle_setting_action(int key);
    void sendExit();
    void exitAll();
    void set_org_addr(unsigned int addr);
    void set_mdev_list(std::vector<sp<Volume>> &mList);

    void sys_reboot(int cmd = REBOOT_SHUTDOWN);

	void disp_cam_param(int higlight);
    void disp_bottom_info(bool high = false);
    bool check_save_path_none();
    bool check_save_path_usb();
    int get_dev_type_index(char *dev_type);
    void get_storage_info();
    void disp_bottom_space();
    void update_bottom_space();
    bool get_save_path_avail();
    void get_save_path_remain();
    bool check_dev_exist(int action = -1);
    void convert_space_to_str(u64 size,char *str, int len);

    void caculate_rest_info(u64 size = 0);
    bool switch_dhcp_mode(int iDHCP);
    void send_clear_msg_box(int delay = 1000);
    void send_delay_msg(int msg_id, int delay);
    void send_bat_low();
    void send_read_bat();

    void send_update_mid_msg(int interval = 1000);
    void set_update_mid();
    void increase_rec_time();
    bool decrease_rest_time();
    void disp_mid();
    void flick_light();
    void flick_low_bat_lig();


    void add_qr_res(int type,sp<struct _action_info_> &mAdd,int control_act = -1);
    void update_sys_cfg(int item, int val);
    void set_sys_setting(sp<struct _sys_setting_> &mSysSetting);
    void disp_stitch_progress(sp<struct _stich_progress_> &mProgress);
    void disp_qr_res(bool high = true);

    // action from controller
//    void disp_control_def(int save_org,int rts,int hdmi);
    void reset_last_info();
    bool is_bat_low();
    bool is_bat_charge();
    void set_light(u8 val);
    void set_light_direct(u8 val);

    bool check_battery_change(bool bUpload = false);
    bool check_live_save_org();
    void func_low_bat();
    int get_battery_charging(bool *bCharge);
    int read_tmp(double *int_tmp,double *tmp);
    void set_flick_light();
    void set_light();
    bool check_cam_busy();
    void set_cap_delay(int delay);

	void dispIconByLoc(const ICON_INFO* pInfo);


	void setGyroCalcDelay(int iDelay);
	
	// void set_disp_control(bool state);

    bool switchEtherIpMode(int iMode);

    void update_menu_disp(const ICON_INFO *icon_light,const ICON_INFO *icon_normal = nullptr);


    /************************************** 灯光管理 START *************************************************/
    void setLightDirect(u8 val);
    void setLight(u8 val);
    void setLight();
    /************************************** 灯光管理 END *************************************************/


    /************************************** 菜单相关 START *************************************************/
    int getMenuSelectIndex(int menu);
    int getCurMenuCurSelectIndex();
    void updateMenuCurPageAndSelect(int menu, int iSelect);   
    int getMenuLastSelectIndex(int menu);
    int getCurMenuLastSelectIndex();    
    void updateMenu();
    void setCurMenu(int menu,int back_menu = -1);
    void cfgPicVidLiveSelectMode(int iMode);

    /*
     * 系统信息
     */
    void dispSysInfo();


    /*
     * 设置页部分
     */
    void dispSetItem(struct stSetItem* pItem, bool iSelected);
    void procSetMenuKeyEvent();
    void setMenuCfgInit();
    void setSysMenuInit(MENU_INFO* pParentMenu);
    void setCommonMenuInit(MENU_INFO* pParentMenu, std::vector<struct stSetItem*>& pItemLists);    
    void updateMenuPage();
    void updateInnerSetPage(std::vector<struct stSetItem*>& setItemList, bool bUpdateLast);    
    void dispSettingPage(std::vector<struct stSetItem*>& setItemsList);
    void updateSetItemCurVal(std::vector<struct stSetItem*>& setItemList, const char* name, int iSetVal);
    int get_setting_select(int type);
    /************************************** 菜单相关 END *************************************************/


	/* 
	 * 网络接口
	 */
	void sendWifiConfig(sp<WifiConfig> &mConfig);
	void handleorSetWifiConfig(sp<WifiConfig> &mConfig);


	void handleGyroCalcEvent();

	/*
	 * 消息处理
	 */
	void handleKeyMsg(int iKey);				/* 按键消息处理 */
	void handleDispStrTypeMsg(sp<DISP_TYPE>& disp_type);
	void handleDispErrMsg(sp<ERR_TYPE_INFO>& mErrInfo);
	void handleLongKeyMsg(int key, int64 ts);
	void handleDispLightMsg(int menu, int state, int interval);
	void handleUpdateMid();
	
    /********************************************* 拍照部分 ****************************************************/
    void setTakePicDelay(int iDelay);
    int convCapDelay2Index(int iDelay);
    int convIndex2CapDelay(int iIndex);


    /********************************************* 按键处理 ****************************************************/
	void commUpKeyProc();
    void commDownKeyProc();
    void procBackKeyEvent();
	void procPowerKeyEvent();
	void procUpKeyEvent();
	void procDownKeyEvent();
	void procSettingKeyEvent();



	/*
	 * 显示
	 */
	void uiShowStatusbarIp();			/* 显示IP地址 */
	void procUpdateIp(const char* ipAddr);
	

	/*
	 * 存储管理器
	 */
	bool queryCurStorageState();


	sp<ARLooper> mLooper;
    sp<ARHandler> mHandler;
    std::thread th_msg_;
	
    std::thread th_sound_;



    bool bExitMsg = false;

    bool bExitUpdate = false;
    bool bExitLight = false;
    bool bExitSound = false;
    bool bExitBat = false;
    bool bSendUpdate = false;
    bool bSendUpdateMid = false;
    sp<ARMessage> mNotify;
	
    int cam_state = 0;

    int cur_menu = -1;
    int last_err = -1;

	int set_photo_delay_index = 0;
	
//option which power key react ,changed by both oled key and ws
    int cur_option = 0;

	//normally changed by up/down/power key in panel
    int org_option = 0;
    sp<pro_cfg> mProCfg;
	
	// dev_type : 1 -- usb , 2 -- sd1 (3 --sd2 if exists)
    std::vector<sp<Volume>> mSaveList;

	// int save_select;
    sp<oled_module> mOLEDModule;


    char used_space[2][8]; //for disp
    char total_space[2][8];

    sp<struct _remain_info_> mRemainInfo;
    sp<struct _rec_info_> mRecInfo;
    sp<oled_light> mOLEDLight;

    int cap_delay = 0;
	

    u8 last_light = 0;
    u8 fli_light = 0;
    u8 front_light;
	
    std::mutex mutexState;


    sp<battery_interface> mBatInterface;
    sp<struct _action_info_> mControlAct;
    sp<BAT_INFO> m_bat_info_;

	
    sp<SYS_INFO> mReadSys;
    sp<struct _ver_info_> mVerInfo;
    sp<struct _wifi_config_> mWifiConfig;

    bool bDispTop = false;
    bool bRoot = false;
    bool bAdb = false;
    int adb_times = 0;
    int adb_root_times = 0;
    int last_down_key = 0;
    int tl_count = -1;
    int aging_times = 0;
    int bat_jump_times = 0;
    int64 last_key_ts = 0;
	

	sp<NetManager> mNetManager;

    sp<dev_manager> mDevManager;

    int mTakePicDelay = 0;

	int	mGyroCalcDelay = 0;		/* 陀螺仪校正的倒计时时间(单位为S) */
	
    bool bLiveSaveOrg = false;
    int pipe_sound[2]; // 0 -- read , 1 -- write

    // during stiching and not receive stich finish
    bool bStiching = false;

    char mLocalIpAddr[32];        /* UI本地保存的IP地址 */


	/*
	 * 是否已经配置SSID
	 */
	bool	mHaveConfigSSID = false;

	/*------------------------------------------------------------------------------
	 * 存储管理部分
	 */


	u32 	mMinStorageSpce;						/* 所有存储设备中最小存储空间大小(单位为MB) */

	u32		mCanTakePicNum;							/* 可拍照的张数 */
	u32		mCanTakeVidTime;						/* 可录像的时长(秒数) */
	u32		mCanTakeLiveTime;						/* 可直播录像的时长(秒数) */

	std::mutex				mRemoteDevLock;
	std::vector<sp<Volume>> mRemoteStorageList;		/* 存储列表 */

	std::mutex				mLocaLDevLock;
	std::vector<sp<Volume>> mLocalStorageList;		/* 存储列表 */

	bool	bFirstDev = true;
	int		mSavePathIndex = -1;


	/*
	 * 菜单管理相关
	 */
	std::vector<sp<MENU_INFO>>      mMenuLists;
    std::vector<struct stSetItem*>  mSetItemsList;
    std::vector<struct stSetItem*>  mPhotoDelayList;
    std::vector<struct stSetItem*>  mAebList;
    std::vector<struct stPicVideoCfg*> mPicItemsList;
    std::vector<struct stPicVideoCfg*> mVidItemsList;   
    std::vector<struct stPicVideoCfg*> mLiveItemsList;
};

#endif //PROJECT_OLED_WRAPPER_H
