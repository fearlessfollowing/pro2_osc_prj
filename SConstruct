import os
import platform
import sys
import config


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

Export('SDK_ROOT')
Export('config')


com_env = Environment(
	CC = config.CC, CCFLAGS = config.CFLAGS,
	CXX = config.CXX, CXXFLAGS = config.CXXFLAGS,
	LIBS = config.LINKLIBS,
	LINKFLAGS = config.LDFLAGS
 	)

com_env.Append(CCCOMSTR='CC <+++ $SOURCES')
#com_env.Append(LINKCOMSTR='Link Target $SOURCES')


#print com_env['CCFLAGS']
#print com_env['CXXFLAGS']

Export('com_env')

############################# Monitor ######################################
monitor_obj = SConscript('./init/SConscript')

MONITOR_EXE = 'monitor'
MONITOR_OBJS = monitor_obj
com_env.Program(target = MONITOR_EXE, source = MONITOR_OBJS)


############################## VOLD #########################################
vold_obj = SConscript('./vold/SConscript')
com_env.Program('vold_test', vold_obj)



############################ update_check ##################################


############################ update_app ####################################
	

############################ bootanimation ##################################
bootan_obj = SConscript('./bootlogo/SConscript')
com_env.Program('bootanimation', bootan_obj)


############################ pro2_service ##################################

#pro2_service_env = com_env.Clone()

#Export('pro2_service_env')
#pro2_service_obj = SConscript('./pro2_service/SConscript')
#com_env.Program('pro2service', pro2_service_obj)






