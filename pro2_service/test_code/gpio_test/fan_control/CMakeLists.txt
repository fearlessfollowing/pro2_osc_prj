cmake_minimum_required(VERSION 3.6)
project(fan_control)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SOURCE main.cpp ../fan_control.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})

find_library(LIB_LOG_PATH libins_log.so)
MESSAGE(STATUS, " log path " ${LIB_LOG_PATH})

#third android lib
find_library(LIB_PATH libgpio.so)
target_link_libraries(${PROJECT_NAME} ${LIB_LOG_PATH} ${LIB_PATH} log android)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})