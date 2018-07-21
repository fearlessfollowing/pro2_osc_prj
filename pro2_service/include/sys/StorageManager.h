#ifndef _STORAGE_MANAGER_H_
#define _STORAGE_MANAGER_H_


/*
 * 对应存储卡
 */
class Volum {

};


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

	

	~StorageManager();
	
private:

	StorageManager();
	int getCurStorageMode();			/* 获取当前的存储模式: 6+1, 6, 1 */

	int 	mRemovStorageSpace;			/* 可移动SD大卡或USB移动硬盘 */
	bool	mReovStorageExist;			/* 是否存在 */

};



#endif /* _STORAGE_MANAGER_H_ */
