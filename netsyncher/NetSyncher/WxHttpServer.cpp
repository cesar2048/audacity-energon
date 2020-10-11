#include "WxHttpServer.h"

#include <wx/wxprec.h>
#include <wx/socket.h>
#include <wx/wxprec.h>

#include "HttpProtocol.h"
#include "WebApp.h"


// ------------ Http server WX implementations ---------------------

class WxIOStream : public IOStream {
private:

	// processes the read amount, or any read error
	uint32_t postRead() {
		if (socket.Error()) {
			return 0;
		}

		int read = socket.LastCount();
		return read;
	}

public:
	wxSocketBase socket;

	WxIOStream() {

	}

	virtual uint32_t peek(uint8_t * buffer, uint32_t len) override
	{
		socket.Peek(buffer, len);
		uint32_t bytesRead = postRead();

		if (bytesRead == 0) {
			DebugLog("WxIOStream::peek() got zero bytes, reading now\n");
			// force peek by doing a blocking read and unread
			bytesRead = this->read(buffer, len);

			if (bytesRead != 0) {
				this->socket.Unread(buffer, bytesRead);
			}
			// DebugLog("WxIOStream::peek()=>read() got %i bytes (send to unread)\n", bytesRead);
		}
		else {
			DebugLog("WxIOStream::peek() got %i bytes\n", bytesRead);
		}

		return bytesRead;
	}

	// Heredado vía InputStream
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override
	{
		socket.SetTimeout(5);
		socket.Read(buffer, len);
		uint32_t read = this->postRead();
		DebugLog("WxIOStream::read() got %i bytes\n", read);
		return read;
	}

	// Heredado vía IOStream
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override
	{
		DebugLog("WxIOStream::write() %i bytes\n", len);
		this->socket.Write(buffer, len);
		return this->socket.LastWriteCount();
	}

	// Inherited vía IOStream
	virtual void close() override
	{
		/// do nothing here
	}

	// Inherited vía IOStream
	virtual bool isconnected() override
	{
		return this->socket.IsConnected();
	}
};







class WxNetwork : public HttpServer::INetwork {
	wxSocketServer* server;
	int port;
	bool alive;

public:
	// Heredado vía INetwork
	virtual void Listen(int port) override
	{
		this->alive = true;
		this->port = port;

		wxIPV4address addr;
		addr.AnyAddress();
		addr.Service(this->port);

		this->server = new wxSocketServer(addr, wxSOCKET_NONE);
	}

	// Heredado vía INetwork
	virtual shared_ptr<IOStream> Accept() override
	{
		WxIOStream* stream = new WxIOStream();
		bool hasConnection = false;

		while (this->alive && !hasConnection) {
			hasConnection = this->server->WaitForAccept(0, 10);
			if (hasConnection) {
				DebugLog("Connection accepted\n");
				this->server->AcceptWith(stream->socket, false);
			}
		}

		return shared_ptr<IOStream>(stream);
	}

	// Heredado vía INetwork
	virtual void Close() override
	{
		this->alive = false;
		this->server->Close();
	}
};




class WxWebServer : public HttpServer
{
public:
	WxWebServer() {
		WxNetwork* networkHandler = new WxNetwork();
		this->SetHandlers(networkHandler);
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

shared_ptr<HttpServer> AllocateWebServer()
{
	return shared_ptr<HttpServer>(new WxWebServer());
}

