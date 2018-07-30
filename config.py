# -*- coding: utf-8 -*-  
###########################################################################################################
# File: config.py
# Author: skymixos
# Created: 2018-07-05
# Descriptor: Defined some Enviorn var and Control Options
# Version: V1.0
##########################################################################################################
import os
import platform


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

SDK_ROOT += '/'

MONITOR_DIR = SDK_ROOT + 'init'
LOGD_DIR = SDK_ROOT + 'logd'
LOGCAT_DIR = SDK_ROOT + 'logcat'
PROP_TOOLS= SDK_ROOT + 'prop_tools'
EXTLIBS = SDK_ROOT + 'extlib/'


LIBCUTILS = EXTLIBS + 'libcutils'
LIBLOG = EXTLIBS + 'liblog'
LIBSYSUTILS = EXTLIBS + 'libsysutils'
LIBUTILS = EXTLIBS + 'libutils'
LIBEV = EXTLIBS + 'libev-master'

PRO2_SERVICE = SDK_ROOT + 'pro2_service'
UPDATE_CHECK = SDK_ROOT + 'update_check'
UPDATE_TOOL = SDK_ROOT + 'update_tool'
UPDATE_APP = SDK_ROOT + 'update_app'
VOLD_DIR = SDK_ROOT + 'vold'


# Common flags
COM_FLAGS = ''
COM_FLAGS += ' -DHAVE_SYS_UIO_H -DHAVE_PTHREADS -DHAVE_ANDROID_OS '
COM_FLAGS += ' -I' + SDK_ROOT + 'include '
COM_FLAGS += ' -I' + SDK_ROOT + 'include/init '
COM_FLAGS += ' -I' + PRO2_SERVICE + '/include '

#COM_FLAGS += $(EXTRA_INC_PATH)

#------------------------------- 菜单相关的配置（通过开关来控制） START --------------------------------------------------

# 使能AEB菜单项
COM_FLAGS += ' -DENABLE_MENU_AEB '


#------------------------------- 菜单相关的配置（通过开关来控制） END   --------------------------------------------------


# 使能老化模式开关（仅用于工厂测试）
COM_FLAGS += ' -DENABLE_AGEING_MODE '


# LED 调试信息开关
COM_FLAGS += ' -DDEBUG_OLED '

# 输入事件管理器调试开关
#COMFLAGS += ' -DDEBUG_INPUT_MANAGER'

COM_FLAGS += ' -DENABLE_PESUDO_SN '

# 电池调试信息开关
COM_FLAGS += ' -DDEBUG_BATTERY '

# 设置页调试信息开关
COM_FLAGS += ' -DDEBUG_SETTING_PAGE '

# diable baterry check, print too much error info
#COM_FLAGS += ' -DDISABLE_BATTERY_CHECK'

COM_FLAGS += ' -fexceptions -Wall -Wunused-variable '

# toolchains options
CROSS_TOOL  = 'aarch64'

#------- toolchains path -------------------------------------------------------
if os.getenv('CROSS_CC'):
    CROSS_TOOL = os.getenv('CROSS_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    EXEC_PATH   = '/usr/local/bin'
elif CROSS_TOOL == 'aarch64':
    PLATFORM    = 'aarch64'
    EXEC_PATH   = ''

#BUILD = 'debug'
BUILD = 'release'


TARGET_MONIOTR_NAME = 'monitor'

#------- GCC settings ----------------------------------------------------------
if PLATFORM == 'gcc':
    # toolchains
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    LINK = PREFIX + 'gcc'
    TARGET_EXT = 'axf'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    CFLAGS = DEVICE
    #LFLAGS = DEVICE
    #LFLAGS += ' -Wl,--gc-sections,-cref,-Map=' + MAP_FILE
    #LFLAGS += ' -T ' + LINK_FILE + '.ld'

    CPATH = ''
    LPATH = ''



    if BUILD == 'debug':
    	CFLAGS += ' -O0 -g'
    else:
        CFLAGS += ' -O2'

elif PLATFORM == 'aarch64':
    # toolchains
    PREFIX = '' 
    CC = PREFIX + 'gcc'
    AS = PREFIX + 'gcc'
    AR = PREFIX + 'ar'
    CXX = PREFIX + 'g++'
    LINK = PREFIX + 'gcc'
    SIZE = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY = PREFIX + 'objcopy'

    CFLAGS = COM_FLAGS
    CXXFLAGS = COM_FLAGS
    CXXFLAGS += ' -std=c++11 -frtti '
    LDFLAGS = ''
    LINKLIBS = ''
    CPATH = ''
    LPATH = ''

#    if BUILD == 'debug':
#        CFLAGS += ' -O0 -g'
#    else:
#        CFLAGS += ' -O2'


