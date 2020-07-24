#pragma once

#include <wx/wxprec.h>
#include <wx/socket.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class NetSyncher
{
public:
	NetSyncher();
	void Listen();
private:
	wxSocketServer* server;
};

