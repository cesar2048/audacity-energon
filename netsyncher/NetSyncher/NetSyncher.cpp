#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "NetSyncher.h"
#include "HttpProtocol.h"
#include <chrono>

NetSyncher::NetSyncher() {
}

void NetSyncher::Listen() {
	this->server.Listen(8080);
	wxString Foobar;
	Foobar.Printf(wxT("Hello I have %d cookies."), 10);
	wxMessageBox(Foobar);
}

// ------------
class WxInputStream : public IOStream {
private:
	wxSocketBase *socket;
	int lastRead;
public:
	WxInputStream(wxSocketBase *socket) {
		this->socket = socket;
		this->lastRead = 0;
	}
	// Heredado vía InputStream
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override
	{
		if (lastRead == -1) {
			lastRead = 0;
			return 0;
		}
		socket->Read(buffer, len);
		int read = socket->LastReadCount();
		if (read < len) {
			lastRead = -1;
		}
		return read;
	}

	// Heredado vía IOStream
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override
	{
		this->socket->Write(buffer, len);
		return this->socket->LastWriteCount();
	}
};

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
	int count = 0;
	char tempLine[2048];
	long dur = 0;

	while (this->isServerThreadAlive) {
		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::this_thread::yield();

		wxSocketBase socket;
		if (this->server->AcceptWith(socket, false)) {
			std::chrono::system_clock::time_point ini = std::chrono::system_clock::now();

			WxInputStream ioSocket(&socket);

			HttpProtocol hp(&ioSocket);
			HttpRequestMsg req = hp.readRequest();

			HttpResponseMsg res;
			res.setHeader("Content-Type", "application/json");

			

			sprintf_s(tempLine, 2048, "{\"Hello\":\"world\",\"Count\":\"%d\",\"Duration\":\"%d\"}", count++, dur);
			res.write(tempLine);

			hp.sendResponse(res);
			socket.Close();

			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - ini).count();
			// this->isServerThreadAlive = false;
		}
	}
}

