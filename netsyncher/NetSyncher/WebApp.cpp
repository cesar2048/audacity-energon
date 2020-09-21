#include "WebApp.h"

// ------------

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

	// Heredado vía IOStream
	virtual void close() override
	{
		/// do nothing here
	}
};

class WxNetwork : public HttpServer::INetwork {
	wxSocketServer* server;
	int port;

public:
	// Heredado vía INetwork
	virtual void Listen(int port) override
	{
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
		
		while (!hasConnection) {
			hasConnection = this->server->WaitForAccept(0, 10);
			if (hasConnection) {
				DebugLog("Connection accepted\n");
				this->server->AcceptWith(stream->socket, false);
			}
		}

		return shared_ptr<IOStream>(stream);
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


WebApp::WebApp()
{
	this->network = new WxNetwork();
	this->server = new HttpServer(network, this);
	this->wsServer = new WebsocketServer();

	this->server->registerUpdateHandler("websocket", this->wsServer);
}

void WebApp::Listen(int port)
{
	this->server->Listen(port);
}

shared_ptr<HttpResponseMsg> WebApp::onRequest(HttpRequestMsg* req)
{
	char tempLine[2048];
	shared_ptr<MultipartStream> file = req->readFile("filename");

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

	HttpResponseMsg* res = new HttpResponseMsg();
	res->setHeader("Content-Type", "application/json");
	sprintf_s(tempLine, 2048, "{\n\"Hello\":\"world\",\n"
		"\"Received\":%d,\n"
		"\"Expected\":%d\n}", totalRead, totalExpected);
	res->write(tempLine);

	return shared_ptr<HttpResponseMsg>(res);
}



/*

void HttpServer::Listen(int port) {
	// create a new thread
	// https://stackoverflow.com/a/10673671

	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(port);

	this->server = new wxSocketServer(addr, wxSOCKET_NONE);
	this->serverThread = new std::thread(&HttpServer::ListenLoop, this, port);
}

void HttpServer::registerUpdateHandler(const char * key, shared_ptr<IUpgradeHandler> handler)
{
	this->upgradeHandlers[key] = handler;
}


void HttpServer::ListenLoop(int port) {
	// how to print to VisualStudio debug console 
	// https://stackoverflow.com/questions/1333527/how-do-i-print-to-the-debug-output-window-in-a-win32-app

	// slow connection to localhost
	// https://github.com/golang/go/issues/23366#issuecomment-374397983

	long elapsed = 0;
	WebsocketProtocol ws;
	WebApp app;

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
				auto handler = this->upgradeHandlers.find(*upgrade);
				if (handler != this->upgradeHandlers.end()) {
					handler->second->Upgrade(&http, &ioSocket);
				}
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
*/

/*
void WebApp::Upgrade(HttpProtocol* http,  HttpRequestMsg *req, IOStream * stream)
{
	auto clientKey = req->getHeader("sec-websocket-key");
	HttpResponseMsg res;
	res.statusCode = 101;
	res.setHeader("Upgrade", "websocket");
	res.setHeader("Connection", "Upgrade");
	res.setHeader("Sec-WebSocket-Protocol", "recording");
	res.setHeader("Sec-WebSocket-Accept", ws.CalculateSignature(clientKey->c_str()));
	http->sendResponse(res);

	stream->write(out.buffer, wsBytes);
}
*/