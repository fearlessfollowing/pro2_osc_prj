/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: MidProto.cpp
** 功能描述: 协议管理(负责协议封装及派发)
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-08-04		创建文件，添加注释
******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <vector>

#include <sys/types.h>
#include <sys/wait.h>


#include <dirent.h>

#include <log/stlog.h>
#include <util/msg_util.h>
#include <util/ARMessage.h>

#include <sys/Process.h>
#include <sys/NetlinkManager.h>
#include <sys/VolumeManager.h> 
#include <sys/NetlinkEvent.h>

#include <sys/MidProto.h>

#include <hw/MenuUI.h>
#include <trans/fifo.h>

using namespace std;

#undef  TAG
#define TAG "MidProto"

MidProtoManager *MidProtoManager::sInstance = NULL;

static Mutex gMsgSendLock;
static Mutex gMsgDispatchLock;


#define ACTION_NAME(n) case n: return #n
const char *getProtoActionName(int iAction)
{
    switch (iAction) {
        ACTION_NAME(ACTION_REQ_SYNC);
        ACTION_NAME(ACTION_PIC);
        ACTION_NAME(ACTION_VIDEO);
        ACTION_NAME(ACTION_LIVE);
        ACTION_NAME(ACTION_PREVIEW);
        ACTION_NAME(ACTION_CALIBRATION);
        ACTION_NAME(ACTION_QR);
        ACTION_NAME(ACTION_SET_OPTION);
        ACTION_NAME(ACTION_LOW_BAT);
        ACTION_NAME(ACTION_SPEED_TEST);
        ACTION_NAME(ACTION_POWER_OFF);
        ACTION_NAME(ACTION_GYRO);
        ACTION_NAME(ACTION_NOISE);
        ACTION_NAME(ACTION_CUSTOM_PARAM);
        ACTION_NAME(ACTION_LIVE_ORIGIN);
        ACTION_NAME(ACTION_AGEING);
        ACTION_NAME(ACTION_AWB);
        ACTION_NAME(ACTION_SET_STICH);
        ACTION_NAME(ACTION_QUERY_STORAGE);
        ACTION_NAME(ACTION_FORMAT_TFCARD);

#ifdef ENABLE_MENU_STITCH_BOX
        ACTION_NAME(MENU_STITCH_BOX);
#endif

        ACTION_NAME(ACTION_QUIT_UDISK_MODE);
        ACTION_NAME(ACTION_UPDATE_REC_LEFT_SEC);
        ACTION_NAME(ACTION_UPDATE_LIVE_REC_LEFT_SEC);


    default: return "Unkown Action";
    }    
}


#define SENDER_NAME(n) case n: return #n
const char *getSendSrc(int iSend)
{
    switch (iSend) {
        SENDER_NAME(ACTION_REQ_SYNC);
        SENDER_NAME(ACTION_PIC);

    default: return "Unkown Sender";
    }    
}


MidProtoManager* MidProtoManager::Instance() 
{
    if (!sInstance)
        sInstance = new MidProtoManager();
    return sInstance;
}

MidProtoManager::MidProtoManager()
{
    Log.d(TAG, "[%s: %d] Constructor MidProtoManager now...", __FILE__, __LINE__);
    
    /* 构造用于传输的FIFO对象 */
}

void MidProtoManager::setRecvUi(sp<MenuUI> pMenuUI)
{
    mpUI = pMenuUI;
}

MidProtoManager::~MidProtoManager()
{
    Log.d(TAG, "[%s: %d] deConstructor MidProtoManager now...", __FILE__, __LINE__);
    /* 释放传输层对象 */
    
    if (sInstance) {
        delete sInstance;
    }
}

#if 0

void MidProtoManager::handleUiActionReq(int iEventType, int iAction)
{
    switch (iEventType) {
        case EVENT_BATTERY: {
            break;
        }

        case EVENT_NET_CHANGE: {
            break;
        }

        case EVENT_OLED_KEY: {
            break;
        }

        case EVENT_AGEING_TEST: {
            break;
        }

    }
}





void MidProtoManager::handleVolumeManagerActionReq(int iAction)
{

}
#endif


/*
 * 发送消息
 */
 bool MidProtoManager::sendProtoMsg(int iSendSrc, int iEventType, int iAction)
{
    #if 0
    Json::FastWriter writer;
    Json::Value root; 
    bool bNeedSend = false;

    Log.d(TAG, "[%s: %d] sendProtoMsg src[%s], action[%s]", 
                __FILE__, __LINE__, getSendSrc(iSendSrc), getActionName(iAction));

    switch (iSendSrc) {
        case SEND_PROTO_SRC_UI: {
            break;
        }

        case SEND_PROTO_SRC_VOL: {  /* 查询容量 */
            break;
        }

        default: {
            Log.e(TAG, "[%s: %d] Unkown Send Source!!", __FILE__, __LINE__);
            break;
        }
    }

    if (bNeedSend) {
        mTranFifo->write_fifo(iEventType, root.c_str());
    } 
    #endif

    return false;
        
}
    
int MidProtoManager::disptachMsg(int iMsgType, Json::Value* pJsonRoot)
{
    return 0;
}

void MidProtoManager::start()
{
    Log.d(TAG, "[%s: %d] Start MidProtoManager now...", __FILE__, __LINE__);
    mTranFifo = (sp<fifo>)(new fifo());

}

void MidProtoManager::setNotify(sp<ARMessage> notify)
{
    mNotify = notify;
}
