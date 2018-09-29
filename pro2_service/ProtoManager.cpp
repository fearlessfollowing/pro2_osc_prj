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
#include <util/ARMessage.h>

#include <json/json.h>

#include <sys/Mutex.h>

/*********************************************************************************************
 *  输出日志的TAG(用于刷选日志)
 *********************************************************************************************/
#undef    TAG
#define   TAG "ProtoManager"

/*********************************************************************************************
 *  宏定义
 *********************************************************************************************/


/*********************************************************************************************
 *  外部函数
 *********************************************************************************************/


/*********************************************************************************************
 *  全局变量
 *********************************************************************************************/
ProtoManager *ProtoManager::sInstance = NULL;
static Mutex gProtoManagerMutex;


ProtoManager* ProtoManager::Instance() 
{
    AutoMutex _l(gProtoManagerMutex);
    if (!sInstance)
        sInstance = new ProtoManager();
    return sInstance;
}


ProtoManager::ProtoManager()
{
    Log.d(TAG, "[%s: %d] Constructor ProtoManager now ...", __FILE__, __LINE__);
    mCurRecvData.clear();
}


ProtoManager::~ProtoManager()
{

}


/*
 * 发送同步请求
 */
void ProtoManager::sendHttpSyncReq(const std::string &url, ReqCallback req_callback)
{

}


void ProtoManager::sendHttpSyncReq(const std::string &url, ReqCallback req_callback, const char* extra_headers, const char* post_data)
{

}

void ProtoManager::onHttpEvent(mg_connection *connection, int event_type, void *event_data)
{

}


