#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <memory>
#include <iostream>
#include <string>
#include <json/value.h>
#include <json/json.h>
#include <sstream>
#include <stdio.h>

#include <prop_cfg.h>
#include "CoreServer.h"

#define SEND_SYNC_DATA_PATH     "/home/nvidia/insta360/fifo/ins_fifo_to_server"
#define RECV_SYNC_DATA_PATH     "/home/nvidia/insta360/fifo/ins_fifo_to_client"
#define RECV_ASYNC_DATA_PATH    "/home/nvidia/insta360/fifo/ins_fifo_to_client_a"
#define SEND_RESET_CMD_PATH     "/home/nvidia/insta360/fifo/ins_fifo_to_server_father"
#define RECV_RESET_CMD_PATH     "/home/nvidia/insta360/fifo/ins_fifo_to_client_father"

#undef  TAG
#define TAG "CoreServer"

static const int HEAD_LEN = 8;
static const int CONTENT_LEN_OFF = 4;

InterRpc::InterRpc(): mSyncSendFd(-1),
                      mSyncRecvFd(-1),
                      mAsyncSyncFd(-1),
                      mWriteSeq(0),
                      mReadSeq(0)

{
    mSyncReqResult.clear();
    memset(mSyncBuf, 0, sizeof(mSyncBuf));

    init();
}


InterRpc::~InterRpc()
{
    deInit();
    mSyncSendFd = -1;
    mSyncRecvFd = -1;
    mAsyncSyncFd = -1;
}


bool InterRpc::sendSyncReq(Json::Value& jsonReq, int iTimeout)
{
    bool bResult = false;
    std::string reqStr = "";
    ostringstream os;
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";

    AutoMutex _l(mSyncWriteLock);

    if (mSyncSendFd < 0) {
        mSyncSendFd = open(SEND_SYNC_DATA_PATH, O_WRONLY);
    }

    if (mSyncSendFd < 0) {
        Log.e(TAG, "[%s: %d] ---- Open Sync Send Channel failed!", __FILE__, __LINE__);
        return bResult;
    } 

    memset(mSyncBuf, 0, sizeof(mSyncBuf));

    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &os);
    resultStr = os.str();

    /* 1.将同送的请求写入发送通道 StreamWriterBuilder */
    u32 uContentLen = resultStr.length();
    if (uContentLen > MAX_DATA_LEN - HEAD_LEN) {

    } else {
        /* 填充数据 */
        fillDataTypeU32(mSyncBuf, );
    }

    /* 2.读取响应 */
}


bool InterRpc::init()
{
    bool bResult;
    int iRet = 0;

    iRet = mkfifo(SEND_SYNC_DATA_PATH, 0700);
    iRet |= mkfifo(RECV_SYNC_DATA_PATH, 0700);
    iRet |= mkfifo(RECV_ASYNC_DATA_PATH, 0700);
    iRet |= mkfifo(SEND_RESET_CMD_PATH, 0700);
    iRet |= mkfifo(RECV_RESET_CMD_PATH, 0700);

    if (iRet) {
        bResult = false;
    } else {
        bResult = true;
    }
    return bResult;
}

void InterRpc::fillDataTypeU32(char* pBuf, u32 uData)
{
    pBuf[3] = uData & 0xFF;
    uData >>= 8;
    pBuf[2] = uData & 0xFF;
    uData >>= 8;
    pBuf[1] = uData & 0xFF;
    uData >>= 8;
    pBuf[0] = uData & 0xFF;

}


void InterRpc::deInit()
{

    unlink(SEND_SYNC_DATA_PATH);
    unlink(RECV_SYNC_DATA_PATH);
    unlink(RECV_ASYNC_DATA_PATH);
    unlink(SEND_RESET_CMD_PATH);
    unlink(RECV_RESET_CMD_PATH);                

}