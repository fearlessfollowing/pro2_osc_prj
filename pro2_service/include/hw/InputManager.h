/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: InputManager.h
** 功能描述: 输入管理器对象的定义
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/
#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include <sys/ins_types.h>
#include <hw/MenuUI.h>
#include <common/sp.h>


class InputManager {

public:
    InputManager(const sp<MenuUI> &handler);
    virtual ~InputManager() {}
	
	/* 
	 * 1.含有一个获取事件的线程
	 * 2.方法
	 *	- 动态支持扫描输入设备并加入检测
	 *	- 
	 */

	void exit();
	void start();
	
	 
private:

    int mCtrlPipe[2]; // 0 -- read , 1 -- write
	
    int last_down_key = 0;
    int64 last_key_ts = 0;
	
    std::mutex mutexKey;
	
	bool haveInstance = false;
	
	struct pollfd *ufds = nullptr;
	int nfds;

	void writePipe(int *p, int val);
	int openDevice(const char *device);

	int inputEventLoop();
	bool scanDir();
	int getKey(u16 code);
	void reportEvent(int iKey);
	void reportLongPressEvent(int iKey, int64 iTs);
	
	sp<MenuUI> mHander;		/* 得到消息后,通过hander投递消息 */
    std::thread loopThread;			/* 循环线程 */
};

#endif /* _INPUT_MANAGER_H_ */
