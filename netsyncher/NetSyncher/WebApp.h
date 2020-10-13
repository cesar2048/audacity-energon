#pragma once

#include "time.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>
#include <map>

#include "WebsocketProtocol.h"
#include "WebApp.h"
#include "NetSyncher.h"

using namespace std; 

class WebApp : public HttpServer::IRouteHandler, public IWebsocketApp
{
	NetSynch::NetSyncher* syncher;

	shared_ptr<HttpResponseMsg> post_upload(HttpRequestMsg* req);
	shared_ptr<HttpResponseMsg> get(HttpRequestMsg* req);
	shared_ptr<HttpResponseMsg> not_found(HttpRequestMsg* req);
	
	// Inherited from IRouteHandler
	virtual shared_ptr<HttpResponseMsg> OnRequest(HttpRequestMsg* req) override;

	// Inherited from IWebsocketApp
	virtual void onMessage(WebSocketBase * conn, string message) override;
	virtual void onOpen(WebSocketBase * conn) override;
	virtual void onClose(WebSocketBase * conn) override;

public:
	WebApp(NetSynch::NetSyncher* syncher);
};
