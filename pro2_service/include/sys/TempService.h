#ifndef _TMP_SERVICE_H_
#define _TMP_SERVICE_H_

#include <mutex>
#include <thread>

class TempService {

public:
    TempService();
    ~TempService();

    std::shared_ptr<TempService>& Instance();

    bool startService();
    bool stopService();

private:

    float           mBatteryTmp;
    float           mCpuTmp;
    float           mGpuTmp;
    float           mModuleTmp;

    int             mCtrlPipe[2]; // 0 -- read , 1 -- write


    int             serviceLooper();
    bool            reportSysTemp();
    void            writePipe(int p, int val);
    
    bool            getNvTemp();
    bool            getBatteryTemp();
    bool            getModuleTemp();

    std::thread                         mLooperThread;
    std::mutex                          mLock; 
    std::mutex                          mInstanceLock;
    bool                                mRunning;
    static std::shared_ptr<TempService> mInstance;
    static bool                         mHaveInstance;
};


#endif /* _TMP_SERVICE_H_ */