#include <stdlib.h>
#include <string.h>
#include <unordered_set>

#include "WebsocketProtocol.h"
#include <thread>

uint32_t writeWord(uint8_t* outBuffer, uint32_t word, uint32_t pos) {
	outBuffer[pos++] = word & 0xFF;
	outBuffer[pos++] = (word << 8) & 0xFF;
	outBuffer[pos++] = (word << 16) & 0xFF;
	outBuffer[pos++] = (word << 24) & 0xFF;
	return pos;
}


// -------

WebsocketProtocol::WebsocketProtocol() {
	this->bufferLength = WS_INITIAL_BUFFER;
	this->buffer = (uint8_t*)malloc(WS_INITIAL_BUFFER);
}

WebsocketProtocol::~WebsocketProtocol() {
	this->bufferLength = 0;
	free(this->buffer);
}

char* WebsocketProtocol::CalculateSignature(const char* clientKey) {
	char stringToHash[128];
	char hashResult[SHA1_LEN_OUT_BYTES + 1];

	// generate string to SHA
	stringToHash[0] = NULL;
	strcat(stringToHash, clientKey);
	strcat(stringToHash, wsGUID);

	// SHA the string
	SHA1(hashResult, stringToHash, strlen(stringToHash));

	// format the hash for printing
#if _DEBUG
	char *result = (char*)malloc(41);
	size_t offset;
	for (offset = 0; offset < 20; offset++) {
		sprintf((result + (2 * offset)), "%02x", hashResult[offset] & 0xff);
	}
#endif

	// convert to base64
	int b64OutSize = b64e_size(SHA1_LEN_OUT_BYTES + 1);
	char* b64String = (char*) malloc(sizeof(char) * b64OutSize);
	b64_encode((unsigned char*) hashResult, SHA1_LEN_OUT_BYTES, (unsigned char*) b64String);

	return b64String;
}

uint32_t WebsocketProtocol::WriteFrame(WSOpcode opcode, const uint8_t* inBuffer, uint32_t inSize, uint8_t* outBuffer, uint32_t outSize,bool mask)
{
	/*
	  0               1               2               3
	  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	 +-+-+-+-+-------+-+-------------+-------------------------------+
	 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
	 | |1|2|3|       |K|             |                               |
	 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	 |     Extended payload length continued, if payload len == 127  |
	 + - - - - - - - - - - - - - - - +-------------------------------+
	 |                               |Masking-key, if MASK set to 1  |
	 +-------------------------------+-------------------------------+
	 | Masking-key (continued)       |          Payload Data         |
	 +-------------------------------- - - - - - - - - - - - - - - - +
	 :                     Payload Data continued ...                :
	 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	 |                     Payload Data continued ...                |
	 +---------------------------------------------------------------+
	*/
	// verify min buffer length
	if (outSize < 2) {
		return -1;
	}

	// 7bit [+ 16 bit | + 64 bit]
	int bytesForLength = 1;
	int lengthOpcode = inSize;
	if (inSize > UINT16_MAX) {
		bytesForLength = 1 + 8;
		lengthOpcode = 127;
	} else if (inSize > 125) {
		bytesForLength = 1 + 2;
		lengthOpcode = 126;
	}

	// opcode [+ mask]
	int bytesOverhead = 1;
	if (mask) {
		bytesOverhead = 1 + 4;
	}

	uint8_t isFinal = 0;
	if (outSize >= bytesOverhead + bytesForLength + inSize) {
		isFinal = 1;
	}

	// encode fin, opcode, mask bits and length
	int pos = 0;
	outBuffer[pos++] = opcode | (isFinal << 7);
	outBuffer[pos++] = (mask << 7) | lengthOpcode;

	// encode length (extended)
	if (inSize > 125) {
		outBuffer[pos++] = inSize & 0xFF;
		outBuffer[pos++] = (inSize << 8) & 0xFF;
		if (inSize > UINT16_MAX) {
			outBuffer[pos++] = (inSize << 16) & 0xFF;
			outBuffer[pos++] = (inSize << 24) & 0xFF;
			// zeroing the last 4 bytes, given that with the first 4 bytes
			// (32 bit) we are talking about a 4Gb memory buffer already
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
		}
	}

	if (mask) {
		uint32_t maskValue = (uint32_t)rand();
		pos = writeWord(outBuffer, maskValue, pos);
	}

	for (uint32_t i = 0; i < inSize; i++) {
		outBuffer[pos++] = inBuffer[i];
	}

	return pos;
}

std::shared_ptr<WS_MSG> WebsocketProtocol::ReadFrame(uint8_t * buffer, uint32_t bufferLength)
{
	std::shared_ptr<WS_MSG> frame = std::make_shared<WS_MSG>();
	// frame->buffer = std::shared_ptr<uint8_t*>(new uint8_t[20], std::default_delete<uint8_t[]>() );
	// https://stackoverflow.com/questions/13061979/shared-ptr-to-an-array-should-it-be-used

	uint32_t pos = 0;
	bool isFin		= buffer[pos] & 0x80;
	WSOpcode opCode = WSOpcode(buffer[pos++] & 0x07);

	bool isMasked		= buffer[pos] & 0x80;
	uint8_t lengthShort = (uint8_t) (buffer[pos++] & 0x7F);
	uint32_t length     = 0;
	if (lengthShort <= 125) {
		length = lengthShort;
	} else {
		length = buffer[pos++];
		length += buffer[pos++] << 8 & 0xFF;
		if (lengthShort == 127) {
			length += buffer[pos++] << 16 & 0xFF;
			length += buffer[pos++] << 24 & 0xFF;
			// ignore the last 4 bytes of length
			pos += 4;
		}
	}

	
	uint8_t* mask = buffer + pos;
	if (isMasked) {
		pos += 4;
	}

	for (uint32_t i = pos, j = 0; i < bufferLength; i++, j = (j + 1) % 4) {
		this->buffer[i - pos] = buffer[i];
		if (isMasked) {
			this->buffer[i - pos] ^= mask[j];
		}
	}

	frame->buffer = this->buffer;
	frame->length = length;
	frame->opcode = opCode;

	return frame;
}


// --------------- WebsocketBase -------------------



WebSocketBase::WebSocketBase(shared_ptr<IOStream> stream)
	: stream(stream)
{
}

void WebSocketBase::Send(const char * buffer)
{
	this->Send((uint8_t*)buffer, strlen(buffer));
}

void WebSocketBase::Send(const uint8_t * buffer, uint32_t len)
{
	if (this->stream != nullptr) {
		Buffer out(len + MAX_HEADER_BYTES); // length + max overhead
		uint32_t wsBytes = this->protocol.WriteFrame(WSOpcode::TextFrame, buffer, len, out.buffer, out.len, false);
		stream->write(out.buffer, wsBytes);
	}
}

void WebSocketBase::Send(string & message)
{
	uint8_t* buffer = (uint8_t*) message.c_str();
	uint32_t len = message.size();

	this->Send(buffer, len);
}

void WebSocketBase::Close()
{
	Buffer out(MAX_HEADER_BYTES);
	uint32_t wsBytes = this->protocol.WriteFrame(WSOpcode::CloseConnection, NULL, 0, out.buffer, out.len, false);
	stream->write(out.buffer, wsBytes);
}


// --------------- WebsocketImpl -------------------

class WebSocketImpl : public WebSocketBase {
	IWebsocketApp* app;
	std::thread* wsThread;
	bool _alive;

public:

	WebSocketImpl(shared_ptr<IOStream> stream, IWebsocketApp* app)
		: WebSocketBase(stream), app(app), _alive(false), wsThread(NULL)
	{
	}

	~WebSocketImpl()
	{
		this->_alive = false;
		delete wsThread;
	}

	void setThread(std::thread* thread) {
		this->_alive = true;
		this->wsThread = thread;
	}

	bool alive() { return this->_alive; }

	void alive(bool status) { this->_alive = status; }

	WebsocketProtocol* protocolHandler() {
		return &this->protocol;
	}

	IOStream* getStream() {
		return this->stream.get();
	}

	void join() {
		if (this->wsThread != NULL && this->wsThread->joinable()) {
			this->wsThread->join();
		}
	}

	void Close()
	{
		this->_alive = false;
		WebSocketBase::Close();
	}
};


// -------------- WebsocketServerImpl -------------------


class WebsocketServerImpl : public WebsocketServer
{
	unordered_set<WebSocketImpl*> connections;
	WebsocketProtocol protocol;
	IWebsocketApp* app;

public:
	WebsocketServerImpl(IWebsocketApp* app)
		: app(app)
	{
	}

	~WebsocketServerImpl() {
		auto end = connections.end();

		// flag all threads to finish
		for (auto it = connections.begin(); it != end; it++) {
			(*it)->Close();
		}

		// wait for all threads to end and delete their objects
		for (auto it = connections.begin(); it != end; it++) {
			(*it)->join();
			delete *it;
		}
	}

	// Heredado v�a IUpgradeHandler
	virtual shared_ptr<HttpResponseMsg> AcceptUpgrade(HttpRequestMsg* req) override
	{
		auto clientKey = req->getHeader("sec-websocket-key");
		auto res = HttpServer::createResponse();
		res->setStatus(101);
		res->setHeader("Upgrade", "websocket");
		res->setHeader("Connection", "Upgrade");
		res->setHeader("Sec-WebSocket-Protocol", "recording");
		res->setHeader("Sec-WebSocket-Accept", this->protocol.CalculateSignature(clientKey->c_str()));

		return res;
	}

	// Heredado v�a IUpgradeHandler
	virtual void HandleStream(shared_ptr<IOStream> stream) override
	{
		WebSocketImpl* ws = new WebSocketImpl(stream, this->app);
		this->connections.insert(ws);
		
		auto thread = new std::thread(&WebsocketServerImpl::HandleLoop, this, ws);
		ws->setThread(thread);
	}

private:

	void HandleLoop(WebSocketImpl* conn) {
		app->onOpen(conn);

		while (conn->alive()) {
			Buffer in(2048); // TODO: allow reading of frames bigger than this
			uint32_t bytesRead = conn->getStream()->read(in.buffer, in.len);
			if (bytesRead == 0 && !conn->getStream()->isconnected())
			{
				conn->alive(false);
			}
			else
			{
				auto msg = conn->protocolHandler()->ReadFrame(in.buffer, in.len);

				if (msg->opcode == WSOpcode::TextFrame) {
					string text((char*)msg->buffer, msg->length);
					app->onMessage(conn, text);
				}
				else if (msg->opcode == WSOpcode::CloseConnection) {
					// send close response, rfc: https://tools.ietf.org/html/rfc6455#section-1.4
					conn->Close();
					app->onClose(conn);
				}
			}
		}
	}
};


// --------------- WebsocketServer -------------------

shared_ptr<WebsocketServer> WebsocketServer::CreateWebsocketserver(IWebsocketApp * app)
{
	shared_ptr<WebsocketServer> ptr(new WebsocketServerImpl(app));
	return ptr;
}