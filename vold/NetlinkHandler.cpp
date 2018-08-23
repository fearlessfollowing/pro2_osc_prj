#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <log/stlog.h>

#include "NetlinkEvent.h"
#include "NetlinkHandler.h"

// #include "VolumeManager.h"

#undef  TAG
#define TAG "Vold"


NetlinkHandler::NetlinkHandler(int listenerSocket) : NetlinkListener(listenerSocket) 
{
}


NetlinkHandler::~NetlinkHandler() 
{
}

int NetlinkHandler::start()
{
    return this->startListener();
}


int NetlinkHandler::stop() 
{
    return this->stopListener();
}


/*
 * onEvent - 接收到NetlinkEvent事件后，传递给VolumeManager进行处理
 * 
 */
void NetlinkHandler::onEvent(NetlinkEvent *evt) 
{
    Log.d(TAG, ">>>>>>>>>>>>>>> Use VolumeManager deal NetlinkEvent.... now");
   
    #if 0

    VolumeManager *vm = VolumeManager::Instance();
    const char *subsys = evt->getSubsystem();

    if (!subsys) {
        Log.w(TAG, "No subsystem found in netlink event");
        return;
    }

    if (!strcmp(subsys, "block")) {		
        vm->handleBlockEvent(evt);
		
	#ifdef USE_USB_MODE_SWITCH
    } else if (!strcmp(subsys, "usb") || !strcmp(subsys, "scsi_device")) {
        Log.w(TAG, "subsystem found in netlink event");
        MiscManager *mm = MiscManager::Instance();
        mm->handleEvent(evt);
	#endif
    }
    #endif

}

