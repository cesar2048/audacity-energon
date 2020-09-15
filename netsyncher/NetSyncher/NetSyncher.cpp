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
class WxIOStream : public IOStream {
private:
	wxSocketBase *socket;

	// processes the read amount, or any read error
	uint32_t postRead() {
		if (socket->Error()) {
			return 0;
		}

		int read = socket->LastReadCount();
		return read;
	}

public:
	WxIOStream(wxSocketBase *socket) {
		this->socket = socket;
	}

	virtual uint32_t peek(uint8_t * buffer, uint32_t len) override
	{
		socket->SetTimeout(5);
		socket->Peek(buffer, len);
		return postRead();
	}

	// Heredado vía InputStream
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override
	{
		socket->SetTimeout(5);
		socket->Read(buffer, len);
		return postRead();
	}

	// Heredado vía IOStream
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override
	{
		this->socket->Write(buffer, len);
		return this->socket->LastWriteCount();
	}
};

// ---------------------------------

void WritePiecesToFile(WxIOStream *ios) {
	const uint32_t KiloByte = 1024;
	const uint32_t MegaByte = KiloByte * 1024;
	const uint32_t BuffSize = 5 * MegaByte;

	uint8_t* buffer = (uint8_t*)malloc(BuffSize);
	char tempLine[2048];

	FILE* f = fopen("outputdata.bin", "wb");

	uint32_t bytesRead = 0, bytesUsed = 0;
	do {
		bytesRead = ios->read(buffer, BuffSize);
		fwrite(&bytesRead, sizeof(uint32_t), 1, f);
		fwrite(buffer, sizeof(uint8_t), bytesRead, f);

		sprintf(tempLine, "%d bytes written to file\n", bytesRead);
		OutputDebugStringA(tempLine);
	} while (bytesRead != 0);

	free(buffer);
	fclose(f);
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

			WxIOStream ioSocket(&socket);
			HttpProtocol hp(&ioSocket);

			// HttpRequestMsg req = hp.readRequest();
			WritePiecesToFile(&ioSocket);

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

