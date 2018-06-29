# Makefile for update_tool

CPPEXEC = update_app
DIR = update_app

TARGET = update_app


SOURCES := $(PRO2_SERVICE)/util/arlog.c

SOURCES += update_app.cpp
SOURCES += $(PRO2_SERVICE)/util/update_oled.cpp
SOURCES += $(PRO2_SERVICE)/util/stlog.cpp
SOURCES += $(PRO2_SERVICE)/util/update_util.cpp
SOURCES += $(PRO2_SERVICE)/util/md5.cpp
SOURCES += $(PRO2_SERVICE)/hw/oled_module.cpp
SOURCES += $(PRO2_SERVICE)/hw/ins_i2c.cpp
SOURCES += $(PRO2_SERVICE)/hw/ins_gpio.cpp


SOURCES += $(LIBLOG)/logd_write.c
SOURCES += $(LIBLOG)/log_event_write.c
SOURCES += $(LIBLOG)/logprint.c
SOURCES += $(LIBLOG)/event_tag_map.c
SOURCES += $(LIBLOG)/log_read.c
SOURCES += $(LIBLOG)/log_time.cpp

#SOURCES += $(LIBCUTILS)/socket_local_server.c
#SOURCES += $(LIBCUTILS)/socket_local_client.c
#SOURCES += $(LIBCUTILS)/klog.c
#SOURCES += $(LIBCUTILS)/uevent.c
SOURCES += $(LIBCUTILS)/system_properties.c



# Common standard libs
TARGET_LDLIBS = -lrt -lpthread

EXTRA_INC_PATH += -I $(PRO2_SERVICE)/include -I$(MONITOR_DIR)/include/init -I./ -I./include

# Common standard libs
TARGET_LDLIBS = -lrt -lpthread -lev


include $(SDK_DIR)/common_defs.mk

#External library directories listed in EXTLIB_DIR variable
ifdef EXTLIB_DIR
#TARGET_CFLAGS += $(foreach path,$(EXTLIB_DIR),-I$(path)/include)
#TARGET_LDFLAGS += $(foreach path,$(EXTLIB_DIR),-L$(path)/lib)
endif


.PHONY: all default install

all: $(CPPEXEC)

default: all

install: all
	$(INSTALL) $(BUILD)/$(CPPEXEC) 
	$(INSTALL_ROOT)/bin/$(CPPEXEC)	
	
include $(SDK_DIR)/common_cc.mk

	
