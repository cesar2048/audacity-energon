#pragma once

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <memory>
#include <map>

using namespace std;

/// High level net synchronizer application
namespace NetSynch {

	class IClientDevice
	{
	public:
		virtual void onStartRecording() = 0;
		virtual void onStopRecording() = 0;
	};

	class NetSyncher
	{
		std::map<int, shared_ptr<IClientDevice>> clients;
		int id;

	public:

		NetSyncher();
		void StartRecording();
		void StopRecording();

		bool acceptClient(shared_ptr<IClientDevice> client);
	};

}

