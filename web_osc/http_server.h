#ifndef _HTT_SERVER_H_
#define _HTT_SERVER_H_

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <vector>

#include "web_http.h"


typedef void OnRspCallback(mg_connection *c, std::string);
using ReqHandler = std::function<bool (std::string, std::string, mg_connection *c, OnRspCallback)>;

#define		METHOD_GET	(1 << 0)
#define		METHOD_POST	(1 << 1)

struct HttpRequest {
	std::string 	mUrl;
	int 			mReqMethod;
	ReqHandler		mHandler;

	HttpRequest(std::string url, int method, ReqHandler handler) {
		mUrl = url;
		mReqMethod = method;
		mHandler = handler;
	}

};


class HttpServer {
public:
					HttpServer() {}
					~HttpServer() {}
	void 			Init(const std::string &port); 
	bool 			startHttpServer(); 
	bool 			stopHttpServer(); 

	bool			registerUrlHandler(std::shared_ptr<struct HttpRequest>);

	static 			std::string 		s_web_dir;
	static 			mg_serve_http_opts	s_server_option;

private:

	void 	OnHttpEvent(mg_connection *connection, int event_type, void *event_data);
	void 			HandleEvent(mg_connection *connection, http_message *http_req);
	static void 	SendRsp(mg_connection *connection, std::string rsp);

	std::vector<std::shared_ptr<struct HttpRequest>>	mSupportRequest;

	std::mutex 											mRegLock;
	std::string 										mPort;    
	mg_mgr 												mMgr;          
};

#endif /* _HTT_SERVER_H_ */