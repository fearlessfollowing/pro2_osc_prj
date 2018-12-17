#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include <json/value.h>
#include <json/json.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "http_server.h"

static Json::Value gOscInfo;
static Json::Value gOscState;
static Json::Value gOptions;

#define OSC_INFO_TEMP_PATH_NAME		"/home/nvidia/insta360/etc/osc_info.json"
#define OSC_DEFAULT_OPTIONS_PATH	"/home/nvidia/insta360/etc/default_options.json"

#define SN_FIRM_VER_PATH_NAME	"/home/nvidia/insta360/etc/sn_firm.json"
#define UP_TIME_PATH			"/proc/uptime"


static void printJson(Json::Value& json)
{
    std::ostringstream osOutput;  

    std::string resultStr = "";
    Json::StreamWriterBuilder builder;

    // builder.settings_["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(json, &osOutput);
    resultStr = osOutput.str();

	printf("%s\n", resultStr.c_str());

}

static std::string convJson2String(Json::Value& json)
{
    std::ostringstream osOutput;  

    std::string resultStr = "";
    Json::StreamWriterBuilder builder;

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

	writer->write(json, &osOutput);
    resultStr = osOutput.str();
	return resultStr;
}


static int getUptime()
{
	int iUptime = 1000;
	FILE* fp = fopen(UP_TIME_PATH, "r");
	if (fp) {
		char buf[512] = {0};
		char num[512] = {0};
		int i = 0;

		fgets(buf, 512, fp);
		while (buf[i] != '.') {
			num[i] = buf[i];
			i++;
		}

		iUptime = atoi(num);
		fclose(fp);
	}
	return iUptime;
}


static bool genDefaultOptions(Json::Value& optionsJson)
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
	optionsJson["previewFormat"]["framerate"] = 30;


	optionsJson["previewFormatSupport"][0]["width"] = 1920;
	optionsJson["previewFormatSupport"][0]["height"] = 960;
	optionsJson["previewFormatSupport"][0]["framerate"] = 30;

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


static bool oscCfgInit()
{
	gOscInfo.clear();

	if (access(OSC_INFO_TEMP_PATH_NAME, F_OK) == 0) {	/* 加载配置文件里的配置 */
		std::ifstream ifs;  
		ifs.open(OSC_INFO_TEMP_PATH_NAME, std::ios::binary); 
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		JSONCPP_STRING errs;

		if (parseFromStream(builder, ifs, &gOscInfo, &errs)) {
			fprintf(stderr, "parse [%s] success.\n", OSC_INFO_TEMP_PATH_NAME);
		} 	
		ifs.close();	
	} else {	/* 构造一个默认的配置 */
		Json::Value endPoint;

		endPoint["httpPort"] = 80;
		endPoint["httpUpdatesPort"] = 80;

		gOscInfo["manufacturer"] = "Shenzhen Arashi Vision";
		gOscInfo["model"] = "Insta360 Pro2";
		gOscInfo["serialNumber"] = "000000";
		gOscInfo["firmwareVersion"] = "1.0.0";
		gOscInfo["supportUrl"] = "https://support.insta360.com";
		gOscInfo["gps"] = true;
		gOscInfo["gyro"] = true;		
		gOscInfo["uptime"] = 0;
		gOscInfo["endpoints"] = endPoint;

		gOscInfo["apiLevel"][0] = 2;

		gOscInfo["api"][0] = "/osc/info";
		gOscInfo["api"][1] = "/osc/state";
		gOscInfo["api"][2] = "/osc/checkForUpdates";
		gOscInfo["api"][3] = "/osc/commands/execute";
		gOscInfo["api"][4] = "/osc/commands/status";
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
				gOscInfo["serialNumber"] = root["serialNumber"];
			}

			if (root.isMember("firmwareVersion")) {
				gOscInfo["firmwareVersion"] = root["firmwareVersion"];
			}
		} 		
		ifs.close();
	}

	printJson(gOscInfo);

	genDefaultOptions(gOptions);

	printJson(gOptions);

	return true;
}


Json::Value& getCurOptions()
{
	return gOptions;
}


static void sendResponse(mg_connection *c, std::string reply,  bool bSendClose)
{
	if (reply.length() > 0) {
		mg_printf(c, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n%s\r\n", 
            	(uint32_t)reply.length(), reply.c_str());
	} else {
		mg_printf(c, "HTTP/1.1 200 OK\r\n"	\
				"Connection: close\r\n"	\
				"Content-Type: application/json;charset=utf-8\r\n"	\
				"X-Content-Type-Options:nosniff\r\n"	\
				"Content-Length: %u\r\n\r\n", 
            	0);
	}
	
	
	if (bSendClose) {
		c->flags |= MG_F_SEND_AND_CLOSE;
	}
}



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
 * 
 */
bool oscInfoHandler(mg_connection *conn, std::string body)
{
	printf("----------------> path: /osc/info handler\n");

	/* 获取开机到此刻的时间 */
	gOscInfo["uptime"] = getUptime();
	sendResponse(conn, convJson2String(gOscInfo), true);
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
bool oscStateHandler(mg_connection *conn, std::string body)
{
	printf("----------------> path: /osc/state handler\n");

	/* 获取开机到此刻的时间 
	 */
	Json::Value standardState;
	standardState["storageUri"] = "/mnt/sdcard";
	standardState["batteryLevel"] = 0.89f;
	standardState["_batteryState"] = "disconnect";
	standardState["_apiVersion"] = 2;
	
	gOscState["fingerprint"] = "FIG_0004";
	gOscState["state"] = standardState;
	sendResponse(conn, convJson2String(gOscState), true);
	
	return true;
}



bool oscCheckForUpdateHandler(mg_connection *conn, std::string body)
{
	// do sth
	std::cout << "handle fun1" << std::endl;
	std::cout << "body: " << body << std::endl;


	return true;
}


#define OSC_CMD_GET_OPTIONS			"camera.getOptions"
#define OSC_CMD_SET_OPTIONS			"camera.setOptions"
#define OSC_CMD_LIST_FILES			"camera.listFiles"

#define OSC_CMD_TAKE_PICTURE		"camera.takePicture"
#define OSC_CMD_START_CAPTURE		"camera.startCapture"
#define OSC_CMD_STOP_CAPTURE		"camera.stopCapture"

#define OSC_CMD_DELETE				"camera.delete"
#define OSC_CMD_GET_LIVE_PREVIEW	"camera.getLivePreview"


#define OSC_CMD_UPLOAD_FILE			"camera.uploadFile"
#define OSC_CMD_SWITCH_WIFI			"camera.switchWifi"
#define OSC_CMD_PROCESS_PICTURE		"camera.processPicture"
#define OSC_CMD_RESET				"camera.reset"

#define INVALID_PARAMETER_NAME 		"invalidParameterName"
#define MISSING_PARAMETER			"missingParameter"
#define INVALID_PARAMETER_VAL		"invalidParameterValue"


#define _param						"parameters"
#define _state  					"state"
#define _done						"done"
#define _error  					"error"
#define _code  						"code"
#define _message					"message"
#define _fileUrls					"fileUrls"
#define _results					"results"



/*
Example:
Request:
{
    "name": "camera.getOptions",
    "parameters": {
        "optionNames": ["captureModeSupport"]
    }
}
Response:
{
    "name": "camera.getOptions",
    "results": {
        "options": {
            "captureModeSupport": [
                "image",
                "interval",
                "video"
            ]
        }
    },
    "state": "done"
}
*/

bool handleGetOptionsRequest(Json::Value& sysCurOptions, Json::Value& jsonReq, Json::Value& jsonReply)
{
	jsonReply.clear();
	jsonReply["name"] = OSC_CMD_GET_OPTIONS;

	if (jsonReq.isMember(_param)) {
		if (jsonReq[_param].isMember("optionNames")) {
			Json::Value optionNames = jsonReq[_param]["optionNames"];
			Json::Value resultOptions;
			int i = 0;

			printf("optionNames size: %d\n", optionNames.size());
			
			for (i = 0; i < optionNames.size(); i++) {
				std::string optionMember = optionNames[i].asString();
				if (sysCurOptions.isMember(optionMember)) {
					resultOptions[optionMember] = sysCurOptions[optionMember];
				}
			}
			
			if (i >= optionNames.size()) {
				jsonReply["results"]["options"] = resultOptions;
				jsonReply["state"] = "done";
			} else {
				jsonReply["state"] = "error";
				jsonReply["error"]["code"] = "invalidParameterName";
				jsonReply["error"]["message"] = "Parameter optionNames contains unrecognized or unsupported";
			}
		}
	} else {
		jsonReply["state"] = "error";
		jsonReply["error"]["code"] = "invalidParameterName";
		jsonReply["error"]["message"] = "Parameter format is invalid";
	}
	return true;
}



/*
Example:
{
    "name": "camera.setOptions",
    "parameters": {
        "options": {
        	"captureMode": "video"
        }
    }
}

{
    "name": "camera.setOptions",
    "state": "done"
}
*/
bool handleSetOptionsRequest(Json::Value& sysCurOptions, Json::Value& jsonReq, Json::Value& jsonReply)
{
	jsonReply.clear();
	jsonReply["name"] = OSC_CMD_SET_OPTIONS;
	bool bParseResult = true;

	if (jsonReq.isMember(_param)) {
		if (jsonReq[_param].isMember("options")) {
			Json::Value optionsMap = jsonReq[_param]["options"];

			Json::Value::Members members;  
        	members = optionsMap.getMemberNames();   // 获取所有key的值
			printf("setOptions -> options: \n");
			printJson(optionsMap);


			for (Json::Value::Members::iterator iterMember = members.begin(); 
										iterMember != members.end(); iterMember++) {	// 遍历每个key 
				
				/* 1.首先检查该keyName是否在curOptions的成员
				 * 2.检查keyVal是否为curOptions对应的option支持的值
				 */
				std::string strKey = *iterMember; 
				if (sysCurOptions.isMember(strKey)) {	/* keyName为curOptions的成员 */
					/* TODO: 对设置的值合法性进行校验 */
					sysCurOptions[strKey] = optionsMap[strKey];
				} else {
					fprintf(stderr, "Unsupport keyName[%s],break now\n", strKey.c_str());
					bParseResult = false;
					break;
				}
			}

			if (bParseResult) {
				jsonReply["state"] = "done";
			} else {
				jsonReply["state"] = "error";
				jsonReply["error"]["code"] = "invalidParameterName";
				jsonReply["error"]["message"] = "Parameter optionNames contains unrecognized or unsupported";
			}
		}
	} else {
		jsonReply["state"] = "error";
		jsonReply["error"]["code"] = "invalidParameterName";
		jsonReply["error"]["message"] = "Parameter format is invalid";
	}
	return true;
}



bool handleTakePictureRequest(Json::Value& jsonReply)
{
	return true;
}

bool handleSwitchWifiRequest(Json::Value& jsonReply)
{
	return true;
}

bool handleDeleteRequest(Json::Value& jsonReply)
{	
	return true;
}


bool handleGetLivePreviewRequest(Json::Value& jsonReply)
{
	return true;
}


bool handleListFileRequest(Json::Value& jsonReply)
{
	return true;
}


bool handleStartCaptureRequest(Json::Value& jsonReply)
{	
	return true;
}


bool handleStopCaptureRequest(Json::Value& jsonReply)
{
	return true;
}

bool handleUploadFileRequest(Json::Value& jsonReply)
{
	return true;
}

bool handleResetRequest(Json::Value& jsonReply)
{
	return true;
}


bool handleProcessPictureRequest(Json::Value& jsonReply)
{
	return true;
}





void genErrorReply(Json::Value& replyJson, std::string code, std::string errMsg)
{
	replyJson[_state] = _error;
	replyJson[_error][_code] = code;
	replyJson[_error][_message] = errMsg;
}



std::string extraAbsDirFromUri(std::string fileUrl)
{
	const char *delim = "/";
	std::vector<std::string> vUris;
	char cUri[1024] = {0};

	strncpy(cUri, fileUrl.c_str(), (fileUrl.length() > 1024)? 1024: fileUrl.length());
    char* p = strtok(cUri, delim);
    while(p) {
		std::string tmpStr = p;
		vUris.push_back(tmpStr);
        p = strtok(NULL, delim);
    }

	if (vUris.size() < 3) {
		return "none";
	} else {
		std::string result = "/";
		u32 i = 2;
		for (i = 2; i < vUris.size() - 1; i++) {
			result += vUris.at(i);
			
			if (i != vUris.size() - 2)
				result += "/";
		}
		return result;
	}
}

/*
Example:
camera.delete
{
    "name": "camera.delete",
    "parameters": {	
		"fileUrls": ["http://192.168.1.1/files/150100525831424d4207a52390afc300/100RICOH/R0012284.JPG"]
    }
}
参数的特殊情况:
"fileUrls"列表中只包含"all", "image", "video"


成功:
{
    "name": "camera.delete",
    "state": "done"
}

失败:
1.参数错误
{
	"name":"camera.delete",
	"state":"error",
	"error": {
		"code": "missingParameter",	// "missingParameter": 指定的fileUrls不存在; "invalidParameterName":不认识的参数名; "invalidParameterValue":参数名识别，值非法
		"message":"XXXX"
	}
}
2.文件删除失败
{
	"name":"camera.delete",
	"state":"done",
	"results":{
		"fileUrls":[]		// 删除失败的URL列表
	}
}
*/
bool endWith(const std::string &str, const std::string &tail) 
{
	return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}
 
bool startWith(const std::string &str, const std::string &head) 
{
	return str.compare(0, head.size(), head) == 0;
}


bool handleDeleteFile(std::string rootPath, Json::Value& reqBody, Json::Value& reqReply)
{
	reqReply.clear();
	reqReply["name"] = OSC_CMD_DELETE;
	Json::Value delFailedUris;
	bool bParseUri = true;	

	if (reqBody.isMember(_param)) {
		if (reqBody[_param].isMember(_fileUrls)) {	/* 存在‘fileUrls’字段，分为两种情况: 1.按类别删除；2.按指定的urls删除 */
			Json::Value delUrls = reqBody[_param][_fileUrls];
			if (delUrls.size() == 1 && (delUrls[0].asString() == "all" || delUrls[0].asString() == "ALL" 
										|| delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE"
										|| delUrls[0].asString() == "video" || delUrls[0].asString() == "VIDEO")) {
				
				if (access(rootPath.c_str(), F_OK) == 0) {

#define DELETE_TYPE_ALL		1
#define DELETE_TYPE_IMAGE	2
#define DELETE_TYPE_VIDEO	3

					int iDelType = 1;

					if (delUrls[0].asString() == "all" || delUrls[0].asString() == "ALL") {	/* 删除顶层目录下所有以PIC, VID开头的目录 */
						iDelType = DELETE_TYPE_ALL;
					} else if (delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE") {
						iDelType = DELETE_TYPE_IMAGE;
					} else if (delUrls[0].asString() == "image" || delUrls[0].asString() == "IMAGE") {
						iDelType = DELETE_TYPE_VIDEO;
					}

					DIR *dir;
					struct dirent *de;

    				dir = opendir(rootPath.c_str());
    				if (dir != NULL) {
						while ((de = readdir(dir))) {
							if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
								continue;

							std::string dirName = de->d_name;
							bool bIsPicDir = false;
							bool bIsVidDir = false;
							if ((bIsPicDir = startWith(dirName, "PIC")) || (bIsVidDir = startWith(dirName, "VID"))) {
								std::string dstPath = rootPath + "/" + dirName;
								bool bRealDel = false;
								switch (iDelType) {
									case DELETE_TYPE_ALL: {
										bRealDel = true;
										break;
									}

									case DELETE_TYPE_IMAGE: {
										if (bIsPicDir) bRealDel = true;
										break;
									}

									case DELETE_TYPE_VIDEO: {
										if (bIsVidDir) bRealDel = true;
										break;
									}
								}
								if (bRealDel) {
									std::string rmCmd = "rm -rf " + dstPath;
									printf("Remove dir [%s]\n", rmCmd.c_str());
									system(rmCmd.c_str());
								}
							}
						}
						closedir(dir);
					}
					reqReply[_state] = _done;
				} else {	/* 存储设备不存在,返回错误 */
					genErrorReply(reqReply, INVALID_PARAMETER_VAL, "Storage device not exist");
				}
			} else {
				/* 得到对应的删除列表 */
				std::string dirPath;
				for (int i = 0; i < delUrls.size(); i++) {
					dirPath = extraAbsDirFromUri(delUrls[i].asString());
					printf("dir path: %s\n", dirPath.c_str());
					if (access(dirPath.c_str(), F_OK) == 0) {	/* 删除整个目录 */
						std::string rmCmd = "rm -rf " + dirPath;
						int retry = 0;
						for (retry = 0; retry < 3; retry++) {
							system(rmCmd.c_str());
							if (access(dirPath.c_str(), F_OK) != 0) break;
						}

						if (retry >= 3) {	/* 删除失败，将该fileUrl加入到删除失败返回列表中 */
							delFailedUris[_fileUrls].append(dirPath);
						}
					} else {
						fprintf(stderr, "Uri: %s not exist\n", dirPath.c_str());
						bParseUri = false;
						break;
					}
				}

				if (bParseUri) {
					/* 根据删除失败列表来决定返回值 */
					if (delFailedUris.isMember(_fileUrls)) {	/* 有未删除的uri */
						reqReply[_state] = _done;
						reqReply[_results] = delFailedUris;
					} else {	/* 全部删除成功 */
						reqReply[_state] = _done;
					}
				} else {
					genErrorReply(reqReply, INVALID_PARAMETER_VAL, "Parameter url " + dirPath + " not exist");			
				}
			}
		} else {
			genErrorReply(reqReply, MISSING_PARAMETER, "Missing parameter fileUrls");
		}
	} else {
		genErrorReply(reqReply, INVALID_PARAMETER_NAME, "Missing parameter");
	}
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

bool oscCommandHandler(mg_connection *conn, std::string body)
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
			
			if (oscCmd == OSC_CMD_GET_OPTIONS) {
				handleGetOptionsRequest(getCurOptions(), reqBody, reqReply);
				sendResponse(conn, convJson2String(reqReply), true);
			} else if (oscCmd == OSC_CMD_SET_OPTIONS) {
				handleSetOptionsRequest(getCurOptions(), reqBody, reqReply);
				sendResponse(conn, convJson2String(reqReply), true);
			} else if (oscCmd == OSC_CMD_LIST_FILES) {

			} else if (oscCmd == OSC_CMD_TAKE_PICTURE) {
				handleTakePictureRequest(reqReply);
			} else if (oscCmd == OSC_CMD_START_CAPTURE) {

			} else if (oscCmd == OSC_CMD_STOP_CAPTURE) {

			} else if (oscCmd == OSC_CMD_DELETE) {
				std::string volPath = "/mnt/sdcard";
				handleDeleteFile(volPath, reqBody, reqReply);		/* Debug */
			}
		}
	} else {
		fprintf(stderr, "Parse request body failed\n");
	}

	// sendResponse(conn, "", true);
	return true;
}


bool oscStatusHandler(mg_connection *conn, std::string body)
{
	// do sth
	std::cout << "handle fun2" << std::endl;
	std::cout << "body: " << body << std::endl;
	return true;
}


int main(int argc, char *argv[]) 
{

	if (!oscCfgInit()) {
		fprintf(stderr, "OSC Configure error, exit now\n");
		return -1;
	}

	HttpServer* httpServer = HttpServer::Instance();


	httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/info", METHOD_GET | METHOD_POST, oscInfoHandler));
	httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/state", METHOD_GET | METHOD_POST, oscStateHandler));
	httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/checkForUpdate", METHOD_GET | METHOD_POST, oscCheckForUpdateHandler));

	httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/commands/execute", METHOD_GET | METHOD_POST, oscCommandHandler));
	httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/commands/status", METHOD_GET | METHOD_POST, oscStatusHandler));

	httpServer->startHttpServer();

	return 0;
}