#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "NetSyncher.h"
#include "HttpProtocol.h"

NetSyncher::NetSyncher() {
}

void NetSyncher::Listen() {
	this->server.Listen(8080);
	wxString Foobar;
	Foobar.Printf(wxT("Hello I have %d cookies."), 10);
	wxMessageBox(Foobar);
}

// ------------
class WxInputStream : public InputStream {
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
	while (this->isServerThreadAlive) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		wxSocketBase socket;
		if (this->server->AcceptWith(socket, false)) {
			WxInputStream wis(&socket);
			HttpProtocol hp;

			hp.readRequest(&wis);

			char response[] = "HTTP/1.1 200 ok\r\nContent-Type: application/json\r\n\r\n{\"Hello\":\"world\"}";
			socket.Write(response, strlen(response));
			socket.Close();
			// this->isServerThreadAlive = false;
		}
	}
}

