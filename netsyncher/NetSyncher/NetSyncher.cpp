#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <chrono>
#include "time.h"

#include <wx/wxprec.h>

#include "NetSyncher.h"
#include "HttpProtocol.h"
#include "WebApp.h"
#include "WebsocketProtocol.h"

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

		int read = socket->LastCount();
		return read;
	}

public:
	WxIOStream(wxSocketBase *socket) {
		this->socket = socket;
	}

	virtual uint32_t peek(uint8_t * buffer, uint32_t len) override
	{
		socket->Peek(buffer, len);
		uint32_t bytesRead = postRead();
		
		
		if (bytesRead == 0) {
			DebugLog("WxIOStream::peek() got %i bytes, reading now\n", bytesRead);
			// force peek by doing a blocking read and unread
			bytesRead = this->read(buffer, len);

			if (bytesRead != 0) {
				this->socket->Unread(buffer, bytesRead);
			}
			DebugLog("WxIOStream::peek()=>read() got %i bytes (send to unread)\n", bytesRead);
		}
		else {
			DebugLog("WxIOStream::peek() got %i bytes\n", bytesRead);
		}

		return bytesRead;
	}

	// Heredado v�a InputStream
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override
	{
		socket->SetTimeout(5);
		socket->Read(buffer, len);
		uint32_t read = this->postRead();
		DebugLog("WxIOStream::read() got %i bytes\n", read);
		return read;
	}

	// Heredado v�a IOStream
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override
	{
		DebugLog("WxIOStream::write() %i bytes\n", len);
		this->socket->Write(buffer, len);
		return this->socket->LastWriteCount();
	}
};

// ---------------------------------

void WritePiecesToFile(const char *fileName, WxIOStream *ios) {
	const uint32_t KiloByte = 1024;
	const uint32_t MegaByte = KiloByte * 1024;
	const uint32_t BuffSize = 5 * MegaByte;

	uint8_t* buffer = (uint8_t*)malloc(BuffSize);
	char tempLine[2048];

	FILE* f = fopen(fileName, "wb");

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
	// create a new thread
	// https://stackoverflow.com/a/10673671

	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(port);

	this->server = new wxSocketServer(addr, wxSOCKET_NONE);
	this->serverThread = new std::thread(&HttpServer::ListenLoop, this, port);
}

void HttpServer::ListenLoop(int port) {
	// how to print to VisualStudio debug console 
	// https://stackoverflow.com/questions/1333527/how-do-i-print-to-the-debug-output-window-in-a-win32-app

	// slow connection to localhost
	// https://github.com/golang/go/issues/23366#issuecomment-374397983

	long elapsed = 0;
	WebApp app;
	WebsocketProtocol ws;

	while (this->isServerThreadAlive) {
		wxSocketBase socket;
		bool hasConnection = this->server->WaitForAccept(0, 10);
		if (hasConnection) {
			DebugLog("Connection accepted\n");
			this->server->AcceptWith(socket, false);

			std::chrono::system_clock::time_point ini = std::chrono::system_clock::now();

			WxIOStream ioSocket(&socket);
			HttpProtocol http(&ioSocket);
			
			HttpRequestMsg req = http.readRequest();
			auto upgrade = req.getHeader("upgrade");
			if (upgrade != nullptr && *upgrade == "websocket") {
				auto clientKey = req.getHeader("sec-websocket-key");
				HttpResponseMsg res;
				res.statusCode = 101;
				res.setHeader("Upgrade", "websocket");
				res.setHeader("Connection", "Upgrade");
				res.setHeader("Sec-WebSocket-Protocol", "recording");
				res.setHeader("Sec-WebSocket-Accept", ws.CalculateSignature(clientKey->c_str()));
				http.sendResponse(res);

				Buffer b = Buffer::fromString("Hello world");
				Buffer out(256);
				uint32_t wsBytes = ws.WriteFrame(WSOpcode::TextFrame, b.buffer, b.len, out.buffer, out.len, false);
				socket.Write(out.buffer, wsBytes);
			}
			else {
				auto res = app.onRequest(req);
				http.sendResponse(*res);
				socket.Close();
			}


			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - ini).count();
		}
	}
}

