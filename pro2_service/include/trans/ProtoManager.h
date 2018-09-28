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

#include "http_util.h"

// 此处必须用function类，typedef再后面函数指针赋值无效
using ReqCallback = std::function<void (std::string)>;


/*
 * 传输管理器对象 - 负责提供与服务器交互接口(使用http)
 */
class ProtoManager {

public:
    virtual                         ~ProtoManager();

    static ProtoManager*            Instance();                         /* 状态机实例 */

    /* 查询服务器的状态 */
    std::string     getServerState();

    /* 设置服务器的状态 */
    bool            setServerState();


    /* 启动预览 */
    bool            sendStartPreview(Json::Value& startPreviewReq);


    /* 停止预览 */
    bool            sendStopPreview(Json::Value& stopPreviewReq);


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

    /* 查询GPS状态 */
    bool            sendQueryGpsStateReq(Json::Value& queryGpsStateReq);

    /* 更新Timelapse的剩余值 */
    bool            sendUpdateTimeLapseLeftReq(Json::Value& updateTlLeftReq);

    /* 更新录像,直播的剩余时间 */
    bool            sendUpdateRecLeftTimeReq(Json::Value& updateRecLeftReq);

    /* 查询小卡容量：同步返回(预览状态下大概需要40ms) */
    bool            sendUpdateRecLeftTimeReq(Json::Value& updateRecLeftReq);

    /* 格式化小卡： 单独格式化一张;或者格式化所有 */
    bool            sendFormatmSDReq(Json::Value& formatmSDReq);


    /* 启动测速 */
    bool            sendSpeedTestReq(Json::Value& speedTestReq);


    /* 查询进入U盘模式 */
    bool            sendEnterUdiskModeReq(Json::Value& enterUdiskModeReq);


    /* 启动陀螺仪校正 */
    bool            sendGyroCalcReq(Json::Value& gyroCalcReq);


    /* 更新存储路径： */
    bool            sendSavePathChangeReq(Json::Value& savePathChangeReq);

    /* 发送存储设备列表 */
    bool            sendStorageListReq(Json::Value& storageListReq);

    /* 请求同步 */
    bool            sendStateSyncReq(Json::Value& stateSyncReq);

    /* 白平衡校正 */
    bool            sendWbCalcReq(Json::Value& wbCalcReq);
    

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
	
    int             s_exit_flag;

	ReqCallback     s_req_callback;
    Json::Value     mCurRecvData;       /* 用于存储当前请求接收到的数据(json) */

                    ProtoManager()
    void            sendHttpSyncReq(const std::string &url, ReqCallback req_callback);
    void            sendHttpSyncReq(const std::string &url, ReqCallback req_callback, const char* extra_headers, const char* post_data);

    void            onHttpEvent(mg_connection *connection, int event_type, void *event_data);
};


#endif /* _TRAN_MANAGER_H_ */