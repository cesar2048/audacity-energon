#include "HttpProtocol.h"
#include <stdarg.h>
#include <chrono>

#include <windows.h>

#include "stringextension.h"

// ------------- IHttpServer ---------------------


HttpServer::~HttpServer() {
	if (this->serverThread) {
		this->isServerThreadAlive = true;
		this->serverThread->join();
		delete this->serverThread;
	}
}

void HttpServer::SetHandlers(INetwork * network) {
	this->network = network;
}

void HttpServer::SetRouteHandler(IRouteHandler * handler) {
	this->handler = handler;
}

void HttpServer::Listen(int port) {
	// how to create a new thread
	// https://stackoverflow.com/a/10673671

	if (!this->isServerThreadAlive) {
		this->isServerThreadAlive = true;
		// TODO: handle unable to bind error
		this->network->Listen(port);
		this->serverThread = new std::thread(&HttpServer::HandleLoop, this/*, additional args */);
	}
}


void HttpServer::HandleLoop(/* additional args */) {
	long elapsed = 0;

	while (this->isServerThreadAlive) {
		auto stream = this->network->Accept();

		DebugLog("Connection accepted\n");
		std::chrono::system_clock::time_point ini = std::chrono::system_clock::now();

		HttpProtocol http(stream.get());
		HttpRequestMsg req = http.readRequest();

		// check for upgrade
		auto upgrade = req.getHeader("upgrade");
		auto handler = this->getUpdateHandler(upgrade);
		if (handler != nullptr) {
			auto res = handler->AcceptUpgrade(&req);
			http.sendResponse(*res);

			// delegate connection to the handler
			handler->HandleStream(stream);
		} else {
			auto res = this->handler->OnRequest(&req);
			http.sendResponse(*res);
			stream->close();

			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - ini).count();
		}
	}
}


void HttpServer::RegisterUpdateHandler(const string& key, IUpgradeHandler* handler)
{
	if (handler != nullptr) {
		this->upgradeHandlers[key] = handler;
	}
}

HttpServer::IUpgradeHandler* HttpServer::getUpdateHandler(shared_ptr<string> key)
{
	if (key != nullptr) {
		auto handler = this->upgradeHandlers.find(*key);
		if (handler != this->upgradeHandlers.end()) {
			return handler->second;
		}
	}

	return nullptr;
}



// -------------- HttpProtocol -----------------------------


HttpProtocol::HttpProtocol(IOStream *iostream) :iostream(iostream) {
}


HttpRequestMsg HttpProtocol::readRequest()
{
	string text = readHeadersBlock(this->iostream);

	std::vector<string> lines = split(text, '\n');
	std::vector<string> parts = split(lines[0], ' ');

	// more than 3 parts is protocolo error, we don't care
	HttpRequestMsg msg = HttpRequestMsg(this->iostream);
	if (parts.size() >= 3) {
		msg.method = parts[0];
		msg.uri = parts[1];
	}

	parseHeadersIntoMap(lines, msg.headers);

	// check for multipart
	auto contentType = getMapValue(msg.headers, "content-type");
	if (contentType != nullptr) {
		auto attributes = getAttributeMap(*contentType);
		auto value = getMapValue(attributes, "value");

		if (value && *value == "multipart/form-data") {
			auto boundary = getMapValue(attributes, "boundary");
			if (boundary) {
				msg.setMultipartBoundary(*boundary);
			}
		}
	}
	
	return msg;
}

void HttpProtocol::sendResponse(HttpResponseMsg& msg)
{
	char temporaryLine[BUFFER_LEN];
	uint32_t contentLength = msg.size();
	uint8_t* contentBuffer = msg.getBuffer();

	// preparation
	sprintf_s(temporaryLine, BUFFER_LEN, "%d", contentLength);
	msg.headers["Content-Length"] = string(temporaryLine);

	// status line
	sprintf_s(temporaryLine, BUFFER_LEN, "HTTP/1.1 %d %s\r\n", msg.statusCode, "OK");
	this->iostream->write(temporaryLine);

	// headers
	map<string,string>::iterator it;
	for (it = msg.headers.begin(); it != msg.headers.end(); it++) {
		const char* key = (*it).first.c_str();
		const char* val = (*it).second.c_str();
		sprintf_s(temporaryLine, BUFFER_LEN, "%s: %s\r\n", key, val);
		this->iostream->write((uint8_t*)temporaryLine, strlen(temporaryLine));
	}

	// content
	this->iostream->write("\r\n");
	this->iostream->write(contentBuffer, contentLength);
}


// -------------- HttpResponseMsg -----------------------------


void HttpResponseMsg::setHeader(const char* key, const char* value)
{
	this->headers[key] = value;
}


// -------------- HttpRequestMsg -----------------------------


HttpRequestMsg::HttpRequestMsg(IOStream * stream)
	: stream(stream)
{
}

void HttpRequestMsg::setMultipartBoundary(string boundary)
{
	this->multipartBoundary = boundary;
}

shared_ptr<string> HttpRequestMsg::getHeader(string key)
{
	return getMapValue(this->headers, key);
}

map<string, string> HttpRequestMsg::readMimePartHeaders()
{
	string mimeHeadersText = readHeadersBlock(this->stream);
	std::vector<string> lines = split(mimeHeadersText, '\n');
	map<string, string> mimeHeaders;
	parseHeadersIntoMap(lines, mimeHeaders);
	return mimeHeaders;
}

shared_ptr<MultipartStream> HttpRequestMsg::readFile(const char * name)
{
	bool isFile = false;

	if (this->method == "POST" && this->multipartBoundary.size() > 0) {
		auto mimeHeaders = this->readMimePartHeaders();
		auto contentLength = getHeaderAsInt(mimeHeaders, "content-length");

		if (contentLength == nullptr) {
			contentLength = shared_ptr<int>(new int(-1));
		}

		MultipartStream* ptr = new MultipartStream(this->stream, this->multipartBoundary, *contentLength);
		return shared_ptr<MultipartStream>(ptr);
	}

	return nullptr;
}


// ----------------------- MultipartStream -----------------------


MultipartStream::MultipartStream(IOStream* stream, string boundary, int contentLength)
	: stream(stream), available(-1), boundary(boundary), length(contentLength)
{
}

int MultipartStream::getLength()
{
	return this->length;
}

uint32_t MultipartStream::peek(uint8_t * buffer, uint32_t len)
{
	throw string("Not implemented");
}

uint32_t MultipartStream::read(uint8_t * buffer, uint32_t len)
{
	if (this->available == 0) {
		return 0;
	}

	string endBoundary = "\r\n--" + this->boundary + "--\r\n";
	Buffer endBuffer = endBoundary.c_str();
	
	uint32_t bytesRead = this->stream->peek(buffer, len);
	uint32_t bytesToConsume = bytesRead;

	int boundaryPos = findSequenceInBuffer(Buffer(len, buffer), endBuffer);
	if (boundaryPos != -1) {
		bytesToConsume = boundaryPos + endBuffer.len;
		bytesRead = boundaryPos;
		this->available = 0;
	}

	this->stream->read(buffer, bytesToConsume);
	return bytesRead;
}

uint32_t MultipartStream::write(uint8_t * buffer, uint32_t len)
{
	return uint32_t();
}

void MultipartStream::close()
{
	// nothing to do here
}


// -------------- IOStream -------------------------


uint32_t IOStream::write(const char * string)
{
	uint32_t len = strlen(string);
	uint32_t res = this->write((uint8_t*)string, len);
	return res;
}


// -------------- functions -------------------------


std::chrono::system_clock::time_point last;
void DebugLog(const char *format ...)
{
	// how to process variable arguments
	// https://stackoverflow.com/questions/5977326/call-printf-using-va-list
	char buffer[2048];
	char formatBuffer[2048];
	va_list arg;

	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
	last = now;
	if (elapsed < 0) {
		elapsed = 0;
	}

	sprintf(formatBuffer, "[%5d], %s", elapsed, format);

	va_start(arg, format);
	vsprintf(buffer, formatBuffer, arg);
	va_end(arg);

	OutputDebugStringA(buffer);
}


std::shared_ptr<MemBuffer> readUntilPosition(IOStream* iostream, const std::function<int(Buffer)>& getEndPos)
{
	std::shared_ptr<MemBuffer> outBuffer(new MemBuffer());
	uint8_t buffer[BUFFER_LEN];
	uint32_t readBytes = 1;
	int endFoundHere = -1;

	while (readBytes != 0 && endFoundHere == -1) {
		readBytes = iostream->peek(buffer, BUFFER_LEN);

		endFoundHere = getEndPos({ BUFFER_LEN, buffer });
		if (endFoundHere != -1) {
			readBytes = endFoundHere;
		}

		iostream->read(buffer, readBytes); // advance stream
		outBuffer->write(buffer, readBytes); // save to memory
	}

	return outBuffer;
}

// reads from the stream until the end is found
// the end string is part of the returned buffer
std::shared_ptr<MemBuffer> readUntil(IOStream* iostream, uint8_t* endSequence, uint32_t endLength)
{
	return readUntilPosition(iostream, [=](Buffer b) {
		int found = findSequenceInBuffer(b.buffer, b.len, endSequence, endLength);
		return (found == -1) ? -1 : found + b.len;
	});
}


string readHeadersBlock(IOStream* iostream)
{
	Buffer endSeq = Buffer::fromString("\r\n\r\n");
	Buffer spaces = Buffer::fromString("\r\n\t ");
	bool skipSpaces = true;

	shared_ptr<MemBuffer> bytes = readUntilPosition(iostream, [&](Buffer b) {
		int skip = 0;
		if (skipSpaces) {
			skip = findByteSpan(b, spaces);
			if (skip < b.len) {
				skipSpaces = false;
			}
		}

		int found = findSequenceInBuffer(b + skip, endSeq);
		return (found == -1) ? -1 : skip + found + endSeq.len;
	});

	string text((char*)bytes->getBuffer(), bytes->size());
	trim(text);
	return text;
}


void parseHeadersIntoMap(vector<string>& lines, map<string, string>& headers) {

	for (int pos = 1; pos < lines.size(); pos++) {
		vector<string> headerParts = split(lines[pos], ':');
		if (headerParts.size() == 2) {
			string key = headerParts[0];
			string val = headerParts[1];
			toLower(key);
			trim(key);
			trim(val);

			headers.insert_or_assign(key, val);
		}
	}
}

shared_ptr<string> getMapValue(const map<string, string>& headers, string key)
{
	auto it = headers.find(key);
	if (it != headers.end()) {
		return shared_ptr<string>(new string(it->second));
	}
	return nullptr;
}

shared_ptr<int> getHeaderAsInt(const map<string, string> headers, string key)
{
	auto value = getMapValue(headers, key);
	if (value != nullptr) {
		try {
			shared_ptr<int> result(new int);
			*result = stoi(*value);
			return result;
		}
		catch (const std::invalid_argument& ia) {
			ia.what();
		}
	}
	return nullptr;
}

map<string, string> getAttributeMap(string stringValue) {
	map<string, string> attributes;

	auto parts = split(stringValue, ';');
	for (int i = 0; i < parts.size(); i++) {
		string part = parts[i];
		trim(part);

		std::vector<string> keyValuePair = split(part, '=');
		if (keyValuePair.size() == 1) {
			attributes["value"] = part;
		}
		else if (keyValuePair.size() == 2) {
			string key = keyValuePair[0];
			string val = keyValuePair[1];

			attributes[key] = val;
		}
	}

	return attributes;
}
