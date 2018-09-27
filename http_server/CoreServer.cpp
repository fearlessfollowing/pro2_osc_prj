#include <CoreServer.h>



InterRpc::InterRpc(): mSyncSendFd(-1),
                      mSyncRecvFd(-1),
                      mAsyncSyncFd(-1),
{
    mSyncReqResult.clear();
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

}


bool InterRpc::init()
{

}

void InterRpc::deInit()
{

}