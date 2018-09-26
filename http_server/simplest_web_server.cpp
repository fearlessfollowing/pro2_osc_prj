// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

#include "http_server.h"
#include <log/arlog.h>
#include <sys/types.h>
#include <log/stlog.h>
#include <system_properties.h>

#include <prop_cfg.h>

#undef  TAG
#define TAG "HttpServer"

#define LIST_ROOT_PATH  "/mnt"

static const char *s_http_port = "10000";


static void default_signal_handler(int sig)
{
    Log.d(TAG, "sig is %d", sig);
    if (sig != 15) {
        Log.d(TAG, "other handler sig error sig %d", sig);
    }

    exit(0);
}

static void pipe_signal_handler(int sig)
{
    Log.d(TAG, "ignore pipe signal handler");
}

static void registerSig(__sighandler_t func)
{
    signal(SIGTERM, func);
    signal(SIGHUP, func);
    signal(SIGUSR1, func);
    signal(SIGQUIT, func);
    signal(SIGINT, func);
    signal(SIGKILL, func);
}


static void printHttpReq(struct http_message* p)
{
    char tmpBuf[1024] = {0};
    char tmpName[512] = {0};
    char tmpVal[512] = {0};

    Log.d(TAG, "============== Request Line ===================");
    
    strncpy(tmpBuf, p->method.p, p->method.len);
    Log.d(TAG, "Request Method: %s", tmpBuf);
    
    memset(tmpBuf, 0, sizeof(tmpBuf));
    strncpy(tmpBuf, p->uri.p, p->uri.len);
    Log.d(TAG, "URI: %s", tmpBuf);
    
    memset(tmpBuf, 0, sizeof(tmpBuf));
    strncpy(tmpBuf, p->proto.p, p->proto.len);
    Log.d(TAG, "Proto Version: %s", tmpBuf);

    Log.d(TAG, "============== Head Line ===================");
    for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
        memset(tmpName, 0, sizeof(tmpName));
        memset(tmpVal, 0, sizeof(tmpVal));

        if (p->header_names[i].p) {
            strncpy(tmpName, p->header_names[i].p, p->header_names[i].len);
            strncpy(tmpVal, p->header_values[i].p, p->header_values[i].len);

            Log.d(TAG, "Name: %s, Value:%s", tmpName, tmpVal);
        } else {
            break;
        }
    }

    Log.d(TAG, "============== Request Body ===================");
    Log.d(TAG, "Body: %s", p->body.p);

}


static void ev_handler(struct mg_connection *nc, int ev, void *p) 
{
    if (ev == MG_EV_HTTP_REQUEST) {
        // mg_serve_http(nc, (struct http_message *) p, s_http_server_opts);
        Log.d(TAG, "[%s: %d] Get Http Request now ...", __FILE__, __LINE__);
        printHttpReq((struct http_message *)p);
    }
}


int main(void) 
{
    int iRet = -1;
    struct mg_mgr mgr;
    struct mg_connection *nc;

    registerSig(default_signal_handler);
    signal(SIGPIPE, pipe_signal_handler);

    arlog_configure(true, true, HTTP_SERVER_LOG_PATH, false);	/* 配置日志 */

    iRet = __system_properties_init();		/* 属性区域初始化 */
    if (iRet) {
        Log.e(TAG, "File Http Web server init properties failed %d", iRet);
        return -1;
    }

    Log.d(TAG, "Service: http_server starting ^_^ !!");

    mg_mgr_init(&mgr, NULL);

    Log.d(TAG, "Starting web server on port %s MG_ENABLE_HTTP %d MG_ENABLE_TUN %d\n", s_http_port, MG_ENABLE_HTTP, MG_ENABLE_TUN);
    nc = mg_bind(&mgr, s_http_port, ev_handler);
    if (nc == NULL) {
        Log.e(TAG, "Failed to create listener");
        iRet = -1;
        goto EXIT;
    }

    mg_set_protocol_http_websocket(nc);

    while (true) {
        mg_mgr_poll(&mgr, 1000);
    }

EXIT:

    mg_mgr_free(&mgr);
    arlog_close();	
    return 0;
}
