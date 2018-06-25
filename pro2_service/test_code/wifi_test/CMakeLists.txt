cmake_minimum_required(VERSION 3.6)
project(wifi_test)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -fexceptions -pie -fPIE -Wall -Wunused-variable -Werror")

set(SRC_FILES main.cpp ifc_utils.c BandwidthController.cpp TetherController.cpp NetdConstants.cpp SoftapController.cpp stringprintf.cpp file.cpp NatController.cpp RouteController.cpp UidRanges.cpp DummyNetwork.cpp Network.cpp logwrap.c NetworkController.cpp VirtualNetwork.cpp LocalNetwork.cpp PhysicalNetwork.cpp IdletimerController.cpp FirewallController.cpp StrictController.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

find_library(LIB_PATH libcrypto_static.a)
MESSAGE(STATUS, "LIB_PATH " ${LIB_PATH})
target_link_libraries(${PROJECT_NAME} ${LIB_PATH})

find_library(LIB_PATH_BASE libbase.a)
MESSAGE(STATUS, "LIB_PATH_BASE " ${LIB_PATH_BASE})
target_link_libraries(${PROJECT_NAME} ${LIB_PATH_BASE})

target_link_libraries(${PROJECT_NAME} log android)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})