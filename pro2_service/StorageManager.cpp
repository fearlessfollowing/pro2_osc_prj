#include <future>
#include <vector>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <common/include_common.h>

#include <util/ARHandler.h>
#include <util/ARMessage.h>


#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>

#include <sys/ioctl.h>

#include <sys/net_manager.h>
#include <util/bytes_int_convert.h>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <prop_cfg.h>

#include <trans/fifo.h>

#include <sys/StorageManager.h>

using namespace std;

#define MIN_STORAGE_LEFT_SIZE	(1024 * 1024 * 1024)


#define TAG "StorageManager"

static sp<StorageManager> gSysStorageManager = NULL;
static std::mutex gSysStorageMutex;

static const char *dev_type[SET_STORAGE_MAX] = {"sd", "usb"};


enum {
	STORAGE_ACTION_ADD,
	STORAGE_ACTION_REMOVE,
	STORAGE_ACTION_CHANGE,
	STORAGE_ACTION_MAX
};

sp<StorageManager> StorageManager::getSystemStorageManagerInstance()
{
    unique_lock<mutex> lock(gSysStorageMutex);
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
	sp<Volume> tmpVol;
	
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

	sp<ARMessage> tmpMsg = (sp<ARMessage>)(new ARMessage(MSG_GET_OLED_KEY));

	/* 清除远端的存储设备列表 */
	mRemoteStorageList.clear();

	//tmpMsg->what = MSG_GET_OLED_KEY;
	tmpMsg->set<int>("what", ACTION_QUERY_STORAGE);

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


int StorageManager::getDevTypeIndex(char *type)
{
    int storage_index;
    if (strcmp(type, dev_type[SET_STORAGE_SD]) == 0) {
        storage_index = SET_STORAGE_SD;
    } else if (strcmp(type, dev_type[SET_STORAGE_USB]) == 0) {
        storage_index = SET_STORAGE_USB;
    } else {
        Log.e(TAG, "error dev type %s ", type);
    }
    return storage_index;
}


void StorageManager::updateNativeStorageDevList(std::vector<sp<Volume>> & mDevList, )
{
	int dev_change_type;
	
		// 0 -- add, 1 -- remove, -1 -- do nothing
	int bAdd = CHANGE;
	bool bDispBox = false;
	bool bChange = true;
		
	if (bFirstDev) {	/* 第一次发送 */
			
		bFirstDev = false;
		mLocalStorageList.clear();
		
		switch (mDevList.size()) {
			case 0:
				mSavePathIndex = -1;
				break;
				
			case 1: {
				sp<Volume> dev = sp<Volume>(new Volume());
				memcpy(dev.get(), mDevList.at(0).get(), sizeof(Volume));
				mLocalStorageList.push_back(dev);
				mSavePathIndex = 0;
				break;
			}
					
				
			case 2:
				for (u32 i = 0; i < mDevList.size(); i++) {
					sp<Volume> dev = sp<Volume>(new Volume());
					memcpy(dev.get(), mDevList.at(i).get(), sizeof(Volume));
					if (getDevTypeIndex(mDevList.at(i)->dev_type) == SET_STORAGE_USB) {
						mSavePathIndex = i;
					}
					mLocalStorageList.push_back(dev);
				}
				break;
					
			default:
				Log.d(TAG, "strange bFirstDev mList.size() is %d", mDevList.size());
				break;
		}
		send_save_path_change();	/* 将当前选中的存储路径发送给Camerad */
	} else {
		Log.d(TAG, " new save list is %d , org save list %d", mDevList.size(), mLocalStorageList.size());
		if (mDevList.size() == 0) {
			bAdd = REMOVE;
			mSavePathIndex = -1;
		
			if (mLocalStorageList.size() == 0) {
				Log.d(TAG, "strange save list size (%d %d)", mDevList.size(), mLocalStorageList.size());
				mLocalStorageList.clear();
				//send_save_path_change();
				bChange = false;
			} else {
				dev_change_type = getDevTypeIndex(mLocalStorageList.at(0)->dev_type);
				mLocalStorageList.clear();
				bDispBox = true;
			}
		} else {
			if (mDevList.size() < mLocalStorageList.size()) {
				//remove
				bAdd = REMOVE;
				switch (mDevList.size()) {
					case 1:
						if (getDevTypeIndex(mDevList.at(0)->dev_type) == SET_STORAGE_SD) {
							dev_change_type = SET_STORAGE_USB;
							mSavePathIndex = 0;
						} else {
							dev_change_type = SET_STORAGE_SD;
							bChange = false;
						}
						mLocalStorageList.clear();
						mLocalStorageList.push_back(mDevList.at(0));
						bDispBox = true;
						break;

					default:
						Log.d(TAG,"2strange save list size (%d %d)", mDevList.size(), mLocalStorageList.size());
						break;
				}
			} else if (mDevList.size() > mLocalStorageList.size()) {	//add
				bAdd = ADD;
				if (mLocalStorageList.size() == 0) {
					dev_change_type = getDevTypeIndex(mDevList.at(0)->dev_type);
					sp<Volume> dev = sp<Volume>(new Volume());
					memcpy(dev.get(), mDevList.at(0).get(), sizeof(Volume));
					mSavePathIndex = 0;
					mLocalStorageList.push_back(dev);
					bDispBox = true;
				} else {
					switch (mDevList.size()) {
						case 2:
							dev_change_type = getDevTypeIndex(mLocalStorageList.at(0)->dev_type);
							if (dev_change_type == SET_STORAGE_USB) {
								// new insert is sd
								dev_change_type = SET_STORAGE_SD;
							} else {
								// new insert is usb
								dev_change_type = SET_STORAGE_USB;
							}
							
							mLocalStorageList.clear();
							for (unsigned int i = 0; i < mDevList.size(); i++) {
								sp<Volume> dev = sp<Volume>(new Volume());
								memcpy(dev.get(), mDevList.at(i).get(), sizeof(Volume));
								if (dev_change_type == SET_STORAGE_USB && getDevTypeIndex(mDevList.at(i)->dev_type) == SET_STORAGE_USB) {
									mSavePathIndex = i;
									bChange = true;
								}
								mLocalStorageList.push_back(dev);
							}
							bDispBox = true;
							break;

						default:
							Log.d(TAG, "3strange save list size (%d %d)", mDevList.size(), mLocalStorageList.size());
							break;
					}
				}
			} else {
				Log.d(TAG, "5strange save list size (%d %d)", mDevList.size(), mLocalStorageList.size());
			}
		}
	
		if (mLocalStorageList.size() == 0 ) {
			if (mSavePathIndex != -1) {
				mSavePathIndex = -1;
			}
		} else if (mSavePathIndex >= (int)mLocalStorageList.size()) {
			mSavePathIndex = mLocalStorageList.size() - 1;
			Log.e(TAG, "force path select %d ", mSavePathIndex);
		}
		
		if (bDispBox) {
			disp_dev_msg_box(bAdd, dev_change_type, bChange);
		}
	}
}




int StorageManager::getCurStorageMode()
{
	return 0;
}









