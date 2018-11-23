/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2、Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: TempService.cpp
** 功能描述: 温度管理服务
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月23日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <util/ARHandler.h>
#include <util/ARMessage.h>
#include <util/msg_util.h>
#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>
#include <prop_cfg.h>
#include <system_properties.h>

#include <log/log_wrapper.h>

#include <sys/TempService.h>
#include <sys/ProtoManager.h>

#undef      TAG
#define     TAG "TempService"

#define MAX(a,b) (((a) > (b)) ? (a):(b))


bool TempService::mHaveInstance = false;

enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};

std::shared_ptr<TempService>& TempService::Instance()
{
    {
        std::unique_lock<std::mutex> lock(mInstanceLock);   
        if (mHaveInstance == false) {
            mInstance = std::make_shared<TempService>();
        }
    }
    return mInstance;
}


void TempService::writePipe(int p, int val)
{
    char c = (char)val;
    int  rc;

    rc = write(p, &c, 1);
    if (rc != 1) {
        LOGDBG(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}



TempService::TempService()
{
    mRunning = false;
    pipe(mCtrlPipe);
    LOGDBG(TAG, "---> constructor TempService now ...");
}


TempService::~TempService()
{
    LOGDBG(TAG, "---> deConstructor TempService now ...");
    close(mCtrlPipe[0]);
    close(mCtrlPipe[1]);
}

#define CPU_TEMP_PATH   "/sys/class/thermal/thermal_zone2/temp"
#define GPU_TEMP_PATH   "/sys/class/thermal/thermal_zone1/temp"
#define PROP_MODUE_TMP  "module.temp"



bool TempService::getNvTemp()
{

}

bool TempService::getBatteryTemp()
{

}

bool TempService::getModuleTemp()
{
    mModuleTmp = 45.0f;
}


bool TempService::reportSysTemp()
{   
    Json::Value param;
    
    param["nv_temp"] = MAX(mCpuTmp, mGpuTmp);
    param["bat_temp"] = mBatteryTmp;
    param["module_temp"] = mModuleTmp;

    return ProtoManager::Instance()->sendUpdateSysTempReq(param);
}


int TempService::serviceLooper()
{
    struct timeval to = {2, 0};

    while (true) {
        fd_set read_fds;
        int rc = 0;
        int max = -1;

        FD_SET(mCtrlPipe[0], &read_fds);	
        if (mCtrlPipe[0] > max)
            max = mCtrlPipe[0];    

        if ((rc = select(max + 1, &read_fds, NULL, NULL, &to)) < 0) {	
            LOGDBG(TAG, "----> select error occured here ...");
            continue;
        } else if (!rc) {   /* timeout */
            /* 读取并上报温度信息： CPU/GPU, BATTERY, MODULE */
            getNvTemp();
            getBatteryTemp();
            getModuleTemp();

            if (reportSysTemp()) {
                LOGDBG(TAG, "Report Sys Temperature Suc.");
            }
        }

        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {	    /* Pipe事件 */
            char c = CtrlPipe_Shutdown;
            TEMP_FAILURE_RETRY(read(mCtrlPipe[0], &c, 1));	
            if (c == CtrlPipe_Shutdown) {
                break;
            }
        }
    }

    LOGDBG(TAG, "---> exit serviceLooper normally.");
}


bool TempService::startService()
{
    {
        std::unique_lock<std::mutex> lock(mLock);   
        if (!mRunning) {
            mLooperThread = std::thread([this]{ serviceLooper(); });
        }
    }
}


bool TempService::stopService()
{
    std::unique_lock<std::mutex> lock(mLock);   
    if (mRunning) {
        writePipe(mCtrlPipe[1], CtrlPipe_Shutdown);
        if (mLooperThread.joinable()) {
            mLooperThread.join();
            mRunning = false;
        }
    } 
}
