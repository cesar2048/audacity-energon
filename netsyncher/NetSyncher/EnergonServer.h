#pragma once

#include "time.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>
#include <map>

#include "WebsocketProtocol.h"
#include "EnergonServer.h"
#include "NetSyncher.h"

using namespace std; 

class IEnergonSubscriber {
public:
	virtual void onClientConnected() = 0;
};

class EnergonServer : public HttpServer::IRouteHandler, public IWebsocketApp
{
	NetSynch::NetSyncher* syncher;
	IEnergonSubscriber* subscriber;

	// httpServer operation
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
	EnergonServer(NetSynch::NetSyncher* syncher);

	void setSubscriber(IEnergonSubscriber* subscriber);
};

