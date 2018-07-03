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
#include <hw/oled_handler.h>

#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <util/bytes_int_convert.h>
#include <sys/pro_cfg.h>
#include <hw/lan.h>
#include <hw/dev_manager.h>
#include <hw/tx_softap.h>
#include <hw/oled_module.h>
#include <util/icon_ascii.h>
#include <sys/pro_uevent.h>
#include <hw/oled_light.h>
#include <sys/action_info.h>
#include <hw/battery_interface.h>
#include <sys/net_manager.h>
#include <util/GitVersion.h>
#include <log/stlog.h>
#include <hw/oled_handler.h>
#include <system_properties.h>
#include <prop_cfg.h>

using namespace std;


#include <ui_res.h>

#define TAG "oled_handler"


class oled_arhandler : public ARHandler {
public:
    oled_arhandler(oled_handler *source): mHandler(source)
    {
    }

    virtual ~oled_arhandler() override
    {
    }

    virtual void handleMessage(const sp<ARMessage> &msg) override
    {
        mHandler->handleMessage(msg);
    }
private:
    oled_handler *mHandler;
};




/*************************************************************************
** 方法名称: oled_handler
** 方法功能: 构造函数
** 入口参数: notify - 异步消息强指针引用
** 返 回 值: 无 
**
**
*************************************************************************/
oled_handler::oled_handler(const sp<ARMessage> &notify): mNotify(notify)
{
    init_handler_thread();	/* 初始化消息处理线程 */
	
    init();					/* oled_handler内部成员初始化 */
	
    init_poll_thread();		/* 初始化网络检测线程(用于检测网络状态的变化) */
	
    send_init_disp();		/* 给消息处理线程发送初始化显示消息 */
}


oled_handler::~oled_handler()
{
    deinit();
}


/*************************************************************************
** 方法名称: send_init_disp
** 方法功能: 发送初始化显示消息
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_handler
**
*************************************************************************/
void oled_handler::send_init_disp()
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_INIT);
    msg->post();
}


int oled_handler::get_setting_select(int type)
{
    return mSettingItems.iSelect[type];
}

void oled_handler::set_setting_select(int type,int val)
{
    mSettingItems.iSelect[type] = val;
}

void oled_handler::disp_top_info()
{
    unsigned int org_ip = org_addr;
    if (mProCfg->get_val(KEY_WIFI_ON)) {
        disp_icon(ICON_WIFI_OPEN_0_0_16_16);
    } else {
        disp_icon(ICON_WIFI_CLOSE_0_0_16_16);
    }

    set_org_addr(10002);
    oled_disp_ip(org_ip);

    //disp battery icon
    oled_disp_battery();
    bDispTop = true;
}


/*************************************************************************
** 方法名称: init_cfg_select
** 方法功能: 根据配置初始化选择项
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void oled_handler::init_cfg_select()
{
    init_menu_select();
	
    mProCfg->read_wifi_cfg(mWifiConfig);
    if (mProCfg->get_val(KEY_WIFI_ON) == 1) {	/* 如果WiFi处于打开状态,开启WiFi */
        start_wifi();
    } else {	/* WiFi处于关闭状态 */
        wifi_stop();		/* 停止WiFi */
        disp_wifi(false);

        if (get_setting_select(SET_DHCP_MODE) == 0) {
            switch_dhcp_mode(0);
        }
    }
}




/*************************************************************************
** 方法名称: oled_init_disp
** 方法功能: 初始化显示
** 入口参数: 无
** 返 回 值: 无 
** 调     用: handleMessage
**
*************************************************************************/
void oled_handler::oled_init_disp()
{
    read_sn();						/* 获取系统序列号 */
    read_uuid();					/* 读取设备的UUID */
    read_ver_info();				/* 读取系统的版本信息 */ 
	
    init_cfg_select();				/* 根据配置初始化选择项 */

	//disp top before check battery 170623 for met enter low power of protect at beginning
    bDispTop = true;				/* 显示顶部标志设置为true */

	check_battery_change(true);		/* 检查电池的状态 */
}


void oled_handler::start_qr_func()
{
    send_option_to_fifo(ACTION_QR);
}


void oled_handler::exit_qr_func()
{
    send_option_to_fifo(ACTION_QR);
}


void oled_handler::send_update_light(int menu, int state, int interval, bool bLight, int sound_id)
{
//    Log.d(TAG,"send_update_light　(%d %d %d %d) ",bSendUpdate,menu,state,interval);
    if (sound_id != -1 && get_setting_select(SET_SPEAK_ON) == 1) {
        flick_light();
        play_sound(sound_id);
	
        //force to 0 ,for play sounds cost times
        interval = 0;
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



void oled_handler::write_p(int *p, int val)
{
    char c = (char)val;
    int  rc;
    rc = write_pipe(p, &c, 1);
    if (rc != 1) {
        Log.d("Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}



void oled_handler::play_sound(u32 type)
{
	Log.d(TAG, "play_sound speaker state [%d]", get_setting_select(SET_SPEAK_ON));

    if (get_setting_select(SET_SPEAK_ON) == 1)  {
        if (type >= 0 && type <= sizeof(sound_str) / sizeof(sound_str[0])) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "aplay %s", sound_str[type]);
			Log.d(TAG, "cmd is %s", cmd);
            exec_sh(cmd);
        } else {
            Log.d(TAG, "sound type %d exceed \n", type);
        }
    }
}


void oled_handler::sound_thread()
{
    while (!bExitSound) {
        if (get_setting_select(SET_SPEAK_ON) == 1) {

        }
        msg_util::sleep_ms(INTERVAL_1HZ);
    }
}


void oled_handler::init_sound_thread()
{
    th_sound_ = thread([this] { sound_thread(); });
}


void oled_handler::select_up()
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
    Log.d(TAG,"cur_menu %d select_up select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d",cur_menu,
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total);
#endif

    mSelect->select--;

#ifdef DEBUG_INPUT_KEY	
    Log.d(TAG,"select %d total %d mSelect->page_num %d",
          mSelect->select,mSelect->total,mSelect->page_num);
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
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total);
#endif

    if (bUpdatePage) {
        update_menu_page();
    } else {
        update_menu();
    }
}



void oled_handler::select_down()
{
    bool bUpdatePage = false;
    SELECT_INFO * mSelect = get_select_info();
    CHECK_NE(mSelect, nullptr);

#ifdef DEBUG_INPUT_KEY	
	Log.d(TAG," select_down select %d mSelect->last_select %d "
                  "mSelect->page_num %d mSelect->cur_page %d "
                  "mSelect->page_max %d mSelect->total %d",
          mSelect->select,mSelect->last_select,mSelect->page_num,
          mSelect->cur_page,mSelect->page_max,mSelect->total);
#endif

    mSelect->last_select = mSelect->select;
    mSelect->select++;
    if (mSelect->select + ( mSelect->cur_page * mSelect->page_max )>= mSelect->total) {
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
void oled_handler::init_handler_thread()
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

void oled_handler::set_lan(int lan)
{
    mProCfg->set_val(KEY_LAN, lan);
    update_disp_func(lan);
}

void oled_handler::update_disp_func(int lan)
{
}

int oled_handler::get_menu_select_by_power(int menu)
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
void oled_handler::set_menu_select_info(int menu, int iSelect)
{
    mMenuInfos[menu].mSelectInfo.cur_page = iSelect / mMenuInfos[menu].mSelectInfo.page_max;
    mMenuInfos[menu].mSelectInfo.select = iSelect % mMenuInfos[menu].mSelectInfo.page_max;
}



/*************************************************************************
** 方法名称: init_menu_select
** 方法功能: 根据配置初始化选择项
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void oled_handler::init_menu_select()
{
    int val;

    for (int i = MENU_PIC_SET_DEF; i <= MENU_LIVE_SET_DEF; i++) {
        switch (i) {
            case MENU_PIC_SET_DEF:	/* 为菜单MENU_PIC_SET_DEF设置拍照的默认配置 */
                val = mProCfg->get_val(KEY_PIC_DEF);
		
				Log.d(TAG, "pic set def %d", val);

				switch (val) {
                    case PIC_CUSTOM:

					#ifdef ENABLE_8K_3D_SAVE 
					case PIC_8K_3D_SAVE:
					#endif

					case PIC_8K_PANO_SAVE:
                    case PIC_8K_3D_SAVE_OF:
                        
                    case PIC_8K_PANO_SAVE_OF:	// optical flow stich

#ifdef OPEN_HDR_RTS
                    case PIC_HDR_RTS:
#endif
                    case PIC_HDR:
                    case PIC_BURST:
                    case PIC_RAW:
                        set_menu_select_info(i, val);
                        break;
					
					//user define
                    SWITCH_DEF_ERROR(val)
                }
                break;
				
            case MENU_VIDEO_SET_DEF:
                val = mProCfg->get_val(KEY_VIDEO_DEF);
                switch (val) {
                    case VID_CUSTOM:
                    case VID_8K_50M_30FPS_PANO_RTS_OFF:
                    case VID_6K_50M_30FPS_3D_RTS_OFF:
                    case VID_4K_50M_120FPS_PANO_RTS_OFF:
                    case VID_4K_20M_60FPS_3D_RTS_OFF:
                    case VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON:
                    case VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON:

				#ifndef ARIAL_LIVE
                    case VID_ARIAL:
				#endif
                        set_menu_select_info(i,val);
                        break;
						
                    SWITCH_DEF_ERROR(val)
                }
                break;
				
            case MENU_LIVE_SET_DEF:
                val = mProCfg->get_val(KEY_LIVE_DEF);
                switch (val) {
                    case LIVE_CUSTOM:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:

				#ifdef ARIAL_LIVE
                    case VID_ARIAL:
				#endif
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

    for (int i = 0; i < SETTING_MAX; i++) {
        switch (i) {
            case SET_WIFI_AP:
                val = mProCfg->get_val(KEY_WIFI_AP);
                set_setting_select(i, val);
                break;

            case SET_DHCP_MODE:
                val = mProCfg->get_val(KEY_DHCP);
                set_setting_select(i, val);
                break;

            case SET_FREQ:
                val = mProCfg->get_val(KEY_PAL_NTSC);
                set_setting_select(i, val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);
                break;

            case SET_SPEAK_ON:
                val = mProCfg->get_val(KEY_SPEAKER);
                set_setting_select(i, val);
                break;

            case SET_BOTTOM_LOGO:
                val = mProCfg->get_val(KEY_SET_LOGO);
                set_setting_select(i, val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);
                break;

            case SET_LIGHT_ON:	/* 开机时根据配置,来决定是否开机后关闭前灯 */
                val = mProCfg->get_val(KEY_LIGHT_ON);
                set_setting_select(i, val);

                //off the light 170802
                if (val == 0) {
                    set_light_direct(LIGHT_OFF);
                }
                break;
				
            case SET_AUD_ON:
                val = mProCfg->get_val(KEY_AUD_ON);
                set_setting_select(i, val);
                break;

            case SET_SPATIAL_AUD:
                val = mProCfg->get_val(KEY_AUD_SPATIAL);
                set_setting_select(i, val);
                break;

            case SET_GYRO_ON:
                val = mProCfg->get_val(KEY_GYRO_ON);
                set_setting_select(i, val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);
                break;

            case SET_FAN_ON:
                val = mProCfg->get_val(KEY_FAN);
                set_setting_select(i, val);
                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_FAN);
                break;

            case SET_STORAGE:
            case SET_RESTORE:
            case SET_INFO:
            case SET_START_GYRO:
            case SET_NOISE:
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
void oled_handler::init()
{
    Log.d(TAG, "version:%s\n", GIT_SHA1);
    CHECK_EQ(sizeof(mMenuInfos) / sizeof(mMenuInfos[0]), MENU_MAX);
    CHECK_EQ(mControlAct, nullptr);
    CHECK_EQ(sizeof(mPICAction) / sizeof(mPICAction[0]), PIC_CUSTOM);
    CHECK_EQ(sizeof(mVIDAction) / sizeof(mVIDAction[0]), VID_CUSTOM);
    CHECK_EQ(sizeof(mLiveAction) / sizeof(mLiveAction[0]), LIVE_CUSTOM);
    CHECK_EQ(sizeof(astSysRead) / sizeof(astSysRead[0]), SYS_KEY_MAX);

    CHECK_EQ(sizeof(setting_icon_normal) / sizeof(setting_icon_normal[0]), SETTING_MAX);
    CHECK_EQ(sizeof(setting_icon_lights) / sizeof(setting_icon_lights[0]), SETTING_MAX);

    mSaveList.clear();
    cam_state = STATE_IDLE;
    mOLEDModule = sp<oled_module>(new oled_module());
    CHECK_NE(mOLEDModule, nullptr);

    //move ahead avoiding scan dev finished before oled disp
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

    //init_pipe(mCtrlPipe);
	
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

    mWifiConfig = sp<WIFI_CONFIG>(new WIFI_CONFIG());
    CHECK_NE(mWifiConfig, nullptr);
	
    memset(mWifiConfig.get(), 0, sizeof(WIFI_CONFIG));
    mpNetManager = sp<net_manager>(new net_manager());
    CHECK_NE(mpNetManager, nullptr);

}


/*
 * 网络状态管理:
 * 启动一个秒钟检测一次的监听线程
 */


void oled_handler::check_net_status()
{
    sp<net_dev_info> mpDevInfo = sp<net_dev_info>(new net_dev_info());

	/* 测试发现,IP地址及网络状态没有发生变化,但是不停的在发消息 */
    if (mpNetManager->check_net_change(mpDevInfo)) {
		Log.d(TAG, "net device status changed, new ip: %d", mpDevInfo->dev_addr);
        send_disp_ip((int) mpDevInfo->dev_addr, mpDevInfo->dev_type);
    }
}


sp<ARMessage> oled_handler::obtainMessage(uint32_t what)
{
    return mHandler->obtainMessage(what);
}

void oled_handler::start_poll_thread()
{
    while (!bExitPoll) {
        check_net_status();
        msg_util::sleep_ms(1000);
    }
}

void oled_handler::stop_poll_thread()
{
    Log.d(TAG, "stop_poll_thread  %d", bExitPoll);
    if (!bExitPoll) {
        bExitPoll = true;
        if (th_poll_.joinable()) {
            th_poll_.join();
        } else {
            Log.e(TAG, " th_poll_ not joinable ");
        }
    }
    Log.d(TAG, "stop_poll_thread  %d over",bExitPoll);
}



/*************************************************************************
** 方法名称: init_poll_thread
** 方法功能: 创建网络状态检测线程
** 入口参数: 无
** 返 回 值: 无 
**
**
*************************************************************************/
void oled_handler::init_poll_thread()
{
    th_poll_ = thread([this]
                     {
                         start_poll_thread();
                     });
}


void oled_handler::stop_bat_thread()
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

void oled_handler::sendExit()
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


void oled_handler::send_disp_str(sp<DISP_TYPE> &sp_disp)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_STR_TYPE);
    msg->set<sp<DISP_TYPE>>("disp_type", sp_disp);
    msg->post();
}

void oled_handler::send_disp_err(sp<struct _err_type_info_> &mErrInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_ERR_TYPE);
    msg->set<sp<ERR_TYPE_INFO>>("err_type_info", mErrInfo);
    msg->post();
}

void oled_handler::send_sync_init_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_SYNC_INIT_INFO);
    msg->set<sp<SYNC_INIT_INFO>>("sync_info", mSyncInfo);
    msg->post();
}

void oled_handler::send_get_key(int key)
{
    sp<ARMessage> msg = obtainMessage(OLED_GET_KEY);
    msg->set<int>("oled_key", key);
    msg->post();
}

void oled_handler::send_long_press_key(int key,int64 ts)
{
    sp<ARMessage> msg = obtainMessage(OLED_GET_LONG_PRESS_KEY);
//    Log.d(TAG, "last_key_ts is %lld last_down_key %d",
//         ts, key);
    msg->set<int>("key", key);
    msg->set<int64>("ts", ts);
    msg->postWithDelayMs(LONG_PRESS_MSEC);
}

void oled_handler::send_disp_ip(int ip, int net_type)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_IP);
    msg->set<int>("ip", ip);
    msg->post();
}

void oled_handler::send_disp_battery(int battery, bool charge)
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_BATTERY);
    msg->set<int>("battery", battery);
    msg->set<bool>("charge", charge);
    msg->post();
}

void oled_handler::send_wifi_config(sp<WIFI_CONFIG> &mConfig)
{
    sp<ARMessage> msg = obtainMessage(OLED_CONFIG_WIFI);
    msg->set<sp<WIFI_CONFIG>>("wifi_config",mConfig);
    msg->post();
}

void oled_handler::send_sys_info(sp<SYS_INFO> &mSysInfo)
{
    sp<ARMessage> msg = obtainMessage(OLED_SET_SN);
    msg->set<sp<SYS_INFO>>("sys_info",mSysInfo);
    msg->post();
}

void oled_handler::send_update_mid_msg(int interval)
{
    if (!bSendUpdateMid) {
        bSendUpdateMid = true;
        send_delay_msg(OLED_UPDATE_MID, interval);
    } else {
        Log.d(TAG,"set_update_mid true (%d 0x%x)",cur_menu,cam_state);
    }
}

void oled_handler::set_update_mid()
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

int oled_handler::oled_reset_disp(int type)
{
    //reset to original
//    reset_last_info();
    INFO_MENU_STATE(cur_menu,cam_state)
    cam_state = STATE_IDLE;
//keep sys error back to menu top
    disp_sys_err(type,MENU_TOP);
    //fix select if working by controller 0616
    mControlAct = nullptr;
//    init_menu_select();
    //reset wifi config
    return 0;
}


void oled_handler::set_cur_menu_from_exit()
{
    int old_menu = cur_menu;
    int new_menu = get_back_menu(cur_menu);
    CHECK_NE(cur_menu,new_menu);

    Log.d(TAG, "cur_menu is %d new_menu %d\n", cur_menu, new_menu);
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
            Log.e(TAG, "func back to sys err\n");
            break;
		
        case MENU_CALIBRATION:
            Log.d(TAG, "func back from other to calibration\n");
            break;
	
	case MENU_TOP:
	    if (old_menu == MENU_SYS_SETTING)
	    {
	        Log.d(TAG, ">>>>>>>>>>>>>> back from setting");
	    }
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
void oled_handler::set_cur_menu(int menu, int back_menu)
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



void oled_handler::restore_all()
{
    add_state(STATE_RESTORE_ALL);
}

void oled_handler::update_sys_info()
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



void oled_handler::disp_sys_info()
{
    int col = 0; //25

    //force read every time
    read_sn();
    clear_area(0, 16);
    char buf[32];

    if (strlen(mReadSys->sn) <= SN_LEN)
	{
        snprintf(buf, sizeof(buf), "SN:%s", mReadSys->sn);
    } 
	else 
	{
        snprintf(buf, sizeof(buf), "SN:%s", (char *)&mReadSys->sn[strlen(mReadSys->sn) - SN_LEN]);
    }
	
    disp_str((const u8 *)buf, col, 16);
    disp_str((const u8 *)mVerInfo->r_v_str, col, 32);
}



/*
 * disp_msg_box - 显示消息框
 */
void oled_handler::disp_msg_box(int type)
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

		case DISP_LIVE_REC_USB:
            disp_icon(ICON_LIVE_REC_USB_128_64128_64);
            break;

		SWITCH_DEF_ERROR(type);
    }
}


bool oled_handler::check_allow_pic()
{
    bool bRet = false;
//    Log.d(TAG,"check_allow_pic cam_state is 0x%x\n", cam_state);
    if (check_state_preview() || check_state_equal(STATE_IDLE)) 
	{
        bRet = true;
    }
	else 
	{
        Log.e(TAG, " check_allow_pic error state 0x%x ", cam_state);
    }
    return bRet;
}

bool oled_handler::start_speed_test()
{
    bool ret = false;
    if (!check_dev_speed_good(mSaveList.at(save_path_select)->path)) {
        set_cur_menu(MENU_SPEED_TEST);
        ret = true;
    }
    return ret;
}

void oled_handler::fix_live_save_per_act(struct _action_info_ *mAct)
{
    if (mAct->stOrgInfo.save_org != SAVE_OFF) {
        mAct->size_per_act = (mAct->stOrgInfo.stOrgAct.mOrgV.org_br * 6) / 10;
    }
	
   	if (mAct->stStiInfo.stStiAct.mStiL.file_save) {
       mAct->size_per_act += mAct->stStiInfo.stStiAct.mStiV.sti_br / 10;
	}
}

bool oled_handler::check_live_save(ACTION_INFO *mAct)
{
    bool ret = false;
    if (mAct->stOrgInfo.save_org != SAVE_OFF ||
            mAct->stStiInfo.stStiAct.mStiL.file_save ) {
        ret = true;
    }
			
    Log.d(TAG, "check_live_save is %d %d ret %d",
          mAct->stOrgInfo.save_org ,
          mAct->stStiInfo.stStiAct.mStiL.file_save,
          ret);
    return ret;
}

bool oled_handler::start_live_rec(const struct _action_info_ *mAct,ACTION_INFO *dest)
{
    bool bAllow = false;
	
    if (!check_save_path_none()) {
		
        if (check_save_path_usb()) {

		#if 0	// skyway for test
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
                            int size_M = (int)(mSaveList.at(save_path_select)->avail >> 20);
                            if(size_M > AVAIL_SUBSTRACT)
                            {
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

			#endif
        }
    }

    return bAllow;
}


//cmd -- for power off or set option
bool oled_handler::send_option_to_fifo(int option,int cmd,struct _cam_prop_ * pstProp)
{
    bool bAllow = true;
    sp<ARMessage> msg = mNotify->dup();
    sp<ACTION_INFO> mActionInfo = sp<ACTION_INFO>(new ACTION_INFO());
    int item = 0;
	char tmp_buf[4096] = {0};

    switch (option) {
        case ACTION_PIC:
            if (check_dev_exist(option)) {
                // only happen in preview in oled panel, taking pic in live or rec only actvied by controller client
                if (check_allow_pic()) {
//                    if(cap_delay == 0)
//                    {
//                        set_cap_delay(CAL_DELAY);
//                    }
                    oled_disp_type(CAPTURE);
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
//                    Log.d(TAG," pic action item is %d",item);
                    switch(item)
                    {
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
                            memcpy(mActionInfo.get(),
                                   mProCfg->get_def_info(KEY_PIC_DEF),
                                   sizeof(ACTION_INFO));
                            break;
                        SWITCH_DEF_ERROR(item);
                    }
                    msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
                }
                else
                {
                    bAllow = false;
                }
            }
            else
            {
                bAllow = false;
            }
            break;
			
        case ACTION_VIDEO:
            if( check_state_in(STATE_RECORD) && (!check_state_in(STATE_STOP_RECORDING)))
            {
//                Log.d(TAG,"stop rec ");
                oled_disp_type(STOP_RECING);
            }
            else if (check_state_preview())
            {
                if(check_dev_exist(option))
                {
                    if(start_speed_test())
                    {
                        bAllow = false;
                    }
                    else
                    {
                        oled_disp_type(START_RECING);
                        item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
//                        Log.d(TAG, " vid set is %d ", item);
                        switch (item)
                        {
                            case VID_8K_50M_30FPS_PANO_RTS_OFF:
                            case VID_6K_50M_30FPS_3D_RTS_OFF:
                            case VID_4K_50M_120FPS_PANO_RTS_OFF:
                            case VID_4K_20M_60FPS_3D_RTS_OFF:
                            case VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON:
                            case VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON:
#ifndef ARIAL_LIVE
                                case VID_ARIAL:
#endif
                                memcpy(mActionInfo.get(), &mVIDAction[item], sizeof(ACTION_INFO));
                                break;
                            case VID_CUSTOM:
                                //force set audio gain
                                send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD_GAIN, &mProCfg->get_def_info(KEY_VIDEO_DEF)->stProp);
                                memcpy(mActionInfo.get(), mProCfg->get_def_info(KEY_VIDEO_DEF), sizeof(ACTION_INFO));
                                break;
                            SWITCH_DEF_ERROR(item);
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
            if((check_state_in(STATE_LIVE) || check_state_in(STATE_LIVE_CONNECTING)) && (!check_state_in(STATE_STOP_LIVING)))
            {
                oled_disp_type(STOP_LIVING);
            }
            else if (check_state_preview())
            {
                item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
//                Log.d(TAG,"live item is %d",
//                      item);
                switch(item)
                {
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                    case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                    case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
                        oled_disp_type(STRAT_LIVING);
                        memcpy(mActionInfo.get(), &mLiveAction[item], sizeof(ACTION_INFO));
                        break;
#ifdef ARIAL_LIVE
                    case VID_ARIAL:
                        bAllow = start_live_rec(&mLiveAction[item],mActionInfo.get());
#endif
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
                            send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD_GAIN, &mProCfg->get_def_info(KEY_LIVE_DEF)->stProp);
                        }
                        break;
                    SWITCH_DEF_ERROR(item);
                }
                msg->set<sp<ACTION_INFO>>("action_info", mActionInfo);
            }
            else
            {
                Log.w(TAG," act live bad state 0x%x",cam_state);
                bAllow = false;
            }
            break;
			
        case ACTION_PREVIEW:
            if(check_state_preview() || check_state_equal(STATE_IDLE))
            {
//                bAllow = true;
            }
            else
            {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;
        case ACTION_CALIBRATION:
            if((cam_state&STATE_CALIBRATING) != STATE_CALIBRATING)
            {
                set_cap_delay(CAL_DELAY);
                oled_disp_type(START_CALIBRATIONING);
            }
            else
            {
                Log.e(TAG,"calibration happen cam_state 0x%x",cam_state);
                bAllow = false;
            }
            break;
			
        case ACTION_QR:
//            Log.d(TAG,"action qr state 0x%x", cam_state);
            if((check_state_equal(STATE_IDLE) || check_state_preview()))
            {
                oled_disp_type(START_QRING);
            }
            else if(check_state_in(STATE_START_QR) && !check_state_in(STATE_STOP_QRING))
            {
                oled_disp_type(STOP_QRING);
            }
            else
            {
                bAllow = false;
            }
            break;
			
        case ACTION_REQ_SYNC:
        {
            sp<REQ_SYNC> mReqSync = sp<REQ_SYNC>(new REQ_SYNC());
            snprintf(mReqSync->sn,sizeof(mReqSync->sn),"%s",mReadSys->sn);
            snprintf(mReqSync->r_v,sizeof(mReqSync->r_v),"%s",mVerInfo->r_ver);
            snprintf(mReqSync->p_v,sizeof(mReqSync->p_v),"%s",mVerInfo->p_ver);
            snprintf(mReqSync->k_v,sizeof(mReqSync->k_v),"%s",mVerInfo->k_ver);
            msg->set<sp<REQ_SYNC>>("req_sync", mReqSync);
        }
            break;
		
        case ACTION_LOW_BAT:
            msg->set<int>("cmd", cmd);
            break;
//        case ACTION_LOW_PROTECT:
//            break;

        case ACTION_SPEED_TEST:
            if(check_save_path_none())
            {
                Log.e(TAG,"action speed test save_path_select -1");
                bAllow = false;
            }
            else
            {
                if(!check_state_in(STATE_SPEED_TEST))
                {
                    msg->set<char *>("path", mSaveList.at(save_path_select)->path);
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
            if (check_state_in(STATE_IDLE))
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
            if (check_state_in(STATE_IDLE)) {
                oled_disp_type(START_RECING);
                set_cur_menu(MENU_AGEING);
            } else {
                ERR_MENU_STATE(cur_menu, cam_state);
                bAllow = false;
            }
            break;
			
        case ACTION_SET_OPTION:
            msg->set<int>("type", cmd);
            switch (cmd) {
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
                case OPTION_SET_AUD_GAIN:
                    Log.d(TAG,"set aud gain %d",pstProp->audio_gain);
                    msg->set<int>("aud_gain", pstProp->audio_gain);
                    break;
                SWITCH_DEF_ERROR(cmd);
            }
            break;
			
        case ACTION_POWER_OFF:
            if (!check_state_in(STATE_POWER_OFF)) {
                add_state(STATE_POWER_OFF);
                clear_area();
                set_light_direct(LIGHT_OFF);
                msg->set<int>("cmd", cmd);
            } else {
                ERR_MENU_STATE(cur_menu,cam_state);
                bAllow = false;
            }
            break;

	case ACTION_AWB:		
		set_light_direct(FRONT_GREEN);
		break;


        SWITCH_DEF_ERROR(option)
    }

    Log.d(TAG, "oled option %d bAllow %d", option, bAllow);
    if (bAllow) {
		if (option == ACTION_AGEING) {
			msg->set<int>("what", FACTORY_AGEING);
		} else {
			msg->set<int>("what", OLED_KEY);
		}
        msg->set<int>("action", option);
        msg->post();
    }
    return bAllow;
}



/*
 * send_save_path_change - 设备的存储路径发生改变
 */
void oled_handler::send_save_path_change()
{
//    Log.d(TAG,"new save path %s cur_menu %d",new_path,cur_menu);
    switch (cur_menu) {
        case MENU_PIC_INFO:
        case MENU_VIDEO_INFO:
        case MENU_PIC_SET_DEF:
        case MENU_VIDEO_SET_DEF:
            // make update bottom before sent it to ws 20170710
            if (!check_cam_busy()) {	//not statfs while busy ,add 20170804
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
        snprintf(mSavePath->path,sizeof(mSavePath->path), "%s", mSaveList.at(save_path_select)->path);
    } else {
        snprintf(mSavePath->path,sizeof(mSavePath->path), "%s", NONE_PATH);
    }
	
    msg->set<sp<SAVE_PATH>>("save_path", mSavePath);
    msg->post();

}


void oled_handler::send_delay_msg(int msg_id, int delay)
{
    sp<ARMessage> msg = obtainMessage(msg_id);
    msg->postWithDelayMs(delay);
}

void oled_handler::send_bat_low()
{
    sp<ARMessage> msg = obtainMessage(OLED_DISP_BAT_LOW);
    msg->post();
}

void oled_handler::send_read_bat()
{
    sp<ARMessage> msg = obtainMessage(OLED_READ_BAT);
    msg->post();
}

void oled_handler::send_clear_msg_box(int delay)
{
    sp<ARMessage> msg = obtainMessage(OLED_CLEAR_MSG_BOX);
    msg->postWithDelayMs(delay);
}


void oled_handler::send_update_dev_list(std::vector<sp<USB_DEV_INFO>> &mList)
{
    sp<ARMessage> msg = mNotify->dup();

    //update battery in oled
    msg->set<int>("what", UPDATE_DEV);
    msg->set<std::vector<sp<USB_DEV_INFO>>>("dev_list", mList);
    msg->post();
}

const u8* oled_handler::get_disp_str(int str_index)
{
    const u8 *str = mProCfg->get_str(str_index);
    Log.d(TAG,"333 get disp str[%d] %s\n",str_index,str);
    return str;
}
//sta
void oled_handler::wifi_config(sp<WIFI_CONFIG> &config)
{
    if (strlen(config->ssid) > 0 /*&& strlen(config->pwd) > 0*/) {
//        Log.d(TAG,"wifi config %s %s cam_state 0x%x cur_menu %d",
//              config->ssid,config->pwd,cam_state,cur_menu);
        oled_disp_type(QR_FINISH_CORRECT);
        memcpy(mWifiConfig.get(), config.get(), sizeof(WIFI_CONFIG));
        mProCfg->update_wifi_cfg(mWifiConfig);

		// force wifi on
        mProCfg->set_val(KEY_WIFI_ON,1);
        start_wifi_sta(1);
    } else {
        oled_disp_type(QR_FINISH_ERROR);
    }
}



/*************************************************************************
** 方法名称: read_ver_info
** 方法功能: 读取系统版本信息
** 入口参数: 无
** 返 回 值: 无 
** 调     用: oled_init_disp
**
*************************************************************************/
void oled_handler::read_ver_info()
{
    char file_name[64];

	/* 读取系统的版本文件:  */
    if (check_path_exist(VER_FULL_PATH)) 
	{
        snprintf(file_name, sizeof(file_name), "%s", VER_FULL_PATH);
    }  
	else 
	{
        memset(file_name, 0, sizeof(file_name));
    }

    if (strlen(file_name)  > 0) 
	{
        int fd = open(file_name, O_RDONLY);
        CHECK_NE(fd, -1);

        char buf[1024];
        if (read_line(fd, (void *) buf, sizeof(buf)) > 0) 
		{
//            printf("ver len %lu mVerInfo->r_ver %s "
//                           "buf %s "
//                           "strlen buf %lu\n",
//                   strlen(mVerInfo->r_ver),
//                   mVerInfo->r_ver,buf,
//                   strlen(buf));
            snprintf(mVerInfo->r_ver, sizeof(mVerInfo->r_ver), "%s", buf);
        }
		else 
		{
            snprintf(mVerInfo->r_ver,sizeof(mVerInfo->r_ver), "%s", "999");
        }
        close(fd);
    }
	else 
	{
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
void oled_handler::read_sn()
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
void oled_handler::read_uuid()
{
    read_sys_info(SYS_KEY_UUID);
}


bool oled_handler::read_sys_info(int type, const char *name)
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
bool oled_handler::read_sys_info(int type)
{
    char cmd[1024];
    bool bFound = false;

#if 0
    snprintf(cmd, sizeof(cmd), "factool get %s > %s", astSysRead[type].key, SYS_TMP);
	Log.d(TAG, "cmd is %s", cmd);
	
    if (exec_sh(cmd) == 0) {
        bFound = read_sys_info(type, SYS_TMP);
    } else  {
        Log.e(TAG,"error cmd %s\n",cmd);
    }
#endif

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
void oled_handler::set_sync_info(sp<SYNC_INIT_INFO> &mSyncInfo)
{
    int state = mSyncInfo->state;

    snprintf(mVerInfo->a12_ver, sizeof(mVerInfo->a12_ver), "%s", mSyncInfo->a_v);
    snprintf(mVerInfo->c_ver, sizeof(mVerInfo->c_ver), "%s", mSyncInfo->c_v);
    snprintf(mVerInfo->h_ver, sizeof(mVerInfo->h_ver), "%s", mSyncInfo->h_v);

	Log.d(TAG, "sync state 0x%x va:%s vc %s vh %s",
			mSyncInfo->state,mSyncInfo->a_v,
			mSyncInfo->c_v,mSyncInfo->h_v);

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
    else if ((state& STATE_LIVE) == STATE_LIVE)
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
    else if ((state& STATE_LIVE_CONNECTING) == STATE_LIVE_CONNECTING)
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
    else if ((state& STATE_CALIBRATING) == STATE_CALIBRATING)
    {
        oled_disp_type(START_CALIBRATIONING);
    }
    else if ( (state& STATE_PREVIEW) == STATE_PREVIEW)
    {
        oled_disp_type(START_PREVIEW_SUC);
    }
	
}

void oled_handler::write_sys_info(sp<SYS_INFO> &mSysInfo)
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

void oled_handler::set_org_addr(unsigned int addr)
{
//    Log.d(TAG," set_org_addr 0x%x %d", addr,addr);
    org_addr = addr;
}

bool oled_handler::start_wifi_sta(int disp_main)
{
    bool ret = false;

    Log.d(TAG,"start_wifi_sta (%d %d)",
          mProCfg->get_val(KEY_WIFI_ON),
          mProCfg->get_val(KEY_WIFI_AP));

    if (mProCfg->get_val(KEY_WIFI_ON) == 1 && mProCfg->get_val(KEY_WIFI_AP) == 0)
    {
        Log.d(TAG, "wifi ssid %s pwd %s "
                      "disp_main %d cur_menu %d",
              mWifiConfig->ssid,
              mWifiConfig->pwd,
              disp_main,
              cur_menu);
        if (strlen(mWifiConfig->ssid) > 0 /*&& strlen(mWifiConfig->pwd) > 0*/)
        {
            wifi_stop();
            if (disp_main != -1)
            {
                set_cur_menu(MENU_WIFI_CONNECT);
                if(strlen(mWifiConfig->pwd) == 0)
                {
                    Log.d(TAG,"connect no pwd %s", mWifiConfig->ssid);
                    //tx_softsta_start(mWifiConfig->ssid,0, 7);
                }
                else
                {
                    //tx_softsta_start(mWifiConfig->ssid, mWifiConfig->pwd, 7);
                }
//            Log.d(TAG,"tx_softsta_start is %d", iRet);
                func_back();
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


void oled_handler::tx_softap_config_def()
{
    char pwd[128];

//    Log.d(TAG,"S is %s",mReadSys->sn);
    if (strlen(mReadSys->sn) < 6) {
        Log.e(TAG,"sn %s less than 6",mReadSys->sn);
        snprintf(mReadSys->ssid,sizeof(mReadSys->ssid),"Insta360-Pro-%s","654321");
    } else {
        snprintf(mReadSys->ssid,sizeof(mReadSys->ssid),"Insta360-Pro-%s",(char *)&mReadSys->sn[strlen(mReadSys->sn) - 6]);
    }
	
    snprintf(pwd,sizeof(pwd),"%s","88888888");
    int ret =  -1 ; //tx_softap_config(mReadSys->ssid,pwd,0);
//    Log.d(TAG,"config ap %s %s ret %d",mReadSys->ssid,pwd,ret);

    if (ret != 0) {
        Log.e(TAG,"config ap ret %d",ret);
    }
}


int oled_handler::start_wifi_ap(int disp_main)
{
    wifi_stop();
    //tx_softap_start();
    disp_wifi(true,disp_main);
    return 0;
}

void oled_handler::start_wifi(int disp_main)
{
    Log.d(TAG, "start_wifi (%d %d %d) ",
          org_addr,
          get_setting_select(SET_WIFI_AP),cur_menu);

    if (org_addr != 10000) {
        oled_disp_ip(0);
        set_org_addr(10000);
        if (get_setting_select(SET_WIFI_AP) == 1)
        {
            start_wifi_ap(disp_main);
        }
        else
        {
            if (!start_wifi_sta(disp_main))
            {
                if (cur_menu != -1)
                {
                    start_qr_func();
                    //even not connect
                    disp_wifi(true,-1);
                }
                else
                {
                    Log.e(TAG,"not start qr while first init force");
                    //force close 170623
                    disp_wifi(false, 0);
                }
            }
        }
    }
}



int oled_handler::wifi_stop()
{
    int ret = -1;
#if 0
//    Log.w(TAG,"b wifi  mode val %d tx_wifi_status %d",
//          get_setting_select(SET_WIFI_AP),tx_wifi_status());
    oled_disp_ip(0);
    if(tx_softsta_stop() != 0)
    {
        Log.e(TAG, " stop sta fail");
    }
    else
    {
//        Log.e(TAG, " stop sta suc");
        ret = 0;
    }
    if (tx_softap_stop() != 0)
    {
        Log.e(TAG, "stop ap fail");
    }
    else
    {
//        Log.e(TAG, "stop ap suc");
        ret = 0;
    }
#endif  
  msg_util::sleep_ms(10);
    return ret;
}

void oled_handler::disp_wifi(bool bState, int disp_main)
{
    Log.d(TAG,"disp wifi bState %d disp_main %d",
          bState,disp_main);
    if (bState)
    {
        set_mainmenu_item(MAINMENU_WIFI, 1);
        if (check_allow_update_top())
        {
            disp_icon(ICON_WIFI_OPEN_0_0_16_16);
        }
		
        //force wifi on
        mProCfg->set_val(KEY_WIFI_ON,1);
        if (cur_menu == MENU_TOP)
        {
            switch (disp_main)
            {
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
    }
    else
    {
        set_mainmenu_item(MAINMENU_WIFI, 0);
        if (check_allow_update_top())
        {
            disp_icon(ICON_WIFI_CLOSE_0_0_16_16);
        }
		
        //force wifi off
        mProCfg->set_val(KEY_WIFI_ON,0);
        if (cur_menu == MENU_TOP)
        {
            switch (disp_main)
            {
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


void oled_handler::wifi_action()
{
    Log.d(TAG," wifi on %d", mProCfg->get_val(KEY_WIFI_ON));
    if (mProCfg->get_val(KEY_WIFI_ON) == 1)
    {
        mProCfg->set_val(KEY_WIFI_ON,0);
        wifi_stop();
        disp_wifi(false,1);
    }
    else
    {
        mProCfg->set_val(KEY_WIFI_ON,1);
        start_wifi(1);
    }
}


int oled_handler::get_back_menu(int item)
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

void oled_handler::set_back_menu(int item,int menu)
{
    if (menu == -1)
    {
        if (cur_menu == -1)
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

void oled_handler::reset_last_info()
{
    tl_count = -1;
}

void oled_handler::func_back()
{
    INFO_MENU_STATE(cur_menu, cam_state)
    if (cur_menu == MENU_SYS_ERR ||
            cur_menu == MENU_DISP_MSG_BOX ||
            cur_menu == MENU_LOW_BAT ||
            cur_menu == MENU_LIVE_REC_TIME/* || cur_menu == MENU_LOW_PROTECT*/)
    {
        set_cur_menu_from_exit();
    }
    else
    {
        switch (cam_state)
        {
            //force back directly while state idle 17.06.08
            case STATE_IDLE:
                set_cur_menu_from_exit();
                break;

            case STATE_PREVIEW:
                switch (cur_menu)
                {
                    case MENU_PIC_INFO:
                    case MENU_VIDEO_INFO:
                    case MENU_LIVE_INFO:
                        if (send_option_to_fifo(ACTION_PREVIEW))
                        {
                            oled_disp_type(STOP_PREVIEWING);
                        }
                        else
                        {
                            Log.e(TAG,"stop preview fail");
                        }
                        break;

                     //preview state sent from http req while calibrating ,qr scan,gyro_start
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
                switch (cur_menu)
                {
                    case MENU_QR_SCAN:
                        exit_qr_func();
                        break;
                    default:
                        Log.d(TAG, "strange enter (%d 0x%x)",
                              cur_menu, cam_state);
                        if (check_state_in(STATE_LIVE))
                        {
                            if (cur_menu != MENU_LIVE_INFO)
                            {
                                set_cur_menu(MENU_LIVE_INFO);
                            }
                        }
                        else if (check_state_in(STATE_RECORD))
                        {
                            if (cur_menu != MENU_VIDEO_INFO)
                            {
                                set_cur_menu(MENU_VIDEO_INFO);
                            }
                        }
                        else if (check_state_in(STATE_CALIBRATING))
                        {
                            if (cur_menu != MENU_CALIBRATION)
                            {
                                set_cur_menu(MENU_CALIBRATION);
                            }
                        }
                        else if (check_state_in(STATE_SPEED_TEST))
                        {
                            if (cur_menu != MENU_SPEED_TEST)
                            {
                                set_cur_menu(MENU_SPEED_TEST);
                            }
                        }
                        else if (check_state_in(STATE_START_GYRO))
                        {
                            if (cur_menu != MENU_GYRO_START)
                            {
                                set_cur_menu(MENU_GYRO_START);
                            }
                        }
                        else if (check_state_in(STATE_NOISE_SAMPLE))
                        {
                            if (cur_menu != MENU_NOSIE_SAMPLE)
                            {
                                set_cur_menu(MENU_NOSIE_SAMPLE);
                            }
                        }
                        else
                        {
                            ERR_MENU_STATE(cur_menu,cam_state)
                        }
                        break;
                }
                break;
        }
    }
}

void oled_handler::disp_scroll()
{

}


void oled_handler::update_menu_disp(const int *icon_light,const int *icon_normal)
{
    SELECT_INFO * mSelect = get_select_info();
    int last_select = get_last_select();
	
//    Log.d(TAG,"cur_menu %d last_select %d get_select() %d "
//                  " mSelect->cur_page %d mSelect->page_max %d total %d",
//          cur_menu,last_select, get_select(), mSelect->cur_page ,
//          mSelect->page_max,mSelect->total);
    if (icon_normal != nullptr) {
        //sometimes force last_select to -1 to avoid update last select
        if (last_select != -1) {
            disp_icon(icon_normal[last_select + mSelect->cur_page * mSelect->page_max]);
        }
    }
    disp_icon(icon_light[get_menu_select_by_power(cur_menu)]);
}

void oled_handler::update_menu_page()
{
    CHECK_EQ(cur_menu,MENU_SYS_SETTING);
    disp_sys_setting();
}

void oled_handler::set_mainmenu_item(int item,int val)
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

void oled_handler::update_menu_sys_setting(bool bUpdateLast)
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
            disp_icon(mSettingItems.icon_normal[last][get_setting_select(last)]);
        }
    }
    disp_icon(mSettingItems.icon_light[cur][get_setting_select(cur)]);
}

void oled_handler::add_qr_res(int type,sp<ACTION_INFO> &mAdd,int control_act)
{
    CHECK_NE(type,-1);
    CHECK_NE(mAdd,nullptr);
    int menu;
    int max;
    int key;
    sp<ACTION_INFO> mRes = sp<ACTION_INFO>(new ACTION_INFO());
    memcpy(mRes.get(), mAdd.get(),sizeof(ACTION_INFO));

    Log.d(TAG, "add_qr_res type (%d %d)", type, control_act);
    switch (control_act)
    {
        case ACTION_PIC:
        case ACTION_VIDEO:
        case ACTION_LIVE:
            mControlAct = mRes;
            //force 0 to 10
            if (mControlAct->size_per_act == 0)
            {
                Log.d(TAG,"force control size_per_act is %d", mControlAct->size_per_act);
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

void oled_handler::update_menu()
{
    int item = get_menu_select_by_power(cur_menu);
    switch (cur_menu)
    {
        case MENU_TOP:
            update_menu_disp(main_menu[1], main_menu[0]);
            break;
		
        case MENU_SYS_SETTING:
            update_menu_sys_setting(true);
            break;
		
        case MENU_PIC_SET_DEF:
//            Log.d(TAG," pic set def item is %d", item);
            mProCfg->set_def_info(KEY_PIC_DEF,item);
            update_menu_disp(pic_def_setting_menu[1]);
            if(item < PIC_CUSTOM)
            {
                disp_org_rts((int)(mPICAction[item].stOrgInfo.save_org != SAVE_OFF),
                             (int)(mPICAction[item].stStiInfo.stich_mode != STITCH_OFF));
            }
            else
            {
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
            if(item < VID_CUSTOM)
            {
                disp_org_rts((int)(mVIDAction[item].stOrgInfo.save_org != SAVE_OFF),
                             (int)(mVIDAction[item].stStiInfo.stich_mode != STITCH_OFF));
            }
            else
            {
                disp_org_rts((int)(mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             (int)(mProCfg->get_def_info(KEY_VIDEO_DEF)->stStiInfo.stich_mode != STITCH_OFF));
            }
           update_bottom_space();
            break;

        case MENU_LIVE_SET_DEF:
            mProCfg->set_def_info(KEY_LIVE_DEF,item);
            update_menu_disp(live_def_setting_menu[1]);
            if (item < LIVE_CUSTOM)
            {
                disp_org_rts((int)(mLiveAction[item].stOrgInfo.save_org != SAVE_OFF),0,
                             (int)(mLiveAction[item].stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            }
            else
            {
                disp_org_rts((int)(mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF),
                             /*mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.file_save,*/0,
                             (int)(mProCfg->get_def_info(KEY_LIVE_DEF)->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
            }
            disp_live_ready();
            break;

        case MENU_SYS_DEV_INFO:
//            update_sys_info();
            break;

            SWITCH_DEF_ERROR(cur_menu)
    }
}


void oled_handler::get_storage_info()
{
    int storage_index;

    memset(total_space,0,sizeof(total_space));
    memset(used_space,0,sizeof(used_space));

//    Log.d(TAG,"get total mSaveList.size() %d", mSaveList.size());
    for (u32 i = 0; i < mSaveList.size(); i++)
    {
        struct statfs diskInfo;
        u64 totalsize = 0;
        u64 used_size = 0;
        storage_index = get_dev_type_index(mSaveList.at(i)->dev_type);

        if (access(mSaveList.at(i)->path, F_OK) != -1)
        {
            statfs(mSaveList.at(i)->path, &diskInfo);
            u64 blocksize = diskInfo.f_bsize;    //每个block里包含的字节数
            totalsize = blocksize * diskInfo.f_blocks;   //总的字节数，f_blocks为block的数目
            used_size = (diskInfo.f_blocks - diskInfo.f_bavail) * blocksize;   //可用空间大小
            convert_space_to_str(totalsize,
                                 total_space[storage_index],
                                 sizeof(total_space[storage_index]));
            convert_space_to_str(used_size,
                                 used_space[storage_index],
                                 sizeof(used_space[storage_index]));
        }
        else
        {
            Log.d(TAG, "%s not access", mSaveList.at(i)->path);
        }
    }
}



bool oled_handler::get_save_path_avail()
{
    bool ret = false;

    if (!check_save_path_none())
    {
        int times = 3;
        int i = 0;
        for (i = 0; i < times; i++)
        {
            struct statfs diskInfo;
            if (access(mSaveList.at(save_path_select)->path, F_OK) != -1)
            {
                statfs(mSaveList.at(save_path_select)->path, &diskInfo);

                /*
                 * 老化模式下为了能让时间尽可能长,虚拟出足够长的录像时间
                 */
				#ifdef ENABLE_AGEING_MODE
                mSaveList.at(save_path_select)->avail = 0x10000000000;
				#else
                mSaveList.at(save_path_select)->avail = diskInfo.f_bavail *diskInfo.f_bsize;
				#endif
				
                Log.d(TAG, "remain ( %s %llu)\n",
                      mSaveList.at(save_path_select)->path,
                      mSaveList.at(save_path_select)->avail);
                break;
            }
            else
            {
                Log.d(TAG, "fail to access(%d) %s", i, mSaveList.at(save_path_select)->path);
                msg_util::sleep_ms(10);
            }
        }

        if( i == times )
        {
            Log.d(TAG,"still fail to access %s",
                  mSaveList.at(save_path_select)->path);
            mSaveList.at(save_path_select)->avail = 0;
        }

        ret = true;
    }

    return ret;
}


void oled_handler::get_save_path_remain()
{
    if(get_save_path_avail())
    {
        caculate_rest_info(mSaveList.at(save_path_select)->avail);
    }
}


void oled_handler::caculate_rest_info(u64 size)
{
    INFO_MENU_STATE(cur_menu,cam_state);
    int size_M = (int)(size >> 20);
//        Log.d(TAG,"size M %d %d",size_M);
    if( size_M >  AVAIL_SUBSTRACT)
    {
        int rest_sec = 0;
        int rest_min = 0;

        //deliberately minus 1024
        size_M -= AVAIL_SUBSTRACT;;
        int item = 0;
        Log.d(TAG," curmenu %d item %d",cur_menu,item);
        switch(cur_menu)
        {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if(mControlAct != nullptr)
                {
                    mRemainInfo->remain_pic_num = size_M/mControlAct->size_per_act;
                    Log.d(TAG,"0remain(%d %d)",
                          size_M,mRemainInfo->remain_pic_num);
                }
                else
                {
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
                    switch(item)
                    {
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
                            Log.d(TAG,"1remain(%d %d %d)",
                                  size_M,mPICAction[item].size_per_act,
                                  mRemainInfo->remain_pic_num);
                            break;
                        case PIC_CUSTOM:
                            mRemainInfo->remain_pic_num = size_M/mProCfg->get_def_info(KEY_PIC_DEF)->size_per_act;
                            Log.d(TAG,"3remain(%d %d)",
                                  size_M,mRemainInfo->remain_pic_num);
                            break;
                        SWITCH_DEF_ERROR(item)
                    }
                }
                break;
            case MENU_VIDEO_INFO:
            case MENU_VIDEO_SET_DEF:
                //not cal while rec already started
                if(!(check_state_in(STATE_RECORD) && (mRecInfo->rec_hour > 0 || mRecInfo->rec_min > 0 || mRecInfo->rec_sec > 0)))
                {
                    if(mControlAct != nullptr)
                    {
                        Log.d(TAG,"vid size_per_act %d",mControlAct->size_per_act);
                        rest_sec = size_M/mControlAct->size_per_act;
                    }
                    else
                    {
                        item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
                        switch(item)
                        {
                            case VID_8K_50M_30FPS_PANO_RTS_OFF:
                            case VID_6K_50M_30FPS_3D_RTS_OFF:
                            case VID_4K_50M_120FPS_PANO_RTS_OFF:
                            case VID_4K_20M_60FPS_3D_RTS_OFF:
                            case VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON:
                            case VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON:
                                Log.d(TAG,"2vid size_per_act %d",
                                      mVIDAction[item].size_per_act);
                                rest_sec = (size_M/mVIDAction[item].size_per_act);
                                break;
                            case VID_CUSTOM:
                                Log.d(TAG,"3vid size_per_act %d",
                                      mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act);
                                rest_sec = (size_M/mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act);
                                break;
                            SWITCH_DEF_ERROR(item)
                        }
                    }
                    rest_min = rest_sec/60;
                    mRemainInfo->remain_sec = rest_sec%60;
                    mRemainInfo->remain_hour = rest_min/60;
                    mRemainInfo->remain_min = rest_min%60;
                    Log.d(TAG,"remain( %d %d "
                                  " %d  %d  %d)",
                          size_M,
                          mRemainInfo->remain_hour,
                          mRemainInfo->remain_min,
                          mRemainInfo->remain_sec);
                }
                break;
            SWITCH_DEF_ERROR(cur_menu)
        }
    }
    else
    {
        Log.d(TAG,"reset remainb");
        memset(mRemainInfo.get(),0, sizeof(REMAIN_INFO));
    }
}

void oled_handler::convert_space_to_str(u64 size, char *str, int len)
{
    double size_b = (double)size;
    double info_G = (size_b/1024/1024/1024);
    snprintf(str,len,"%.1fG",info_G);
}

bool oled_handler::check_save_path_none()
{
    bool bRet = false;
	Log.d(TAG, "save_path_select is %d", save_path_select);
    if (save_path_select == -1) {
        bRet = true;
    }
    return bRet;
}

bool oled_handler::check_save_path_usb()
{
    bool bRet = false;
    Log.d(TAG,"save_path_select is %d",save_path_select);
    if (save_path_select != -1 && (get_dev_type_index(mSaveList.at(save_path_select)->dev_type) == SET_STORAGE_USB)) {
        bRet = true;
    } else {
        disp_msg_box(DISP_LIVE_REC_USB);
    }
    return bRet;
}


/* 
 * disp_bottom_space - 显示底部的空间信息
 */
void oled_handler::disp_bottom_space()
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
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:

					#ifdef ARIAL_LIVE
                        case VID_ARIAL:
					#endif
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
    }
    else
    {	/* 存储设备不存在, 显示"None" */
        Log.d(TAG,"reset remain a");
        memset(mRemainInfo.get(), 0, sizeof(REMAIN_INFO));
        disp_icon(ICON_LIVE_INFO_NONE_7848_50X16);
    }
	
}

void oled_handler::update_bottom_space()
{
    get_save_path_remain();
    disp_bottom_space();
}

void oled_handler::disp_bottom_info(bool high)
{
    disp_cam_param(0);
    update_bottom_space();
}

struct _select_info_ * oled_handler::get_select_info()
{
    return (SELECT_INFO *)&(mMenuInfos[cur_menu].mSelectInfo);
}

void oled_handler::disp_sys_setting()
{
    SELECT_INFO * mSelect = get_select_info();
    int start = mSelect->cur_page * mSelect->page_max;
    const int end = start + mSelect->page_max;
    const int select = get_menu_select_by_power(cur_menu);

    int item = 0;
//    Log.d(TAG,"start %d end  %d select %d "
//            "ICON_SET_INFO_LIGHT_25_32_96_16 %d" ,
//          start,end,select,ICON_SET_INFO_LIGHT_25_32_96_16);
    while (start < end)
    {
        if (start < mSelect->total)
        {
            int val = get_setting_select(start);
            if (start == select)
            {
//                Log.d(TAG,"select %d high icon %d",select,
//                      mSettingItems.icon_light[start][val]);
                disp_icon(mSettingItems.icon_light[start][val]);
            }
            else
            {
//                Log.d(TAG,"start %d normal icon %d",start,
//                      mSettingItems.icon_normal[start][val]);
                disp_icon(mSettingItems.icon_normal[start][val]);
            }
        }
        else
        {
//            Log.d(TAG,"item is %d", item);
            clear_icon(mSettingItems.clear_icons[(item - 1)]);
        }
        start++;
        item++;
    }
}

void oled_handler::disp_storage_setting()
{
    char dev_space[128];

    get_storage_info();
    for (u32 i = 0; i < sizeof(used_space)/sizeof(used_space[0]); i++)
    {
        switch (i)
        {
            case SET_STORAGE_SD:
            case SET_STORAGE_USB:
                if (strlen(used_space[i]) == 0)
                {
                    snprintf(dev_space,sizeof(dev_space),"%s:None", dev_type[i]);
                }
                else
                {
                    snprintf(dev_space,sizeof(dev_space),"%s:%s/%s",
                             dev_type[i],used_space[i],total_space[i]);
                }
				
                Log.d(TAG, "disp storage[%d] %s", i, dev_space);
                disp_str((const u8 *)dev_space,storage_area[i].x,storage_area[i].y,false,storage_area[i].w);
                break;
				
            SWITCH_DEF_ERROR(i)
        }
    }
}

void oled_handler::disp_org_rts(sp<struct _action_info_> &mAct,int hdmi)
{
    disp_org_rts((int) (mAct->stOrgInfo.save_org != SAVE_OFF),
                 (int) (mAct->stStiInfo.stich_mode != STITCH_OFF),hdmi);
}

void oled_handler::disp_org_rts(int org,int rts,int hdmi)
{
    int new_org_rts = 0;

    if (org == 1)
    {
        if (rts == 1)
        {
            new_org_rts = 0;
        }
        else
        {
            new_org_rts = 1;
        }
    }
    else
    {
        if (rts == 1)
        {
            new_org_rts = 2;
        }
        else
        {
            new_org_rts = 3;
        }
    }
    //force update even not change 0623
//    if(last_org_rts != new_org_rts)
    {
//        last_org_rts = new_org_rts;
        switch(new_org_rts)
        {
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

void oled_handler::disp_qr_res(bool high)
{
    if(high)
    {
        disp_icon(ICON_VINFO_CUSTOMIZE_LIGHT_0_48_78_1678_16);
    }
    else
    {
        disp_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
    }
}


// highlight : 0 -- normal 1 -- high
void oled_handler::disp_cam_param(int higlight)
{
    int item;
    int icon = -1;
//    Log.d(TAG,"disp_cam_param high %d cur_menu %d bDispControl %d",
//          higlight,cur_menu);
    {
        INFO_MENU_STATE(cur_menu,cam_state)
        switch (cur_menu)
        {
            case MENU_PIC_INFO:
            case MENU_PIC_SET_DEF:
                if(mControlAct != nullptr)
                {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                }
                else
                {
                    item = get_menu_select_by_power(MENU_PIC_SET_DEF);
//                    Log.d(TAG, "pic def item %d", item);
                    switch (item)
                    {
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
                            icon = pic_def_setting_menu[higlight][item];
                            disp_org_rts((int) (mPICAction[item].stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mPICAction[item].stStiInfo.stich_mode != STITCH_OFF));
                            break;
                            //user define
                        case PIC_CUSTOM:
                            icon = pic_def_setting_menu[higlight][item];
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
                if(tl_count != -1)
                {
                    disp_org_rts(0,0);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                }
                else if(mControlAct != nullptr)
                {
                    disp_org_rts(mControlAct);
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                }
                else
                {
                    item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
//                    Log.d(TAG, "video def item %d", item);
                    switch (item)
                    {
                        case VID_8K_50M_30FPS_PANO_RTS_OFF:
                        case VID_6K_50M_30FPS_3D_RTS_OFF:
                        case VID_4K_50M_120FPS_PANO_RTS_OFF:
                        case VID_4K_20M_60FPS_3D_RTS_OFF:
                        case VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON:
                        case VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON:
#ifndef ARIAL_LIVE
                        case VID_ARIAL:
#endif
                            icon = vid_def_setting_menu[higlight][item];
                            disp_org_rts((int) (mVIDAction[item].stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mVIDAction[item].stStiInfo.stich_mode != STITCH_OFF));
                            break;
                        case VID_CUSTOM:
                            icon = vid_def_setting_menu[higlight][item];
                            disp_org_rts((int) (mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.save_org != SAVE_OFF),
                                         (int) (mProCfg->get_def_info(KEY_VIDEO_DEF)->stStiInfo.stich_mode != STITCH_OFF));

                            break;
                        SWITCH_DEF_ERROR(item)
                    }
                }
                break;
            case MENU_LIVE_INFO:
            case MENU_LIVE_SET_DEF:
                if(mControlAct != nullptr)
                {
                    disp_org_rts((int)(mControlAct->stOrgInfo.save_org != SAVE_OFF),
                                0/* mControlAct->stStiInfo.stStiAct.mStiL.file_save*/,(int)
                            (mControlAct->stStiInfo.stStiAct.mStiL.hdmi_on == HDMI_ON));
                    clear_icon(ICON_VINFO_CUSTOMIZE_NORMAL_0_48_78_1678_16);
                }
                else
                {
                    item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
//                    Log.d(TAG, "live def item %d", item);
                    switch (item)
                    {
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
                        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
                        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
#ifdef ARIAL_LIVE
                        case VID_ARIAL:
#endif
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
        if (icon != -1)
        {
//            Log.d(TAG, "disp icon %d ICON_CAMERA_INFO01_NORMAL_0_48_78_1678_16 %d",
//                  icon, ICON_CAMERA_INFO01_NORMAL_0_48_78_1678_16);
            disp_icon(icon);
        }
    }
}

void oled_handler::disp_sec(int sec,int x,int y)
{
    char buf[32];

    snprintf(buf,sizeof(buf),"%ds ",sec);
    disp_str((const u8 *)buf,x,y);
}

void oled_handler::disp_calibration_res(int type,int t)
{
    Log.d(TAG," disp_calibration_res type %d",type);
    switch(type)
    {
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
        case 3:
        {
            disp_sec(t,96,32);
        }
            break;
        SWITCH_DEF_ERROR(type)
    }
}

bool oled_handler::is_top_clear(int menu)
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

void oled_handler::disp_low_protect(bool bStart)
{
    clear_area();
    if (bStart) {
        disp_str((const u8 *)"Start Protect", 8, 16);
    } else {
        disp_str((const u8 *)"Protect...", 8, 16);
    }
}

void oled_handler::disp_low_bat()
{
    clear_area();
    disp_str((const u8 *)"Low Battery ", 8, 16);
    set_light_direct(BACK_RED|FRONT_RED);
}

void oled_handler::func_low_protect()
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
        func_back();
    }
    Log.d(TAG,"func_low_protect %d menu %d state 0x%x",
          bSend,cur_menu,cam_state);
#endif
}

void oled_handler::disp_menu(bool dispBottom)
{
//    Log.d(TAG,"disp_menu is %d", cur_menu);
    switch (cur_menu) {
        case MENU_TOP:
            disp_icon(main_icons[mProCfg->get_val(KEY_WIFI_ON)][get_select()]);
            break;
		
        case MENU_PIC_INFO:
            disp_icon(ICON_CAMERA_ICON_0_16_20_32);
//            Log.d(TAG,"pic cam_state 0x%x",cam_state);
            Log.d(TAG,"disp_bottom_info %d",dispBottom);
            if(dispBottom)
            {
                disp_bottom_info();
            }
            if(check_state_preview())
            {
                disp_ready_icon();
            }
            else if(check_state_equal(STATE_START_PREVIEWING) ||
                    check_state_in(STATE_STOP_PREVIEWING))
            {
                disp_waiting();
            }
            else if(check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS))
            {
                if (cap_delay == 0)
                {
                    disp_shooting();
                }
                else
                {
                    //waiting for next update_light msg
                    clear_ready();
                }
            }
            else if (check_state_in(STATE_PIC_STITCHING))
            {
                disp_processing();
            }
            else
            {
                Log.d(TAG,"pic menu error state 0x%x",cam_state);
                if(check_state_equal(STATE_IDLE))
                {
                    func_back();
                }
            }
            break;
			
        case MENU_VIDEO_INFO:
            disp_icon(ICON_VIDEO_ICON_0_16_20_32);
            disp_bottom_info();
            if(check_state_preview())
            {
                disp_ready_icon();
            }
            else if(check_state_equal(STATE_START_PREVIEWING) ||
                    (check_state_in(STATE_STOP_PREVIEWING)) || check_state_in(STATE_START_RECORDING))
            {
                disp_waiting();
            }
            else if (check_state_in(STATE_RECORD))
            {
                Log.d(TAG,"do nothing in rec cam state 0x%x",cam_state);
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
                    func_back();
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
                    func_back();
                }
            }
            break;
			
        case MENU_STORAGE:
            clear_area(0,16);
            disp_icon(ICON_STORAGE_IC_DEFAULT_0016_25X48);
            disp_storage_setting();
            break;
			
        case MENU_SYS_SETTING:
            clear_area(0,16);
            disp_icon(ICON_SET_IC_DEFAULT_25_48_0_1625_48);
            disp_sys_setting();
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
                    func_back();
                }
            }
            break;
			
        case MENU_CALIBRATION:
            Log.d(TAG,"MENU_CALIBRATION cap delay %d",cap_delay);
            if(cap_delay > 0)
            {
                disp_icon(ICON_CALIBRAT_AWAY128_16);
                //wait for next update_light msg
            }
            else
            {
                disp_calibration_res(2);
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
            if (check_state_equal(STATE_START_GYRO))
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
//                Log.d(TAG,"save_path_select is %d",save_path_select);
            if(!check_save_path_none())
            {
                int dev_index = get_dev_type_index(mSaveList.at(save_path_select)->dev_type);
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
                            Log.d(TAG,"STATE_SPEED_TEST save_path_select is -1");
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
                            Log.d(TAG,"save_path_select is -1");
//                        abort();
                            disp_icon(ICON_SPEEDTEST01128_64);
                            break;
                    }
                }
            }
            else
            {
                Log.d(TAG,"card remove while speed test cam_state 0x%x",cam_state);
                func_back();
            }
        }
            break;
        case MENU_RESET_INDICATION:
            aging_times = 0;
            disp_icon(ICON_RESET_IDICATION_128_48128_48);
            break;
        case MENU_WIFI_CONNECT:
            disp_wifi_connect();
            break;

		/*
		 * 显示老化
		 */
        case MENU_AGEING:
            disp_ageing();	/* Aging Menu ->  */
            break;

        case MENU_NOSIE_SAMPLE:
            disp_icon(ICON_SAMPLING_128_48128_48);
            break;
        case MENU_LIVE_REC_TIME:
            break;
        SWITCH_DEF_ERROR(cur_menu);
    }

    if (is_top_clear(cur_menu)) {
        reset_last_info();
        bDispTop = false;
    } else if (!bDispTop) {
        disp_top_info();
    }
}


void oled_handler::disp_wifi_connect()
{
    disp_icon(ICON_WIF_CONNECT_128_64128_64);
}

bool oled_handler::check_dev_exist(int action)
{
    bool bRet = false;
    Log.d(TAG,"check_dev_exist (%d %d)",mSaveList.size(),save_path_select);
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
                if(mRemainInfo->remain_hour > 0  || mRemainInfo->remain_min > 0 || mRemainInfo->remain_sec > 0)
                {
                    bRet = true;
                }
                else
                {
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

void oled_handler::power_menu_cal_setting()
{
    send_option_to_fifo(ACTION_CALIBRATION);
}

bool oled_handler::switch_dhcp_mode(int iDHCP)
{
    bool bRet = false;
	
    Log.d(TAG, "iDHCP is %d mProCfg->get_val(KEY_WIFI_ON)  %d "
                  "org_addr 0x%x",
          iDHCP, mProCfg->get_val(KEY_WIFI_ON), org_addr);

	if (/*(mProCfg->get_val(KEY_WIFI_ON) == 0) &&*/ (10001 != org_addr)) {
        //exec_sh("ifconfig eth0 down");
        //set_org_addr(10001);
	
        if (iDHCP == 0) {	/* 静态地址模式:  默认设置为192.168.1.188 */
            system("ifconfig eth0 192.168.1.188 netmask 255.255.255.0 up");
        } else {	/* DHCP模式 */
			// disp_icon(ICON_WIFI_CLOSE_0_0_16_16);
            exec_sh("ifconfig eth0 down");
            exec_sh("killall dhclient");
			exec_sh("dhclient eth0");
        }
        bRet = true;
    }

    return bRet;
}




/*************************************************************************
** 方法名称: func_power
** 方法功能: POWER/OK键按下的处理
** 入口参数: 
**		无
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void oled_handler::func_power()
{
	Log.d(TAG, "cur_menu %d select %d\n", cur_menu, get_select());

	/* 根据当前的菜单做出不同的处理 */
    switch (cur_menu) {	/* 根据当前的菜单做出响应的处理 */
        case MENU_TOP:	/* 顶层菜单 */
            switch (get_select()) {	/* 获取当前选择的菜单项 */
                case MAINMENU_PIC:	/* 选择的是"拍照"项 */
                    if (send_option_to_fifo(ACTION_PREVIEW)) {	/* 发送预览请求 */
                        oled_disp_type(START_PREVIEWING);
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
					
                case MAINMENU_WIFI:
                    wifi_action();
                    break;
				
                case MAINMENU_CALIBRATION:
                    power_menu_cal_setting();
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
		
        case MENU_SYS_SETTING:	/* 设置子菜单 */
        {
            int item = get_menu_select_by_power(cur_menu);
            int val = get_setting_select(item);
            switch (item) {
                case SET_WIFI_AP:
                    val = ((~val) & 0x00000001);
                    // wifi is on need switch
                    if (mProCfg->get_val(KEY_WIFI_ON) == 1) {
                        mProCfg->set_val(KEY_WIFI_AP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting();
                        start_wifi(1);
                    } else {
                        mProCfg->set_val(KEY_WIFI_AP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting();
                    }
                    break;
					
                case SET_DHCP_MODE:
                    val = ((~val) & 0x00000001);
                    if (switch_dhcp_mode(val)) {
                        mProCfg->set_val(KEY_DHCP, val);
                        set_setting_select(item, val);
                        update_menu_sys_setting();
                    }
                    break;
					
                case SET_FREQ:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_PAL_NTSC, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting();
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_FLICKER);
                    break;
					
                case SET_SPEAK_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_SPEAKER, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting();
                    break;
					
                case SET_BOTTOM_LOGO:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_SET_LOGO, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting();
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_LOGO);
                    break;
					
                case SET_LIGHT_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_LIGHT_ON, val);
                    set_setting_select(item, val);
                    if (val == 1) {
                        set_light();
                    } else {
                        set_light_direct(LIGHT_OFF);
                    }
                    update_menu_sys_setting();
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
                    update_menu_sys_setting();
                    if (get_setting_select(SET_AUD_ON) == 1) {
                        send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_AUD);
                    }
                    break;
					
                case SET_GYRO_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_GYRO_ON, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting();
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_GYRO_ON);
                    break;
					
                case SET_FAN_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_FAN, val);
                    set_setting_select(item, val);
                    if (val == 0) {
                        disp_msg_box(DISP_ALERT_FAN_OFF);
                    } else {
                        update_menu_sys_setting();
                    }
                    send_option_to_fifo(ACTION_SET_OPTION, OPTION_SET_FAN);
                    break;
					
                case SET_AUD_ON:
                    val = ((~val) & 0x00000001);
                    mProCfg->set_val(KEY_AUD_ON, val);
                    set_setting_select(item, val);
                    update_menu_sys_setting();
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
				
                SWITCH_DEF_ERROR(get_menu_select_by_power(cur_menu))
            }
        }
            break;
//        case MENU_CALIBRATION_SETTING:
//            power_menu_cal_setting();
//            break;


        case MENU_PIC_SET_DEF:
            func_back();
            break;
		
        case MENU_VIDEO_SET_DEF:
            func_back();
            break;
		
        case MENU_LIVE_SET_DEF:
            func_back();
            break;

        case MENU_GYRO_START:
            if (check_state_equal(STATE_IDLE)) {
                send_option_to_fifo(ACTION_GYRO);
            } else {
                Log.d(TAG,"gyro start cam_state 0x%x",cam_state);
            }
            break;
			
        case MENU_SPEED_TEST:
            if (check_state_equal(STATE_PREVIEW)) {
                send_option_to_fifo(ACTION_SPEED_TEST);
            } else {
                Log.d(TAG,"speed cam_state 0x%x",cam_state);
            }
            break;
			
        case MENU_RESET_INDICATION:
            if (check_state_equal(STATE_IDLE)) {
                restore_all();
            }
            break;
			
        case MENU_LIVE_REC_TIME:
            send_option_to_fifo(ACTION_LIVE);
            break;
		
        SWITCH_DEF_ERROR(cur_menu)
    }
}


int oled_handler::get_select()
{
    return mMenuInfos[cur_menu].mSelectInfo.select;
}

int oled_handler::get_last_select()
{
    return mMenuInfos[cur_menu].mSelectInfo.last_select;
}


void oled_handler::func_setting()
{
//    Log.d(TAG," func_setting cur_menu %d cam_state %d",
//              cur_menu, cam_state);
    switch (cur_menu) {
    case MENU_TOP:
        if (get_select() == MAINMENU_WIFI) {
                Log.d(TAG, "wif state %d ap %d\n", mProCfg->get_val(KEY_WIFI_ON), get_setting_select(SET_WIFI_AP));
                if (/*mProCfg->get_val(KEY_WIFI_ON) == 0 &&*/ get_setting_select(SET_WIFI_AP) == 0) {
                    start_qr_func();
                }
            } else {
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
        Log.d(TAG, "aging_times %d state 0x%x\n", aging_times, cam_state);
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



/* SKYMIXOS
 * 1. 检查到SD卡存在并且里面有factory.json文件时,设置当前菜单为MENU_AGEING
 * 
 */

/*************************************************************************
** 方法名称: func_up
** 方法功能: 按键事件处理
** 入口参数: 
**		iKey - 键值
** 返 回 值: 无 
** 调     用: 
**
*************************************************************************/
void oled_handler::func_up()
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
void oled_handler::func_down()
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



void oled_handler::exit_sys_err()
{
    Log.d(TAG, "exit_sys_err ( %d 0x%x )", cur_menu, cam_state);
	
    if (cur_menu == MENU_SYS_ERR || ((MENU_LOW_BAT == cur_menu) && check_state_equal(STATE_IDLE))) {

        //force set front light
        if (get_setting_select(SET_LIGHT_ON) == 1) {
            set_light_direct(front_light);
        } else {
            set_light_direct(LIGHT_OFF);
        }
		
        func_back();
    }
}


bool oled_handler::check_cur_menu_support_key(int iKey)
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
void oled_handler::handle_oled_key(int iKey)
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
            func_up();
            break;
		
        case OLED_KEY_DOWN:		/* "DOWN"键的处理 */
        case 0x72:
            func_down();
            break;
		
        case OLED_KEY_BACK:		/* "BACK"键的处理 */
        case 0x66:
            func_back();
            break;
		
        case OLED_KEY_SETTING:	/* "Setting"键的处理 */
        case 0x160:
            func_setting();
            break;
		
        case OLED_KEY_POWER:	/* "POWER"键的处理 */
        case 0x74:
            func_power();
            break;
		
        default:
            break;
        }
    } else {
        exit_sys_err();
    }
}



void oled_handler::clear_area(u8 x,u8 y,u8 w,u8 h)
{
    mOLEDModule->clear_area(x,y,w,h);
}

void oled_handler::clear_area(u8 x, u8 y)
{
    mOLEDModule->clear_area(x,y);
}

bool oled_handler::check_allow_update_top()
{
    return !is_top_clear(cur_menu);
}


int oled_handler::oled_disp_ip(unsigned int addr)
{
//    Log.d(TAG," org adr %u new addr %u",org_addr,addr);
    if (org_addr != addr) {
        u8 org_ip[32];
        u8 ip[32];

        int_to_ip(org_addr, org_ip, sizeof(org_ip));
        int_to_ip(addr, ip, sizeof(ip));

        if (check_allow_update_top()) {
            mOLEDModule->disp_ip((const u8 *)ip);
        }

        set_org_addr(addr);
    }
    return 0;
}


int oled_handler::oled_disp_battery()
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

            if (m_bat_info_->battery_level >= 100) {
                snprintf((char *) buf, sizeof(buf), "%d", 100);
            } else {
                snprintf((char *)buf, sizeof(buf), "%d", m_bat_info_->battery_level);
            }

            disp_str_fill(buf, x, 0);
            disp_icon(icon);
        }
    } else {
        //disp nothing while no bat
        clear_area(103, 0, 25, 16);
    }
	
    set_light();
    return 0;
}

void oled_handler::disp_waiting()
{
    disp_icon(ICON_CAMERA_WAITING_2016_76X32);
}

void oled_handler::disp_connecting()
{
    disp_icon(ICON_LIVE_RECONNECTING_76_32_20_1676_32);
}

void oled_handler::disp_saving()
{
    disp_icon(ICON_CAMERA_SAVING_2016_76X32);
}

void oled_handler::clear_ready()
{
    clear_icon(ICON_CAMERA_READY_20_16_76_32);
}

void oled_handler::disp_live_ready()
{
    bool bReady = true;
    int item = get_menu_select_by_power(MENU_LIVE_SET_DEF);

    switch (item) {
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON:
        case VID_4K_20M_24FPS_3D_30M_24FPS_RTS_ON_HDMI:
        case VID_4K_20M_24FPS_PANO_30M_30FPS_RTS_ON_HDMI:
			
#ifdef ARIAL_LIVE
        case VID_ARIAL:
#endif
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
    Log.d(TAG, "disp_live_ready %d %d", item,bReady);

    if (bReady) {
        disp_icon(ICON_CAMERA_READY_20_16_76_32);
    } else {
        disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
    }
}


void oled_handler::disp_ready_icon(bool bDispReady)
{
    if (!check_save_path_none()) {
        disp_icon(ICON_CAMERA_READY_20_16_76_32);
    } else {
        disp_icon(ICON_VIDEO_NOSDCARD_76_32_20_1676_32);
    }
}

    
void oled_handler::disp_shooting()
{
    Log.d(TAG, "check_save_path_none() is %d\n", check_save_path_none());
    disp_icon(ICON_CAMERA_SHOOTING_2016_76X32);
    set_light(fli_light);
}

void oled_handler::disp_processing()
{
	disp_icon(ICON_PROCESS_76_3276_32);
	send_update_light(MENU_PIC_INFO, STATE_PIC_STITCHING, INTERVAL_5HZ);
}

bool oled_handler::check_state_preview()
{
	return check_state_equal(STATE_PREVIEW);
}

bool oled_handler::check_state_equal(int state)
{
	return (cam_state == state);
}

bool oled_handler::check_state_in(int state)
{
    bool bRet = false;
    if ((cam_state & state) == state) {
        bRet = true;
    }
    return bRet;
}

void oled_handler::update_by_controller(int action)
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

void oled_handler::add_state(int state)
{
    cam_state |= state;
    switch (state) {
        case STATE_STOP_RECORDING:
            disp_saving();
            break;
		
        case STATE_START_PREVIEWING:
        case STATE_STOP_PREVIEWING:
        case STATE_START_RECORDING:
//        case STATE_STOP_RECORDING:
        case STATE_START_LIVING:
        case STATE_STOP_LIVING:
            disp_waiting();
            break;
		
        case STATE_TAKE_CAPTURE_IN_PROCESS:
        case STATE_PIC_STITCHING:
            //force pic menu to make bottom info updated ,if req from http
            set_cur_menu(MENU_PIC_INFO);
#if 0
            if(cur_menu != MENU_PIC_INFO)
            {

            }
            else
            {
                if(state == STATE_TAKE_CAPTURE_IN_PROCESS)
                {
                    //clear ready icon waiting for disp 5,4,3...
                    clear_icon(ICON_CAMERA_READY_20_16_76_32);
                }
                else
                {
                    disp_processing();
                }
            }
#endif
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
//            Log.d(TAG," add STATE_LIVE cam_state 0x%x cur_menu %d",
//                  cam_state, cur_menu);
//            set_cur_menu(MENU_LIVE_INFO);
            break;

        case STATE_LIVE_CONNECTING:
            rm_state(STATE_START_LIVING | STATE_LIVE);
            disp_connecting();
//            set_cur_menu(MENU_LIVE_INFO);
            break;

        case STATE_PREVIEW:
            INFO_MENU_STATE(cur_menu,cam_state);
            rm_state(STATE_START_PREVIEWING|STATE_STOP_PREVIEWING);
            if (!(check_state_in(STATE_LIVE) || check_state_in(STATE_RECORD))) {
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
                        Log.e(TAG, "add preview but do nothing while menu %d\n",cur_menu);
                        break;
                        //state preview with req sync state
                    default:
                        Log.d(TAG, "STATE_PREVIEW default cur_menu %d\n", cur_menu);
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
            if(cur_menu != MENU_QR_SCAN)
            {
                set_cur_menu(MENU_QR_SCAN);
            }
            break;
			
        case STATE_START_QR:
            Log.d(TAG,"start qr cur_menu %d", cur_menu);
            rm_state(STATE_START_QRING);
            set_cur_menu(MENU_QR_SCAN);
            break;
			
        case STATE_STOP_QRING:
            if(cur_menu == MENU_QR_SCAN)
            {
                disp_menu();
            }
            else
            {
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
            func_back();
            break;
			
        case STATE_POWER_OFF:
            Log.d(TAG,"do nothing for power off");
            break;
		
        SWITCH_DEF_ERROR(state)
    }
}

void oled_handler::rm_state(int state)
{
    cam_state &= ~state;
}

void oled_handler::set_tl_count(int count)
{
//    Log.d(TAG,"set tl count %d",count);
    tl_count = count;
}

void oled_handler::disp_tl_count(int count)
{
    if (count < 0) {
        Log.e(TAG,"error tl count %d",tl_count);
    } else if (count == 0) {
        clear_ready();
        char buf[32];
        snprintf(buf,sizeof(buf),"%d",count);
        disp_str((const u8 *)buf,57,24);
        clear_icon(ICON_LIVE_INFO_HDMI_78_48_50_1650_16);
    } else {
        if (check_state_in(STATE_RECORD) && !check_state_in(STATE_STOP_RECORDING))
        {
            char buf[32];
            snprintf(buf,sizeof(buf), "%d", count);
            disp_str((const u8 *)buf,57,24);
            set_light(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(FLASH_LIGHT);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
            set_light(LIGHT_OFF);
            msg_util::sleep_ms(INTERVAL_5HZ/2);
        }
        else
        {
            Log.e(TAG,"tl count error state 0x%x",cam_state);
        }
    }
}

void oled_handler::minus_cam_state(int state)
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
        Log.d(TAG,"minus_cam_state　state preview (%d 0x%x)",
              cur_menu,state);
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
        Log.w(TAG," minus error cam_state 0x%x", cam_state);
    }
}



void oled_handler::disp_err_code(int code, int back_menu)
{
    bool bFound = false;
    char err_code[128];

    memset(err_code,0,sizeof(err_code));

    set_back_menu(MENU_SYS_ERR,back_menu);
    for(u32 i = 0; i < sizeof(mErrDetails)/sizeof(mErrDetails[0]); i++)
    {
        if(mErrDetails[i].code == code)
        {
            if(mErrDetails[i].icon != -1)
            {
                set_light_direct(BACK_RED|FRONT_RED);
                Log.d(TAG,"disp error code (%d %d %d)",i,mErrDetails[i].icon,code);
                disp_icon(mErrDetails[i].icon);
                bFound = true;
            }
            else
            {
                snprintf(err_code,sizeof(err_code),"%s",mErrDetails[i].str);
            }
            break;
        }
    }
    if(!bFound)
    {
        if(strlen(err_code) == 0)
        {
            disp_icon(ICON_ERROR_128_64128_64);
            snprintf(err_code,sizeof(err_code),"%d",code);
            disp_str((const u8 *)err_code,64,16);
        }
        else
        {
            clear_area();
            disp_str((const u8 *)err_code,16,16);
        }
        Log.d(TAG,"disp err code %s\n",err_code);
    }
	
    mControlAct = nullptr;
    reset_last_info();
	
    //force cur menu sys_err
    set_light_direct(BACK_RED | FRONT_RED);
    cur_menu = MENU_SYS_ERR;
    bDispTop = false;
	
    Log.d(TAG,"disp_err_code code %d "
                  "back_menu %d cur_menu %d "
                  "bFound %d cam_state 0x%x",
          code,back_menu,cur_menu,bFound,cam_state);
}

void oled_handler::disp_err_str(int type)
{
    for (u32 i = 0; i < sizeof(mSysErr)/sizeof(mSysErr[0]); i++) {
        if (type == mSysErr[i].type) {
            disp_str((const u8 *) mSysErr[i].code, 64, 16);
            break;
        }
    }
}

void oled_handler::disp_sys_err(int type,int back_menu)
{
    Log.d(TAG, "disp_sys_err cur menu %d"
                  " state 0x%x back_menu %d type %d\n",
          cur_menu, cam_state, back_menu, type);

    //met error at the
    if (cur_menu == -1 && check_state_equal(STATE_IDLE)) {
        Log.e(TAG, " met error at the beginning\n");
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
void oled_handler::set_flick_light()
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
#endif
}



bool oled_handler::check_cam_busy()
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

void oled_handler::set_light()
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

int oled_handler::oled_disp_err(sp<struct _err_type_info_> &mErr)
{
    int type = mErr->type;
    int err_code = mErr->err_code;

    Log.d(TAG, "oled_disp_err type %d "
            "err_code %d cur_menu %d cam_state 0x%x\n", type, err_code, cur_menu, cam_state);

    if (err_code == -1) {
        oled_disp_type(type);
    } else {

        int back_menu = MENU_TOP;
        tl_count = -1;
		
        //make it good code
        err_code = abs(err_code);
        switch (type) {
            case START_PREVIEW_FAIL:
                rm_state(STATE_START_PREVIEWING);
                back_menu = MENU_TOP;
                break;
            case CAPTURE_FAIL:
                rm_state(STATE_TAKE_CAPTURE_IN_PROCESS|STATE_PIC_STITCHING);
                back_menu = MENU_PIC_INFO;
                break;
            case START_REC_FAIL:
                rm_state(STATE_START_RECORDING|STATE_RECORD);
                back_menu = MENU_VIDEO_INFO;
                break;
            case START_LIVE_FAIL:
                rm_state(STATE_START_LIVING|STATE_LIVE);
                back_menu = MENU_LIVE_INFO;
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
                back_menu = MENU_TOP;
                break;
            case STOP_REC_FAIL:
                rm_state(STATE_STOP_RECORDING | STATE_RECORD);
                back_menu = MENU_VIDEO_INFO;
                break;
            case STOP_LIVE_FAIL:
                back_menu = MENU_LIVE_INFO;
                rm_state(STATE_STOP_LIVING | STATE_LIVE);
                break;
            case RESET_ALL:
                oled_disp_type(RESET_ALL);
                err_code = -1;
                break;
            case SPEED_TEST_FAIL:
                back_menu = get_back_menu(MENU_SPEED_TEST);
                rm_state(STATE_SPEED_TEST);
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



//all disp is at bottom
int oled_handler::oled_disp_type(int type)
{
    Log.d(TAG, "oled_disp_type (%d %d 0x%x)\n",
          type, cur_menu, cam_state);

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
                func_back();
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
			
            //rec fail res only operated by oled key
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
                if (mControlAct != nullptr) {
                    mControlAct = nullptr;
                }
				
                //fix select for changed by controller or timelapse
                if (cur_menu == MENU_VIDEO_INFO) {
                    disp_bottom_info();
                } else {
                    Log.e(TAG, "error cur_menu %d\n", cur_menu);
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
                    set_cap_delay(mControlAct->delay);
                } else {
                    int item = get_menu_select_by_power(MENU_PIC_SET_DEF);
                    switch (item)
                    {
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
                            set_cap_delay(mPICAction[item].delay);
                            break;
						
                            //user define
                        case PIC_CUSTOM:
                            set_cap_delay(mProCfg->get_def_info(KEY_PIC_DEF)->delay);
                            break;

                        SWITCH_DEF_ERROR(item)
                    }
                }
                add_state(STATE_TAKE_CAPTURE_IN_PROCESS);
                set_cur_menu(MENU_PIC_INFO);
                send_update_light(MENU_PIC_INFO, STATE_TAKE_CAPTURE_IN_PROCESS, INTERVAL_1HZ);
            }
            break;
			
        case CAPTURE_SUC:
            if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS) || check_state_in(STATE_PIC_STITCHING)) {
                //disp capture suc
                minus_cam_state(STATE_TAKE_CAPTURE_IN_PROCESS | STATE_PIC_STITCHING);
//                Log.w(TAG,"CAPTURE_SUC cur_menu %d", cur_menu);
                //waiting
                if(cur_menu == MENU_PIC_INFO)
                {
                    if(mControlAct != nullptr)
                    {
                        Log.d(TAG,"control cap suc");
                        mControlAct = nullptr;
                        disp_bottom_info();
                    }
                    else
                    {
                        mRemainInfo->remain_pic_num--;
                        Log.d(TAG, "remain pic %d", mRemainInfo->remain_pic_num);
                        disp_bottom_space();
                    }
                }
                else
                {
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
            if(!check_state_in(STATE_LIVE))
            {
                tl_count = -1;
                set_update_mid();
                add_state(STATE_LIVE);
                set_cur_menu(MENU_LIVE_INFO);
            }
            else
            {
                Log.e(TAG,"start live suc error state 0x%x",cam_state);
            }
            break;
			
        case START_LIVE_CONNECTING:
            if(!check_state_in(STATE_LIVE_CONNECTING))
            {
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
            if (check_state_in(STATE_LIVE) || check_state_in(STATE_LIVE_CONNECTING))
            {
                minus_cam_state(STATE_LIVE|STATE_STOP_LIVING|STATE_LIVE_CONNECTING);
                if (mControlAct != nullptr)
                {
                    mControlAct = nullptr;
                    if (cur_menu == MENU_LIVE_INFO)
                    {
                        disp_cam_param(0);
                    }
                    else
                    {
                        Log.e(TAG, "error capture　suc cur_menu %d ", cur_menu);
                    }
                }
            }
            break;
			
        case STOP_LIVE_FAIL:
            rm_state(STATE_STOP_LIVING | STATE_LIVE);
            disp_sys_err(type);
            break;
			
        case PIC_ORG_FINISH:
            if(!check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS))
            {
                Log.e(TAG,"pic org finish error state 0x%x",cam_state);
            }
            else
            {
                if(!check_state_in(STATE_PIC_STITCHING))
                {
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
		
        case START_PREVIEWING:
            add_state(STATE_START_PREVIEWING);
            break;
		
        case START_PREVIEW_SUC:
            if (!check_state_in(STATE_PREVIEW))
            {
                add_state(STATE_PREVIEW);
            }
            break;
			
        case START_PREVIEW_FAIL:
            Log.d(TAG,"START_PREVIEW_FAIL cur_menu %d cam state %d",cur_menu,cam_state);
            rm_state(STATE_START_PREVIEWING);
            disp_sys_err(type, MENU_TOP);
            break;
			
        case STOP_PREVIEWING:
            add_state(STATE_STOP_PREVIEWING);
            break;
		
        case STOP_PREVIEW_SUC:
            if (check_state_in(STATE_PREVIEW))
            {
                minus_cam_state(STATE_PREVIEW | STATE_STOP_PREVIEWING);
            }
            break;
			
        case STOP_PREVIEW_FAIL:
            Log.d(TAG, "STOP_PREVIEW_FAIL fail cur_menu %d %d", cur_menu, cam_state);
            rm_state(STATE_STOP_PREVIEWING | STATE_PREVIEW);
            disp_sys_err(type);
            break;
			
        case START_CALIBRATIONING:
            Log.d(TAG,"cap delay %d cur_menu %d",cap_delay,cur_menu);
            send_update_light(MENU_CALIBRATION,STATE_CALIBRATING,INTERVAL_1HZ);
            if (cur_menu != MENU_CALIBRATION)
            {
                set_cur_menu(MENU_CALIBRATION);
            }
            add_state(STATE_CALIBRATING);
            break;
			
        case CALIBRATION_SUC:
            if (check_state_in(STATE_CALIBRATING))
            {
                disp_calibration_res(0);
                msg_util::sleep_ms(1000);
                minus_cam_state(STATE_CALIBRATING);
            }
            break;
			
        case CALIBRATION_FAIL:
            rm_state(STATE_CALIBRATING);
            disp_sys_err(type,get_back_menu(cur_menu));
            break;
			
        case SYNC_REC_AND_PREVIEW:
            if (!check_state_in(STATE_RECORD))
            {
                cam_state = STATE_PREVIEW;
                add_state(STATE_RECORD);
                //disp video menu before add state_record
                set_cur_menu(MENU_VIDEO_INFO);
                Log.d(TAG," set_update_mid a");
                set_update_mid();
            }
            break;
			
        case SYNC_LIVE_AND_PREVIEW:
            Log.d(TAG,"SYNC_LIVE_AND_PREVIEW for state 0x%x",cam_state);
            //not sync in state_live and state_live_connecting
            if (!check_state_in(STATE_LIVE))
            {
                //must before add live_state for keeping state live_connecting avoiding recalculate time 170804
                set_update_mid();
                cam_state = STATE_PREVIEW;
                add_state(STATE_LIVE);
                set_cur_menu(MENU_LIVE_INFO);
                Log.d(TAG," set_update_mid b");
            }
            break;
			
        case SYNC_LIVE_CONNECT_AND_PREVIEW:
            if (!check_state_in(STATE_LIVE_CONNECTING))
            {
                cam_state = STATE_PREVIEW;
                add_state(STATE_LIVE_CONNECTING);
                set_cur_menu(MENU_LIVE_INFO);
            }
            break;
			
        case START_QRING:
            add_state(STATE_START_QRING);
            break;
		
        case START_QR_SUC:
            if(!check_state_in(STATE_START_QR))
            {
                add_state(STATE_START_QR);
            }
            break;
			
        case START_QR_FAIL:
            rm_state(STATE_START_QRING);
            Log.d(TAG,"cur menu %d back menu %d",
                  cur_menu,get_back_menu(cur_menu));
            disp_sys_err(type,get_back_menu(cur_menu));
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
        //disp low battery and will power off,but if insert electric adapter ,will retrieve back
//        case LOW_BAT_SUC:
//            Log.d(TAG,"low bat suc");
//            finish_low_bat();
//            break;
//        case LOW_BAT_FAIL:
//            Log.d(TAG,"low bat fail");
//            finish_low_bat();
//            break;
        case SET_CUS_PARAM:
            Log.d(TAG,"do nothing for custom param");
            break;
        case TIMELPASE_COUNT:
            INFO_MENU_STATE(cur_menu,cam_state)
            Log.d(TAG,"tl_count %d",tl_count);
            if(!check_state_in(STATE_RECORD))
            {
                Log.e(TAG," TIMELPASE_COUNT cam_state 0x%x",cam_state);
            }
            else
            {
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
void oled_handler::exitAll()
{
    mLooper->quit();
}

void oled_handler::disp_str_fill(const u8 *str, const u8 x, const u8 y, bool high)
{
    mOLEDModule->ssd1306_disp_16_str_fill(str,x,y,high);
}

void oled_handler::disp_str(const u8 *str,const u8 x,const u8 y, bool high,int width)
{
    mOLEDModule->ssd1306_disp_16_str(str,x,y,high,width);
}

void oled_handler::clear_icon(u32 type)
{
    mOLEDModule->clear_icon(type);
}

void oled_handler::disp_icon(u32 type)
{
    mOLEDModule->disp_icon(type);
}

void oled_handler::disp_ageing()
{
    clear_area();
    disp_str((const u8 *)"Aging start ...", 8, 16);
}

int oled_handler::get_dev_type_index(char *type)
{
    int storage_index;
    if (strcmp(type,dev_type[SET_STORAGE_SD]) == 0) {
        storage_index = SET_STORAGE_SD;
    } else if (strcmp(type,dev_type[SET_STORAGE_USB]) == 0) {
        storage_index = SET_STORAGE_USB;
    } else {
        Log.e(TAG, "error dev type %s \n", type);
#ifdef ENABLE_ABORT
        abort();
#else
        storage_index = SET_STORAGE_SD;
#endif
    }
    return storage_index;
}

void oled_handler::disp_dev_msg_box(int bAdd, int type, bool bChange)
{
    Log.d(TAG, "bAdd %d type %d\n", bAdd, type);
	
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


void oled_handler::set_mdev_list(std::vector <sp<USB_DEV_INFO>> &mList)
{
    static bool bFirst = true;
    int dev_change_type;

    // 0 -- add, 1 -- remove, -1 -- do nothing
    int bAdd = CHANGE;
    bool bDispBox = false;
    bool bChange = true;
	
    if (bFirst) {

        bFirst = false;
        mSaveList.clear();
        switch (mList.size()) {
            case 0:
                save_path_select = -1;
                break;
            case 1:
            {
                sp<USB_DEV_INFO> dev = sp<USB_DEV_INFO>(new USB_DEV_INFO());
                memcpy(dev.get(),mList.at(0).get(),sizeof(USB_DEV_INFO));
//                snprintf(new_path, sizeof(new_path),"%s",mList.at(0)->path);
                mSaveList.push_back(dev);
                save_path_select = 0;
            }
                break;
			
            case 2:
                for(u32 i = 0; i < mList.size(); i++)
                {
                    sp<USB_DEV_INFO> dev = sp<USB_DEV_INFO>(new USB_DEV_INFO());
                    memcpy(dev.get(),mList.at(i).get(),sizeof(USB_DEV_INFO));
                    if(get_dev_type_index(mList.at(i)->dev_type) == SET_STORAGE_USB)
                    {
//                        snprintf(new_path, sizeof(new_path),"%s",mList.at(i)->path);
                        save_path_select = i;
                    }
                    mSaveList.push_back(dev);
                }
                break;
				
//            SWITCH_DEF_ERROR(mList.size())
            default:
                Log.d(TAG,"strange bFirst mList.size() is %d",mList.size());
                break;
        }
        send_save_path_change();
    } else {
        Log.d(TAG, " new save list is %d , org save list %d", mList.size() ,mSaveList.size());

        if (mList.size() == 0) {
            bAdd = REMOVE;
            save_path_select = -1;
            if (mSaveList.size() == 0) {
                Log.d(TAG,"strange save list size (%d %d)",mList.size(),mSaveList.size());
                mSaveList.clear();
                send_save_path_change();
                bChange = false;
            } else {
                dev_change_type = get_dev_type_index(mSaveList.at(0)->dev_type);
                mSaveList.clear();
                bDispBox = true;
            }
        } else {
            if (mList.size() < mSaveList.size()) {
                //remove
                bAdd = REMOVE;
                switch (mList.size()) {
                    case 1:
                        if (get_dev_type_index(mList.at(0)->dev_type) == SET_STORAGE_SD) {
                            dev_change_type = SET_STORAGE_USB;
                            save_path_select = 0;
                        } else {
                            dev_change_type = SET_STORAGE_SD;
                            bChange = false;
                        }
                        mSaveList.clear();
                        mSaveList.push_back(mList.at(0));
                        bDispBox = true;
                        break;

                    default:
                        Log.d(TAG,"2strange save list size (%d %d)",mList.size(),mSaveList.size());
                        break;
                }
            }
            //add
            else if (mList.size() > mSaveList.size())
            {
                bAdd = ADD;
                if (mSaveList.size() == 0)
                {
                    dev_change_type = get_dev_type_index(mList.at(0)->dev_type);
                    sp<USB_DEV_INFO> dev = sp<USB_DEV_INFO>(new USB_DEV_INFO());
                    memcpy(dev.get(), mList.at(0).get(), sizeof(USB_DEV_INFO));
//                    snprintf(new_path, sizeof(new_path), "%s", mList.at(0)->path);
                    save_path_select = 0;
                    mSaveList.push_back(dev);
                    bDispBox = true;
                }
                else
                {
//old is usb
//                    CHECK_EQ(mList.size(), 2);
                    switch(mList.size())
                    {
                        case 2:
                            dev_change_type = get_dev_type_index(mSaveList.at(0)->dev_type);
                            if (dev_change_type == SET_STORAGE_USB)
                            {
                                // new insert is sd
                                dev_change_type = SET_STORAGE_SD;
                            }
                            else
                            {
                                // new insert is usb
                                dev_change_type = SET_STORAGE_USB;
                            }
                            mSaveList.clear();
                            for (unsigned int i = 0; i < mList.size(); i++)
                            {
                                sp<USB_DEV_INFO> dev = sp<USB_DEV_INFO>(new USB_DEV_INFO());
                                memcpy(dev.get(),
                                       mList.at(i).get(),
                                       sizeof(USB_DEV_INFO));
                                if(dev_change_type == SET_STORAGE_USB && get_dev_type_index(mList.at(i)->dev_type) == SET_STORAGE_USB)
                                {
//                            snprintf(new_path, sizeof(new_path), "%s",
//                                 mList.at(i)->path);
                                    save_path_select = i;
                                    bChange = true;
                                }
                                mSaveList.push_back(dev);
                            }
                            bDispBox = true;
                            break;
                        default:
                            Log.d(TAG, "3strange save list size (%d %d)",mList.size(),mSaveList.size());
                            break;
                    }
                }
            } else {
                Log.d(TAG, "5strange save list size (%d %d)", mList.size(), mSaveList.size());
            }
        }

        if (mSaveList.size() == 0 ) {
            if (save_path_select != -1) {
                Log.e(TAG,"force path select -1");
                save_path_select = -1;
            }
        } else if (save_path_select >= (int)mSaveList.size()) {
            save_path_select = mSaveList.size() - 1;
            Log.e(TAG,"force path select %d ",save_path_select);
        }
		
        if (bDispBox) {
            disp_dev_msg_box(bAdd, dev_change_type, bChange);
        }
    }
//    Log.d(TAG,"bChange %d save_path_select %d mSaveList.size() %d",
//          bChange,save_path_select,mSaveList.size());
}

bool oled_handler::decrease_rest_time()
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

void oled_handler::increase_rec_time()
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

void oled_handler::flick_light()
{
	Log.d(TAG, "last_light 0x%x fli_light 0x%x", last_light, fli_light);
	
    if ((last_light & 0xf8) != fli_light) {
        set_light(fli_light);
    } else {
        set_light(LIGHT_OFF);
    }
}

void oled_handler::flick_low_bat_lig()
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
void oled_handler::disp_mid()
{
    char disp[32];
    snprintf(disp, sizeof(disp), "%02d:%02d:%02d",
             mRecInfo->rec_hour, mRecInfo->rec_min, mRecInfo->rec_sec);
//    Log.d(TAG," disp rec mid %s tl_count %d",disp,tl_count);
    if (tl_count == -1) {
        disp_str((const u8 *)disp, 37, 24);
    }
}

bool oled_handler::is_bat_low()
{
    bool ret = false;
    if (mBatInterface->isSuc() && !m_bat_info_->bCharge && m_bat_info_->battery_level <= BAT_LOW_VAL) {
        ret = true;
    }
    return ret;
}

bool oled_handler::check_bat_protect()
{
    bool bRet = false;
#if 0
//    Log.d(TAG," check_bat_protect (%d %d %d)",
//          cur_menu,m_bat_info_->bCharge ,m_bat_info_->battery_level);
    if(cur_menu != MENU_LOW_PROTECT)
    {
        if(!m_bat_info_->bCharge && m_bat_info_->battery_level <= BAT_LOW_PROTECT)
        {
            Log.d(TAG,"low bat protect %d",
                  m_bat_info_->battery_level);
            set_cur_menu(MENU_LOW_PROTECT);
            func_low_protect();
            bRet = true;
        }
    }
    else
    {
        if(m_bat_info_->bCharge || m_bat_info_->battery_level > BAT_LOW_PROTECT)
        {
            Log.d(TAG,"exit low protect(%d %d)",m_bat_info_->bCharge,
                  m_bat_info_->battery_level);
            func_back();
        }
    }
#endif
    return bRet;
}

void oled_handler::func_low_bat()
{
#if 1
    send_option_to_fifo(ACTION_LOW_BAT, REBOOT_SHUTDOWN);
#else
    int times = 10;
    bool bSend = true;
    bool bCharge;
    Log.d(TAG,"func_low_bat");
    for(int i = 0; i < times; i++)
    {
        disp_sec(times - i,52,48);
        msg_util::sleep_ms(1000);
        if (get_battery_charging(&bCharge) == 0)
        {
            if (bCharge)
            {
                m_bat_info_->bCharge = bCharge;
                bSend = false;
                break;
            }
        }
    }
    Log.d(TAG,"func_low_bat2 %d",bSend);
    if(bSend)
    {
        send_option_to_fifo(ACTION_LOW_BAT,REBOOT_SHUTDOWN);
        clear_area();
        disp_str((const u8 *)"Power off...",8,16);
        cam_state = STATE_IDLE;
    }
    else
    {
        rm_state(STATE_LOW_BAT);
//        //update bat info
//        oled_disp_battery();
        // back for battery charge
        func_back();
    }
    Log.d(TAG,"func bat low %d menu %d state 0x%x",bSend,cur_menu,cam_state);
#endif
}


void oled_handler::set_light_direct(u8 val)
{
    if (last_light != val) {
        last_light = val;
        mOLEDLight->set_light_val(val);
    }
}

void oled_handler::set_light(u8 val)
{
#ifdef ENABLE_LIGHT
    if (get_setting_select(SET_LIGHT_ON) == 1) {
        set_light_direct(val | front_light);
    }
#endif
}



/*
 * check_rec_tl - 检查录像的剩余时间
 */
bool oled_handler::check_rec_tl()
{
    bool ret = false;
    if (mControlAct != nullptr) {
        if (mControlAct->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
            ret = true;
        }
    } else {
        int item = get_menu_select_by_power(MENU_VIDEO_SET_DEF);
        switch (item) {
            case VID_8K_50M_30FPS_PANO_RTS_OFF:
            case VID_6K_50M_30FPS_3D_RTS_OFF:
            case VID_4K_50M_120FPS_PANO_RTS_OFF:
            case VID_4K_20M_60FPS_3D_RTS_OFF:
            case VID_4K_20M_24FPS_3D_50M_24FPS_RTS_ON:
            case VID_4K_20M_24FPS_PANO_50M_30FPS_RTS_ON:
			#ifndef ARIAL_LIVE
                case VID_ARIAL:
			#endif
//                Log.d(TAG,"mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
                if (mVIDAction[item].stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
                    ret = true;
                }
                break;
				
            case VID_CUSTOM:
                Log.d(TAG, "mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int %d",
                      mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int);
                if (mProCfg->get_def_info(KEY_VIDEO_DEF)->stOrgInfo.stOrgAct.mOrgV.tim_lap_int > 0) {
                    if (mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act == 0) {
                        mProCfg->get_def_info(KEY_VIDEO_DEF)->size_per_act = 10;
                    }
                    ret = true;
                }
                break;
            SWITCH_DEF_ERROR(item)
        }
    }

    if (!ret) {
        tl_count = -1;
    }
    return ret;
}




/*************************************************************************
** 方法名称: handleMessage
** 方法功能: 消息处理
** 入口参数: msg - 消息对象强指针引用
** 返 回 值: 无 
** 调     用: 消息处理线程
**
*************************************************************************/
void oled_handler::handleMessage(const sp<ARMessage> &msg)
{
    uint32_t what = msg->what();
	Log.d(TAG, "what %d", what);

    if (OLED_EXIT == what) {	/* 退出消息处理循环 */
        exitAll();
    } else {
        switch (what) {
            case OLED_DISP_STR_TYPE:
			{
	            unique_lock<mutex> lock(mutexState);
	            sp<DISP_TYPE> disp_type;
	            CHECK_EQ(msg->find<sp<DISP_TYPE>>("disp_type", &disp_type), true);
					
                Log.d(TAG, "OLED_DISP_STR_TYPE (%d %d %d 0x%x)",
                      disp_type->qr_type, disp_type->type, cur_menu, cam_state);

                switch (cur_menu) {
                    case MENU_DISP_MSG_BOX:
                    case MENU_SPEED_TEST:
                        func_back();
                        break;
					
                    case MENU_LOW_BAT:
                        if (!(disp_type->type == START_LOW_BAT_SUC || START_LOW_BAT_FAIL == disp_type->type || disp_type->type == RESET_ALL || disp_type->type == START_FORCE_IDLE)) { 
                            Log.d(TAG,"MENU_LOW_BAT not allow (0x%x %d)",cam_state,disp_type->type);
                            return;
                        }

                    default:
                        if (disp_type->type != RESET_ALL) {
                            exit_sys_err();
                        }
                        break;
                }
					
                //add param from controller or qr scan
                if (disp_type->qr_type != -1) {
                    CHECK_NE(disp_type->mAct, nullptr);
                    add_qr_res(disp_type->qr_type, disp_type->mAct, disp_type->control_act);
                } else if (disp_type->tl_count != -1) {
                    set_tl_count(disp_type->tl_count);
                }
					
                oled_disp_type(disp_type->type);
				break;
            }
                

			/*
			 * 显示错误信息 - 含有错误类型和出错码
			 */
            case OLED_DISP_ERR_TYPE:
            {
                unique_lock<mutex> lock(mutexState);
                sp<ERR_TYPE_INFO> mErrInfo;
                CHECK_EQ(msg->find<sp<ERR_TYPE_INFO>>("err_type_info", &mErrInfo), true);

                switch (cur_menu) {
                    case MENU_DISP_MSG_BOX:
                    case MENU_SPEED_TEST:
                        func_back();
                        break;
					
                    default:
                        if (mErrInfo->type != RESET_ALL) {
                            exit_sys_err();
                        }
                        break;
                }
                oled_disp_err(mErrInfo);
				break;
            }


			/*
			 * 处理得到的按键消息
			 */
			case OLED_GET_KEY:	/* 接收到按键消息 */
            {
                int key = -1;
                CHECK_EQ(msg->find<int>("oled_key", &key), true);
                handle_oled_key(key);
                break;
            }

			/*
			 * 处理长按消息
			 */
            case OLED_GET_LONG_PRESS_KEY:	/* 长按键消息:  只响应电源/确认键的长按消息(关机) */
            {
                int key;
                int64 ts;
                CHECK_EQ(msg->find<int>("key", &key), true);
                CHECK_EQ(msg->find<int64>("ts", &ts), true);
                {
                    //unique_lock<mutex> lock(mutexKey);
					Log.d(TAG, "long press key 0x%x last_down_key 0x%x ts %lld last_key_ts %lld\n",
                              						key, last_down_key, ts, last_key_ts);
                    if (ts == last_key_ts && last_down_key == key) {
                        Log.d(TAG, " long press key 0x%x\n", key);
                        if (key == OLED_KEY_POWER) {
                            sys_reboot();
                        }
                    }
                }
                break;
            }

			/* 显示IP地址 -   (状态栏处理) */
            case OLED_DISP_IP: {   /* 显示IP消息 */
                int ip;
                CHECK_EQ(msg->find<int>("ip", &ip), true);
                oled_disp_ip((unsigned int)ip);
                break;
            }

			/*
			 * 配置WIFI (UI-CORE处理)
			 */
            case OLED_CONFIG_WIFI:
            {
                sp<WIFI_CONFIG> mConfig;
                CHECK_EQ(msg->find<sp<WIFI_CONFIG>>("wifi_config", &mConfig), true);
                wifi_config(mConfig);
            }
                break;

			/*
			 * 写SN (UI-CORE处理)
			 */
            case OLED_SET_SN:	/* 设置SN */
            {
                sp<SYS_INFO> mSysInfo;
                CHECK_EQ(msg->find<sp<SYS_INFO>>("sys_info", &mSysInfo), true);
                write_sys_info(mSysInfo);
                break;
            }

			/*
			 * 同步初始化
			 */
            case OLED_SYNC_INIT_INFO:	/* 同步初始化信息(来自control_center) */
            {
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
            case OLED_DISP_INIT:	/* 1.初始化显示消息 */
            {
                oled_init_disp();	/* 初始化显示 */
                send_option_to_fifo(ACTION_REQ_SYNC);	/* 发送请求同步消息 */
                break;
            }


			/*
			 * 更新存储设备列表(来自Netlink)
			 */
            case OLED_UPDATE_DEV_INFO: {
                vector<sp<USB_DEV_INFO>> mList;
                CHECK_EQ(msg->find<vector<sp<USB_DEV_INFO>>>("dev_list", &mList), true);
                set_mdev_list(mList);
				
                // make send dev_list after sent save for update_bottom while current is pic ,vid menu
                send_update_dev_list(mList);
                break;
            }


			/*
			 * 更新显示时间(只有在录像,直播的UI)
			 */
            case OLED_UPDATE_MID:
            {
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
                    ERR_MENU_STATE(cur_menu,cam_state);
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
            case OLED_READ_BAT:
            {
                unique_lock<mutex> lock(mutexState);
                check_battery_change();
                break;
            }

			/*
			 * 灯显示
			 */
            case OLED_DISP_LIGHT:
            {
                int menu;
                int interval;
                int state;

                CHECK_EQ(msg->find<int>("menu", &menu), true);
                CHECK_EQ(msg->find<int>("interval", &interval), true);
                CHECK_EQ(msg->find<int>("state", &state), true);
//                Log.d(TAG," (%d %d  %d %d)", menu,state,interval,  cap_delay);
                bSendUpdate = false;
                {
                    unique_lock<mutex> lock(mutexState);
                    {
                        switch (menu) {
                            case MENU_PIC_INFO:
                                if (check_state_in(STATE_TAKE_CAPTURE_IN_PROCESS)) {
                                    if (cap_delay == 0) {
                                        if (menu == cur_menu) {
                                            disp_shooting();
                                        }
                                    } else {
                                        if (menu == cur_menu) {
                                            disp_sec(cap_delay, 52, 24);
                                        }
#ifdef ENABLE_SOUND
                                        send_update_light(menu, state, INTERVAL_1HZ, true, SND_ONE_T);
#endif
                                    }
                                    cap_delay--;
                                } else if (check_state_in(STATE_PIC_STITCHING)) {
                                    send_update_light(menu, state, INTERVAL_5HZ, true);
                                } else {
                                    Log.d(TAG,"update pic light error state 0x%x",cam_state);
                                    set_light();
                                }
                                break;
								
                            case MENU_CALIBRATION:
                                Log.d(TAG,"cap_delay is %d",cap_delay);
                                if ((cam_state & STATE_CALIBRATING) == STATE_CALIBRATING) {
                                    if (cap_delay <= 0) {
                                        send_update_light(menu, state, INTERVAL_5HZ, true);
                                        if (cap_delay == 0) {
                                            if (cur_menu == menu) {
                                                disp_calibration_res(2);
                                            }
                                        }
                                    } else {
#ifdef ENABLE_SOUND
                                        send_update_light(menu, state, INTERVAL_1HZ, true, SND_ONE_T);
#endif
                                        if (cur_menu == menu) {
                                            disp_calibration_res(3, cap_delay);
                                        }
                                    }
                                    cap_delay--;
                                } else {
                                    Log.d(TAG,"update calibration light error state 0x%x",cam_state);
                                    set_light();
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
                break;

			/*
			 * 清除消息框
			 */
            case OLED_CLEAR_MSG_BOX:
                if (cur_menu == MENU_DISP_MSG_BOX) {
                    //back from msg box quickly
                    func_back();
                } else {
                    Log.d(TAG,"other cur_menu %d",cur_menu);
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
void oled_handler::sys_reboot(int cmd)
{
    switch (cmd) {
	case REBOOT_NORMAL:
        send_option_to_fifo(ACTION_POWER_OFF, cmd);
        break;
		
    case REBOOT_SHUTDOWN:
        send_option_to_fifo(ACTION_POWER_OFF, cmd);
        break;
	
    SWITCH_DEF_ERROR(cmd)
    }
}


void oled_handler::set_cap_delay(int delay)
{
    cap_delay = delay;
}

bool oled_handler::check_live_save_org()
{
    bool ret = false;
    if (check_state_in(STATE_LIVE)) {
        if (mControlAct != nullptr) {
            Log.d(TAG, "check_live_save_org mControlAct->stOrgInfo.save_org  "
                          "is %d", mControlAct->stOrgInfo.save_org);

            if (mControlAct->stOrgInfo.save_org != SAVE_OFF) {
                ret = true;
            }
        } else {
            int item = get_menu_select_by_power(MENU_LIVE_SET_DEF);
            Log.d(TAG, "check_live_save_org %d", item);
            switch (item) {

				#ifdef ARIAL_LIVE
                case VID_ARIAL:
                    ret = true;
				#endif
                    break;

				case LIVE_CUSTOM:
                    Log.d(TAG, "check_live_save_org mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org  is %d",
                          mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org);
                    if (mProCfg->get_def_info(KEY_LIVE_DEF)->stOrgInfo.save_org != SAVE_OFF)
                    {
                        ret = true;
                    }
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
bool oled_handler::check_battery_change(bool bUpload)
{

#ifndef DISABLE_BATTERY_CKECK


    bool bUpdate = mBatInterface->read_bat_update(m_bat_info_);
	
	Log.d(TAG,"mBatInterface isSuc %d", mBatInterface->isSuc());
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

    if (is_bat_low()) {	/* 如果电池的电量过低 */
		#ifdef OPEN_BAT_LOW
        if (cur_menu != MENU_LOW_BAT) {	/* 如果当前显示的菜单非电池电量低菜单 */
			
            Log.d(TAG, "bat low menu %d state 0x%x", cur_menu, cam_state);

			if (check_state_in(STATE_RECORD) || check_live_save_org()) {
                set_cur_menu(MENU_LOW_BAT, MENU_TOP);
                add_state(STATE_LOW_BAT);
                func_low_bat();
            }
        }
		#endif
    }

	/* 发送读取电池延时消息 */
    send_delay_msg(OLED_READ_BAT, BAT_INTERVAL);

    return bUpdate;

#else
	return false;
#endif
	
}


int oled_handler::get_battery_charging(bool *bCharge)
{
    return mBatInterface->read_charge(bCharge);
}

int oled_handler::read_tmp(double *int_tmp, double *tmp)
{
    return mBatInterface->read_tmp(int_tmp,tmp);
}

void oled_handler::deinit()
{
    Log.d(TAG, "deinit\n");
	
    set_light_direct(LIGHT_OFF);
    mDevManager = nullptr;
    mpNetManager = nullptr;

    stop_poll_thread();

    sendExit();

    Log.d(TAG, "deinit2");
}


