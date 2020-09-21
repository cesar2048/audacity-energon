#include "NetSyncher.h"

NetSyncher::NetSyncher() {
}

void NetSyncher::onStartRecording()
{
	if (this->client != nullptr) {
		this->client->startRecording();
	}
}

void NetSyncher::onStopRecording()
{
	if (this->client != nullptr) {
		this->client->stopRecording();
	}
}

bool NetSyncher::acceptClient(shared_ptr<IClientDevice> client)
{
	this->client = client;
	return true;
}

