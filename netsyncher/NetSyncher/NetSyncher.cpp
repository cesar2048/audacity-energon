#include "NetSyncher.h"

NetSyncher::NetSyncher() {
}

void NetSyncher::StartRecording()
{
	if (this->client != nullptr) {
		this->client->onStartRecording();
	}
}

void NetSyncher::StopRecording()
{
	if (this->client != nullptr) {
		this->client->onStopRecording();
	}
}

bool NetSyncher::acceptClient(shared_ptr<IClientDevice> client)
{
	this->client = client;
	return true;
}

