#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <chrono>
#include "time.h"

#include <wx/wxprec.h>
#include "debugapi.h"

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
			// force peek by doing a blocking read and unread
			bytesRead = this->read(buffer, len);

			if (bytesRead != 0) {
				this->socket->Unread(buffer, bytesRead);
			}
		}

		return bytesRead;
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

			// sprintf(tempLine, "request-%d.mp4", count);
			// WritePiecesToFile(tempLine, &ioSocket);
			
			HttpRequestMsg req = hp.readRequest();
			shared_ptr<MultipartStream> file = req.readFile("filename");

			int totalRead = 0;
			int totalExpected = 0;
			if (file != nullptr) {
				int read = -1;
				totalExpected = file->getLength();

				uint8_t buffer[4096];
				time_t ts = time(NULL);
				sprintf(tempLine, "video-%d.mp4", ts);
				FILE* f = fopen(tempLine, "wb");
				do {
					read = file->read(buffer, 2048);
					totalRead += read;
					fwrite(buffer, sizeof(uint8_t), read, f);
				} while (read != 0);
				fclose(f);
			}

			HttpResponseMsg res;
			res.setHeader("Content-Type", "application/json");
			sprintf_s(tempLine, 2048, "{\"Hello\":\"world\",\n"
				"\"Count\":%d,\n"
				"\"Duration\":%d,"
				"\"Received\":%d,"
				"\"Expected\":%d"
				"\n}", count++, dur, totalRead, totalExpected);
			res.write(tempLine);

			hp.sendResponse(res);
			socket.Close();

			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - ini).count();
		}
	}
}

