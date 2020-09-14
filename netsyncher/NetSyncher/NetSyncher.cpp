#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "NetSyncher.h"
#include "HttpProtocol.h"
#include <chrono>
#include "debugapi.h"

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
	// this->ListenLoop(port);
}

void HttpServer::ListenLoop(int port) {

	// how to print to VisualStudio debug console 
	// https://stackoverflow.com/questions/1333527/how-do-i-print-to-the-debug-output-window-in-a-win32-app
	//
	// slow connection to localhost
	// https://github.com/golang/go/issues/23366#issuecomment-374397983

	int count = 0;
	char tempLine[2048];
	long dur = 0;

	while (this->isServerThreadAlive) {
		wxSocketBase socket;
		bool hasConnection = this->server->WaitForAccept(0, 10);
		if (hasConnection) {
			OutputDebugString(L"Connection accepted\n");

			this->server->AcceptWith(socket, false);
		
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
		}
	}
}

