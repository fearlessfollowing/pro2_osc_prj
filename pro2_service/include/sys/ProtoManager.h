/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ProtoManager.h
** 功能描述: 协议管理器(提供大部分与服务器交互的协议接口),整个UI程序当作是一个http client
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年9月28日
** 修改记录:
** V1.0			Skymixos		2018-09-28		创建文件，添加注释
******************************************************************************************************/
#ifndef _TRAN_MANAGER_H_
#define _TRAN_MANAGER_H_

#include <string>
#include <string>
#include <functional>
#include <system_properties.h>
#include <sys/VolumeManager.h>

#include <prop_cfg.h>

#include <json/value.h>
#include <json/json.h>

#include <util/http_util.h>
#include <hw/MenuUI.h>


// 此处必须用function类，typedef再后面函数指针赋值无效
using ReqCallback = std::function<void (std::string)>;

enum {
    ERROR_FORMAT_SUC = 0,
    ERROR_FORMAT_REQ_FAILED = -1,
    ERROR_FORMAT_STATE_NOT_ALLOW = -2,
    ERROR_FORMAT_FAILED = -3,
};


enum {
    PROTO_MANAGER_REQ_SUC = 0,
    PROTO_MANAGER_REQ_CONN_FAILED = -1,
    PROTO_MANAGER_REQ_PARSE_REPLY_FAIL = -2,
    PROTO_MANAGER_REQ_CONN_CLOSEED = -3,
};

/*
 * 传输管理器对象 - 负责提供与服务器交互接口(使用http)
 */
class ProtoManager {

public:
    virtual                         ~ProtoManager();

    static ProtoManager*            Instance();                         /* 状态机实例 */


#if 0
    /* 查询服务器的状态 */
    std::string     getServerState();

    /* 设置服务器的状态 */
    bool            setServerState();



    /* 请求拍照 */
    bool            sendTakePicReq(Json::Value& takePicReq);


    /* 请求录像 */
    bool            sendTakeVideoReq(Json::Value& takeVideoReq);


    /* 停止录像 */
    bool            sendStopVideoReq(Json::Value& stopVideoReq);

    /* 请求直播 */
    bool            sendStartLiveReq(Json::Value& startLiveReq);

    /* 停止直播 */
    bool            sendStopLiveReq(Json::Value& startLiveReq);

    /* 设置模板参数 */
    bool            sendSetCustomLensReq(Json::Value& customLensReq);

    /* 拼接校准 */
    bool            sendStichCalcReq(Json::Value& stitchCalcReq);


    /* 更新录像,直播的剩余时间 */
    bool            sendUpdateRecLeftTimeReq(Json::Value& updateRecLeftReq);

    /* 查询小卡容量：同步返回(预览状态下大概需要40ms) */
    bool            sendUpdateRecLeftTimeReq(Json::Value& updateRecLeftReq);

    /* 启动测速 */
    bool            sendSpeedTestReq(Json::Value& speedTestReq);
#endif

    /* 获取服务器的状态 */
    bool            getServerState(int64* saveState);

    /* 启动预览 */
    bool            sendStartPreview();

    /* 停止预览 */
    bool            sendStopPreview();


    /* 查询进入U盘模式（注: 进入U盘模式之前需要禁止InputManager上报,在返回结果之后再使能） */
    bool            sendSwitchUdiskModeReq(bool bEnterExitFlag);

    /* 更新可拍timelapse的剩余值 */
    bool            sendUpdateTakeTimelapseLeft(u32 leftVal);

    /* 更新录像,直播进行的时间及剩余时间 */
    bool            sendUpdateRecordLeftSec(u32 uRecSec, u32 uLeftRecSecs, u32 uLiveSec, u32 uLiveRecLeftSec)


    /* 请求同步 */
    bool            sendStateSyncReq(REQ_SYNC* pReqSyncInfo);

    /* 查询GPS状态 */
    int             sendQueryGpsState();

    /* 格式化小卡： 单独格式化一张;或者格式化所有(进入格式化之前需要禁止InputManager上报) */
    int             sendFormatmSDReq(int iIndex);

#if 0
    /* 启动陀螺仪校正 */
    bool            sendGyroCalcReq(Json::Value& gyroCalcReq);


    /* 更新存储路径： */
    bool            sendSavePathChangeReq(Json::Value& savePathChangeReq);

    /* 发送存储设备列表 */
    bool            sendStorageListReq(Json::Value& storageListReq);


    /* 白平衡校正 */
    bool            sendWbCalcReq(Json::Value& wbCalcReq);
#endif    

    /*------------------------------------- 设置页 -----------------------------------
     * 1.设置视频分段
     * 2.设置底部Logo
     * 3.开关陀螺仪
     * 4.设置音频模式
     * 5.设置风扇的开关
     * 6.设置LOG模式
     * 7.设置Flick
     * -----------------------------
     * 
     */
    bool            sendSetOptionsReq(Json::Value& optionsReq);



private:

    static ProtoManager*    sInstance;


    static int              mSyncReqErrno;
    Mutex                   mSyncReqLock;
    bool                    mSyncReqExitFlag; 

    bool                    mAsyncReqExitFlag;

    Json::Value             mCurRecvData;       /* 用于存储当前请求接收到的数据(json) */
    static Json::Value*     mSaveSyncReqRes;

                    ProtoManager();

    bool            getSyncReqExitFlag();
    void            setSyncReqExitFlag(bool bFlag);

    int             sendHttpSyncReq(const std::string &url, Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, const char* pPostData);

    static void     onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData);
};


#endif /* _TRAN_MANAGER_H_ */