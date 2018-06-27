/*****************************************************************************************************
**					Copyrigith(C) 2018	Insta360 Pro2 Camera Project
** --------------------------------------------------------------------------------------------------
** 文件名称: InputManager.cpp
** 功能描述: 输入管理器（用于处理按键事件）
**
**
**
** 作     者: Skymixos
** 版     本: V1.0
** 日     期: 2018年05月04日
** 修改记录:
** V1.0			Skymixos		2018-05-04		创建文件，添加注释
******************************************************************************************************/
#include <dirent.h>
#include <fcntl.h>
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
#include <sys/pro_uevent.h>
#include <thread>
#include <vector>
#include <sys/ins_types.h>
#include <hw/InputManager.h>
#include <util/util.h>

#undef TAG
#define TAG "InputManager"
#define POLL_FD_NUM (2)

enum {
    UP = 0,
    DOWN = 1,
};

#define LONG_PRESS_MSEC (2000)
#define SHORT_PRESS_THOR	(100)	// 100ms
using namespace std;


InputManager::InputManager(const sp<oled_handler> &handler): mHander(handler)
{
	/* 构造一个线程对象 */
	if (!haveInstance)
	{
		haveInstance = true;
    	init_pipe(mCtrlPipe);
		loopThread = thread([this]{ inputEventLoop();});
	}
}


void InputManager::writePipe(int *p, int val)
{
    char c = (char)val;
    int  rc;
    rc = write_pipe(p, &c, 1);
    if (rc != 1)
    {
        Log.d(TAG, "Error writing to control pipe (%s) val %d", strerror(errno), val);
        return;
    }
}

void InputManager::exit()
{
	/* 等待线程退出 */

	Log.d(TAG, "stop detect mCtrlPipe[0] %d", mCtrlPipe[0]);

	if (mCtrlPipe[0] != -1)
	{
        writePipe(mCtrlPipe, Pipe_Shutdown);
        if (loopThread.joinable())
        {
            loopThread .join();
        }
        close_pipe(mCtrlPipe);
    }
	
    Log.d(TAG, "stop detect mCtrlPipe[0] %d over", mCtrlPipe[0]);
}


int InputManager::openDevice(const char *device)
{
    int version;
    int fd;
	
    struct pollfd *new_ufds;

    char name[80];

    struct input_id id;

    fd = open(device, O_RDWR);
    if(fd < 0)
    {
        Log.d(TAG, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }

    if (ioctl(fd, EVIOCGVERSION, &version))
    {
        Log.d(TAG, "could not get driver version for %s, %s\n", device, strerror(errno));
        return -1;
    }


    if (ioctl(fd, EVIOCGID, &id))
    {
        Log.d(TAG,"could not get driver id for %s, %s\n", device, strerror(errno));
        return -1;
    }

    name[sizeof(name) - 1] = '\0';
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1)
    {
        Log.d(TAG, "could not get device name for %s, %s\n", device, strerror(errno));
        name[0] = '\0';
        return -1;
    }
    else
    {
        if (strcmp(name, "gpio-keys") == 0)
        {
            Log.d(TAG, "found input device %s", device);
        }
        else
        {
            return -1;
        }
    }


    new_ufds = (pollfd *)realloc(ufds, sizeof(ufds[0]) * (nfds + POLL_FD_NUM));
    if (new_ufds == NULL)
    {
        Log.d(TAG, "out of memory\n");
        return -1;
    }
    ufds = new_ufds;

#if 0
    new_device_names = (char **)realloc(device_names, sizeof(device_names[0]) * (nfds + POLL_FD_NUM));
    if (new_device_names == NULL)
    {
        Log.d(TAG,"out of memory\n");
        return -1;
    }
    device_names = new_device_names;
    device_names[nfds] = strdup(device);
#endif

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;

    nfds++;
	Log.d(TAG, "open_device device %s over nfds %d ufds 0x%p", device, nfds, ufds);
	
    return 0;
}


bool InputManager::scanDir()
{
    bool bFound = false;
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;

    const char *dirname = "/dev/input";

    dir = opendir(dirname);

    if (dir == NULL)
        return -1;

    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((de = readdir(dir)))
    {
        if (de->d_name[0] == '.' && (de->d_name[1] == '\0' || (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;

        strcpy(filename, de->d_name);
        if (openDevice(devname) == 0)
        {
            bFound = true;
            break;
        }
    }
    closedir(dir);
    return bFound;
}


int InputManager::getKey(u16 code)
{
    const int keys[] = {0x73, 0x72, 0x101, 0x100, 0x74};
    int key = -1;
    for (u32 i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
    {
        if (keys[i] == code)
        {
            key = i;
            break;
        }
    }
    return key;
}


void InputManager::reportEvent(int iKey)
{	
	if (mHander)
	{
		sp<ARMessage> msg = mHander->obtainMessage(OLED_GET_KEY);
		msg->set<int>("oled_key", iKey);
		msg->post();

	}
	else
	{
		Log.e(TAG, "invalid mHander, can not send key evnet, please check!!!\n");
	}
}


void InputManager::reportLongPressEvent(int iKey, int64 iTs)
{
	if (mHander)
	{
		sp<ARMessage> msg = mHander->obtainMessage(OLED_GET_LONG_PRESS_KEY);		
		Log.d(TAG, "last_key_ts is %lld last_down_key %d", iTs, iKey);
		
		msg->set<int>("key", iKey);
		msg->set<int64>("ts", iTs);
		msg->postWithDelayMs(LONG_PRESS_MSEC);

	}
	else
	{
		Log.e(TAG, "invalid mHander, can not send key long press evnet, please check!!!\n");
	}
}


int InputManager::inputEventLoop()
{
	int res;
	struct input_event event;
	bool bExit = false;
	int iKey;
	int64 key_ts;
	int64 key_interval = 0;
	
	nfds = POLL_FD_NUM;
	ufds = (pollfd *)calloc(nfds, sizeof(ufds[0]));

    if (!scanDir())
    {
        Log.e(TAG, "no dev input found ");
        Log.e(TAG, "no dev input found (%s:%s:%d)", __FILE__, __FUNCTION__, __LINE__);
        abort();
    }

    ufds[1].fd = mCtrlPipe[0];
    ufds[1].events = POLLIN;

	
	while (true)
	{
		int pollres = poll(ufds, nfds, -1);
		
		if (pollres < 0)
		{
			Log.e(TAG, "poll error");
			break;
		}
		else if (pollres == 0)
		{
			Log.e(TAG, "poll happen but no data");
			continue;
		}
	
		if (ufds[1].revents && (ufds[1].revents & POLLIN))
		{

			#ifdef DEBUG_INPUT_MANAGER
			Log.d(TAG, "mPipeEvent poll %d, returned %d nfds %d "
					  "ufds[1].revents 0x%x\n", nfds, pollres, nfds, ufds[1].revents);
			#endif
			
			char c = Pipe_Wakeup;
			read_pipe(mCtrlPipe, &c, 1);

			
			if (c == Pipe_Shutdown)
			{
				Log.d(TAG, "oled_handler rec pipe shutdown\n");
				break;
			}
		}
	
		for (int i = POLL_FD_NUM; i < nfds; i++)
		{
		
			#ifdef DEBUG_INPUT_MANAGER
			Log.d(TAG, "rec event[%d] 0x%x cur time %ld\n", i, ufds[i].revents, msg_util::get_cur_time_ms());
			#endif
			
			{
				if (ufds[i].revents & POLLIN)
				{
					res = read(ufds[i].fd, &event, sizeof(event));
					if (res < (int)sizeof(event))
					{
						Log.d(TAG, "could not get event\n");
						return -1;
					}
					else
					{
						
						#ifdef DEBUG_INPUT_MANAGER
						Log.d(TAG, "get event %04x %04x %08x  "
										  "new_time %ld \n",
								  event.type, event.code, event.value,
								   msg_util::get_cur_time_us());
	
						#endif

						if (event.code != 0)
						{
							unique_lock<mutex> lock(mutexKey);
							key_ts = event.time.tv_sec * 1000000LL + event.time.tv_usec;

							key_interval = key_ts - last_key_ts;

							Log.d(TAG, " event.code is 0x%x, interval = %d ms\n", event.code, key_interval / 1000);
						
							switch (event.value)
							{
								case UP:
									if ((key_interval > SHORT_PRESS_THOR ) && (key_interval <  (LONG_PRESS_MSEC * 1000)))
									{
										if (event.code == last_down_key) {
											reportEvent(event.code);
										}
										else
										{
											Log.d(TAG, "up key mismatch(0x%x ,0x%x)\n", event.code, last_down_key);
										}
									}
									last_key_ts = key_ts;
									last_down_key = -1;
									break;
								
								case DOWN:
									last_down_key = event.code;	//iKey;
									last_key_ts = key_ts;

									#ifdef ENABLE_POWER_OFF
									reportLongPressEvent(last_down_key, last_key_ts);
									#endif

									break;
									SWITCH_DEF_ERROR(event.value);
							}	// switch (event.value)
						}
					}
				}
			}
		}
	}
	
	for (int i = POLL_FD_NUM; i < nfds; i++)
	{
		if (ufds[i].fd > 0)
		{
			close(ufds[i].fd);
			ufds[i].fd = -1;
		}
	}

	if (ufds)
	{
		free(ufds);
		ufds = nullptr;
	}
	
	Log.d(TAG, "exit get event loop");
	return 0;
}


