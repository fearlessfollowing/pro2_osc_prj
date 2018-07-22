#include <sys/StorageManager.h>


using namespace std;

#define MIN_STORAGE_LEFT_SIZE	(1024 * 1024 * 1024)


static sp<StorageManager> gSysStorageManager = NULL;
static sp<mutex> gSysStorageMutex = NULL;


enum {
	STORAGE_ACTION_ADD,
	STORAGE_ACTION_REMOVE,
	STORAGE_ACTION_CHANGE,
	STORAGE_ACTION_MAX
};

sp<StorageManager> StorageManager::getSystemStorageManagerInstance()
{
    unique_lock<mutex> lock(gSysStorageManager);
    if (gSysStorageManager != NULL) {
        return gSysStorageManager;
    } else {
        gSysStorageManager = sp<StorageManager> (new StorageManager());
    }
    return gSysStorageManager;
}

StorageManager::StorageManager():mMinStorageSpce(0),
										mCanTakePicNum(0),
										mCanTakeVidTime(0),
										mCanTakeLiveTime(0)
{
	Log.d(TAG, ">>>>> StorageManager constructer");
	mRemoteStorageList.clear();
	mLocalStorageList.clear();
}


StorageManager::~StorageManager()
{
	Log.d(TAG, ">>>>> StorageManager deconstructer");
}



/*
 * 查询存储空间会被阻塞,可设置超时等待时间
 */
bool StorageManager::queryCurStorageState()
{
	mMinStorageSpce = ~0L;
	
	/* 1.检查大卡是否存在
	 * 1.1大卡不存在,直接返回false
	 * 1.2打开存在,跟新大卡的状态,开始查询小卡
	 * 	1.2.1小卡不存在,或查询失败返回false
	 *  1.2.2小卡存在,更新各个卡的容量,返回true
	 */
	if (queryLocalStorageState() == false) {
		return false;
	} else {
		return queryRemoteStorageState();
	}
}


bool StorageManager::queryLocalStorageState()
{
	sp<Volum> tmpVol;
	
	if (mLocalStorageList.size() == 0) {
		return false;
	} else {	
		for (u32 i = 0; i < mLocalStorageList.size(); i++) {
			tmpVol = mLocalStorageList.at(i);
			Log.d(TAG, "Local device [%s], remain size [%ld]M", tmpVol->mName, tmpVol->mLeftSize);
			if (tmpVol->mLeftSize < mMinStorageSpce) {
				mMinStorageSpce = tmpVol->mLeftSize;
			}
		}
	}
	return true;
}


bool StorageManager::queryRemoteStorageState()
{

	sp<ARMessage> tmpMsg;

	/* 清除远端的存储设备列表 */
	mRemoteStorageList.clear();

	tmpMsg.what = MSG_GET_OLED_KEY;
	msg->set<int>("what", ACTION_QUERY_STORAGE);

	/* 发送异步消息给Camerad,并在此处等待 */
	fifo::getSysTranObj()->postTranMessage(tmpMsg, 0);
	
	/* 接收消息线程接收到数据后唤醒调用线程 */

	/* 查询mRemoteStorageList是否有6张卡,并计算出最小卡容量
	 * 容量值小于阀值显示存储空间已满
	 * 容量值大于阀值,
	 */

	
}


u64 StorageManager::getMinLefSpace()
{
	if (mMinStorageSpce < MIN_STORAGE_LEFT_SIZE) {
		return 0;
	} else 
		return mMinStorageSpce;
}



/*
 * 具备动态的异步更新存储容量的功能(对于本地卡)
 */
void StorageManager::updateStorageDevice(int iAction, sp<Volume>& pVol)
{

	/* 唤醒等待线程 */

	switch (iAction) {
	case STORAGE_ACTION_ADD:		/* 新增存储设备 */
		
		break;

	case STORAGE_ACTION_REMOVE:		/* 移除存储设备 */
		break;

	case STORAGE_ACTION_CHANGE:		/* 存储设备改变 */
		break;

	default:
		Log.e(TAG, "Unkown ACTION for StorageManager");
		break;
	}
}


int StorageManager::getCurStorageMode()
{

}









