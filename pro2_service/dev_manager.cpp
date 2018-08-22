//
// Created by vans on 16-12-23.
//

#include <common/include_common.h>
#include <vector>
#include <sys/select.h>
#include <util/ARMessage.h>
#include <common/sp.h>
#include <hw/dev_manager.h>
#include <sys/pro_uevent.h>
#include <log/stlog.h>
#include <util/msg_util.h>
#include <util/util.h>
#include <errno.h>

#include <sys/StorageManager.h>

using namespace std;
#define TAG	"dev_manager"

#define UEVENT_MSG_LEN  (1024)
#define UEVENT_SOCK_LEN (64*1024)


struct net_link_info {
    ~net_link_info()
    {
        if (dev_path != nullptr) {
            free(dev_path);
        }
		
        if (sub_system != nullptr) {
            free(sub_system);
        }
		
        if (dev_name != nullptr) {
            free(dev_name);
        }
		
        if (dev_type != nullptr) {
            free(dev_type);
        }
    }
	
    int action;				/* 拔插的动作:ADD,REMOVE,CHANGE    */
    char *dev_path;			/* 设备文件的路径:  /dev/mmcblkXpX */

    //block or scsi_*
    char *sub_system;		/* 子系统 */
    int major;				/* 主设备号 */
    int minor;				/* 次设备号 */
    char *dev_name;			

    //disk or partition
    char *dev_type;			/* 设备的类型,磁盘或分区 */
    int seq_num;			/* 序列号 */
};

typedef struct net_link_info NET_LINK_INFO;

static const char*
has_prefix(const char* str, const char* end, const char* prefix, size_t prefixlen)
{
    if ((end-str) >= (ptrdiff_t)prefixlen && !memcmp(str, prefix, prefixlen))
        return str + prefixlen;
    else
        return NULL;
}

/* Same as strlen(x) for constant string literals ONLY */
#define PRO_CONST_STRLEN(x)  (sizeof(x)-1)

/* Convenience macro to call has_prefix with a constant string literal  */
#define PRO_HAS_CONST_PREFIX(str,end,prefix)  has_prefix((str),(end),prefix,PRO_CONST_STRLEN(prefix))

static const char *mount_src[] = {
    "/dev/mmcblk1",
    "/dev/sd",
};


/*************************************************************************
** 方法名称: dev_manager
** 方法功能: 设备管理器构造函数
** 入口参数: 
**		notify - 连接UI线程的消息对象
** 返 回 值: 
** 调     用: 
**
*************************************************************************/
dev_manager::dev_manager(const sp<ARMessage> &notify):mNotify(notify)
{
    init();
}



/*************************************************************************
** 方法名称: ~dev_manager
** 方法功能: 设备管理器析构函数
** 入口参数: 
** 返 回 值: 
** 调     用: 
**
*************************************************************************/
dev_manager::~dev_manager()
{
    deinit();
}



/*************************************************************************
** 方法名称: init
** 方法功能: 初始化
** 入口参数: 
**		
** 返 回 值: 
** 调     用: dev_manager构造函数
**
*************************************************************************/
void dev_manager::init()
{
    init_pipe(mCtrlPipe);	/* 初始化命名管道,用于跟检测线程通信 */
    start_detect_thread();	/* 创建检测线程 */
}

void dev_manager::handle_block_event(sp<struct net_link_info> &mLink)
{
    dev_change(mLink->action, 20);
}




/*************************************************************************
** 方法名称: parseAsciiNetlinkMessage
** 方法功能: 解析内核的netlink消息
** 入口参数: 
** 返 回 值: 
** 调     用: ~dev_manager
**
*************************************************************************/
bool dev_manager::parseAsciiNetlinkMessage(char *buffer, int size)
{
    const char *s = buffer;
    const char *end;
	
    int first = 1;

    bool bBlockDisk = false;

	Log.d(TAG, "parseAsciiNetlinkMessage: %s", buffer);

    sp<NET_LINK_INFO> mLink = sp<NET_LINK_INFO>(new NET_LINK_INFO());
	
    /* Ensure the buffer is zero-terminated, the code below depends on this */
    buffer[size - 1] = '\0';

    end = s + size;
    while (s < end) {
        if (first) {
            const char *p;
            /* buffer is 0-terminated, no need to check p < end */
            for (p = s; *p != '@'; p++) {
                if (!*p) { /* no '@', should not happen */
                    return false;
                }
            }
            first = 0;
        } else {
            const char* a;
            if ((a = PRO_HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
                if (!strcmp(a, "add"))
                    mLink->action = ADD;
                else if (!strcmp(a, "remove"))
                    mLink->action = REMOVE;
                else if (!strcmp(a, "change"))
                    mLink->action = CHANGE;
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "DEVPATH=")) != NULL) {
                mLink->dev_path = strdup(a);
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
                mLink->sub_system = strdup(a);
                if (strcmp(mLink->sub_system, "block")) {
                    bBlockDisk = false;
                    break;
                }
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "MAJOR=")) != NULL) {
                mLink->major = atoi(a);
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "MINOR=")) != NULL) {
                mLink->minor = atoi(a);
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "DEVNAME=")) != NULL) {
                mLink->dev_name = strdup(a);
            } else if ((a = PRO_HAS_CONST_PREFIX(s, end, "DEVTYPE=")) != NULL) {
                mLink->dev_type = strdup(a);
                if (strcmp(mLink->dev_type, "disk") == 0) {
                    bBlockDisk = true;
                    break;
                }
            }
            else if ((a = PRO_HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
                mLink->seq_num = atoi(a);
            }
        }
        s += strlen(s) + 1;
    }

    if (bBlockDisk) {
		Log.d(TAG, "DevManager: recv block action (%d %d)\n", mLink->action, mLink->major);
		// handle_block_event(mLink);
		Log.d(TAG, "DevManager: recv block action %d over\n", mLink->action);
    }
    return true;
}



/*************************************************************************
** 方法名称: stop_detect_thread
** 方法功能: 停止热拔插设备检测线程
** 入口参数: 
**		无
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void dev_manager::stop_detect_thread()
{
    Log.d(TAG, "stop detect mCtrlPipe[0] %d\n", mCtrlPipe[0]);

	if (mCtrlPipe[0] != -1) {
        char c = Pipe_Shutdown;
        int  rc;
        rc = write_pipe(mCtrlPipe, &c, 1);
        if (rc != 1) {
            Log.d("Error writing to control pipe (%s)", strerror(errno));
            return;
        }
		
        if (th_detect_.joinable()) {
            th_detect_.join();
        }
        close_pipe(mCtrlPipe);
    }

	Log.d(TAG, "stop detect mCtrlPipe[0] %d over", mCtrlPipe[0]);
}



/*************************************************************************
** 方法名称: start_detect_thread
** 方法功能: 创建热拔插设备检测线程
** 入口参数: 
**		无
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void dev_manager::start_detect_thread()
{
    th_detect_ = thread([this] { uevent_detect_thread(); });
}



/*************************************************************************
** 方法名称: send_notify_msg
** 方法功能: 给上层发送当前的存储设备列表
** 入口参数: 
**		dev_list - 存储列表容器引用
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void dev_manager::send_notify_msg(std::vector<sp<Volume>> &dev_list)
{
    sp<ARMessage> msg = mNotify->dup();
    msg->set<vector<sp<Volume>>>("dev_list", dev_list);
    msg->post();
}

void dev_manager::dev_change(int action, const int MAX_TIMES)
{
    int iTimes = 0;

    while (iTimes++ < MAX_TIMES) {
        if (start_scan(action)) {
            break;
        }
        msg_util::sleep_ms(1000);
    }

    if (iTimes >= MAX_TIMES) {
        Log.e(TAG, "handle action %s fail", (action== ADD)? "add": ((action == REMOVE)?"remove":"change"));
    }
}

void dev_manager::resetDev(int count)
{
//    unique_lock<mutex> lock(mMutex);
    Log.i(TAG,"resetDev count %d",count);
    org_dev_count = count;
}

int dev_manager::check_block_mount(char *dev)
{
    int mount_type = -1;
    for (u32 i = 0; i < sizeof(mount_src)/sizeof(mount_src[0]); i++) {
        if (strstr(dev, mount_src[i]) == dev) {
			
            if (strstr(dev, "mmcblk")  || strstr(dev, ":179,")) {
                mount_type = SET_STORAGE_SD;
            } else {
                mount_type = SET_STORAGE_USB;
            }
            break;
        }
    }

    return mount_type;
}

bool dev_manager::start_scan(int action)
{
    bool bHandle = false;
	char* pPath = NULL;
    int fd = open("/proc/mounts", O_RDONLY);	/* 打开"/proc/mounts"文件 */
	
    vector <sp<Volume>> mDevList;
    mDevList.clear();

    if (fd > 0) {	/* 打开文件成功 */
		
        int type;
        char buf[1024];
        char *delim = (char *)" ";

        memset(buf, 0, sizeof(buf));
		
        while (read_line(fd, buf, sizeof(buf)) > 0) {	/* 依次读取一行数据 */

			char *p = strtok(buf, delim);
            if (p != nullptr) {
				
                type = check_block_mount(p);	/* 检查设备文件名是否合法"/dev/mmcblk1*"或"/dev/sdX" */
                if (type != -1) {
					
                    sp<Volume> mDevInfo = sp<Volume>(new Volume());

                    snprintf(mDevInfo->src, sizeof(mDevInfo->src), "%s", p);
                    if (type == SET_STORAGE_SD) {
                        snprintf(mDevInfo->dev_type, sizeof(mDevInfo->dev_type), "%s", "sd");
                        snprintf(mDevInfo->name, sizeof(mDevInfo->name), "%s", "sd1");
                    } else {
                        snprintf(mDevInfo->dev_type, sizeof(mDevInfo->dev_type), "%s", "usb");
                        snprintf(mDevInfo->name, sizeof(mDevInfo->name), "%s", "usb");
                    }
					
                    pPath = strtok(NULL, delim);	/* 得到设备的挂载路径 */
                    if (pPath) {
                        snprintf(mDevInfo->path, sizeof(mDevInfo->path), "%s", pPath);
                    } else {
                        Log.d(TAG, "no mount path?\n");
                    }
					
                    if (access(mDevInfo->path, F_OK) == -1) {	/* 访问该路径是否存在 */
                        Log.e(TAG," mount path %s not access", mDevInfo->path);
                    } else {
                        mDevList.push_back(mDevInfo);	/* 将该设备推入mDevList临时列表中 */
                    }
                }
            }
            memset(buf, 0, sizeof(buf));
        }

        switch (action) {	/* 判断发起此次扫描的原因 */
            case ADD:		/* 有设备插入 */
                if (mDevList.size() > org_dev_count) {
                    bHandle = true;
                }
                break;
				
            case REMOVE:	/* 有设备移除 */
                if (mDevList.size() < org_dev_count) {
                    bHandle = true;
                }
                break;
				
            case CHANGE:
                bHandle = true;
                break;
			
            default:
                break;
        }
        close(fd);
    }
	
    Log.d(TAG, "start_scan action %d "
                  " bHandle %d mDevList size %d"
                  " org_dev_count %d\n",
          action,bHandle,mDevList.size(),org_dev_count);

    if (bHandle) {	/* 需要处理,上报 */
        send_notify_msg(mDevList);			/* 将新的设备列表发给UI */
        org_dev_count = mDevList.size();	/* 更新当前系统中存在的存储设备个数 */
    } else {
        // handle specail that insert/remove sd and usb at same time 0803
        switch (action) {
            case ADD:
                if (mDevList.size() == 2) {
                    Log.d(TAG,"force add handle");
                    bHandle = true;
                }
                break;
				
            case REMOVE:
                if (mDevList.size() == 0) {
                    Log.d(TAG,"force remove handle");
                    bHandle = true;
                }
                break;
        }
    }
    return bHandle;
}



/*************************************************************************
** 方法名称: start_detect_thread
** 方法功能: 热拔插设备检测线程
** 入口参数: 
**		无
** 返 回 值: 无
** 调     用: 
**
*************************************************************************/
void dev_manager::uevent_detect_thread()
{
    int device_fd;
    char msg[UEVENT_MSG_LEN + 2];
    int n;

	/* 打开监听内核NetLink设备消息的套接字 */
    device_fd = uevent_open_socket(UEVENT_SOCK_LEN, true);
    if (device_fd < 0) {
        Log.d(TAG,"open uevent socket error\n");
        return;
    }

    start_scan();	/* 扫描已经挂载的设备 */

    while (1) {
		
        fd_set read_fds;
        int max_fd = -1;
        int rc = 0;

        FD_ZERO(&read_fds);
        FD_SET(mCtrlPipe[0], &read_fds);
        if (mCtrlPipe[0] > max_fd) {
            max_fd = mCtrlPipe[0];
        }

        FD_SET(device_fd, &read_fds);
        if (device_fd > max_fd) {
            max_fd = device_fd;
        }

        if ((rc = select(max_fd + 1, &read_fds, NULL, NULL, NULL)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            Log.d(TAG, "select fail %s max %d", strerror(errno),max_fd);
            msg_util::sleep_ms(1000);
            continue;
        } else if (!rc) {
            Log.d(TAG, "select rc is 0");
            continue;
        }
		
        if (FD_ISSET(mCtrlPipe[0], &read_fds)) {
            char c = Pipe_Wakeup;
            read_pipe(mCtrlPipe, &c, 1);
            if (c == Pipe_Shutdown) {
                Log.d(TAG, "rec pipe shutdown\n");
                break;
            }
        }

        if (FD_ISSET(device_fd, &read_fds)) {	/* 接收内核发送的NetLink消息 */
			//Log.d(TAG, "kernel have device msg ...");
            if ((n = uevent_kernel_multicast_recv(device_fd, msg, UEVENT_MSG_LEN)) > 0) {
                parseAsciiNetlinkMessage(msg, n);	/* 解析内核传递的消息 */
            }
        }
    }
    Log.d(TAG, "detect thread over\n");
}



/*************************************************************************
** 方法名称: deinit
** 方法功能: 去初始化
** 入口参数: 
** 返 回 值: 
** 调     用: ~dev_manager
**
*************************************************************************/
void dev_manager::deinit()
{
    stop_detect_thread();
}

