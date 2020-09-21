#pragma once

#include "time.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>

#include "HttpProtocol.h"
#include "WebsocketProtocol.h"
#include "WebApp.h"

using namespace std; 

class WebApp : public HttpServer::IRouteHandler
{
	HttpServer::INetwork* network;
	HttpServer* server;
	WebsocketServer* wsServer;

public:
	WebApp();

	void Listen(int port);

	// Heredado vía IRouteHandler
	virtual shared_ptr<HttpResponseMsg> onRequest(HttpRequestMsg* req) override;
};

