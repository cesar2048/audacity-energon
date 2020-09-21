#pragma once

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif
#include <memory>

using namespace std;

/// High level net synchronizer application
class NetSyncher
{
public:
	class IClientDevice
	{
	public:
		virtual void startRecording() = 0;
		virtual void stopRecording() = 0;
	};

	shared_ptr<IClientDevice> client;

	NetSyncher();
	void onStartRecording();
	void onStopRecording();
	bool acceptClient(shared_ptr<IClientDevice> client);
};

