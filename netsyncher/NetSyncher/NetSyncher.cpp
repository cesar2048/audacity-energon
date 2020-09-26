#include "NetSyncher.h"

using namespace NetSynch;

NetSyncher::NetSyncher() :id(0) 
{	
}

void NetSyncher::StartRecording()
{
	auto ptr = clients.begin();
	auto end = clients.end();
	for (; ptr != end; ptr++) {
		ptr->second->onStartRecording();
	}
}

void NetSyncher::StopRecording()
{
	auto ptr = clients.begin();
	auto end = clients.end();
	for (; ptr != end; ptr++) {
		ptr->second->onStopRecording();
	}
}

bool NetSyncher::acceptClient(shared_ptr<IClientDevice> client)
{
	if (client != nullptr) {
		clients[this->id++] = client;
		return true;
	}
	return false;
}

