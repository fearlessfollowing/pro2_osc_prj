//
// Created by vans on 16-12-22.
//
#include <common/include_common.h>
#include <sys/pro_cfg.h>
#include <hw/lan.h>
#include <log/stlog.h>
#include <util/util.h>
#include <sys/action_info.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



using namespace std;
#define TAG "pro_cfg"
#define FILE_SIZE (8192)

static const char *user_cfg = "/data/etc/user_cfg";
static const char *def_cfg = "/data/etc/def_cfg";
static const char *wifi_cfg = "/data/etc/wifi_cfg";

// live end
typedef struct _oled_cur_info_
{
    // pal -- 0, ntsc -- 1
    //save all the int val
    int cfg_val[KEY_PIC_MODE];
    //save_path
//    char path[128];
    ACTION_INFO mActInfo[KEY_LIVE_DEF + 1];
}OLED_CUR_INFO;

static const AUD_INFO def_aud = {
        "aac",
        "s16",
        "stereo",
        48000,
        128
};

static const char *wifi_key[] =
{
    "wifi_ssid:",
    "wifi_pwd:",
};

static const char *key[] =
{
        "def_pic:",
        "def_video:",
        "def_live:",
        "pal_ntsc:",
        "language:",
        "speaker:",
        "set_logo:",
        "light_on:",
        "dhcp:",
        "wifi_on:",
        "fan_on:",
        "aud_on:",
        "aud_spatial:",
        "gyro_on:",
        "wifi_ap:",
        //pic
        "pic_mode:",
        "pic_size_per_act:",
        "pic_delay:",
        "pic_org_mime:",
        "pic_save_org:",
        "pic_org_w:",
        "pic_org_h:",
        "pic_hdr_count:",
        "pic_min_ev:",
        "pic_max_ev:",
        "pic_burst_count:",
        "pic_sti_mime:",
        "pic_sti_mode:",
        "pic_sti_w:",
        "pic_sti_h:",
        //video
        "vid_mode:",
        "vid_size_per_act:",
        "vid_delay:",
        "vid_org_mime:",
        "vid_save_org:",
        "vid_org_w:",
        "vid_org_h:",
        "vid_org_fr:",
        "vid_org_br:",
        "vid_log_mode:",
        "vid_tl:",
        "vid_sti_mime:",
        "vid_sti_mode:",
        "vid_sti_w:",
        "vid_sti_h:",
        "vid_sti_fr:",
        "vid_sti_br:",
        "vid_aud_gain:",
        "vid_aud_mime:",
        "vid_aud_sample_fmt:",
        "vid_aud_ch_layout:",
        "vid_aud_sr:",
        "vid_aud_br:",
        //live
        "live_mode:",
        "live_size_per_act:",
        "live_delay:",
        "live_org_mime:",
        "live_save_org:",
        "live_org_w:",
        "live_org_h:",
        "live_org_fr:",
        "live_org_br:",
        "live_log_mode:",
        "live_sti_mime:",
        "live_sti_mode:",
        "live_sti_w:",
        "live_sti_h:",
        "live_sti_fr:",
        "live_sti_br:",
        "live_sti_hdmi_on:",
        "live_file_save:",
        "live_sti_url:",
        "live_sti_format:",
        "live_aud_gain:",
        "live_aud_mime:",
        "live_aud_sample_fmt:",
        "live_aud_ch_layout:",
        "live_aud_sr:",
        "live_aud_br:",
};

static void int_to_str_val(int val, char *str, int size)
{
    snprintf(str, size, "%d", val);
}

static void int_to_str_val_3d(int val, char *str, int size)
{
    snprintf(str, size, "%03d", val);
}

pro_cfg::pro_cfg()
{
    init();
    read_user_cfg();
//    for (unsigned int i = 0; i < sizeof(gstLan)/sizeof(gstLan[0]);i++)
//    {
//        Log.d(TAG," lan str %s",gstLan[i].lan_array[0]);
//    }
}

pro_cfg::~pro_cfg()
{
    deinit();
}

void pro_cfg::init()
{
    CHECK_EQ(sizeof(key)/sizeof(key[0]),KEY_CFG_MAX);
    mCurInfo = sp<OLED_CUR_INFO>(new OLED_CUR_INFO());
    memset(mCurInfo.get(), 0, sizeof(OLED_CUR_INFO));
}

bool pro_cfg::check_key_valid(u32 key)
{
    if(key >= 0 && key < sizeof(mCurInfo->cfg_val)/sizeof(mCurInfo->cfg_val[0]))
    {
        return true;
    }
    else
    {
        Log.e(TAG,"1error key %d",key);
#ifdef ENABLE_ABORT
        abort();
#endif
    }
    return false;
}
int pro_cfg::get_val(u32 key)
{
    if(check_key_valid(key))
    {
        return mCurInfo->cfg_val[key];
    }
    return 0;
}

void pro_cfg::set_val(u32 key,int val)
{
//    Log.d(TAG, "set key %d val %d", key, val);
    if(check_key_valid(key))
    {
        if(mCurInfo->cfg_val[key] != val)
        {
            mCurInfo->cfg_val[key] = val;
            update_val(key, val);
        }
    }
}

void pro_cfg::update_act_info(int iIndex)
{
    const char *new_line = "\n";
    int fd = open(user_cfg, O_RDWR);
    CHECK_NE(fd, -1);
    char buf[FILE_SIZE];
    char val[512];
    char write_buf[4096];
    memset(buf, 0, sizeof(buf));
    unsigned int write_len = -1;
    unsigned int len = 0;
    u32 read_len = read(fd, buf, sizeof(buf));

    Log.d(TAG, " update_act_info iIndex %d　"
                  "read_len %d strlen buf %d",
          iIndex, read_len,strlen(buf));
//    Log.d(TAG,"buf is %s",buf);
    if(read_len <= 0)
    {
        close(fd);
        create_user_cfg();
        fd = open(user_cfg, O_RDWR);
        CHECK_NE(fd, -1);
        read_len = read(fd, buf, sizeof(buf));
    }

    if (read_len > 0)
    {
        char *pStr,*pStr1;
        int start;
        int end;
        int max = sizeof(val);
        u32 val_start_pos;
        u32 val_end_pos;
        switch(iIndex)
        {
            case KEY_PIC_DEF:
                Log.d(TAG,"update pic save_org %d",
                      mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.save_org);
                start = KEY_PIC_MODE;
                end = KEY_VID_MODE;
                break;
            case KEY_VIDEO_DEF:
                start = KEY_VID_MODE;
                Log.d(TAG," mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_rate );
                if(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_rate == 0)
                {
                    memcpy(&mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }
                end = KEY_VID_AUD_BR + 1;
                break;
            case KEY_LIVE_DEF:
                Log.d(TAG," mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_rate  %d",
                      mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_rate );
                if(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_rate == 0)
                {
                    memcpy(&mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo,&def_aud,sizeof(AUD_INFO));
                }
                start = KEY_LIVE_MODE;
                end = KEY_LIVE_AUD_BR + 1;
                break;
            SWITCH_DEF_ERROR(iIndex)
        }
        memset(write_buf,0,sizeof(write_buf));
        Log.d(TAG,"start is %d end %d KEY_LIVE_AUD_BR %d",start,end, KEY_LIVE_AUD_BR);
        for(int type = start; type < end; type++ )
        {
            switch(type)
            {
                case KEY_PIC_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].mode,val,max);
                    break;
                case KEY_PIC_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].size_per_act,val,max);
                    break;
                case KEY_PIC_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].delay,val,max);
                    break;
                case KEY_PIC_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_PIC_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_PIC_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_PIC_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_PIC_HDR_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count,val,max);
                    break;
                case KEY_PIC_MIN_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev,val,max);
                    break;
                case KEY_PIC_MAX_EV:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev,val,max);
                    break;
                case KEY_PIC_BURST_COUNT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count,val,max);
                    break;
                case KEY_PIC_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_PIC_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_PIC_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_PIC_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.h,val,max);
                    break;
                    //video
                case KEY_VID_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].mode,val,max);
                    break;
                case KEY_VID_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].size_per_act,val,max);
                    break;
                case KEY_VID_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].delay,val,max);
                    break;
                case KEY_VID_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_VID_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_VID_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_VID_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_VID_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr,val,max);
                    break;
                case KEY_VID_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br,val,max);
                    break;
                case KEY_VID_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode,val,max);
                    break;
                case KEY_VID_TL:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int,val,max);
                    break;
                case KEY_VID_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_VID_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_VID_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_VID_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_VID_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr,val,max);
                    break;
                case KEY_VID_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br,val,max);
                    break;
                case KEY_VID_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_VID_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.mime);
                    break;
                case KEY_VID_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_VID_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_VID_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_VID_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.br,val,max);
                    break;
                    //live
                case KEY_LIVE_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].mode,val,max);
                    break;
                case KEY_LIVE_SIZE_PER_ACT:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].size_per_act,val,max);
                    break;
                case KEY_LIVE_DELAY:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].delay,val,max);
                    break;
                case KEY_LIVE_ORG_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.mime,val,max);
                    break;
                case KEY_LIVE_ORG_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.save_org,val,max);
                    break;
                case KEY_LIVE_ORG_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.w,val,max);
                    break;
                case KEY_LIVE_ORG_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.h,val,max);
                    break;
                case KEY_LIVE_ORG_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr,val,max);
                    break;
                case KEY_LIVE_ORG_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br,val,max);
                    break;
                case KEY_LIVE_LOG:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode,val,max);
                    break;
                case KEY_LIVE_STI_MIME:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.mime,val,max);
                    break;
                case KEY_LIVE_STI_MODE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stich_mode,val,max);
                    break;
                case KEY_LIVE_STI_W:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.w,val,max);
                    break;
                case KEY_LIVE_STI_H:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.h,val,max);
                    break;
                case KEY_LIVE_STI_FR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr,val,max);
                    break;
                case KEY_LIVE_STI_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br,val,max);
                    break;
                case KEY_LIVE_STI_HDMI:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on,val,max);
                    break;
                case KEY_LIVE_FILE_SAVE:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save,val,max);
                    break;
                case KEY_LIVE_STI_URL:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.url);
                    break;
                case KEY_LIVE_FORMAT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.format);
                    break;
                case KEY_LIVE_AUD_GAIN:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stProp.audio_gain,val,max);
                    break;
                case KEY_LIVE_AUD_MIME:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.mime);
                    break;
                case KEY_LIVE_AUD_SAMPLE_FMT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_fmt);
                    break;
                case KEY_LIVE_AUD_CH_LAYOUT:
                    snprintf(val,max,"%s",mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.ch_layout);
                    break;
                case KEY_LIVE_AUD_SR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_rate,val,max);
                    break;
                case KEY_LIVE_AUD_BR:
                    int_to_str_val(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.br,val,max);
                    break;
                SWITCH_DEF_ERROR(type)
            }
            strcat(write_buf,key[type]);
            strcat(write_buf,val);
            strcat(write_buf,new_line);
        }
        pStr = strstr(buf,key[start]);
        if(pStr)
        {
            //pStr += strlen(key[start]);
            val_start_pos = (pStr - buf);
            lseek(fd,val_start_pos,SEEK_SET);
            len = strlen(write_buf);
            write_len = write(fd,write_buf,len);
            Log.d(TAG,"val_start_pos is %d write len %d len %d",
                  val_start_pos ,write_len,len);
            if(write_len != len)
            {
                Log.w(TAG, "5 write update_act_info mismatch(%d %d)",
                      write_len, len);
            }
            else
            {
                u32 file_len;
                if(end != KEY_CFG_MAX)
                {
                    pStr1 = strstr(buf,key[end]);
                    if(pStr1)
                    {
                        val_end_pos = (pStr1 - buf);
                        Log.d(TAG,"val_end_pos is %d", val_end_pos);
                        len = (read_len - val_end_pos);
                        write_len = write(fd,&buf[val_end_pos],len);
                        Log.d(TAG,"val_end_pos is %d write len %d len %d",
                              val_start_pos ,write_len,len);
                        if(write_len != len)
                        {
                            Log.w(TAG, "6 write update_act_info mismatch(%d %d)",
                                  write_len, len);
                        }
                        file_len = val_start_pos +
                                       strlen(write_buf) +
                                       write_len;
                        Log.d(TAG,"a write len %d val_end_pos %d "
                                      "file_len %d read_len %d",
                              write_len,val_end_pos, file_len, read_len);
                    }
                    else
                    {
                        Log.e(TAG,"end key %s not found", key[end]);
                    }
                }
                else
                {
                    file_len = val_start_pos + strlen(write_buf);
                }
                Log.d(TAG,"update act info new file len "
                              "%d org read_len %d\n",
                      file_len,read_len);
                //new len less so need ftruncate
                if(file_len < read_len)
                {
                    ftruncate(fd, file_len);
                }
            }
        }
        else
        {
            Log.e(TAG,"update act key %s not found", key[start]);
//            {
//                Log.d(TAG,"update action info to eof");
//                lseek(fd,0,SEEK_END);
//                len = strlen(write_buf);
//                write_len = write(fd,write_buf,len);
//                if(write_len != len)
//                {
//                    Log.w(TAG, "7 write pro_cfg mismatch(%d %d)",
//                          write_len, len);
//                }
//            }
        }

    }
    close(fd);
}

const u8 *pro_cfg::get_str(u32 iIndex)
{
//    Log.d(TAG,"str index %d lan index %d",iIndex,mCurInfo->cur_lan);
    return (const u8 *) (gstStrInfos[iIndex][mCurInfo->cfg_val[KEY_LAN]].dat);
}

struct _action_info_ *pro_cfg::get_def_info(int type)
{
    switch (type)
    {
        case KEY_PIC_DEF:
        case KEY_VIDEO_DEF:
        case KEY_LIVE_DEF:
            return &mCurInfo->mActInfo[type];
        SWITCH_DEF_ERROR(type)
    }
	return NULL;
}

void pro_cfg::set_def_info(int type, int val, sp<struct _action_info_> mActInfo)
{
    if(val != -1)
    {
        set_val(type,val);
    }
    if (mActInfo != nullptr)
    {
//        Log.d(TAG,"set_def_info type %d",type);
        memcpy(&mCurInfo->mActInfo[type],
               mActInfo.get(),
               sizeof(ACTION_INFO));
        update_act_info(type);
//        if(type == KEY_LIVE_DEF)
//        {
//            Log.d(TAG,"url is %s",mCurInfo->mActInfo[type].stStiInfo.stStiAct.mStiL.url);
//            Log.d(TAG,"2url is %s",mActInfo->stStiInfo.stStiAct.mStiL.url);
//        }
    }
}

void pro_cfg::update_wifi_cfg(sp<struct _wifi_config_> &mCfg)
{
    ins_rm_file(wifi_cfg);
    int fd = open(wifi_cfg, O_RDWR|O_CREAT, 0666);
    CHECK_NE(fd, -1);
    char buf[1024];
    int max_key = sizeof(wifi_key) / sizeof(wifi_key[0]);
    u32 write_len;

    for (int i = 0; i < max_key; i++)
    {
        switch (i)
        {
            case KEY_WIFI_SSID:
                snprintf(buf,sizeof(buf),"%s%s\n",wifi_key[i],mCfg->ssid);
                break;
            case KEY_WIFI_PWD:
                snprintf(buf,sizeof(buf),"%s%s",wifi_key[i],mCfg->pwd);
                break;
            default:
                break;
        }
        write_len  = write(fd,buf,strlen(buf));
        Log.d(TAG,"update wifi buf %s",buf);
        if(write_len != strlen(buf))
        {
            Log.e(TAG,"write wifi cfg err (%d %d)",write_len,strlen(buf));
        }
    }
    close(fd);
    Log.d(TAG,"update_wifi_cfg ssid %s pwd %s",mCfg->ssid,mCfg->pwd);
}

void pro_cfg::read_wifi_cfg(sp<struct _wifi_config_> &mCfg)
{
    if(check_path_exist(wifi_cfg))
    {
        int fd = open(wifi_cfg, O_RDWR);
        CHECK_NE(fd, -1);

        char buf[1024];
        int max_key = sizeof(wifi_key) / sizeof(wifi_key[0]);

        lseek(fd,0,SEEK_SET);
        while (read_line(fd, (void *) buf, sizeof(buf)) > 0)
        {
            //skip begging from#
            if (buf[0] == '#' || (buf[0] == '/' && buf[1] == '/'))
            {
                continue;
            }
            for (int i = 0; i < max_key; i++)
            {
                char *pStr = ::strstr(buf, wifi_key[i]);
                if (pStr)
                {
                    pStr += strlen(wifi_key[i]);
//                Log.d(TAG, " %s is %s len %d atoi(pStr) %d",
//                      key[i], pStr, strlen(pStr),atoi(pStr));
                    switch (i)
                    {
                        case KEY_WIFI_SSID:
                            snprintf(mCfg->ssid,sizeof(mCfg->ssid),"%s",pStr);
                            break;
                        case KEY_WIFI_PWD:
                            snprintf(mCfg->pwd,sizeof(mCfg->pwd),"%s",pStr);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        close(fd);
        Log.d(TAG,"ssid %s pwd %s",mCfg->ssid,mCfg->pwd);
    }
    else
    {
        Log.e(TAG,"no %s exist",wifi_cfg);
    }
}

void pro_cfg::read_cfg(const char *name)
{
    int fd = open(name, O_RDWR);
    CHECK_NE(fd, -1);
    char buf[2048];
    int max_key = sizeof(key) / sizeof(key[0]);
//        Log.d(TAG, " max key %d\n", max_key);

    while (read_line(fd, (void *) buf, sizeof(buf)) > 0)
    {
//            Log.d(TAG, "read line len %d\n", iLen);
        //skip begging from#
        if (buf[0] == '#' || (buf[0] == '/' && buf[1] == '/'))
        {
            continue;
        }
        for (int i = 0; i < max_key; i++)
        {
            char *pStr = ::strstr(buf, key[i]);
            if (pStr)
            {
                pStr += strlen(key[i]);
//                Log.d(TAG, " %s is %s len %d atoi(pStr) %d",
//                      key[i], pStr, strlen(pStr),atoi(pStr));
                switch (i)
                {
                    case KEY_PIC_DEF:
                    case KEY_VIDEO_DEF:
                    case KEY_LIVE_DEF:
                    case KEY_PAL_NTSC:
                    case KEY_LAN:
                    case KEY_SPEAKER:
                    case KEY_SET_LOGO:
                    case KEY_LIGHT_ON:
                    case KEY_DHCP:
                    case KEY_WIFI_ON:
                    case KEY_AUD_ON:
                    case KEY_AUD_SPATIAL:
                    case KEY_GYRO_ON:
                    case KEY_FAN:
                    case KEY_WIFI_AP:
                        mCurInfo->cfg_val[i] = atoi(pStr);
                        break;
                    case KEY_PIC_MODE:
                        mCurInfo->mActInfo[KEY_PIC_DEF].mode = atoi(pStr);
                        break;
                    case KEY_PIC_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_PIC_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_PIC_DELAY:
                        mCurInfo->mActInfo[KEY_PIC_DEF].delay = atoi(pStr);
                        break;
                    case KEY_PIC_ORG_MIME:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_PIC_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;
                    case KEY_PIC_ORG_W:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_PIC_ORG_H:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_PIC_HDR_COUNT:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.hdr_count = atoi(pStr);
                        break;
                    case KEY_PIC_MIN_EV:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.min_ev = atoi(pStr);
                        break;
                    case KEY_PIC_MAX_EV:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.max_ev= atoi(pStr);
                        break;
                    case KEY_PIC_BURST_COUNT:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stOrgInfo.stOrgAct.mOrgP.burst_count= atoi(pStr);;
                        break;
                    case KEY_PIC_STI_MIME:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_PIC_STI_MODE:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_PIC_STI_W:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.w = atoi(pStr);
                        break;
                    case KEY_PIC_STI_H:
                        mCurInfo->mActInfo[KEY_PIC_DEF].stStiInfo.h = atoi(pStr);
                        break;
                        //video
                    case KEY_VID_MODE:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].mode = atoi(pStr);
                        break;
                    case KEY_VID_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_VID_DELAY:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].delay = atoi(pStr);
                        break;
                    case KEY_VID_ORG_MIME:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_VID_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.save_org = atoi(pStr);
                        break;
                    case KEY_VID_ORG_W:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_VID_ORG_H:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_VID_ORG_FR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_fr = atoi(pStr);
                        break;
                    case KEY_VID_ORG_BR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.org_br = atoi(pStr);
                        break;
                    case KEY_VID_LOG:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.logMode = atoi(pStr);
                        break;
                    case KEY_VID_TL:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stOrgInfo.stOrgAct.mOrgV.tim_lap_int = atoi(pStr);
                        break;
                    case KEY_VID_STI_MIME:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_VID_STI_MODE:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_VID_STI_W:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.w= atoi(pStr);
                        break;
                    case KEY_VID_STI_H:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.h= atoi(pStr);
                        break;
                    case KEY_VID_STI_FR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_fr = atoi(pStr);
                        break;
                    case KEY_VID_STI_BR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stStiInfo.stStiAct.mStiV.sti_br = atoi(pStr);
                        break;
                    case KEY_VID_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stProp.audio_gain= atoi(pStr);
                        break;
                    case KEY_VID_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.mime),"%s",pStr);
                        break;
                    case KEY_VID_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
                    case KEY_VID_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
                    case KEY_VID_AUD_SR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
                    case KEY_VID_AUD_BR:
                        mCurInfo->mActInfo[KEY_VIDEO_DEF].stAudInfo.br = atoi(pStr);
                        break;
                        //live
                    case KEY_LIVE_MODE:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].mode = atoi(pStr);
                        break;
                    case KEY_LIVE_SIZE_PER_ACT:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].size_per_act = atoi(pStr);
                        break;
                    case KEY_LIVE_DELAY:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].delay = atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_MIME:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.mime = atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_SAVE:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.save_org= atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_W:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.w = atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_H:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.h = atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_FR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_fr = atoi(pStr);
                        break;
                    case KEY_LIVE_ORG_BR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.org_br = atoi(pStr);
                        break;
                    case KEY_LIVE_LOG:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stOrgInfo.stOrgAct.mOrgL.logMode = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_MIME:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.mime = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_MODE:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stich_mode = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_W:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.w = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_H:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.h = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_FR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_fr = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_BR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.sti_br = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_HDMI:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.hdmi_on = atoi(pStr);
                        break;
                    case KEY_LIVE_FILE_SAVE:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.file_save = atoi(pStr);
                        break;
                    case KEY_LIVE_STI_URL:
                        snprintf(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.url,
                                 sizeof(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.url),
                                 "%s",pStr);
                        break;
                    case KEY_LIVE_FORMAT:
                        snprintf(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.format,
                                 sizeof(mCurInfo->mActInfo[KEY_LIVE_DEF].stStiInfo.stStiAct.mStiL.format),
                                 "%s",pStr);
                        break;
                    case KEY_LIVE_AUD_GAIN:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stProp.audio_gain = atoi(pStr);
                        break;
                    case KEY_LIVE_AUD_MIME:
                        snprintf(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.mime,sizeof(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.mime),"%s",pStr);
                        break;
                    case KEY_LIVE_AUD_SAMPLE_FMT:
                        snprintf(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_fmt,sizeof(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_fmt),"%s",pStr);
                        break;
                    case KEY_LIVE_AUD_CH_LAYOUT:
                        snprintf(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.ch_layout,sizeof(mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.ch_layout),"%s",pStr);
                        break;
                    case KEY_LIVE_AUD_SR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.sample_rate = atoi(pStr);
                        break;
                    case KEY_LIVE_AUD_BR:
                        mCurInfo->mActInfo[KEY_LIVE_DEF].stAudInfo.br = atoi(pStr);
                        break;
                    SWITCH_DEF_ERROR(i)
                }
            }
        }
    }
    close(fd);
//    for(u32 i = 0; i < sizeof(mCurInfo->cfg_val)/sizeof(mCurInfo->cfg_val[0]); i++)
//    {
//        Log.d(TAG, " %s val %d", key[i], mCurInfo->cfg_val[i]);
//    }
//
//    Log.d(TAG,"wifi ap is %d",mCurInfo->cfg_val[KEY_WIFI_AP]);
}

void pro_cfg::read_def_cfg()
{
    if (access(def_cfg, R_OK | W_OK) == -1)
    {
        Log.e(TAG, "%s no r/w access", def_cfg);
        create_user_cfg();
    }
    else
    {
        read_cfg(def_cfg);
    }
}

void pro_cfg::create_user_cfg()
{
    char sys_cmd[128];
    Log.e(TAG, "%s no r/w access", user_cfg);

    if (access(def_cfg, R_OK | W_OK) == -1)
    {
        snprintf(sys_cmd, sizeof(sys_cmd), "mkdir -p /data/etc");
        system(sys_cmd);
        snprintf(sys_cmd, sizeof(sys_cmd), "touch %s", def_cfg);
        system(sys_cmd);
    }
//  copy default cf to user
    snprintf(sys_cmd, sizeof(sys_cmd), "cp -pR %s %s", def_cfg, user_cfg);
    system(sys_cmd);
}

void pro_cfg::reset_all()
{
    memset(mCurInfo.get(), 0, sizeof(OLED_CUR_INFO));
    create_user_cfg();
    read_cfg(def_cfg);
}

void pro_cfg::read_user_cfg()
{
    if (access(user_cfg, R_OK | W_OK) == -1)
    {
        create_user_cfg();
    }
    read_cfg(user_cfg);
}

void pro_cfg::update_val(int type, int val)
{
    char acStr[16];
    int_to_str_val_3d(val, acStr, sizeof(acStr));
    update_val(type, (const char *) acStr);
}

void pro_cfg::update_val(int type, const char *val)
{
    const char *new_line = "\n";
    int fd = open(user_cfg, O_RDWR);
    CHECK_NE(fd, -1);
    bool bFound = false;
    char buf[FILE_SIZE];
    memset(buf, 0, sizeof(buf));
    unsigned int write_len = 0;
    unsigned int len = 0;
    u32 read_len = read(fd, buf, sizeof(buf));

//    Log.d(TAG, " update_val key [%d]　%s value %s file len %d "
//                  "strlen buf %d",
//          type, key[type], val, read_len,strlen(buf));

    if (read_len <= 0)
    {
        close(fd);
        create_user_cfg();
        fd = open(user_cfg, O_RDWR);
        CHECK_NE(fd, -1);
        read_len = read(fd, buf, sizeof(buf));
    }
    if(read_len > 0)
    {
        char *pStr = strstr(buf, key[type]);
        if (pStr)
        {
            u32 val_start_pos;
            pStr += strlen(key[type]);
            val_start_pos = pStr - buf;
            lseek(fd, val_start_pos, SEEK_SET);
            len = strlen(val);
            write_len = write(fd, val, len);
            if (write_len != len)
            {
                Log.w(TAG, "0write pro_cfg mismatch(%d %d)",
                      write_len, len);
            }
            bFound = true;
            switch (type)
            {
#if 0
                case KEY_SAVE_PATH:
                {
                u32 val_end_pos;
//                    int next_type = type + 1;
//                    pStr = strstr(pStr, key[next_type]);
                    pStr = strstr(pStr,new_line);
                    val_end_pos = pStr - buf;
                    len = (read_len - val_end_pos);
                    write_len = write(fd, (void *) &buf[val_end_pos], len);
                    if (write_len != len)
                    {
                        Log.w(TAG, "1write pro_cfg mismatch(%d %d)", write_len, len);
                    }
                    else
                    {
                        u32 file_len = val_start_pos + strlen(val) + write_len;
//                        printf("write procfg suc  val_end_pos %d "
//                                      "write_len %d\n", val_end_pos,
//                              write_len);
                        //org len less so need ftruncate
                        if(file_len < read_len)
                        {
                            Log.d(TAG,"new file len %d org read_len %d\n",
                                   file_len,read_len);
                            ftruncate(fd, file_len);
                        }
                    }
                }
                    break;
#endif
                default:
                    break;
            }
        }
    }

    if (!bFound)
    {
        snprintf(buf, sizeof(buf), "%s%s%s", key[type], val, new_line);
        Log.w(TAG, " %s not found just write val %s", key[type], buf);
        lseek(fd, 0, SEEK_END);
        len = strlen(buf);
        write_len = write(fd, buf, len);
        if ( write_len != len)
        {
            Log.w(TAG, "2write pro_cfg mismatch(%d %d)", write_len, len);
        }
    }
    close(fd);
}

void pro_cfg::deinit()
{

}
