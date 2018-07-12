#ifndef _ACTION_INFO_H_
#define _ACTION_INFO_H_

#define CONTROL_SET_CUSTOM (10)

typedef struct _vid_org_ {
    int org_fr;
	
    //M/s
    int org_br;
    int logMode;

	// s
    int tim_lap_int;
    int aud_gain;
} VID_ORG;

typedef struct _live_org_ {
    int org_fr;
    //M/s
    int org_br;
    int logMode;
    int aud_gain;
} LIVE_ORG;

typedef struct _pic_org_ {
    int hdr_count;
    int min_ev;
    int max_ev;
    int burst_count;
} PIC_ORG;

typedef union _org_act_ {
    PIC_ORG mOrgP;
    VID_ORG mOrgV;
    LIVE_ORG mOrgL;
} ORG_ACT;

typedef struct _org_info_ {
    //jpeg,
    int mime;
    int save_org;
    int w;
    int h;
	int locMode;		/* 临时新增 */
    ORG_ACT stOrgAct;
} ORG_INFO;

typedef struct _sti_vid_ {
    int sti_fr;
    int sti_br;
} STI_VID;

typedef struct _sti_live_ {
    int sti_fr;
    int sti_br;
    int hdmi_on;
    int file_save;
    char url[4096];
    char format[32];
} STI_LIVE;

typedef union _sti_act_ {
    STI_LIVE mStiL;
    STI_VID mStiV;
} STI_ACT;

typedef struct _sti_info_ {
    int mime;
    int stich_mode;
//    int sti_res;
    int w;
    int h;
    STI_ACT stStiAct;
} STI_INFO;


//"audio":{"mime":string,"sampleFormat":string,"channelLayout":string,samplerate:int,bitrate:int}},
typedef struct _aud_info_ {
    char mime[8];
    char sample_fmt[8];
    char ch_layout[16];
    int sample_rate;
    //k/s
    int br;
} AUD_INFO;

typedef struct _cam_prop_ {
    int audio_gain;//def is 96
    char len_param[1024];
    char mGammaData[4096];   
} CAM_PROP;

// used for default start
typedef struct _action_info_ {
    int mode;
    // pic size or rec M/s
    int size_per_act;
    int delay;
    ORG_INFO stOrgInfo;
    STI_INFO stStiInfo;
    CAM_PROP stProp;
    AUD_INFO stAudInfo;
    // 0 -- normal 1 -- cube ,2 optical flow
} ACTION_INFO;

//{"flicker":0,"speaker":0,"led_on":0,"fan_n":0,"aud_on":0,"aud_spatial":0,"set_logo":0,}
typedef struct _sys_setting_ {
    int flicker;
    int speaker;
    int led_on;
    int fan_on;
    int aud_on;
    int aud_spatial;
    int set_logo;
    int gyro_on;
    int video_fragment;
} SYS_SETTING;

typedef struct _stich_progress_ {
    int total_cnt;
    int successful_cnt;
    int failing_cnt;
    int task_over;
    double runing_task_progress;
} STICH_PROGRESS;



typedef struct _disp_type_ {
    int type;			// oled_disp_type
    
    //info according to type
    int qr_type; 		// pic, vid or live
    int control_act; 	// control req or save_to_customer
    int tl_count; 		// timelapse
    sp<ACTION_INFO> mAct;

#ifdef ENABLE_STICH_BOX	
    sp<STICH_PROGRESS> mStichProgress;
#endif

    sp<SYS_SETTING> mSysSetting;
	
} DISP_TYPE;

typedef struct _wifi_config_ {
    char ssid[128];
    char pwd[64];
    //wait time
    int sec;
    int bopen;
} WIFI_CONFIG;

typedef struct _err_type_info_ {
    int type;
    int err_code;
} ERR_TYPE_INFO;


#endif 	/* _ACTION_INFO_H_ */
