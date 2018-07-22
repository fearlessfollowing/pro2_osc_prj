#ifndef _STORAGE_MANAGER_H_
#define _STORAGE_MANAGER_H_

/*
 * 存储策略
 */
enum {
	STORAGE_POLICY_ALL_CARD,		/* 6TF + 1SD */
	STORAGE_POLICY_6F,
	STORAGE_POLICY_1SD,
	STORAGE_POLICY_MAX,
};

/*
 * 拍照的模式
 */
enum {
	8K_3D_OF,
	8K_3D_OF_RAW,
	8K_OF,
	8K_OF_RAW,
	8K,
	8K_RAW,
	AEB3,
	AEB3_RAW,
	AEB5,
	AEB5_RAW,
	AEB7,
	AEB7_RAW,
	AEB9,
	AEB9_RAW,
	BURST,
	BURST_RAW,
	CUSTOMER
};


/*
 * 录像的模式
 */
enum {
	8K_30F_3D,
	8K_60F,
	8K_30F,
	8K_5F_STREETVIEW,
	6K_60F_3D,
	6K_30F_3D,
	4K_120F_30,	//	(Binning)
	4K_60F_3D,
	4K_30F_3D,
	4K_60F_RTS,
	4K_30F_RTS,
	4K_30F_3D_RTS,
	2_5K_60F_3D_RTS,
	AERIAL,
	CUSTOMER
};

/*
 * 直播的模式
 */
enum {
	4K_30FPS,
	4K_30FPS_3D,
	4K_30FPS_HDMI,
	4K_30FPS_3D_HDMI,
	CUSTOMER,
};

#define VOLUME_NAME_MAX 32

/*
 * 对应存储卡
 */
typedef struct stVol {
	int  mTyep;						/* 存储设备的类型(SD卡, USB移动硬盘) */
	bool mStatus;					/* true:表示存在;    false表示不存在 */
	u64  mTotalSize;				/* 总大小单位为MB */
	u64  mLeftSize;					/* 剩余空间大小(单位MB) */
	char mName[VOLUME_NAME_MAX];	/* 对于本地存储设备可以用设备名来唯一标识(/dev/sdX) */
} Volume;


enum {
	STORAGE_MODE_ALL,
	STORAGE_MODE_6TF,
	STORAGE_MODE_1SD,
	STORAGE_MODE_MAX
};

/*
 * 存储管理器 
 * 1.记录当前系统的各个存储的状态(卡是否存在,总容量,剩余容量)
 * 2.根据存储系统指定当前拍照,录像,直播支持的模式
 * 3.支持其他的卡操作(如格式化)
 */
class StorageManager {
public:
	static sp<StorageManager> getSystemStorageManagerInstance();

	int queryCurTFStorageSpace();				/* 查询6TF卡的容量,返回容量最小的卡的容量值 */

	/* 目前只支持6+1
	 * 如果只有大卡存在,返回false
	 * 如果只有小卡存储,返回false
	 * 如果大卡小卡都存在,返回true,并更新存储容量
	 */
	bool queryCurStorageState();

	
	~StorageManager();
	
private:

	StorageManager();

	bool	queryLocalStorageState();
	
	int getCurStorageMode();						/* 获取当前的存储模式: 6+1, 6, 1 */

	int 	mRemovStorageSpace;						/* 可移动SD大卡或USB移动硬盘 */
	bool	mReovStorageExist;						/* 是否存在 */

	u32 	mMinStorageSpce;						/* 所有存储设备中最小存储空间大小(单位为MB) */

	u32		mCanTakePicNum;							/* 可拍照的张数 */
	u32		mCanTakeVidTime;						/* 可录像的时长(秒数) */
	u32		mCanTakeLiveTime;						/* 可直播录像的时长(秒数) */
	std::vector<sp<Volume>> mRemoteStorageList;		/* 存储列表 */

	std::mutex				mLocaLDevLock;
	std::vector<sp<Volume>> mLocalStorageList;		/* 存储列表 */
	
};



#endif /* _STORAGE_MANAGER_H_ */
