#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "NetSyncher.h"

NetSyncher::NetSyncher() {
}

void NetSyncher::Listen() {
	this->server.Listen(8080);
	wxString Foobar;
	Foobar.Printf(wxT("Hello I have %d cookies."), 10);
	wxMessageBox(Foobar);
}

// ---------------------------------

HttpServer::HttpServer() {
	this->isServerThreadAlive = true;
}

HttpServer::~HttpServer() {
	if (this->serverThread) {
		this->isServerThreadAlive = true;
		this->serverThread->join();
		delete this->serverThread;
	}
}

void HttpServer::Listen(int port) {
	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(port);

	this->server = new wxSocketServer(addr, wxSOCKET_NONE);
	this->serverThread = new std::thread(&HttpServer::ListenLoop, this, port); // https://stackoverflow.com/a/10673671
}

void HttpServer::ListenLoop(int port) {
	while (this->isServerThreadAlive) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		wxSocketBase socket;
		if (this->server->AcceptWith(socket, false)) {
			char buffer[1024];
			socket.Read(buffer, 1024);

			int i = socket.LastReadCount();

			char response[] = "HTTP/1.1 200 ok\r\nContent-Type: application/json\r\n\r\n{\"Hello\":\"world\"}";
			socket.Write(response, strlen(response));
			socket.Close();
			// this->isServerThreadAlive = false;
		}
	}
}

