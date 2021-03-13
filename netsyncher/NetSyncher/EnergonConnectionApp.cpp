#include "EnergonConnectionApp.h"
#include <iostream>
#include "NetSyncher.h"
#include "EnergonServer.h"
#include "WxHttpServer.h"
#include "ConnectionWindow.h"
#include "osinterface.h"
#include "JsonCode.h"

EnergonConnectionApp::EnergonConnectionApp(wxWindow* parent)
	: parent(parent),
	httpServer(NULL),
	wsServer(NULL),
	conectionWindow(NULL),
	recording(false),
	ns(new NetSynch::NetSyncher()),
	app(new EnergonServer(this->ns))
{
	// this->ns = new NetSynch::NetSyncher();
	// this->app = new EnergonServer(this->ns);
	this->app->setSubscriber(this);
	// this->recording = false;
}

EnergonConnectionApp::~EnergonConnectionApp() {
	delete this->app;
	delete this->ns;
	this->httpServer.reset();
	this->wsServer.reset();
}

void EnergonConnectionApp::OnRecord()
{
	if (!this->recording) {
		this->ns->StartRecording();
		this->recording = true;
	}
}

void EnergonConnectionApp::OnStop()
{
	if (this->recording) {
		this->ns->StopRecording();
		this->recording = false;
	}
}

void EnergonConnectionApp::startServer() {
	if (!this->wsServer) {
		this->wsServer = WebsocketServer::CreateWebsocketserver(this->app);

		this->httpServer = AllocateWebServer();
		this->httpServer->SetRouteHandler(this->app);
		this->httpServer->RegisterUpdateHandler("websocket", this->wsServer.get());
		this->httpServer->Listen(8080);
	}

	if (this->conectionWindow == NULL) {
		vector<string> addresses = FindOSInterfaces(4);
		JsonConnectInfo info(addresses);
		string addressString = info.toJsonString();
		const char* addressData = addressString.c_str();

		this->conectionWindow = new ConnectionWindow(this->parent, "Connection", wxPoint(100, 100), wxSize(450, 340), addressData);
	}
	
	this->conectionWindow->Show();
}

bool EnergonConnectionApp::isRecording()
{
	return this->recording;
}

void EnergonConnectionApp::onClientConnected()
{
	if (this->conectionWindow != NULL) {
		this->conectionWindow->Hide();
	}
}
