#include <iostream>
#include <memory>
#include <sys/ins_types.h>

#include "osc_server.h"






Json::Value& getCurOptions()
{
	return gOptions;
}






/*
 * Example:
 * Request:
 * {
 * 		"name": "camera.getOptions",
 * 		"parameters": {
 * 			"optionNames": ["captureModeSupport"]
 * 		}
 * }
 * Response:
 * {
 * 		"name": "camera.getOptions",
 * 		"results": {
 * 			"options": {
 * 				"captureModeSupport": ["image", "interval", "video"]
 * 			}
 * 		},
 * 		"state": "done"
 * }
 */

bool handleGetOptionsRequest(Json::Value& sysCurOptions, Json::Value& jsonReq, Json::Value& jsonReply)
{
	jsonReply.clear();
	jsonReply["name"] = OSC_CMD_GET_OPTIONS;

	if (jsonReq.isMember(_param)) {
		if (jsonReq[_param].isMember("optionNames")) {
			Json::Value optionNames = jsonReq[_param]["optionNames"];
			Json::Value resultOptions;
			u32 i = 0;

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
 * Example:
 * {
 * 		"name": "camera.setOptions",
 * 		"parameters": {
 * 			"options": {
 * 				"captureMode": "video"
 * 			}
 * 		}
 * }
 * Response:
 * {
 * 		"name": "camera.setOptions",
 * 		"state": "done"
 * }
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


bool handleListFileRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{
    jsonReply[_name] = OSC_CMD_LIST_FILES;
    genErrorReply(jsonReply, DISABLED_COMMAND, "Storage device not exist");
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



/*
 * command: getLiewPreview
 * Example:
 * {
 * 		"name":"camera.getLivePreview"
 * }
 * Response:
 * 1.首先发响应头:
 * HTTP/1.1 200 OK
 * Connection: Keep-Alive
 * Content-Type: multipart/x-mixed-replace; boundary="---osclivepreview---"
 * X-Content-Type-Options: nosniff
 * Transfer-Encoding: Chunked
 * \r\n
 * 5d2ba
 * ---osclivepreview---
 * Content-type: image/jpeg
 * Content-Length: 381548
 * \r\n
 * JFIF图片数据
 * \r\n(\r\n)
 * 5d479
 * ---osclivepreview---
 * Content-type: image/jpeg
 * Content-Length: 381995
 * \r\n
 * JFIF图片数据
 * 
 */

static void sendGetLivePreviewResponseHead(mg_connection *conn)
{
    mg_printf(conn, "HTTP/1.1 200 OK\r\n"	\
            "Connection: Keep-Alive\r\n"	\
            "Content-Type: multipart/x-mixed-replace; boundary=\"---osclivepreview---\"\r\n"	\
            "X-Content-Type-Options:nosniff\r\n"	\
            "Transfer-Encoding: Chunked\r\n");    
}

static void sendOneFrameData(mg_connection *conn, int iFrameId)
{
    printf("send one frame data here, id = %d\n", iFrameId);
    char filePath[512] = {0};
    char buffer[1024] = {0};

    sprintf(filePath, "/home/nvidia/Pictures/pic%d.jpg", iFrameId);
    int iFd = open(filePath, O_RDONLY);
    if (iFd > 0) {
        struct stat fileStat;
        fstat(iFd, &fileStat);
        int iFileSize = fileStat.st_size;
        int iReadLen;
        mg_printf(conn, 
                "%x\r\n"    \
                "---osclivepreview---\r\n"  \
                "Content-type: image/jpeg\r\n"  \
                "Content-Length: %d\r\n\r\n", iFileSize + 78, iFileSize);
        
        while ((iReadLen = read(iFd, buffer, 1024)) > 0) {
            mg_send(conn, buffer, iReadLen);            
        }
        mg_send(conn, "\r\n", strlen("\r\n"));
        close(iFd);
    } else {
        fprintf(stderr, "open File[%s] failed\n", filePath);
    }
    /* 发送头部
     * %x\r\n
     * ---osclivepreview---\r\n             22
     * Content-type: image/jpeg\r\n         26
     * Content-Length: 381548\r\n           24
     * \r\n
     *
     */

}

/*
 * 接收到getLivePreview，创建一个发送预览数据的线程
 * 线程监听是否退出的
 */

bool handleGetLivePreviewRequest(mg_connection *conn, Json::Value& jsonReq, Json::Value& jsonReply)
{

    printf("------------------------------> getLivePrevew request here.....\n");    
    /*
     * 1.发送响应头
     * 2.根据当前的配置帧率启动一个定时器(1/5HZ)，可以直接用select监听改socket
     * 3.发送数据
     */
    sendGetLivePreviewResponseHead(conn);

#if 1
    int iSockFd = conn->sock;
    bool bFirstTime = true;
    int iIndex = 0;
    
    printf("In livepreview response, socket fd = %d\n", iSockFd); 

    while (true) {

        fd_set read_fds;
        fd_set write_fds;
        int rc = 0;
        int max = -1;
        struct timeval timeout;        

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        if (iSockFd > 0) {
            FD_SET(iSockFd, &write_fds);	
            if (iSockFd > max)
                max = iSockFd;
        }

        if (bFirstTime) {
            bFirstTime = false;
            timeout.tv_sec  = 0;
            timeout.tv_usec = 0;            
        } else {
            timeout.tv_sec  = 0;
            timeout.tv_usec = 200*1000;    // 1000 * 1000 us / 5
        }

        if ((rc = select(max + 1, &read_fds, &write_fds, NULL, &timeout)) < 0) {	
            fprintf(stderr, "----> select error occured here, maybe socket closed.\n");
            break;
        } else if (!rc) {   /* 超时了，也需要判断是否可以写数据了 */
            printf("select timeout, maybe send network data slowly.\n");
        }

        if (FD_ISSET(iSockFd, &write_fds)) {    /* 数据发送完成，可以写下一帧数据了 */
            sendOneFrameData(conn, iIndex);
            iIndex = (iIndex+1) % 5;
        }

    }
#endif
	return true;
}



/*
 * Example:
 * {
 * 		"name": "camera.delete",
 * 		"parameters": {	
 * 			"fileUrls": ["http://192.168.1.1/files/150100525831424d4207a52390afc300/100RICOH/R0012284.JPG"]
 * 		}
 * }
 * 参数的特殊情况:
 * "fileUrls"列表中只包含"all", "image", "video"
 * 成功:
 * {
 * 		"name": "camera.delete",
 * 		"state": "done"
 * }
 * 失败
 * 1.参数错误
 * {
 * 		"name":"camera.delete",
 * 		"state":"error",
 * 		"error": {
 * 			"code": "missingParameter",	// "missingParameter": 指定的fileUrls不存在; "invalidParameterName":不认识的参数名; "invalidParameterValue":参数名识别，值非法
 * 			"message":"XXXX"
 * 		}
 * }
 * 2.文件删除失败
 * {
 * 		"name":"camera.delete",
 * 		"state":"done",
 * 		"results":{
 * 			"fileUrls":[]		// 删除失败的URL列表
 * 		}
 * }
 */
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
				for (u32 i = 0; i < delUrls.size(); i++) {
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


int main(int argc, char *argv[]) 
{
	printf("----------------- OSC Server(V1.0) Start ----------------\n");
	OscServer* oscServer = OscServer::Instance();
	oscServer->startOscServer();

	printf("----------------- OSC Server normal exit ----------------\n");
	return 0;
}