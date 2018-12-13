#include <utility>
#include <sys/ins_types.h>
#include "http_server.h"


void HttpServer::Init(const std::string &port)
{
	mPort = port;
	s_server_option.enable_directory_listing = "yes";
	s_server_option.document_root = s_web_dir.c_str();
}


bool HttpServer::startHttpServer()
{
	mg_mgr_init(&mMgr, NULL);
	mg_connection *connection = mg_bind(&mMgr, mPort.c_str(), OnHttpEvent);
	if (connection == NULL)
		return false;

	mg_set_protocol_http_websocket(connection);

	printf("starting http server at port: %s\n", mPort.c_str());

	while (true)
		mg_mgr_poll(&mMgr, 500); 	// ms

	return true;
}


void HttpServer::OnHttpEvent(mg_connection *connection, int event_type, void *event_data)
{
	http_message *httpReq = (http_message *)event_data;
	
	switch (event_type) {
		case MG_EV_HTTP_REQUEST:
			HandleEvent(connection, httpReq);
			break;
	
		case MG_EV_CLOSE: {
			printf("---> close connection socket[%d] now\n", connection->sock);
			break;
		}

		case MG_EV_CONNECT: {
			printf("---> startup connection socket[%d] now\n", connection->sock);			
			break;
		}

		default:
			break;
	}
}




bool HttpServer::registerUrlHandler(std::shared_ptr<struct HttpRequest> uriHandler)
{
	std::shared_ptr<struct HttpRequest> tmpPtr = nullptr;
	bool bResult = false;
	u32 i = 0;
	std::unique_lock<std::mutex> _lock(mRegLock);

	for (i = 0; i < mSupportRequest.size(); i++) {
		tmpPtr = mSupportRequest.at(i);
		if (tmpPtr && uriHandler) {
			if (uriHandler->mUrl == tmpPtr->mUrl) {
				fprintf(stderr, "url have registed, did you cover it\n");
				break;
			}
		}
	}

	if (i >= mSupportRequest.size()) {
		mSupportRequest.push_back(uriHandler);
		bResult = true;
	}

	return bResult;
}


void HttpServer::SendRsp(mg_connection *connection, std::string rsp)
{
	mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(connection, "{ \"result\": %s }", rsp.c_str());
	mg_send_http_chunk(connection, "", 0);
}



void HttpServer::HandleEvent(mg_connection *connection, http_message *httpReq)
{
	/*
	 * message:包括请求行 + 头 + 请求体
	 */
#if DEBUG_HTTP_SERVER	
	std::string req_str = std::string(httpReq->message.p, httpReq->message.len);	
	printf("Request Message: %s\n", req_str.c_str());
#endif

	bool bHandled = false;
	std::string reqMethod = std::string(httpReq->method.p, httpReq->method.len);
	std::string reqUri = std::string(httpReq->uri.p, httpReq->uri.len);
	std::string reqProto = std::string(httpReq->proto.p, httpReq->proto.len);
	std::string reqBody = std::string(httpReq->body.p, httpReq->body.len);

	printf("Req Method: %s", reqMethod.c_str());
	printf("Req Uri: %s", reqUri.c_str());
	printf("Req Proto: %s", reqProto.c_str());

	printf("body: %s\n", reqBody.c_str());


	std::shared_ptr<struct HttpRequest> tmpRequest;
	u32 i = 0;
	for (i = 0; i < mSupportRequest.size(); i++) {
		tmpRequest = mSupportRequest.at(i);
		if (tmpRequest) {
			if (reqUri == tmpRequest->mUrl) {
				int method = 0;

				if (reqMethod == "GET" || reqMethod == "get") {
					method |= METHOD_GET;
				} else if (reqMethod == "POST" || reqMethod == "post") {
					method |= METHOD_POST;
				}

				if (method & tmpRequest->mReqMethod) {	/* 支持该方法 */
					tmpRequest->mHandler(reqUri, reqBody, connection, SendRsp);
					bHandled = true;
				}
			}
		}
	}

	if (!bHandled) {
		mg_http_send_error(connection, 404, NULL);
	}
}

bool HttpServer::stopHttpServer()
{
	mg_mgr_free(&mMgr);
	return true;
}