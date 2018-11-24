#ifndef _TMP_SERVICE_H_
#define _TMP_SERVICE_H_

#include <mutex>
#include <thread>
#include <hw/battery_interface.h>

class TempService {

public:
    TempService();
    ~TempService();

    static std::shared_ptr<TempService>& Instance();

    void startService();
    void stopService();

private:

    float           mBatteryTmp;
    float           mCpuTmp;
    float           mGpuTmp;
    float           mModuleTmp;

    int             mCtrlPipe[2]; // 0 -- read , 1 -- write


    int             serviceLooper();
    bool            reportSysTemp();
    void            writePipe(int p, int val);
    
    void            getNvTemp();
    void            getBatteryTemp();
    void            getModuleTemp();

    std::thread                         mLooperThread;
    std::mutex                          mLock; 
    static std::mutex                   mInstanceLock;
    bool                                mRunning;
    static std::shared_ptr<TempService> mInstance;
    static bool                         mHaveInstance;
    std::shared_ptr<battery_interface>  mBattery;

};


#endif /* _TMP_SERVICE_H_ */