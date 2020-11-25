#pragma once
#include "WebApp.h"


class SyncherCore
{
public:
	SyncherCore(wxWindow* parent);
	~SyncherCore();

	void OnRecord();
	void OnStartServer();
	bool isRecording();

private:
	shared_ptr<HttpServer> server;
	shared_ptr<WebsocketServer> wsServer;

	NetSynch::NetSyncher* ns;
	WebApp* app;
	bool recording;
	wxWindow* parent;
};

