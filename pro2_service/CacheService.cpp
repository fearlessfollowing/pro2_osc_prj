/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2/Titan Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: CacheService.cpp
** 功能描述: 缓存管理器实现
**          将外部存储设备中文件以缓存的形式存储在设备内部存储设备的db中，当需要列出外部存储设备文件时，先从缓存中提取
**          用于加快提取速度
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年11月27日
** 修改记录:
** V1.0			Skymixos		2018-11-27		创建文件，添加注释
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>
#include <prop_cfg.h>

#include <system_properties.h>

#include <sys/CacheService.h>

#include <log/log_wrapper.h>


#undef      TAG
#define     TAG "CacheService"


#define  DEFAULT_CACHE_DB_PATH      "/home/nvidia/insta360/db"
#define  DEFAULT_DB_NAME            "insta360.db"

#define  PROP_DB_PATH    "sys.db_path"

static bool mHaveInstance = false;
static std::mutex gInstanceLock;
static std::shared_ptr<CacheService> gInstance;

std::shared_ptr<CacheService>& CacheService::Instance()
{
    {
        std::unique_lock<std::mutex> lock(gInstanceLock);   
        if (mHaveInstance == false) {
            mHaveInstance = true;
            gInstance = std::make_shared<CacheService>();
        }
    }
    return gInstance;
}


CacheService::CacheService()
{
    LOGDBG(TAG, "---> constructor CacheService now ...");
    init();
}

CacheService::~CacheService()
{
    LOGDBG(TAG, "---> deConstructor CacheService now ...");
    deInit();
}


void CacheService::init()
{
    mListVec.clear();
    
    /* 初始化数据库 */
    const char* pDbPath = DEFAULT_CACHE_DB_PATH;
    if (property_get(PROP_DB_PATH)) {
        pDbPath = property_get(PROP_DB_PATH);
        LOGDBG(TAG, "Use property set db path -> [%s]", pDbPath);
    }

    if (access(pDbPath, F_OK) != 0) {
        mkdir(pDbPath, 0600);
    }

    std::string dbPathName = DEFAULT_CACHE_DB_PATH + "/" + DEFAULT_DB_NAME;
    if (access(dbPathName.c_str(), F_OK) != 0) {
        if (createDatabase(dbPathName.c_str())) {
            LOGDBG(TAG, "--> create db[%s] suc.", dbPathName.c_str());
        } else {
            LOGERR(TAG, "--> create db[%s] failed.", dbPathName.c_str());
        }
    } else {
        LOGDBG(TAG, "--> db[%s] have exist!", dbPathName.c_str());
    }

    mDbPathName = dbPathName;
    mCurTabName = "none";
}


void CacheService::deInit()
{
    LOGDBG(TAG, "In CacheService::deInit, do nothing...");
    mListVec.clear();    
}


/*
 * 在系统数据库中创建表
 */
bool CacheService::createCacheTable(std::string tabName)
{
    sqlite3 *db;
    int  rc;
    char *sql;
    char *zErrMsg = 0;
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH + "/" + DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> Create table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "CREATE TABLE " + tabName;
    sqlCmd += "(PATHNAME  TEXT PRIMARY KEY  NOT NULL,"  \
              "FILEURL    TEXT,"    \
              "NAME       TEXT,"    \
              "THUBNAIL   BLOB,"    \
              "FILESIZE   INTEGER," \
              "FILEATTR   INTEGER," \
              "FILEDATE   NUMERIC," \
              "FILEWIDHT  INTEGER," \
              "FILEHEIGHT INTEGER," \
              "LNG        REAL,"    \
              "LAT        REAL,"    \
              "ISPROCESS  NUMERIC );";

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: <%s>", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Table[%s] created in db[%s] successfully", tabName.c_str(), dbPathName.c_str());   
    }
    
    sqlite3_close(db);
    return bResult;
}


/* 
 * 删除系统数据库中指定的表
 * SQL: DROP TABLE IF EXISTS <tab_name>
 */
bool CacheService::delCacheTable(std::string tabName)
{
    sqlite3 *db;
    int  rc;
    char *sql;
    char *zErrMsg = 0;
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH + "/" + DEFAULT_DB_NAME;
    }

    LOGDBG(TAG, ">> Create table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    LOGDBG(TAG, ">> Drop table[%s] in db[%s]", tabName.c_str(), dbPathName.c_str());
    
    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "DROP TABLE IF EXISTS " + tabName + ";";

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: <%s>", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Table[%s] delete in db[%s] successfully", tabName.c_str(), dbPathName.c_str());   
    }
    sqlite3_close(db);
    return bResult;
}


/*
 * 在指定的表中，插入一行数据
 * 
 */
bool CacheService::insertCacheItem(std::string tabName, std::shared_ptr<CacheItem> ptrCacheItem)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    char *sql;
    char cInsertVal[1024] = {0};
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH + "/" + DEFAULT_DB_NAME;
    }

    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string cmdPart1 = "INSERT INFO " + tabName;
    cmdPart1 += "(PATHNAME,FILEURL,NAME,FILESIZE,FILEATTR,FILEDATE,FILEWIDHT,FILEHEIGHT,LNG,LAT,ISPROCESS) VALUES ";

    std::string sqlCmd = cmdPart1;
    sprintf(cInsertVal, "('%s','%s','%s',%d,%d,'%s',%d,%d,%f,%f,%d);", ptrCacheItem->fullPathName.c_str(),
                                                                       ptrCacheItem->fileUrl.c_str(),
                                                                       ptrCacheItem->fileName.c_str(),
                                                                       ptrCacheItem->fileSize,
                                                                       ptrCacheItem->fileAttr,
                                                                       ptrCacheItem->fileDate.c_str(),
                                                                       ptrCacheItem->u32Width,
                                                                       ptrCacheItem->u32Height,
                                                                       ptrCacheItem->fLng,
                                                                       ptrCacheItem->fLant,
                                                                       (ptrCacheItem->bProcess == true) ? 1:0);
    sqlCmd += cInsertVal;
    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Insert table line successfully");
    }
    sqlite3_close(db);
    return bResult;
}



bool CacheService::delCacheItem(std::string tabName, std::shared_ptr<CacheItem> ptrCacheItem)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int  rc;
    char *sql;
    char cInsertVal[1024] = {0};
    bool bResult = true;
    std::string dbPathName;

    if (mDbPathName.length() > 0) {
        dbPathName = mDbPathName;
    } else {
        dbPathName = DEFAULT_CACHE_DB_PATH + "/" + DEFAULT_DB_NAME;
    }

    rc = sqlite3_open(dbPathName.c_str(), &db);
    if (rc) {
        LOGERR(TAG, "Can't open database[%s]: reason: <%s>", dbPathName.c_str(), sqlite3_errmsg(db));
        return false;
    } 

    std::string sqlCmd = "DELETE FROM " + tabName + " WHERE PATHNAME='" + ptrCacheItem->fullPathName + "';";
    LOGDBG(TAG, "delete cmd: %s", sqlCmd.c_str());

    rc = sqlite3_exec(db, sqlCmd.c_str(), NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        LOGERR(TAG, "SQL error: %s", zErrMsg);
        sqlite3_free(zErrMsg);
        bResult = false;
    } else {
        LOGDBG(TAG, "Delete item[%s] in table[%s] successfully", ptrCacheItem->fullPathName.c_str(), tabName.c_str());
    }
    sqlite3_close(db);
    return bResult;
}



/*
 * 创建数据库
 */
bool CacheService::createDatabase(const char* dbName)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    
    rc = sqlite3_open(dbName, &db);
    if (rc) {
        LOGERR(TAG, "Can't open database(%s): reason [%s]", dbName, sqlite3_errmsg(db));
    } else {
        LOGDBG(TAG, "Opened database[%s] successfully", dbName);
    }
    sqlite3_close(db);
    return (rc == SQLITE_OK) ? true : false;
}



bool CacheService::recurFileList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1024];
    
    if ((dir = opendir(basePath)) == NULL)  {
        LOGDBG(TAG, "Open dir[%s] error...", basePath);
        return false;
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        } else {
            char path_name[1024] = {0};
            struct stat tmpStat;
            sprintf(path_name, "%s/%s", basePath, ptr->d_name);
            if (stat(path_name, &tmpStat)) {
                LOGERR(TAG, "stat path[%s] failed", path_name);
            } else {
                if (S_ISDIR(tmpStat.st_mode)) {
                    memset(base, '\0', sizeof(base));
                    strcpy(base, basePath);
                    strcat(base, "/");
                    strcat(base, ptr->d_name);
                    recurFileList(base);
                } else if (S_ISREG(tmpStat.st_mode)) {
                    // 构造一行记录，填入到对应的tab中
                } else if (S_ISLNK(tmpStat.st_mode)) {
                    LOGDBG(TAG, "file [%s] is link file", path_name);
                }
            }
        }
    }
    closedir(dir);
    return true;
}


bool CacheService::scanVolume(std::string volName)
{
    struct timeval sTime, eTime;
    long usedTime = 0;
    
    if (mCurTabName == "none") {
        mCurTabName = "insta360_tab";
    } else {
        
    }


    /*
     * 扫描指定目录下的所有文件（可加一个过滤器）
     */
    gettimeofday(&sTime, NULL);
    recurFileList((char*)(volName.c_str()));
    gettimeofday(&eTime, NULL);

    usedTime = (eTime.tv_sec - sTime.tv_sec) * 1000 + (eTime.tv_usec - sTime.tv_usec) / 1000;
    LOGDBG(TAG, "Scan dir[%s], total consumer: [%ld]ms", volName.c_str(), usedTime);
}


