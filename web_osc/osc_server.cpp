

#include <sys/ins_types.h>
#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include <json/value.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "osc_server.h"
#include "utils.h"

OscServer* OscServer::sInstance = nullptr;
static std::mutex gInstanceLock;


OscServer* OscServer::Instance()
{
	std::unique_lock<std::mutex> lock(gInstanceLock);    
    if (!sInstance)
        sInstance = new OscServer();
    return sInstance;
}


OscServer::OscServer()
{
    init();
}


OscServer::~OscServer()
{
	deinit();
}

enum {
	OSC_TYPE_PATH_INFO,
	OSC_TYPE_PATH_STATE,
	OSC_TYPE_PATH_CHECKFORUPDATE,
	OSC_TYPE_PATH_CMD,
	OSC_TYPE_PATH_STATUS,
	OSC_TYPE_PATH_MAX
};


/*
 * @/osc/info的处理接口
 * Example
 * {
 *		"api": [
 * 			"/osc/info",
 * 			"/osc/state",
 * 			"/osc/checkForUpdates",
 * 			"/osc/commands/execute",
 * 			"/osc/commands/status"
 * 		],
 * 		"apiLevel": [2],
 * 		"endpoints": {
 *      	"httpPort": 80,
 *      	"httpUpdatesPort": 80
 *  	},    
 *   	"firmwareVersion": "1.11.1",
 *  	"gps": false,
 *  	"gyro": true,
 *  	"manufacturer": "RICOH",
 *  	"model": "RICOH THETA V",
 *  	"serialNumber": "00100016",
 *  	"supportUrl": "https://theta360.com/en/support/",
 *  	"uptime": 261
 * }
 */
bool OscServer::oscInfoHandler(mg_connection *conn)
{
	printf("----------------> path: /osc/info handler\n");
	/* 获取开机到此刻的时间 */
	mOscInfo["uptime"] = getUptime();
	sendResponse(conn, convJson2String(mOscInfo), true);
	return true;
}

/**
 * @/osc/state处理接口 
 * Example: 
 * {
 *		"fingerprint": "FIG_0004",
 *		"state": {
 *			"_apiVersion": 2,
 *			"batteryLevel": 0.89,
 *			"_batteryState": "disconnect",
 *			"_cameraError": [],
 *			"_captureStatus": "idle",
 *			"_capturedPictures": 0,
 *			"_latestFileUrl": "",
 *			"_recordableTime": 0,
 *			"_recordedTime": 0,
 *			"storageUri": "http://192.168.1.1/files/150100525831424d4207a52390afc300/"
 *		}
 *	}
 */
bool OscServer::oscStateHandler(mg_connection *conn)
{
	printf("----------------> path: /osc/state handler\n");

	/* 获取开机到此刻的时间 
	 */
	Json::Value standardState;
	standardState["storageUri"] = "/mnt/sdcard";
	standardState["batteryLevel"] = 0.89f;
	standardState["_batteryState"] = "disconnect";
	standardState["_apiVersion"] = 2;
	
	mOscState["fingerprint"] = "FIG_0004";
	mOscState["state"] = standardState;
	sendResponse(conn, convJson2String(mOscState), true);
	return true;
}


/*
 * camera.takePicture
 * camera.takeVideo
 * camera.listFile
 * camera.getLivePreview 
 * camera.delete
 * camera.getOptions
 * camera.setOptions
 * 6353d4fe9f3b22971f3ce9d1ca80b373  /boot/Image
 */
bool OscServer::oscCommandHandler(mg_connection *conn, std::string body)
{
	printf("----------------> path: /osc/commands/excute handler\n");
	printf("body: %s\n", body.c_str());
	Json::Value reqBody;
	Json::Value reqReply;

	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	JSONCPP_STRING errs;
	Json::CharReader* reader = builder.newCharReader();

	if (reader->parse(body.c_str(), body.c_str() + body.length(), &reqBody, &errs)) {
		if (reqBody.isMember("name")) {
			printf("Command name: %s\n", reqBody["name"].asCString());
			std::string oscCmd = reqBody["name"].asString();
			
			if (oscCmd == OSC_CMD_GET_OPTIONS) {				/* camera.getOptions */
				handleGetOptionsRequest(getCurOptions(), reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_SET_OPTIONS) {			/* camera.setOptions */
				handleSetOptionsRequest(getCurOptions(), reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_LIST_FILES) {			/* camera.listFiles */
                handleListFileRequest(reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_TAKE_PICTURE) {		/* camera.takePicture */
				handleTakePictureRequest(reqReply);
			} else if (oscCmd == OSC_CMD_START_CAPTURE) {		/* camera.startCapture */

			} else if (oscCmd == OSC_CMD_STOP_CAPTURE) {		/* camera.stopCapture */

			} else if (oscCmd == OSC_CMD_DELETE) {				/* camera.delete */
				std::string volPath = "/mnt/sdcard";
				handleDeleteFile(volPath, reqBody, reqReply);		
			} else if (oscCmd == OSC_CMD_GET_LIVE_PREVIEW) {	/* camera.getLivePreview */
                handleGetLivePreviewRequest(conn, reqBody, reqReply);
			}
            
            if (oscCmd != OSC_CMD_GET_LIVE_PREVIEW) {
    			sendResponse(conn, convJson2String(reqReply), true);				
            }
		}
	} else {
		fprintf(stderr, "Parse request body failed\n");
		genErrorReply(reqReply, MISSING_PARAMETER, "Parse parameter failed");
		sendResponse(conn, convJson2String(reqReply), true);				
	}
	return true;
}


/*
 * @/osc/checkForUpdates
 * Example:
 * {
 * 		"error": {
 * 			"code": "unknownCommand",
 * 			"message": "Command executed is unknown."
 * 		},
 * 		"name": "unknown",
 * 		"state": "error"
 * }
 */
bool OscServer::oscCheckForUpdateHandler(mg_connection *conn)
{
	printf("--------------------> oscCheckForUpdateHandler\n");
	Json::Value replyJson;
	replyJson[_name] = "unkown";
	genErrorReply(replyJson, UNKOWN_COMMAND, "Command executed is unknown.");
	sendResponse(conn, convJson2String(replyJson), true);
	return true;
}


void OscServer::fastWorkerLooper()
{
	printf("---------> Fast worker loop thread startup.\n");
	bool bHaveEvent = false;
	std::shared_ptr<struct tOscReq>	tmpReq = nullptr;

	/*
	 * 1.如果请求队列中无请求，进入睡眠状态
	 * 2.如果队首有请求，移除首部请求，并处理请求
	 */
	while (!mFastWorkerExit) {
		{
			std::unique_lock<std::mutex> _lock(mFastReqListLock);
			if (mFastReqList.empty()) {
				bHaveEvent = false;
			} else {
				tmpReq = mFastReqList.at(0);
				if (tmpReq) {
					mFastReqList.erase(mFastReqList.begin());
				}
				bHaveEvent = true;
			}
		}

		if (bHaveEvent) {	/* 有事件,处理请求 */
			if (tmpReq) {
				Json::Value& reqJson = tmpReq->oscReq;
				switch (tmpReq->iReqType) {
					case OSC_TYPE_PATH_INFO: {
						oscInfoHandler(tmpReq->conn);
						break;
					}	

					case OSC_TYPE_PATH_STATE: {
						oscStateHandler(tmpReq->conn);
						break;
					}

					case OSC_TYPE_PATH_CHECKFORUPDATE: {
						oscCheckForUpdateHandler(tmpReq->conn);
						break;
					}

					case OSC_TYPE_PATH_CMD: {
						switch (tmpReq->iReqCmd) {
							case OSC_CMD_GET_OPTIONS: {
								break;
							}

							case OSC_CMD_SET_OPTIONS: {
								break;
							}

							case OSC_CMD_DELETE: {
								break;
							}

							case OSC_CMD_LIST_FILES: {
								break;
							}

							case OSC_CMD_START_CAPTURE: {
								break;
							}

							case OSC_CMD_STOP_CAPTURE: {
								break;
							}

							case OSC_CMD_RESET: {
								break;
							}

							case OSC_CMD_SWITCH_WIFI:
							case OSC_CMD_UPLOAD_FILE:
							default: 
								break;

						}
						break;
					}

					case OSC_TYPE_PATH_STATUS: {	/* 查找指定id的结果 */
						break;
					}

					default: {
						printf("--- Unsupport OSC Type\n");
						break;
					}
				}

			}
		} else {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}
	}
}


void OscServer::previewWorkerLooper()
{
	printf("---------> preview worker loop thread startup.\n");

	while (!mPreviewWorkerExit) {

	}

}


#define OSC_REQ_URI_INFO			"/osc/info"
#define OSC_REQ_URI_STATE			"/osc/state"
#define OSC_REQ_URI_CHECKFORUPDATE	"/osc/checkForUpdates"
#define OSC_REQ_URI_COMMAND_EXECUTE	"/osc/commands/execute"
#define OSC_REQ_URI_COMMAND_STATUS	"/osc/commands/status"				

void OscServer::init()
{
	mFastWorkerExit = false;
	mPreviewWorkerExit = false;
	mCurOptions.clear();
	mOscInfo.clear();
	mOscState.clear();	

	/* 1.注册URI处理接口 */
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_INFO, METHOD_GET));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_STATE, METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_CHECKFORUPDATE, METHOD_GET | METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_COMMAND_EXECUTE, METHOD_GET | METHOD_POST));
	registerUrlHandler(std::make_shared<struct HttpRequest>(OSC_REQ_URI_COMMAND_STATUS, METHOD_GET | METHOD_POST));

	/* 2.生成默认的配置 */
	oscCfgInit();

	/* 3.创建工作线程 */
    mFastWorker = std::thread([this]{ fastWorkerLooper();});
    mPreviewWorker = std::thread([this]{ previewWorkerLooper();});

}

void OscServer::deinit()
{
    
}


void OscServer::genDefaultOptions(Json::Value& optionsJson)
{
	printf("---------------- genDefaultOptions\n");
	// Json::Value& optionsJson = *pOriginOptions;

	optionsJson.clear();

	optionsJson["captureMode"] = "image";
	optionsJson["captureModeSupport"][0] = "image";
	optionsJson["captureModeSupport"][1] = "video";
	optionsJson["captureModeSupport"][2] = "interval";

	optionsJson["captureStatus"] = "idle";
    optionsJson["captureStatusSupport"][0] = "idle";
    optionsJson["captureStatusSupport"][1] = "shooting";
    optionsJson["captureStatusSupport"][2] = "processing";
    optionsJson["captureStatusSupport"][2] = "downloading";


    optionsJson["exposureProgram"] = 4;
    optionsJson["exposureProgramSupport"][0] = 1;
    optionsJson["exposureProgramSupport"][1] = 2;
    optionsJson["exposureProgramSupport"][2] = 4;
    optionsJson["exposureProgramSupport"][3] = 9;


    optionsJson["iso"] = 0;
    optionsJson["isoSupport"][0] = 0;
    optionsJson["isoSupport"][1] = 100;
    optionsJson["isoSupport"][2] = 125;
    optionsJson["isoSupport"][3] = 160;
    optionsJson["isoSupport"][4] = 200;
    optionsJson["isoSupport"][5] = 250;
    optionsJson["isoSupport"][6] = 320;
    optionsJson["isoSupport"][7] = 400;
    optionsJson["isoSupport"][8] = 500;
    optionsJson["isoSupport"][9] = 640;
    optionsJson["isoSupport"][10] = 800;
    optionsJson["isoSupport"][11] = 1000;
    optionsJson["isoSupport"][12] = 1250;
    optionsJson["isoSupport"][13] = 1600;
    optionsJson["isoSupport"][14] = 2000;
    optionsJson["isoSupport"][15] = 2500;
    optionsJson["isoSupport"][16] = 3200;
    optionsJson["isoSupport"][17] = 4000;
    optionsJson["isoSupport"][18] = 5000;
    optionsJson["isoSupport"][19] = 6400;


	optionsJson["shutterSpeed"] = 0;
	optionsJson["shutterSpeedSupport"][0] = 0.0f;
	optionsJson["shutterSpeedSupport"][1] = 0.5f;
	optionsJson["shutterSpeedSupport"][2] = 0.333f;
	optionsJson["shutterSpeedSupport"][3] = 0.25f;
	optionsJson["shutterSpeedSupport"][4] = 0.2f;
	optionsJson["shutterSpeedSupport"][5] = 0.125f;
	optionsJson["shutterSpeedSupport"][6] = 0.1f;
	optionsJson["shutterSpeedSupport"][7] = 0.067f;
	optionsJson["shutterSpeedSupport"][8] = 0.05f;
	optionsJson["shutterSpeedSupport"][9] = 0.04f;
	optionsJson["shutterSpeedSupport"][10] = 0.033f;
	optionsJson["shutterSpeedSupport"][11] = 0.025f;
	optionsJson["shutterSpeedSupport"][12] = 0.02f;
	optionsJson["shutterSpeedSupport"][13] = 0.0167f;
	optionsJson["shutterSpeedSupport"][14] = 0.0125f;
	optionsJson["shutterSpeedSupport"][15] = 0.01f;
	optionsJson["shutterSpeedSupport"][16] = 0.0083f;
	optionsJson["shutterSpeedSupport"][17] = 0.00625f;
	optionsJson["shutterSpeedSupport"][18] = 0.005f;
	optionsJson["shutterSpeedSupport"][19] = 0.004167f;

	optionsJson["shutterSpeedSupport"][20] = 0.003125f;
	optionsJson["shutterSpeedSupport"][21] = 0.0025f;

	optionsJson["shutterSpeedSupport"][22] = 0.002f;
	optionsJson["shutterSpeedSupport"][23] = 0.0015625f;
	optionsJson["shutterSpeedSupport"][24] = 0.00125f;
	optionsJson["shutterSpeedSupport"][25] = 0.001f;
	optionsJson["shutterSpeedSupport"][26] = 0.0008f;
	optionsJson["shutterSpeedSupport"][27] = 0.000625f;

	optionsJson["aperture"] = 2.4f;
	optionsJson["apertureSupport"][0] = 2.4f;


	optionsJson["whiteBalance"] = "auto";
	optionsJson["whiteBalanceSupport"][0] = "auto";
	optionsJson["whiteBalanceSupport"][1] = "incandescent";
	optionsJson["whiteBalanceSupport"][2] = "daylight";
	optionsJson["whiteBalanceSupport"][3] = "cloudy-daylight";

	optionsJson["exposureCompensation"] = 0.0f;

	optionsJson["exposureCompensationSupport"][0] = -3.0f;
	optionsJson["exposureCompensationSupport"][1] = -2.5f;
	optionsJson["exposureCompensationSupport"][2] = -2.0f;
	optionsJson["exposureCompensationSupport"][3] = -1.5f;
	optionsJson["exposureCompensationSupport"][4] = -1.0f;
	optionsJson["exposureCompensationSupport"][5] = -0.5f;
	optionsJson["exposureCompensationSupport"][6] = 0.0f;
	optionsJson["exposureCompensationSupport"][7] = 0.5f;
	optionsJson["exposureCompensationSupport"][8] = 1.0f;
	optionsJson["exposureCompensationSupport"][9] = 1.5f;
	optionsJson["exposureCompensationSupport"][10] = 2.0f;
	optionsJson["exposureCompensationSupport"][11] = 2.5f;
	optionsJson["exposureCompensationSupport"][12] = 3.0f;
 
 
 	optionsJson["fileFormat"]["type"] = "jpeg";
 	optionsJson["fileFormat"]["width"] = 4000;
 	optionsJson["fileFormat"]["height"] = 3000;

 	optionsJson["fileFormatSupport"][0]["type"] = "jpeg";
 	optionsJson["fileFormatSupport"][0]["width"] = 4000;
 	optionsJson["fileFormatSupport"][0]["height"] = 3000;

 	optionsJson["fileFormatSupport"][1]["type"] = "mp4";
 	optionsJson["fileFormatSupport"][1]["width"] = 7680;
 	optionsJson["fileFormatSupport"][1]["height"] = 3840;
 	optionsJson["fileFormatSupport"][1]["framerate"] = 30;

 	optionsJson["fileFormatSupport"][2]["type"] = "mp4";
 	optionsJson["fileFormatSupport"][2]["width"] = 3840;
 	optionsJson["fileFormatSupport"][2]["height"] = 1920;
 	optionsJson["fileFormatSupport"][2]["framerate"] = 30;


	optionsJson["exposureDelay"] = 0;
	optionsJson["exposureDelaySupport"][0] = 0;
	optionsJson["exposureDelaySupport"][1] = 1;
	optionsJson["exposureDelaySupport"][2] = 2;
	optionsJson["exposureDelaySupport"][3] = 3;
	optionsJson["exposureDelaySupport"][4] = 4;
	optionsJson["exposureDelaySupport"][5] = 5;
	optionsJson["exposureDelaySupport"][6] = 6;
	optionsJson["exposureDelaySupport"][7] = 7;
	optionsJson["exposureDelaySupport"][8] = 8;
	optionsJson["exposureDelaySupport"][9] = 9;
	optionsJson["exposureDelaySupport"][10] = 10;
	optionsJson["exposureDelaySupport"][11] = 20;
	optionsJson["exposureDelaySupport"][12] = 30;
	optionsJson["exposureDelaySupport"][13] = 60;


	optionsJson["sleepDelay"] = 65536;
	optionsJson["sleepDelaySupport"][0] = 65536;
    

	optionsJson["offDelay"] = 65536;
	optionsJson["offDelaySupport"][0] = 65536;


	optionsJson["totalSpace"] = 0;
	optionsJson["remainingSpace"] = 0;
	optionsJson["remainingPictures"] = 0;

	optionsJson["gpsInfo"]["lat"] = 0;
	optionsJson["gpsInfo"]["lng"] = 0;

	optionsJson["dateTimeZone"] = "2018:10:17 16:04:29+8:00";


	optionsJson["hdr"] = "off";
	optionsJson["hdrSupport"][0] = "off";
	optionsJson["hdrSupport"][1] = "hdr";

	optionsJson["exposureBracket"]["shots"] = 3;
	optionsJson["exposureBracket"]["increment"] = 1;

	optionsJson["exposureBracketSupport"]["autoMode"] = false;
	optionsJson["exposureBracketSupport"]["shotsSupport"][0] = 3;
	optionsJson["exposureBracketSupport"]["incrementSupport"][0] = 1;
	optionsJson["exposureBracketSupport"]["incrementSupport"][1] = 2;

	optionsJson["gyro"] = true;
	optionsJson["gyroSupport"] = true;
    

	optionsJson["gps"] = true;
	optionsJson["gpsSupport"] = true;


	optionsJson["imageStabilization"] = "on";
	optionsJson["imageStabilizationSupport"][0] = "on";
	optionsJson["imageStabilizationSupport"][1] = "off";

	optionsJson["wifiPassword"] = "88888888";
    
	optionsJson["previewFormat"]["width"] = 1920;
	optionsJson["previewFormat"]["height"] = 960;
	optionsJson["previewFormat"]["framerate"] = 5;

	optionsJson["previewFormatSupport"][0]["width"] = 1920;
	optionsJson["previewFormatSupport"][0]["height"] = 960;
	optionsJson["previewFormatSupport"][0]["framerate"] = 5;

	optionsJson["captureInterval"] = 2;
	optionsJson["captureIntervalSupport"][0] = 2;
	optionsJson["captureIntervalSupport"][1] = 60;


	optionsJson["captureNumber"] = 0;
	optionsJson["captureNumberSupport"][0] = 0;
	optionsJson["captureNumberSupport"][1] = 10;
    

	optionsJson["remainingVideoSeconds"] = 1200;

	optionsJson["pollingDelay"] = 5;

	optionsJson["delayProcessing"] = false;
	optionsJson["delayProcessingSupport"][0] = false;
    

	optionsJson["clientVersion"] = 2;

	optionsJson["photoStitching"] = "ondevice";
	optionsJson["photoStitchingSupport"][0] = "ondevice";
 

	optionsJson["videoStitching"] = "ondevice";
	optionsJson["videoStitchingSupport"][0] = "none";
	optionsJson["videoStitchingSupport"][1] = "ondevice";


	optionsJson["videoGPS"] = "none";
	optionsJson["videoGPSSupport"][0] = "none";

}


void OscServer::oscCfgInit()
{
	mOscInfo.clear();

	if (access(OSC_INFO_TEMP_PATH_NAME, F_OK) == 0) {	/* 加载配置文件里的配置 */
		std::ifstream ifs;  
		ifs.open(OSC_INFO_TEMP_PATH_NAME, std::ios::binary); 
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;

		if (parseFromStream(builder, ifs, &mOscInfo, &errs)) {
			fprintf(stderr, "parse [%s] success.\n", OSC_INFO_TEMP_PATH_NAME);
		} 	
		ifs.close();	
	} else {	/* 构造一个默认的配置 */
		Json::Value endPoint;

		endPoint["httpPort"] = 80;
		endPoint["httpUpdatesPort"] = 80;

		mOscInfo["manufacturer"] = "Shenzhen Arashi Vision";
		mOscInfo["model"] = "Insta360 Pro2";
		mOscInfo["serialNumber"] = "000000";
		mOscInfo["firmwareVersion"] = "1.0.0";
		mOscInfo["supportUrl"] = "https://support.insta360.com";
		mOscInfo["gps"] = true;
		mOscInfo["gyro"] = true;		
		mOscInfo["uptime"] = 0;
		mOscInfo["endpoints"] = endPoint;

		mOscInfo["apiLevel"][0] = 2;

		mOscInfo["api"][0] = "/osc/info";
		mOscInfo["api"][1] = "/osc/state";
		mOscInfo["api"][2] = "/osc/checkForUpdates";
		mOscInfo["api"][3] = "/osc/commands/execute";
		mOscInfo["api"][4] = "/osc/commands/status";
	}

	if (access(SN_FIRM_VER_PATH_NAME, F_OK) == 0) {
		Json::Value root;
		std::ifstream ifs;  
		ifs.open(SN_FIRM_VER_PATH_NAME, std::ios::binary); 
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;

		if (parseFromStream(builder, ifs, &root, &errs)) {
			fprintf(stderr, "parse [%s] success.\n", SN_FIRM_VER_PATH_NAME);
			
			if (root.isMember("serialNumber")) {
				mOscInfo["serialNumber"] = root["serialNumber"];
			}

			if (root.isMember("firmwareVersion")) {
				mOscInfo["firmwareVersion"] = root["firmwareVersion"];
			}
		} 		
		ifs.close();
	}

	printJson(mOscInfo);

	if (access(OSC_DEFAULT_OPTIONS_PATH, F_OK) == 0) {	/* 从配置文件中加载Options */

	} else {
		genDefaultOptions(mCurOptions);		
	}
	printJson(mCurOptions);
}


bool OscServer::startOscServer()
{
	return startHttpServer();
}


void OscServer::stopOscServer()
{

}


bool OscServer::oscServiceEntry(mg_connection *conn, std::string url, std::string body)
{
	if (url == OSC_REQ_URI_INFO) {			/* 构造一个tOscReq,并丢入到mReqFastList中 */

	} else if (url == OSC_REQ_URI_STATE) {

	} else if (url == OSC_REQ_URI_CHECKFORUPDATE) {

	} else if (url == OSC_REQ_URI_COMMAND_EXECUTE) {

	} else if (url == OSC_REQ_URI_COMMAND_STATUS) {

	} 
	return true;
}





bool OscServer::oscStatusHandler(mg_connection *conn, std::string body)
{
	return true;
}



bool OscServer::registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler)
{
	std::shared_ptr<struct HttpRequest> tmpPtr = nullptr;
	bool bResult = false;
	u32 i = 0;

	for (i = 0; i < mSupportRequest.size(); i++) {
		tmpPtr = mSupportRequest.at(i);
		if (tmpPtr && uriHandler) {
			if (uriHandler->mUrl == tmpPtr->mUrl) {
				fprintf(stderr, "url have registed, did you cover it\n");
				break;
			}
		}
	}

	if (i >= mSupportRequest.size()) {
		mSupportRequest.push_back(uriHandler);
		bResult = true;
	}

	return bResult;
}


void OscServer::HandleEvent(mg_connection *connection, http_message *httpReq)
{
	/*
	 * message:包括请求行 + 头 + 请求体
	 */
#ifdef  DEBUG_HTTP_SERVER	
	std::string req_str = std::string(httpReq->message.p, httpReq->message.len);	
	printf("Request Message: %s\n", req_str.c_str());
#endif

	bool bHandled = false;
	std::string reqMethod = std::string(httpReq->method.p, httpReq->method.len);
	std::string reqUri = std::string(httpReq->uri.p, httpReq->uri.len);
	std::string reqProto = std::string(httpReq->proto.p, httpReq->proto.len);
	std::string reqBody = std::string(httpReq->body.p, httpReq->body.len);

#ifdef  DEBUG_HTTP_SERVER	

	printf("Req Method: %s", reqMethod.c_str());
	printf("Req Uri: %s", reqUri.c_str());
	printf("Req Proto: %s", reqProto.c_str());
	printf("body: %s\n", reqBody.c_str());
#endif    

	std::shared_ptr<struct HttpRequest> tmpRequest;
	u32 i = 0;
	for (i = 0; i < sInstance->mSupportRequest.size(); i++) {
		tmpRequest = sInstance->mSupportRequest.at(i);
		if (tmpRequest) {
			if (reqUri == tmpRequest->mUrl) {
				int method = 0;

				if (reqMethod == "GET" || reqMethod == "get") {
					method |= METHOD_GET;
				} else if (reqMethod == "POST" || reqMethod == "post") {
					method |= METHOD_POST;
				}

				if (method & tmpRequest->mReqMethod) {	/* 支持该方法 */
					oscServiceEntry(connection, reqUri, reqBody);
					bHandled = true;
				}
			}
		}
	}

	if (!bHandled) {
		mg_http_send_error(connection, 404, NULL);
	}
}


void OscServer::sendResponse(mg_connection *conn, std::string reply,  bool bSendClose)
{
	if (reply.length() > 0) {
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n%s\r\n", 
            	(uint32_t)reply.length(), reply.c_str());
	} else {
		mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n", 
            	0);
	}
	
	
	if (bSendClose) {
		conn->flags |= MG_F_SEND_AND_CLOSE;
	}
}

void OscServer::genErrorReply(Json::Value& replyJson, std::string code, std::string errMsg)
{
	replyJson[_state] = _error;
	replyJson[_error][_code] = code;
	replyJson[_error][_message] = errMsg;
}