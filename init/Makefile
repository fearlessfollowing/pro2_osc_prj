# monitor Makefile
# Create by skyway 20161216

CEXEC = monitor
DIR = init

SOURCES := builtins.c
SOURCES += devices.c
SOURCES += init_parser.c
SOURCES += parser.c
SOURCES += property_service.c
SOURCES += signal_handler.c
SOURCES += ueventd.c
SOURCES += ueventd_parser.c
SOURCES += util.c
SOURCES += init.c

SOURCES += $(LIBLOG)/logd_write.c
SOURCES += $(LIBLOG)/log_event_write.c
SOURCES += $(LIBLOG)/log_time.cpp


ifndef WITH_MINIGW
SOURCES += $(LIBLOG)/logprint.c
SOURCES += $(LIBLOG)/event_tag_map.c
else
SOURCES += $(LIBLOG)/uio.c
endif

SOURCES += $(LIBLOG)/log_read.c

SOURCES += $(LIBCUTILS)/socket_local_server.c
SOURCES += $(LIBCUTILS)/socket_local_client.c
SOURCES += $(LIBCUTILS)/klog.c
SOURCES += $(LIBCUTILS)/uevent.c
SOURCES += $(LIBCUTILS)/system_properties.c

TAGS_DIRS = $(NULL)

INCLUDE_PATH = -I ../include
INCLUDE_PATH += -I ../include/init
INCLUDE_PATH += -I ./
INCLUDE_PATH += -I ./include

TARGET_CFLAGS += -I ../include -I../include/init -I./ -I./include


include $(SDK_DIR)/common_defs.mk


#External library directories listed in EXTLIB_DIR variable
ifdef EXTLIB_DIR
#TARGET_CFLAGS += $(foreach path,$(EXTLIB_DIR),-I$(path)/include)
#TARGET_LDFLAGS += $(foreach path,$(EXTLIB_DIR),-L$(path)/lib)
endif


# Common standard libs
TARGET_LDLIBS = -lrt


.PHONY: all default install

all: $(CEXEC)

default: all

install: all
	$(INSTALL) $(BUILD)/$(CEXEC) 
	$(INSTALL_ROOT)/bin/$(CEXEC)	
	
include $(SDK_DIR)/common_cc.mk

