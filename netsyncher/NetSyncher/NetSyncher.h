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
		virtual void onStartRecording() = 0;
		virtual void onStopRecording() = 0;
	};

	shared_ptr<IClientDevice> client;

	NetSyncher();
	void StartRecording();
	void StopRecording();
	bool acceptClient(shared_ptr<IClientDevice> client);
};

