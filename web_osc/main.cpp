#include <iostream>
#include <memory>
#include "http_server.h"


bool handle_fun1(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	// do sth
	std::cout << "handle fun1" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

	rsp_callback(c, "rsp1");

	return true;
}

bool handle_fun2(std::string url, std::string body, mg_connection *c, OnRspCallback rsp_callback)
{
	// do sth
	std::cout << "handle fun2" << std::endl;
	std::cout << "url: " << url << std::endl;
	std::cout << "body: " << body << std::endl;

	rsp_callback(c, "rsp2");

	return true;
}



int main(int argc, char *argv[]) 
{
	std::string port = "80";

	// auto httpServer = std::shared_ptr<HttpServer>(new HttpServer);
	// // httpServer->Init(port);

	// httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/info", METHOD_GET | METHOD_POST, handle_fun1));
	// httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/state", METHOD_GET | METHOD_POST, handle_fun1));
	// httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/commands/execute", METHOD_GET | METHOD_POST, handle_fun1));
	// httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/commands/status", METHOD_GET | METHOD_POST, handle_fun1));
	// httpServer->registerUrlHandler(std::make_shared<struct HttpRequest>("/osc/checkForUpdate", METHOD_GET | METHOD_POST, handle_fun1));

	// httpServer->startHttpServer();

	return 0;
}