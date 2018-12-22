#include <iostream>
#include <memory>
#include <sys/ins_types.h>

#include "osc_server.h"


int main(int argc, char *argv[]) 
{
	printf("----------------- OSC Server(V1.0) Start ----------------\n");
	OscServer* oscServer = OscServer::Instance();
	oscServer->startOscServer();

	printf("----------------- OSC Server normal exit ----------------\n");
	return 0;
}