#pragma once
#include "EnergonServer.h"

// forward declarations
class ConnectionWindow;

class EnergonConnectionApp : public IEnergonSubscriber
{
public:
	EnergonConnectionApp(wxWindow* parent);
	~EnergonConnectionApp();

	void OnRecord();
	void OnStop();
	void startServer();
	bool isRecording();

private:
	shared_ptr<HttpServer> httpServer;
	shared_ptr<WebsocketServer> wsServer;
	ConnectionWindow* conectionWindow;

	NetSynch::NetSyncher* ns;
	EnergonServer* app;
	bool recording;
	wxWindow* parent;

	// Heredado vía IEnergonSubscriber
	virtual void onClientConnected() override;
};

