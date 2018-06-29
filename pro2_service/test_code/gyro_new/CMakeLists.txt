cmake_minimum_required(VERSION 3.6)
project(gyro_new)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")

message(STATUS "cxx flags " ${CMAKE_CXX_FLAGS})
include_directories(./ core/driver/include core/mllite core/driver/eMPL core/mpl core/eMPL-hal core/driver/stm32L )
set(SRC_FILES gyro_wrapper.cpp gyro_new.c gyro_local.c ins_i2c.c
        core/driver/eMPL/inv_mpu.c
        core/driver/eMPL/inv_mpu_dmp_motion_driver.c
        core/mllite/mpl.c
        core/mllite/data_builder.c
        core/mllite/hal_outputs.c
        core/mllite/message_layer.c
        core/mllite/ml_math_func.c
        core/mllite/mlmath.c
        core/mllite/results_holder.c
        core/mllite/start_manager.c
        core/mllite/storage_manager.c
        core/eMPL-hal/eMPL_outputs.c
        core/driver/stm32L/log_stm32.c)
#add_source_directories(./core/driver/eMPL ${SRC_FILES})
#add_source_directoires(./core/mllite/ ${SRC_FILES})
#add_source_directoires(./core/mp/ ${SRC_FILES})
add_library(gyro3.0 SHARED ${SRC_FILES})

add_definitions(-DEMPL_TARGET_NV -DMPU9250)
#add_definitions(-DENABLE_DEBUG)
#add_definitions(-DENABLE_ERROR)
#add_definitions(-DENABLE_DEBUG_INTERVAL)
add_definitions(-DUSE_CAL_HW_REGISTERS)
#add_definitions(-DAK89xx_SECONDARY)
#add_definitions(-DAK8975_SECONDARY)
#add_definitions(-DCOMPASS_ENABLED)

set(LIB_LIST libmllite.a libmplmpu.a)
#set(LIB_LIST libmllite.so libmplmpu.so)
foreach(LIB ${LIB_LIST})
    find_library(LIB_PATH ${LIB})
    MESSAGE(STATUS, "liba  " ${LIB_PATH})
    target_link_libraries(gyro3.0 ${LIB_PATH})
    unset(LIB_PATH CACHE)
endforeach()

#find_library(LIB_PATH libmplmpu.so)
#MESSAGE(STATUS ,"lib path " ${LIB_PATH})
#target_link_libraries(${PROJECT_NAME} ${LIB_PATH} log android m)

target_link_libraries(gyro3.0  log android m)
install(TARGETS gyro3.0 LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS gyro3.0 LIBRARY DESTINATION /home/vans/project_new/Android/android-native-libs/arm64-v8a/lib_mine)

set(SOURCES main.cpp)
add_executable(gyro_new ${SOURCES})
find_library(LIB_PATH libgyro3.0.so)
MESSAGE(STATUS, "gyro lib path " ${LIB_PATH})

target_link_libraries(gyro_new ${LIB_I2C_PATH} ${LIB_PATH} log android)
install(TARGETS gyro_new RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
