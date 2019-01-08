#include <iostream>
#include <memory>
#include <sys/ins_types.h>
#include "log_wrapper.h"

#include "osc_server.h"


int main(int argc, char *argv[]) 
{
    LogWrapper::init("/home/nvidia/insta360/log", "osc_log", true);
    
	printf("----------------- OSC Server(V1.0) Start ----------------\n");
	OscServer* oscServer = OscServer::Instance();
	oscServer->startOscServer();

	printf("----------------- OSC Server normal exit ----------------\n");
	return 0;
}