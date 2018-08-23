#ifndef _NETLINKEVENT_H
#define _NETLINKEVENT_H

#include "NetlinkListener.h"

#define NL_PARAMS_MAX 32

class NetlinkEvent {
    int     mSeq;
    char    *mPath;
    int     mAction;
    char    *mSubsystem;
    char    *mParams[NL_PARAMS_MAX];

public:
    const static int NlActionUnknown;
    const static int NlActionAdd;
    const static int NlActionRemove;
    const static int NlActionChange;

    NetlinkEvent();
    virtual ~NetlinkEvent();

    bool decode(char *buffer, int size, int format = NetlinkListener::NETLINK_FORMAT_ASCII);
    const char *findParam(const char *paramName);

    const char *getSubsystem() { return mSubsystem; }
    int getAction() { return mAction; }

    void dump();

 protected:
    bool parseAsciiNetlinkMessage(char *buffer, int size);
    
};

#endif
