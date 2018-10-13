

/*
 * 数字越小代表优先级越高
 */
enum {
    LOG_PRI_ERR = 0,
    LOG_PRI_WARN = 1,
    LOG_PRI_DEBUG = 2,
    LOG_PRI_INFO = 3,
    LOG_PRI_VERBOSE = 4,
};

#define DEFAULT_MAX_LOG_SIZE    (10*1024*1024)  // 10MB
#define DEFAULT_MAX_LOG_COUNT   10
#define DEFAULT_SAVE_LOG_LEVEL  (LOG_PRI_DEBUG)

/*
 * 日志管理器
 * 1.进程内全局唯一
 * 2.可以设置每个日志文件的最大大小以及日志最大分段个数
 * 
 */
class LogManager {

public:
    void v(const char *tag, const char *format, ...);
    void d(const char *tag, const char *format, ...);
    void i(const char *tag, const char *format, ...);
    void w(const char *tag, const char *format, ...);
    void e(const char *tag, const char *format, ...);

    LogManager();
    LogManager(int iMaxLogSize, int iMaxLogCount, int iSaveLevel);
    
    
    ~LogManager();

private:
	char            mLogFilePath[256];      /* 日志文件的名称 */			
	char            mLogFilePrefix[12];			
	int             mMaxLogFileSize;        /* 单个日志文件的最大大小 */				
	int             mMaxLogFileCount;       /* 支持的最大日志文件个数 */				

	FILE *          mCurLogFileHandle;      /* 当前日志文件的文件句柄 */			
	int             mCurLogFileSeq;         /* 当前文件的序列号 */
    int             mCurLogSize;            /* 当前日志文件的大小 */
    int             mSaveLogLevel;          /* 保存日志的最低级别,低于该级别的日志不会保存到文件 */					
	Mutex           mLogMutex;              /* 写日志的互斥锁 */

};


LogManager::LogManager(): mMaxLogFileSize(DEFAULT_MAX_LOG_SIZE),
                         mMaxLogFileCount(DEFAULT_MAX_LOG_COUNT),
                         mSaveLogLevel(DEFAULT_SAVE_LOG_LEVEL)
{

}

LogManager::LogManager(int iMaxLogSize, int iMaxLogCount, int iSaveLevel)
{

}

LogManager::~LogManager()
{

}

void LogManager::v(const char *tag, const char *format, ...)
{

}

void LogManager::d(const char *tag, const char *format, ...)
{

}


void LogManager::i(const char *tag, const char *format, ...)
{

}

void LogManager::w(const char *tag, const char *format, ...)
{

}

void LogManager::e(const char *tag, const char *format, ...)
{

}

/*
 * 全局变量
 */
LogManager Log;