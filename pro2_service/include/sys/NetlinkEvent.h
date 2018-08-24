#ifndef _NETLINKEVENT_H
#define _NETLINKEVENT_H

#include "NetlinkListener.h"

#define NL_PARAMS_MAX 32

enum {
    NETLINK_ACTION_UNKOWN = 0,
    NETLINK_ACTION_ADD = 1,
    NETLINK_ACTION_REMOVE = 2,
    NETLINK_ACTION_CHANGE = 3,
    NETLINK_ACTION_MAX,
};

class NetlinkEvent {
    int     mSeq;
    char    *mPath;
    int     mAction;                    /* 动作 */
    int     mSubsys;                    /* 是USB还是SD */
    char    *mBusAddr;                  /* 总线地址: usb1-2.1 */
    char    *mDevNodeName;              /* 设备节点名: sda, sda1 */
    
    char    *mParams[NL_PARAMS_MAX];

public:

    NetlinkEvent();
    virtual ~NetlinkEvent();

    bool        decode(char *buffer, int size, int format = NetlinkListener::NETLINK_FORMAT_ASCII);
    const char *findParam(const char *paramName);

    int         getSubsystem() { return mSubsys; }
    char*       getDevNodeName() { return mDevNodeName; }

    int         getAction() { return mAction; }
    char*       getBusAddr() { return mBusAddr; }

    void        dump();

 protected:
    bool        parseAsciiNetlinkMessage(char *buffer, int size);
    
};

#endif
