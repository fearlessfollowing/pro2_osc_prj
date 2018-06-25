//
// Created by vans on 17-1-23.
//

#ifndef PROJECT_USB_DEV_INFO_H
#define PROJECT_USB_DEV_INFO_H

typedef struct usb_dev_info {
    //"usb","sd" or "internal"
    char dev_type[8];
    char path[128];
    char name[128];
    unsigned long long total;
    unsigned long long avail;
} USB_DEV_INFO;

#endif //PROJECT_USB_DEV_INFO_H
