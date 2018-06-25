# common_defs.mk
# Copyright 2016-2020 BitEye, Inc.  All rights reserved.
#

# Toolchain and dependency setup
#
# For cross compilation, define:
#   TOOLCHAIN_DIR - where to find standard libraries for the target plaform
#   ARCH - the compiler prefix
#   EXTLIB_DIR - where to find required external libraries
#
# For native compilation, these variables can be left undefined
#
#
ARCH ?= native

CFLAGS :=
LDFLAGS:= 
LDLIBS :=

ifneq ($(TOOLCHAIN_DIR),)
	TOOLCHAIN_DIR := $(abspath $(TOOLCHAIN_DIR))
	TOOL_DIR = $(TOOLCHAIN_DIR)/bin/
	TARGET_CFLAGS += -I$(TOOLCHAIN_DIR)/include
	TARGET_LDFLAGS += -L$(TOOLCHAIN_DIR)/lib
endif

ifneq ($(ARCH),native)
	export ARCH
	export TOOL_ROOT = $(TOOL_DIR)$(ARCH)
	export CC = $(TOOL_ROOT)-gcc
	export AR = $(TOOL_ROOT)-ar
endif

#
# Install directory
#
# INSTALL_ROOT can be defined externally to
# customize the make install behavior
#
#INSTALL_ROOT ?= /home/skymixos/work/pro2/app/image/insta360_pro2_image/skymixos_orig/bin
INSTALL_ROOT ?= /home/nvidia/work/image/insta360_pro2_image/pro2_osc/release/bin
#INSTALL_ROOT ?= $(DIDI_SDK_DIR)/build/$(ARCH)
INSTALL_ROOT := $(abspath $(INSTALL_ROOT))

INSTALL := install -D

BUILD_ROOT = $(SDK_DIR)/build/$(ARCH)/obj
BUILD = $(BUILD_ROOT)/$(DIR)
#BUILD = $(DIR)



# Common flags
COMFLAGS = -DHAVE_SYS_UIO_H -DHAVE_PTHREADS -DHAVE_ANDROID_OS

COMFLAGS += -I$(SDK_DIR)/include
COMFLAGS += -I$(SDK_DIR)/include/init

COMFLAGS += $(EXTRA_INC_PATH)

# enable ageing mode for factory test
COMFLAGS += -DENABLE_AGEING_MODE

# enable inputmanager debug
#COMFLAGS += -DDEBUG_INPUT_MANAGER


COMFLAGS += -DDEBUG_BATTERY

# diable baterry check, print too much error info
#COMFLAGS += -DDISABLE_BATTERY_CHECK


CFLAGS += -fexceptions -Wall -Wunused-variable 
#-Werror
CFLAGS += -DHAVE_SYS_UIO_H  -DENABLE_AGEING_MODE  -DHAVE_ANDROID_OS -DHAVE_PTHREADS
CFLAGS += $(COMFLAGS)

CXXFLAGS +=  -std=c++11 -frtti -fexceptions  -Wall
CXXFLAGS += $(COMFLAGS)


TARGET_CFLAGS += -Wall -Wunused 
TARGET_CFLAGS += $(addprefix -D,$(sort $(DEFINES)))


CFLAGS += $(TARGET_CLFAGS)
CXXFLAGS += $(TARGET_CXXFLAGS)


SOURCE_C := $(filter %.c, $(SOURCES))
SOURCE_CXX := $(filter %.cpp, $(SOURCES))

COBJS = $(SOURCE_C:%.c=$(BUILD)/%.o)
CDEPS = $(SOURCE_C:%.c=$(BUILD)/%.d)

CPPOBJS = $(SOURCE_CXX:%.cpp=$(BUILD)/%.o)
CPPDEPS = $(SOURCE_CXX:%.cpp=$(BUILD)/%.d)

BUILD_DATE := $(shell date '+%Y-%m-%d')
BUILD_TIME := $(shell date '+%H:%M:%S')
BUILD_USER := $(USER)

ifeq ($(TYPE),RELEASE)
DEFINES += BUILD_RELEASE
else
BUILD_TAG ?= eng
endif

BUILD_VERSION := $(MAJOR_VERSION).$(MINOR_VERSION)
ifneq ($(MAINTENANCE_VERSION),)
BUILD_VERSION := $(BUILD_VERSION).$(MAINTENANCE_VERSION)
endif

ifneq ($(BUILD_TAG),)
BUILD_VERSION := $(BUILD_VERSION)-$(BUILD_TAG)
endif

DEFINES += BUILD_VERSION=\"$(BUILD_VERSION)\"
DEFINES += BUILD_SCM_REV=\"$(BUILD_SCM_REV)\"
DEFINES += BUILD_DATE=\"$(BUILD_DATE)\"
DEFINES += BUILD_TIME=\"$(BUILD_TIME)\"
DEFINES += BUILD_USER=\"$(BUILD_USER)\"

