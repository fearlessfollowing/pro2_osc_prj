#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <log/stlog.h>

#include <sys/NetlinkEvent.h>

const int QLOG_NL_EVENT  = 112;


#undef  TAG
#define TAG "NetlinkEvent"

const int NetlinkEvent::NlActionUnknown = 0;
const int NetlinkEvent::NlActionAdd     = 1;
const int NetlinkEvent::NlActionRemove  = 2;
const int NetlinkEvent::NlActionChange  = 3;


NetlinkEvent::NetlinkEvent() 
{
    mAction = NlActionUnknown;
    memset(mParams, 0, sizeof(mParams));
    mPath = NULL;
    mSubsystem = NULL;
}

NetlinkEvent::~NetlinkEvent() 
{
    int i;
    if (mPath)
        free(mPath);
    if (mSubsystem)
        free(mSubsystem);
    for (i = 0; i < NL_PARAMS_MAX; i++) {
        if (!mParams[i])
            break;
        free(mParams[i]);
    }
}


void NetlinkEvent::dump() 
{
    #if 0
    int i;

    for (i = 0; i < NL_PARAMS_MAX; i++) {
        if (!mParams[i])
            break;
        Log.d(TAG, "NL param '%s'\n", mParams[i]);
    }
    #else


    #endif
}


static const char*
has_prefix(const char* str, const char* end, const char* prefix, size_t prefixlen)
{
    if ((end-str) >= (ptrdiff_t)prefixlen && !memcmp(str, prefix, prefixlen))
        return str + prefixlen;
    else
        return NULL;
}

/* Same as strlen(x) for constant string literals ONLY */
#define CONST_STRLEN(x)  (sizeof(x)-1)

/* Convenience macro to call has_prefix with a constant string literal  */
#define HAS_CONST_PREFIX(str,end,prefix)  has_prefix((str),(end),prefix,CONST_STRLEN(prefix))


#if 0
add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1

#endif


bool NetlinkEvent::parseAsciiNetlinkMessage(char *buffer, int size) 
{
    const char *s = buffer;
    const char *end;
    const char *pAction = s;        /* Action字段(add, remove, changed) */
    const char *pArgs   = NULL;     /* 参数字段 */
    const char *pBlockEvt = NULL;   /* 是否为块设备消息 */
    const char *pAt = NULL;
    const char *pSlash = NULL;
    const char *pDevNode = NULL;
    char cBusAddr[256] = {0};
    int param_idx = 0;
    int first = 1;
    

    if (size == 0)
        return false;

    Log.d(TAG, ">>> parseAsciiNetlinkMessage: %s", buffer);

    buffer[size-1] = '\0';
    end = s + size;

    /* 对于非"block"的事件，直接丢弃 */
    pBlockEvt = strstr(s, "block");
    pAt = strchr(s, '@');

    /*
     * 对于Ubuntu linux其接收的消息数据格式如下：
     * add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
     * add@/devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1        
     */

    if ((pBlockEvt != NULL) && (pAt != NULL)) {
        if (!strncmp(pAction, "add", strlen("add"))) {
            mAction = NlActionAdd;
        } else if (!strncmp(pAction, "remove", strlen("remove"))) {
            mAction = NlActionRemove;
        } else {
            return false;   /* 目前只处理 'add', 'remove'两种事件 */
        }

        /* 设备的子系统类型
         * 设备所处的总线地址
         * 设备文件名称
         */
        if (!strncmp(s, "usb", strlen("usb"))) {
            mSubsys = VOLUME_SUBSYS_USB;
        } else if (!strncmp(s, "mmcblk1", strlen("mmcblk1"))) {     /* "mmcblk0"是内部的EMMC,直接跳过 */
            mSubsys = VOLUME_SUBSYS_SD;
        } else {
            return false;
        }

        if (mSubsys == VOLUME_SUBSYS_USB) {
            const char* pColon = NULL;
            pColon = strchr(s, ':');
            if (pColon) {
                strncpy(cBusAddr, pAt + 1, pColon - (pAt + 1));
                Log.d(TAG, "Usb bus addr: %s" cBusAddr);

                pSlash = strrchr(cBusAddr, '/');
                if (pSlash) {
                    mBusAddr = strdup(pSlash + 1);
                    Log.d(TAG, "usb bus addr: %s", mBusAddr);
                } else {
                    return false;
                }

                pDevNode = strrchr(s, '/');
                if (pDevNode) {
                    mDevNodeName = strdup(pDevNode + 1);
                    Log.d(TAG, "dev node name: %s", mDevNodeName);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

    } else {
        return false;
    }

    #if 0

    while (s < end) {
        if (first) {
            const char *p;

            /* buffer is 0-terminated, no need to check p < end */
            for (p = s; *p != '@'; p++) {
                if (!*p) {  /* 字符串中没有'@'，这种情况不应该发生 */
                    return false;
                }
            }

            /*
             * /devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg
             * /devices/3530000.xhci/usb1/1-2/1-2.1/1-2.1:1.0/host40/target40:0:0/40:0:0:0/block/sdg/sdg1
             */
            mPath = strdup(p+1);
            first = 0;
        } else {
            const char* a;
            if ((a = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
                if (!strcmp(a, "add"))
                    mAction = NlActionAdd;
                else if (!strcmp(a, "remove"))
                    mAction = NlActionRemove;
                else if (!strcmp(a, "change"))
                    mAction = NlActionChange;
            } else if ((a = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
                mSeq = atoi(a);
            } else if ((a = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
                mSubsystem = strdup(a);
            } else if (param_idx < NL_PARAMS_MAX) {
                mParams[param_idx++] = strdup(s);
            }
        }
        s += strlen(s) + 1;
    }
    #endif

    return true;
}


bool NetlinkEvent::decode(char *buffer, int size, int format) 
{
    if (format == NetlinkListener::NETLINK_FORMAT_ASCII) {
        return parseAsciiNetlinkMessage(buffer, size);
    } else {
        Log.e(TAG, "[%s: %d] Not support format[%d]", __FILE__, __LINE__, format);
        return false;
    }
}

const char *NetlinkEvent::findParam(const char *paramName)
{
    size_t len = strlen(paramName);
    for (int i = 0; i < NL_PARAMS_MAX && mParams[i] != NULL; ++i) {
        const char *ptr = mParams[i] + len;
        if (!strncmp(mParams[i], paramName, len) && *ptr == '=')
            return ++ptr;
    }

    Log.e("NetlinkEvent::FindParam(): Parameter '%s' not found", paramName);
    return NULL;
}
