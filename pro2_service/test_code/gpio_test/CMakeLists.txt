cmake_minimum_required(VERSION 3.6)
project(gpio_test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SOURCE ../ins_gpio.cpp)
set(LIB_NAME gpio)
add_library(${LIB_NAME} SHARED ${SOURCE})

#
##set_target_properties (${LIB_NAME} PROPERTIES VERSION 1.2 SOVERSION 4)
##message(STATUS," PROJECT_BINARY_DIR " ${PROJECT_BINARY_DIR})
##set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
##SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#

find_library(LIB_LOG_PATH libins_log.so)
target_link_libraries(${LIB_NAME} ${LIB_LOG_PATH} log android)

message(STATUS, "env is " $ENV{CMAKE_LIBRARY_PATH})
target_link_libraries(${LIB_NAME} ${LIB_LOG_PATH} log android)
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ${LIB_NAME} LIBRARY DESTINATION $ENV{CMAKE_LIBRARY_PATH})

set(SRC_FILES main.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})
find_library(LIB_PATH libgpio.so)
target_link_libraries(${PROJECT_NAME} ${LIB_LOG_PATH} ${LIB_PATH} log android)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})

