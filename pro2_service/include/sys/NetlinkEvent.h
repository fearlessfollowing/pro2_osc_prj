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
    int     mAction;                    /* 动作 */
    int     mSubsys;                    /* 是USB还是SD */
    char    mBusAddr[64];
    char    mDevNodeName[64];
public:

    NetlinkEvent();
    virtual ~NetlinkEvent();

    bool        decode(char *buffer, int size, int format = NetlinkListener::NETLINK_FORMAT_ASCII);

    int         getSubsystem() { return mSubsys; }
    char*       getDevNodeName() { return mDevNodeName; }

    int         getAction() { return mAction; }
    char*       getBusAddr() { return mBusAddr; }

 protected:
    bool        parseAsciiNetlinkMessage(char *buffer, int size);
    
};

#endif
