#include "SyncherCore.h"
#include <iostream>
#include "NetSyncher.h"
#include "WebApp.h"
#include "WxHttpServer.h"
#include "ConnectionWindow.h"
#include "osinterface.h"
#include "JsonCode.h"

SyncherCore::SyncherCore(wxWindow* parent)
	: wsServer(NULL),
	server(NULL),
	parent(parent)
{
	this->ns = new NetSynch::NetSyncher();
	this->app = new WebApp(this->ns);
	this->recording = false;
}

SyncherCore::~SyncherCore() {
	delete this->app;
	delete this->ns;
	this->server.reset();
	this->wsServer.reset();
}

void SyncherCore::OnRecord()
{
	if (!this->recording) {
		this->ns->StartRecording();
	}
	else {
		this->ns->StopRecording();
	}
	this->recording = !this->recording;
}

void SyncherCore::OnStartServer() {
	if (!this->wsServer) {
		this->wsServer = WebsocketServer::CreateWebsocketserver(this->app);

		this->server = AllocateWebServer();
		this->server->SetRouteHandler(this->app);
		this->server->RegisterUpdateHandler("websocket", this->wsServer.get());
		this->server->Listen(8080);
	}

	vector<string> addresses = FindOSInterfaces(4);
	JsonConnectInfo info(addresses);
	const char* addressData = info.toJsonString().c_str();

	ConnectionWindow* conectionWindow = new ConnectionWindow(this->parent, "Connection", wxPoint(100, 100), wxSize(450, 340), addressData);
	conectionWindow->Show();
}
bool SyncherCore::isRecording()
{
	return this->recording;
}
;

