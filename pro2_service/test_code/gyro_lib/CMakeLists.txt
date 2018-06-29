cmake_minimum_required(VERSION 3.6)
project(gyro_test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SOURCE ../gyro.cpp ../stlog/stlog.cpp gyro_wrapper.cpp ../arlog.c)

#add_executable(${PROJECT_NAME} ${SOURCE})
add_library(gyro2.0 SHARED ${SOURCE})
#third android lib
find_library(LIB_I2C_PATH libi2c.so)
MESSAGE(STATUS, "i2c lib path " ${LIB_I2C_PATH})

find_library(LIB_LOG_PATH libins_log.so)
message(STATUS "lib log path" ${LIB_LOG_PATH})

target_link_libraries(gyro2.0 ${LIB_LOG_PATH} ${LIB_I2C_PATH} log android)

MESSAGE(STATUS, "CMAKE_LIBRARY_PATH path " /home/vans/project_new/Android/android-native-libs/arm64-v8a/lib_mine)
install(TARGETS gyro2.0 LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS gyro2.0 LIBRARY DESTINATION /home/vans/project_new/Android/android-native-libs/arm64-v8a/lib_mine)

set(SRC_FILES main.cpp)
add_executable(gyro_test ${SRC_FILES})
find_library(LIB_PATH libgyro2.0.so)
MESSAGE(STATUS, "gyro lib path " ${LIB_PATH})

target_link_libraries(gyro_test ${LIB_I2C_PATH} ${LIB_PATH} log android)
install(TARGETS gyro_test RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
