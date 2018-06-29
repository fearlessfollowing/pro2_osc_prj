cmake_minimum_required(VERSION 3.6)
project(rtc_test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")
set(SOURCE main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})

#third android lib
target_link_libraries(${PROJECT_NAME} log android)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})