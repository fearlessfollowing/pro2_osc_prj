#include <iostream>
#include <memory>
#include <sys/ins_types.h>

#include "osc_server.h"



#if 0
bool handleListFileRequest(Json::Value& jsonReq, Json::Value& jsonReply)
{
    jsonReply[_name] = OSC_CMD_LIST_FILES;
    genErrorReply(jsonReply, DISABLED_COMMAND, "Storage device not exist");
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

#endif


int main(int argc, char *argv[]) 
{
	printf("----------------- OSC Server(V1.0) Start ----------------\n");
	OscServer* oscServer = OscServer::Instance();
	oscServer->startOscServer();

	printf("----------------- OSC Server normal exit ----------------\n");
	return 0;
}