#ifndef PROJECT_PRO_CFG_H
#define PROJECT_PRO_CFG_H
#include <common/sp.h>
#include <sys/ins_types.h>

enum {
    MODE_3D,
    MODE_PANO,
    MODE_MAX,
};

enum {
    SET_ORG_DEF,
    SET_ORG_RAW,
    SET_ORG_OFF,
    SET_ORG_MAX
};

enum {
    SET_PIC_MODE,
    SET_PIC_SAVE,
    SET_PIC_RTS,
    SET_PIC_STICH_RES,
    SET_PIC_MAX,
};

typedef enum _res_stich_pic {
//    STITCH_PIC_OFF,
    //3d:7680*7680, pano:7680*3840
    STITCH_PIC_7680X7680_3840,
    STITCH_PIC_5760X5760_2880,
    STITCH_PIC_3840X3840_1920,
    STITCH_PIC_MAX,
} RES_STITCH_PIC;

//video start
enum {
    SET_VID_MODE,
    SET_VID_SAVE,
    SET_VID_ORG_RES,
    SET_VID_RTS,
    SET_VID_STI_RES,
    SET_VID_ENC,
    SET_VID_PAL_NTSC,
    SET_VID_BR,
    SET_VID_MAX,
};

typedef enum _br_video {
    BR_30M_REC,
    BR_20M_REC,
    BR_10M_REC,
    BR_VIDEO_MAX,
} BR_VIDEO;

typedef enum _org_res_video_pano {
    RES_VIDEO_3840x2160,
    RES_VIDEO_2560x1440,
    RES_VIDEO_1920x1080,
    //best for 3d
    RES_VIDEO_3200x2400,
    RES_VIDEO_2160x1620,
    RES_VIDEO_1920x1440,
    RES_VIDEO_PANO_MAX,
} ORG_RES_VIDEO_PANO;

typedef enum _org_res_video_3D {
    RES_VIDEO_3200x2400_3D,
    RES_VIDEO_2160x1620_3D,
    RES_VIDEO_1920x1440_3D,
    RES_VIDEO_3D_MAX,
} ORG_RES_VIDEO_3D;

// both video and live use same stitch res
typedef enum _res_stich_video {
//    STITCH_VIDEO_OFF,
    STITCH_VIDEO_3840X3840_1920,
    STITCH_VIDEO_2880X2880_1440,
    STITCH_VIDEO_1920X1920_960,
    STITCH_VIDEO_MAX
} RES_STITCH_VIDEO;

typedef enum _fr_num {
    FR_NTSC_30,
    FR_NTSC_24,
    FR_PAL_25,
} FR_NUM;


//live start
enum {
    SET_LIVE_MODE,
    SET_LIVE_PROJECT,
    SET_LIVE_HDMI,
    SET_LIVE_ENC,
    SET_LIVE_STI_RES,
    SET_LIVE_PAL_NTSC,
    SET_LIVE_BR,
    SET_LIVE_MAX,
};

typedef enum _br_live_ {
    BR_20M_LIVE,
    BR_10M_LIVE,
    BR_8M,
    BR_5M,
    BR_3M,
    BR_LIVE_MAX
} BR_LIVE;

typedef enum _res_stich_live {
//    STITCH_VIDEO_OFF,
    STITCH_LIVE_3840X3840_1920,
    STITCH_LIVE_2880X2880_1440,
    STITCH_LIVE_1920X1920_960,
    STITCH_LIVE_MAX
} RES_STITCH_LIVE;
// live end

enum {
    KEY_WIFI_SSID,
    KEY_WIFI_PWD,
};


enum {
    /*
     * Setting Item
     */
    KEY_ALLCARD_PIC_DEF,    /* KEY_PIC_DEF -> KEY_ALLCARD_PIC_DEF */
    KEY_ALLCVIDEO_DEF,
    KEY_LIVE_DEF,
	KEY_PAL_NTSC,           //flicker
    KEY_LAN,
    KEY_SPEAKER,            /* Speaker */
    KEY_SET_LOGO,           /* Logo */
    KEY_LIGHT_ON,
    KEY_DHCP,               /* DHCP/Static */
    KEY_WIFI_ON,            /* WIFI */
    KEY_FAN,                /* Fan */
    KEY_AUD_ON,             /* Audio */
    KEY_AUD_SPATIAL,        /* Audio Spatial */
    KEY_GYRO_ON,
    KEY_VID_SEG,            /* Video Segement */
    KEY_WIFI_AP,
//    KEY_SAVE_PATH,

    /**
     * Action Info 
     */
    //above is int vale
    // pic
    KEY_PIC_MODE,
    KEY_PIC_SIZE_PER_ACT,
    KEY_PIC_DELAY,
    KEY_PIC_ORG_MIME,
    KEY_PIC_ORG_SAVE,
    KEY_PIC_ORG_W,
    KEY_PIC_ORG_H,
    KEY_PIC_HDR_COUNT,
    KEY_PIC_MIN_EV,
    KEY_PIC_MAX_EV,
    KEY_PIC_BURST_COUNT,
    KEY_PIC_STI_MIME,
    KEY_PIC_STI_MODE,
    KEY_PIC_STI_W,
    KEY_PIC_STI_H,
	KEY_PIC_LEN_PARAM,
	KEY_PIC_GAMMA,
    
    //video
    KEY_VID_MODE,
    KEY_VID_SIZE_PER_ACT,
    KEY_VID_DELAY,
    KEY_VID_ORG_MIME,
    KEY_VID_ORG_SAVE,
    KEY_VID_ORG_W,
    KEY_VID_ORG_H,
    KEY_VID_ORG_FR,
    KEY_VID_ORG_BR,
    KEY_VID_LOG,
    KEY_VID_TL,
    KEY_VID_STI_MIME,
    KEY_VID_STI_MODE,
    KEY_VID_STI_W,
    KEY_VID_STI_H,
    KEY_VID_STI_FR,
    KEY_VID_STI_BR,
    KEY_VID_AUD_GAIN,
    KEY_VID_AUD_MIME,
    KEY_VID_AUD_SAMPLE_FMT,
    KEY_VID_AUD_CH_LAYOUT,
    KEY_VID_AUD_SR,
    KEY_VID_AUD_BR,
	KEY_VID_LEN_PARAM,
	KEY_VID_GAMMA,
    
    //live
    KEY_LIVE_MODE,
    KEY_LIVE_SIZE_PER_ACT,
    KEY_LIVE_DELAY,
    KEY_LIVE_ORG_MIME,
    KEY_LIVE_ORG_SAVE,
    KEY_LIVE_ORG_W,
    KEY_LIVE_ORG_H,
    KEY_LIVE_ORG_FR,
    KEY_LIVE_ORG_BR,
    KEY_LIVE_LOG,
    KEY_LIVE_STI_MIME,
    KEY_LIVE_STI_MODE,
    KEY_LIVE_STI_W,
    KEY_LIVE_STI_H,
    KEY_LIVE_STI_FR,
    KEY_LIVE_STI_BR,
    KEY_LIVE_STI_HDMI,
    KEY_LIVE_FILE_SAVE,
    KEY_LIVE_STI_URL,
    KEY_LIVE_FORMAT,
    KEY_LIVE_AUD_GAIN,
    KEY_LIVE_AUD_MIME,
    KEY_LIVE_AUD_SAMPLE_FMT,
    KEY_LIVE_AUD_CH_LAYOUT,
    KEY_LIVE_AUD_SR,
    KEY_LIVE_AUD_BR,
	KEY_LIVE_LEN_PARAM,
	KEY_LIVE_GAMMA,
    
    KEY_CFG_MAX,
};


#define NONE_PATH "none"

struct _oled_cur_info_;
struct _action_info_;
struct _wifi_config_;

class pro_cfg {
public:
    explicit pro_cfg();
    ~pro_cfg();

    const u8 *get_str(u32 iIndex);
    bool check_key_valid(u32 key);
    void set_val(u32 key, int val);
    int get_val(u32 key);
    void set_cur_lan(int val);
    void update_val(int type, int val);
    void update_val(int type,const char *val);

    void set_def_info(int type, int val = -1, sp<struct _action_info_> mActInfo = nullptr);
    struct _action_info_ * get_def_info(int type);
    void reset_all(bool deleteCfg = true);
    void read_wifi_cfg(sp<struct _wifi_config_> &mCfg);
    void update_wifi_cfg(sp<struct _wifi_config_> &mCfg);

private:
    void init();
    void deinit();
    void create_user_cfg();
    void read_user_cfg();
    void read_def_cfg();
    void update_act_info(int iIndex);
    void read_cfg(const char *name );
    void read_wifi_cfg();
    int reset_def();

    sp<struct _oled_cur_info_> mCurInfo;
};


#endif /* PROJECT_PRO_CFG_H */
