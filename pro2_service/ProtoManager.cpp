/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: ProtoManager.cpp
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
#include <thread>
#include <sys/ins_types.h>
#include <vector>
#include <mutex>
#include <common/sp.h>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <sstream>
#include <sys/Mutex.h>
#include <sys/ProtoManager.h>


/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef    TAG
#define   TAG "ProtoManager"

/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/
#define _name               "name"
#define _param              "parameters"
#define _count              "count"
#define _tl_left            "tl_left"
#define _mode               "mode"
#define _state              "state"
#define _done               "done"
#define _method             "method"
#define _results            "results"
#define _index              "index"
#define _error              "error"
#define _code               "code"
#define _rec_left_sec       "rec_left_sec"
#define _live_rec_left_sec  "live_rec_left_sec"
#define _rec_left_sec       "rec_sec"
#define _live_rec_sec       "live_rec_sec"

#define _origin             "origin"
#define _stitch             "stitch"
#define _audio              "audio"
#define _mime               "mime"
#define _width              "width"
#define _height             "height"
#define _prefix             "prefix"
#define _save_origin        "saveOrigin"
#define _frame_rate         "framerate"
#define _live_auto_connect  "autoConnect"
#define _bit_rate           "bitrate"
#define _duration           "duration"
#define _sample_fmt         "sampleFormat"
#define _channel_layout     "channelLayout"
#define _sample_rate        "samplerate"
#define _file_type          "fileType"



#define REQ_UPDATE_TIMELAPSE_LEFT   "camera._update_tl_left_count"
#define REQ_SWITCH_UDISK_MODE       "camera._change_udisk_mode"
#define REQ_SYNC_INFO               "camera._request_sync"
#define REQ_QUERY_GPS_STATE         "camera._queryGpsStatus"
#define REQ_FORMAT_TFCARD           "camera._formatCameraMoudle"
#define REQ_UPDATE_REC_LIVE_INFO    "camera._update_rec_live_info"

#define REQ_START_PREVIEW           "camera._startPreview"
#define REQ_STOP_PREVIEW            "camera._stopPreview"

#define REQ_TAKE_PIC                "camera._takePicture"
#define REQ_START_REC               "camera._startRecording"
#define REQ_STOP_REC                "camera._stopRecording"
#define REQ_START_LIVE              "camera._startLive"
#define REQ_STOP_LIVE               "camera._stopLive"

#define REQ_QUERY_STORAGE           "camera._queryStorage"

#define REQ_GET_SET_CAM_STATE       "camera._getSetCamState"

/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
ProtoManager *ProtoManager::sInstance = NULL;
static Mutex gProtoManagerMutex;
static Mutex gSyncReqMutex;

static const std::string gReqUrl = "http://127.0.0.1:20000/ui/commands/execute";
static const char* gPExtraHeaders = "Content-Type:application/json\r\nReq-Src:ProtoManager\r\n";     // Req-Src:ProtoManager\r\n

int ProtoManager::mSyncReqErrno = 0;
Json::Value* ProtoManager::mSaveSyncReqRes = NULL;


ProtoManager* ProtoManager::Instance() 
{
    AutoMutex _l(gProtoManagerMutex);
    if (!sInstance)
        sInstance = new ProtoManager();
    return sInstance;
}


ProtoManager::ProtoManager(): mSyncReqExitFlag(false), 
                              mAsyncReqExitFlag(false)
{
    Log.d(TAG, "[%s: %d] Constructor ProtoManager now ...", __FILE__, __LINE__);
    mCurRecvData.clear();
}


ProtoManager::~ProtoManager()
{

}




/*
 * 发送同步请求(支持头部参数及post的数据))),支持超时时间
 */
int ProtoManager::sendHttpSyncReq(const std::string &url, Json::Value* pJsonRes, 
                                    const char* pExtraHeaders, const char* pPostData)
{
	AutoMutex _l(gSyncReqMutex);

    mg_mgr mgr;
    mSaveSyncReqRes = pJsonRes;

	mg_mgr_init(&mgr, NULL);

    struct mg_connection* connection = mg_connect_http(&mgr, onSyncHttpEvent, 
                                                        url.c_str(), pExtraHeaders, pPostData);
	mg_set_protocol_http_websocket(connection);

	Log.d(TAG, "Send http request URL: %s", url.c_str());
	Log.d(TAG, "Extra Headers: %s", pExtraHeaders);
	Log.d(TAG, "Post data: %s", pPostData);

    setSyncReqExitFlag(false);

	while (false == getSyncReqExitFlag())
		mg_mgr_poll(&mgr, 50);

	mg_mgr_free(&mgr);

    return mSyncReqErrno;
}



void ProtoManager::onSyncHttpEvent(mg_connection *conn, int iEventType, void *pEventData)
{
	http_message *hm = (struct http_message *)pEventData;
	int iConnState;
    ProtoManager* pm = ProtoManager::Instance();

	switch (iEventType)  {
	    case MG_EV_CONNECT: {
		    iConnState = *(int *)pEventData;
		    if (iConnState != 0)  {
                Log.e(TAG, "[%s: %d] Connect to Server Failed, Error Code: %d", __FILE__, __LINE__, iConnState);
                pm->setSyncReqExitFlag(true);
                mSyncReqErrno = PROTO_MANAGER_REQ_CONN_FAILED;
		    }
		    break;
        }
	
        case MG_EV_HTTP_REPLY: {
		    Log.d(TAG, "Got reply:\n%.*s\n", (int)hm->body.len, hm->body.p);
            if (mSaveSyncReqRes) {
                Json::Reader reader;
                if (!reader.parse(std::string(hm->body.p, hm->body.len), (*mSaveSyncReqRes), false)) {
                    Log.e(TAG, "[%s: %d] Parse Http Reply Failed!", __FILE__, __LINE__);
                    mSyncReqErrno = PROTO_MANAGER_REQ_PARSE_REPLY_FAIL;
                }
            } else {
                Log.e(TAG, "[%s: %d] Invalid mSaveSyncReqRes, maybe client needn't reply results", __FILE__, __LINE__);
            }
		    conn->flags |= MG_F_SEND_AND_CLOSE;
            pm->setSyncReqExitFlag(true);
            mSyncReqErrno = PROTO_MANAGER_REQ_SUC;
		    break;
	    }

	    case MG_EV_CLOSE: {
		    if (pm->getSyncReqExitFlag() == false) {
			    Log.d(TAG, "[%s: %d] Server closed connection", __FILE__, __LINE__);
                pm->setSyncReqExitFlag(true);
                mSyncReqErrno = PROTO_MANAGER_REQ_CONN_CLOSEED;
		    };
		    break;
        }
	    
        default:
		    break;
	}
}


/* getServerState
 * @param
 * 获取服务器的状态
 * 成功返回值大于0
 */
bool ProtoManager::getServerState(int64* saveState)
{

    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_method] = "get";      /* 获取服务器的状态 */

    root[_name] = REQ_GET_SET_CAM_STATE;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &os);
            resultStr = os.str();
            Log.d(TAG, "getServerState -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    *saveState = jsonRes["value"].asInt64();
                    bRet = true;
                    Log.d(TAG, "[%s: %d] Get Server State: 0x%x", *saveState);
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] getServerState -> Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}


/* sendStartPreview
 * @param 
 * 发送启动预览请求
 */
bool ProtoManager::sendStartPreview()
{
    int iResult = -1;
    bool bRet = false;
    const char* pPreviewMode = NULL;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;
    Json::Value originParam;
    Json::Value stitchParam;
    Json::Value audioParam;
    Json::Value imageParam;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    pPreviewMode = property_get(PROP_PREVIEW_MODE);
    if (pPreviewMode && !strcmp(pPreviewMode, "3d_top_left")) {
        originParam[_mime]          = "h264";
        originParam[_width]         = 1920;
        originParam[_height]        = 1440;
        originParam[_frame_rate]    = 30;
        originParam[_bit_rate]      = 15000;

        stitchParam[_mime]          = "h264";
        stitchParam[_width]         = 1920;
        stitchParam[_height]        = 1920;
        stitchParam[_frame_rate]    = 30;
        stitchParam[_bit_rate]      = 1000;
        stitchParam[_mode]          = pPreviewMode;

    } else {    /* 默认为pano */
        originParam[_mime]          = "h264";
        originParam[_width]         = 1920;
        originParam[_height]        = 1440;
        originParam[_frame_rate]    = 30;
        originParam[_bit_rate]      = 15000;

        stitchParam[_mime]          = "h264";
        stitchParam[_width]         = 1920;
        stitchParam[_height]        = 960;
        stitchParam[_frame_rate]    = 30;
        stitchParam[_bit_rate]      = 1000;
    }

    audioParam[_mime]               = "aac";
    audioParam[_sample_fmt]         = "s16";
    audioParam[_channel_layout]     = "stereo";
    audioParam[_sample_rate]        = 48000;
    audioParam[_bit_rate]           = 128;


#ifdef ENABLE_PREVIEW_STABLE
    param["stabilization"] = true;
#endif

#ifdef ENABLE_PREVIEW_IMAGE_PROPERTY
    imageParam['sharpness'] = 4
    // imageParam['wb'] = 0
    // imageParam['iso_value'] = 0
    // imageParam['shutter_value']= 0
    // imageParam['brightness']= 0
    imageParam['contrast']= 55 #0-255
    // imageParam['saturation']= 0
    // imageParam['hue']= 0
    imageParam['ev_bias'] = 0  // (-96), (-64), (-32), 0, (32), (64), (96)
    // imageParam['ae_meter']= 0
    // imageParam['dig_effect']    = 0
    // imageParam['flicker']       = 0    
    param["imageProperty"] = imageParam;
#endif 

    param[_origin] = originParam;
    param[_stitch] = stitchParam;
    param[_audio] = audioParam;

    root[_name] = REQ_START_PREVIEW;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }
        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}


/*
 * 检查是否允许进入U盘模式(同步请求)
 */
#if 0
{
    "name": "camera._change_udisk_mode",
    "parameters": {
        "mode":1            # 进入U盘模式
    }
}
#endif



bool ProtoManager::sendSwitchUdiskModeReq(bool bEnterExitFlag)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    if (bEnterExitFlag) {
        param[_mode] = 1;      /* 进入Udisk模式 */
    } else {        
        param[_mode] = 0;      /* 退出Udisk模式 */
    }
    root[_name] = REQ_SWITCH_UDISK_MODE;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            writer->write(jsonRes, &os);
            resultStr = os.str();
            Log.d(TAG, "sendEnterUdiskModeReq -> request Result: %s", resultStr.c_str());

            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] sendEnterUdiskModeReq -> Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}


/*
 * sendUpdateRecordLeftSec
 * @param   uRecSec - 已录像的时长
 *          uLeftRecSecs - 可录像的剩余时长
 *          uLiveSec - 已直播的时长
 *          uLiveRecLeftSec - 可直播录像的剩余时长
 * 
 * 更新已录像/直播的时长及剩余时长
 */
bool ProtoManager::sendUpdateRecordLeftSec(u32 uRecSec, u32 uLeftRecSecs, u32 uLiveSec, u32 uLiveRecLeftSec)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_rec_sec] = uRecSec;
    param[_rec_left_sec] = uLeftRecSecs;
    param[_live_rec_sec] = uLiveSec;
    param[_live_rec_left_sec] = uLiveRecLeftSec;
    root[_name] = REQ_UPDATE_REC_LIVE_INFO;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}


/*
 * sendUpdateTakeTimelapseLeft
 * @param leftVal - 剩余张数
 * 更新能拍timelapse的剩余张数
 */
bool ProtoManager::sendUpdateTakeTimelapseLeft(u32 leftVal)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_tl_left] = leftVal;
    root[_name] = REQ_UPDATE_TIMELAPSE_LEFT;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}


bool ProtoManager::sendStateSyncReq(REQ_SYNC* pReqSyncInfo)
{
    int iResult = -1;
    bool bRet = false;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());


    param["sn"]  = pReqSyncInfo->sn;
    param["r_v"] = pReqSyncInfo->r_v;  
    param["p_v"] = pReqSyncInfo->p_v;
    param["k_v"] = pReqSyncInfo->k_v;

    root[_name] = REQ_SYNC_INFO;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            /* 解析响应值来判断是否允许 */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    bRet = true;
                }
            } else {
                bRet = false;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
            bRet = false;
        }
    }
    return bRet;
}




int ProtoManager::sendQueryGpsState()
{
    int iResult = -1;

    Json::Value jsonRes;   
    Json::Value root;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    root[_name] = REQ_QUERY_GPS_STATE;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {
                    iResult = jsonRes[_results][_state].asInt();
                    Log.d(TAG, "[%s: %d] Query Gps State Result = %d", __FILE__, __LINE__, iResult);
                } else {
                    Log.e(TAG, "[%s: %d] Reply 'state' val not 'done' ", __FILE__, __LINE__);
                }
            } else {
                Log.e(TAG, "[%s: %d] Reply content not 'state' member??", __FILE__, __LINE__);
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
        }
    }
    return iResult;    
}



/*
 * 返回值: 
 *  成功返回0
 *  失败: 通信失败返回-1; 状态不允许返回-2
 */
int ProtoManager::sendFormatmSDReq(int iIndex)
{
    int iResult = -1;

    Json::Value jsonRes;   
    Json::Value root;
    Json::Value param;

    std::ostringstream os;
    std::string resultStr = "";
    std::string sendStr = "";
    Json::StreamWriterBuilder builder;

    builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    param[_index] = iIndex;
    root[_name] = REQ_FORMAT_TFCARD;
    root[_param] = param;
	writer->write(root, &os);
    sendStr = os.str();

    iResult = sendHttpSyncReq(gReqUrl, &jsonRes, gPExtraHeaders, sendStr.c_str());
    switch (iResult) {
        case PROTO_MANAGER_REQ_SUC: {   /* 接收到了replay,解析Rely */
            if (jsonRes.isMember(_state)) {
                if (jsonRes[_state] == _done) {     /* 格式化成功(所有的卡或单张卡) */
                    Log.d(TAG, "[%s: %d] Format Tf Card Success", __FILE__, __LINE__);
                    iResult = ERROR_FORMAT_SUC;
                } else {
                    if (jsonRes.isMember(_error) && jsonRes[_error].isMember(_code)) {
                        if (jsonRes[_error][_code].asInt() == 0xFF) {
                            iResult = ERROR_FORMAT_STATE_NOT_ALLOW;
                        } else {
                            iResult = ERROR_FORMAT_FAILED;
                        }
                    } else {
                        iResult = ERROR_FORMAT_REQ_FAILED;
                    }
                }
            } else {
                Log.e(TAG, "[%s: %d] Reply content not 'state' member??", __FILE__, __LINE__);
                iResult = ERROR_FORMAT_REQ_FAILED;
            }
            break;
        }

        default: {  /* 通信错误 */
            Log.e(TAG, "[%s: %d] Maybe Transfer Error", __FILE__, __LINE__);
            iResult = ERROR_FORMAT_REQ_FAILED;            
        }
    }
    return iResult;      
}



bool ProtoManager::getSyncReqExitFlag()
{
    AutoMutex _l(mSyncReqLock);
    return mSyncReqExitFlag;
}

void ProtoManager::setSyncReqExitFlag(bool bFlag)
{
    AutoMutex _l(mSyncReqLock);
    mSyncReqExitFlag = bFlag;
}


