#pragma once

#include "time.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>

#include "WebsocketProtocol.h"
#include "WebApp.h"
#include "NetSyncher.h"

using namespace std; 

class WebApp : public HttpServer::IRouteHandler, public WebsocketServer::IWebsocketApplication
{
	NetSynch::NetSyncher* syncher;
	
	// Heredado v�a IRouteHandler
	virtual shared_ptr<HttpResponseMsg> OnRequest(HttpRequestMsg* req) override;

	// Heredado v�a IWebsocketApplication
	virtual shared_ptr<WebsocketServer::IMessageHandler> CreateHandler() override;

public:
	WebApp(NetSynch::NetSyncher* syncher);
};
