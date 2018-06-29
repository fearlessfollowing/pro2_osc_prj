cmake_minimum_required(VERSION 3.6)
project(i2c_test)
#project(usbreset)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SOURCE ../ins_i2c.cpp)
set(LIB_NAME i2c)
add_library(${LIB_NAME} SHARED ${SOURCE})

find_library(LIB_LOG_PATH libins_log.so)
message(STATUS "lib log path" ${LIB_LOG_PATH})

#third android lib
target_link_libraries(${LIB_NAME} ${LIB_LOG_PATH} log android)
#MESSAGE(STATUS, "link third lib " ${PROJECT_NAME})
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION /home/vans/project_new/Android/android-native-libs/arm64-v8a/lib_mine)

set(SRC_FILES ins_i2c_test.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

#find_library(LOG_PATH libins_log.so)
#MESSAGE(STATUS, "libins_log path " ${LOG_PATH})

find_library(LIB_PATH libi2c.so)
MESSAGE(STATUS, "i2c lib path " ${LIB_PATH})

target_link_libraries(${PROJECT_NAME} ${LIB_PATH} ${LIB_LOG_PATH} log android)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})