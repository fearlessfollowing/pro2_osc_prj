######################################################################################################################
# >>>>>>>>>>>>>>>>>>>>> Insta360 Pro2 Project <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
# Makefile for Pro2_service
# Created by skymixos 2018-06-01
#
######################################################################################################################

CPPEXEC = pro2_service
DIR = pro2_service

TARGET_USES_LOGD=true
WITH_MINGW :=

SOURCES := util/arlog.c
SOURCES += util/ARLooper.cpp
SOURCES += util/ARMessage.cpp
SOURCES += util/bytes_int_convert.cpp
SOURCES += util/util.cpp
SOURCES += util/stlog.cpp
SOURCES += util/cJSON.c
SOURCES += util/pro_version.cpp

SOURCES += hw/oled_module.cpp
SOURCES += hw/oled_light.cpp
SOURCES += hw/battery_interface.cpp
SOURCES += hw/ins_i2c.cpp
SOURCES += hw/ins_gpio.cpp


SOURCES += pro_uevent.c
SOURCES += fifo.cpp


SOURCES += $(LIBLOG)/logd_write.c
SOURCES += $(LIBLOG)/log_event_write.c
SOURCES += $(LIBLOG)/logprint.c
SOURCES += $(LIBLOG)/event_tag_map.c
SOURCES += $(LIBLOG)/log_read.c
SOURCES += $(LIBLOG)/log_time.cpp

SOURCES += $(LIBCUTILS)/system_properties.c



SOURCES += oled_handler.cpp
SOURCES += InputManager.cpp
SOURCES += net_manager.cpp
SOURCES += poll_timer.cpp
SOURCES += pro_cfg.cpp
SOURCES += dev_manager.cpp
SOURCES += main.cpp


TAGS_DIRS = $(NULL)

EXTRA_INC_PATH += -I ../include -I../include/init -I./ -I./include

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
 

