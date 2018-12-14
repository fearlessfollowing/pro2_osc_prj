#include <iostream>
#include <memory>
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

#if 0
bootloader:		  Titan
内核:	 		   Pro2
文件系统: 		    Titan


Pro2 bootloader， kernel titian文件系统
#endif 


static bool genDefaultOptions(Json::Value& optionsJson)
{

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
	optionsJson["shutterSpeedSupport"][0] = 0;
	optionsJson["shutterSpeedSupport"][1] = 0.5f;
	optionsJson["shutterSpeedSupport"][2] = 0.333f;
	optionsJson["shutterSpeedSupport"][3] = 0.25f;
	optionsJson["shutterSpeedSupport"][4] = 0.2f;
	optionsJson["shutterSpeedSupport"][5] = 0.125f;
	optionsJson["shutterSpeedSupport"][6] = 0.1f;
	optionsJson["shutterSpeedSupport"][7] = 0.067f;


    "shutterSpeedSupport":[0.05,
                           0.04, 0.033, 0.025, 0.02, 0.0167, 0.0125, 0.01,
                           0.0083, 0.00625, 0.005, 0.004167, 0.003125, 0.0025,
                           0.002, 0.0015625, 0.00125, 0.001, 0.0008, 0.000625,
                           0.0005, 0.0004, 0.0003125, 0.00025, 0.0002,
                           0.00015625, 0.000125],



	optionsJson["aperture"] = 2.4f;
	optionsJson["apertureSupport"][0] = 2.4f;


	optionsJson["whiteBalance"] = "auto";
	optionsJson["whiteBalanceSupport"][0] = "auto";
	optionsJson["whiteBalanceSupport"][1] = "incandescent";
	optionsJson["whiteBalanceSupport"][2] = "daylight";
	optionsJson["whiteBalanceSupport"][3] = "cloudy-daylight";

	optionsJson["exposureCompensation"] = 0;
    
    "exposureCompensationSupport":[-3, -2.5, -2, -1.5, -1, -0.5, 0, 0.5, 1, 1.5,
                                    2, 2.5, 3],
 
 
 	optionsJson["fileFormat"]["type"] = "jpeg";
 	optionsJson["fileFormat"]["width"] = 4000;
 	optionsJson["fileFormat"]["height"] = 3000;

    "fileFormatSupport": [
        {
            "type":"jpeg",
            "width": 4000,
            "height": 3000
        },
        {
            "type":"mp4",
            "width": 7680,
            "height": 3840,
            "framerate": 30
        },
        {
            "type":"mp4",
            "width": 5120,
            "height": 2560,
            "framerate": 30
        },
        {
            "type":"mp4",
            "width": 6400,
            "height": 3200,
            "framerate": 30
        },
        {
            "type":"mp4",
            "width": 4320,
            "height": 2160,
            "framerate": 30
        },
        {
            "type":"mp4",
            "width": 3840,
            "height": 1920,
            "framerate": 30
        },
        {
            "type":"mp4",
            "width": 3840,
            "height": 1920,
            "framerate": 120
        }
    ],

    "exposureDelay":0,
    "exposureDelaySupport":[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 60],


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
	optionsJson["hdr"][0] = "off";
	optionsJson["hdr"][1] = "hdr";


    "exposureBracket":{
        "shots": 3,
        "increment": 1
    },

    "exposureBracketSupport":{
        "autoMode": false,
        "shotsSupport": [3],
        "incrementSupport": [1, 2]
    },

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




	return true;
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


bool handleGetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{

}

bool handleSetOptionsRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{

}



bool handleTakePictureRequest(Json::Value& jsonReply)
{

}

bool handleSwitchWifiRequest(Json::Value& jsonReply)
{

}

bool handleDeleteRequest(Json::Value& jsonReply)
{

}


bool handleGetLivePreviewRequest(Json::Value& jsonReply)
{

}




bool handleListFileRequest(Json::Value& jsonReply)
{

}


bool handleStartCaptureRequest(Json::Value& jsonReply)
{

}


bool handleStopCaptureRequest(Json::Value& jsonReply)
{

}

bool handleUploadFileRequest(Json::Value& jsonReply)
{

}

bool handleResetRequest(Json::Value& jsonReply)
{

}


bool handleProcessPictureRequest(Json::Value& jsonReply)
{

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
				handleGetOptionsRequest(reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_SET_OPTIONS) {
				handleSetOptionsRequest(reqBody, reqReply);
			} else if (oscCmd == OSC_CMD_LIST_FILES) {

			} else if (oscCmd == OSC_CMD_TAKE_PICTURE) {
				handleTakePictureRequest(reqReply);
			} else if (oscCmd == OSC_CMD_START_CAPTURE) {

			} else if (oscCmd == OSC_CMD_STOP_CAPTURE) {

			} else if (oscCmd == OSC_CMD_DELETE) {

			}



		}
	} else {
		fprintf(stderr, "Parse request body failed\n");
	}

	sendResponse(conn, "", true);
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