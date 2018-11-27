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

#define MAX_VAL(a,b) (((a) > (b)) ? (a):(b))

#define CPU_TEMP_PATH   "/sys/class/thermal/thermal_zone2/temp"
#define GPU_TEMP_PATH   "/sys/class/thermal/thermal_zone1/temp"
#define PROP_MODUE_TMP  "module.temp"

#define INVALID_TMP_VAL     1000.0f

bool TempService::mHaveInstance = false;
static std::mutex gInstanceLock;
static std::shared_ptr<TempService> gInstance;

enum {
    CtrlPipe_Shutdown = 0,                  /* 关闭管道通知: 线程退出时使用 */
    CtrlPipe_Wakeup   = 1,                  /* 唤醒消息: 长按监听线程执行完依次检测后会睡眠等待唤醒消息的到来 */
    CtrlPipe_Cancel   = 2,                  /* 取消消息: 通知长按监听线程取消本次监听,说明按键已经松开 */
};

std::shared_ptr<TempService>& TempService::Instance()
{
    {
        std::unique_lock<std::mutex> lock(gInstanceLock);   
        if (mHaveInstance == false) {
            mHaveInstance = true;
            gInstance = std::make_shared<TempService>();
        }
    }
    return gInstance;
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
    mBattery = std::make_shared<battery_interface>();
    LOGDBG(TAG, "---> constructor TempService now ...");
}


TempService::~TempService()
{
    LOGDBG(TAG, "---> deConstructor TempService now ...");
    close(mCtrlPipe[0]);
    close(mCtrlPipe[1]);
}


void TempService::getNvTemp()
{
    mCpuTmp = INVALID_TMP_VAL;
    mGpuTmp = INVALID_TMP_VAL;

    char cCpuBuf[512] = {0};
    char cGpuBuf[512] = {0};
    int uCpuTemp = 0;
    int uGpuTemp = 0;

    FILE* fp1 = fopen(CPU_TEMP_PATH, "r");
    if (fp1) {
        fgets(cCpuBuf, sizeof(cCpuBuf), fp1);
        cCpuBuf[strlen(cCpuBuf) -1] = '\0';
        uCpuTemp = atoi(cCpuBuf);
        mCpuTmp = uCpuTemp / 1000.0f;
        fclose(fp1);
    }


    FILE* fp2 = fopen(GPU_TEMP_PATH, "r");
    if (fp2) {
        fgets(cGpuBuf, sizeof(cGpuBuf), fp2);
        cGpuBuf[strlen(cGpuBuf) -1] = '\0';
        uGpuTemp = atoi(cGpuBuf);
        mGpuTmp = uGpuTemp / 1000.0f;
        fclose(fp2);
    }

#ifdef ENABLE_DEBUG_TMPSERVICE
    LOGDBG(TAG, "CPU temp[%f]C, GPU temp[%f]C", mCpuTmp, mGpuTmp);
#endif
}

void TempService::getBatteryTemp()
{
    mBatteryTmp = INVALID_TMP_VAL;

    bool bCharge = false;
    double dInterTemp = 0.0f;
    double dOuterTemp = 0.0f;

    if (mBattery->read_charge(&bCharge) == 0) { /* 电池存在 */
        if(!mBattery->read_tmp(&dInterTemp, &dOuterTemp)) {
            mBatteryTmp = static_cast<int>(MAX_VAL(dInterTemp, dOuterTemp));
#ifdef ENABLE_DEBUG_TMPSERVICE
            LOGDBG(TAG, "Current Battery temp: [%f]C", mBatteryTmp);
#endif
        } else {
            LOGERR(TAG, "---> read battery temp failed");
        }
    } else {
#ifdef ENABLE_DEBUG_TMPSERVICE
        LOGDBG(TAG, "battery not exist !!!");
#endif 
    }
}

void TempService::getModuleTemp()
{
    mModuleTmp = INVALID_TMP_VAL;

    int iModuleTemp = 0;
    const char* pModuleTemp = property_get(PROP_MODUE_TMP);
    if (pModuleTemp) {
        iModuleTemp = atoi(pModuleTemp);
        mModuleTmp = iModuleTemp / 1.0f;
    #ifdef ENABLE_DEBUG_TMPSERVICE
        LOGDBG(TAG, "Current Module temp: [%f]C", mModuleTmp);
    #endif
    } else {
    #ifdef ENABLE_DEBUG_TMPSERVICE
        LOGERR(TAG, "--> get Module Temp prop failed");
    #endif
    }
}


bool TempService::reportSysTemp()
{   
    Json::Value param;
    param["nv_temp"] = MAX_VAL(mCpuTmp, mGpuTmp);
    param["bat_temp"] = mBatteryTmp;
    param["module_temp"] = mModuleTmp;
    return ProtoManager::Instance()->sendUpdateSysTempReq(param);
}

int TempService::serviceLooper()
{
    fd_set read_fds;
    struct timeval to;    
    int rc = 0;
    int max = -1;    
    const char* pPollTime = NULL;

    while (true) {

        to.tv_sec = 3;
        to.tv_usec = 0;

        pPollTime = property_get(PROP_POLL_SYS_TMP_PERIOD);
        if (pPollTime) {
            to.tv_sec = atoi(pPollTime);
        }

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
                // LOGDBG(TAG, "Report Sys Temperature Suc.");
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


void TempService::startService()
{
    {
        std::unique_lock<std::mutex> lock(mLock);   
        if (!mRunning) {
            mLooperThread = std::thread([this]{ serviceLooper(); });
        }
    }
}


void TempService::stopService()
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
