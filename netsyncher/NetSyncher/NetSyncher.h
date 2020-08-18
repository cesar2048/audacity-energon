#pragma once

#include <thread>
#include <chrono>

#include <wx/wxprec.h>
#include <wx/socket.h>

#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

class HttpServer
{
public:
	HttpServer();
	~HttpServer();
	void Listen(int port);
private:
	wxSocketServer* server;
	std::thread* serverThread;
	bool isServerThreadAlive;

	void ListenLoop(int port);
};


class NetSyncher
{
public:
	NetSyncher();
	void Listen();
private:
	HttpServer server;
};

