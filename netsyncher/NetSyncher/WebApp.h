#pragma once

#include "time.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>

#include "HttpProtocol.h"
#include "WebsocketProtocol.h"
#include "WebApp.h"

using namespace std; 

HttpServer* AllocateWebServer();

class WebApp : public HttpServer::IRouteHandler, public WebsocketServer::IMessageHandler
{
	// Heredado vía IRouteHandler
	virtual shared_ptr<HttpResponseMsg> OnRequest(HttpRequestMsg* req) override;

	// Heredado vía IMessageHandler
	virtual void OnMessage(uint8_t * buffer, uint32_t len) override;
};