#include "HttpProtocol.h"
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ctype.h>


// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

static inline void toLower(std::string &data) {
	std::transform(data.begin(), data.end(), data.begin(),
		[](unsigned char c) { return std::tolower(c); });
}

static inline std::vector<string> split(std::string &s, char byChar) {
	uint32_t lineLength = 0, consumed = 0;
	std::vector<string> lines;
	std::string remainder(s);

	while (remainder.length()) {
		lineLength = remainder.find(byChar);
		if (lineLength == string::npos) {
			lineLength = remainder.length() - 1;
		}
		string part = remainder.substr(0, lineLength);

		remainder = remainder.substr(lineLength + 1);
		consumed += lineLength + 1;

		if (part.length() > 0) {
			lines.push_back(part);
		}
	}
	return lines;
}


// -------------- IOStream -------------------------


uint32_t IOStream::write(const char * string)
{
	uint32_t len = strlen(string);
	uint32_t res = this->write((uint8_t*)string, len);
	return res;
}


// -------------- HttpProtocol -----------------------------


HttpProtocol::HttpProtocol(IOStream *iostream) :iostream(iostream) {
}

string HttpProtocol::getHeadersFromStream()
{
	const char CRLFx2[] = "\r\n\r\n";
	char buffer[BUFFER_LEN];
	string text;
	uint32_t read = 1, bytesToStringify = 1;

	while (read != 0 && bytesToStringify == read) {
		read = this->iostream->peek((uint8_t*)buffer, BUFFER_LEN);
		if (read) {
			bytesToStringify = read;
			char* endOfHeaders = strstr(buffer, CRLFx2);
			if (endOfHeaders != NULL) {
				bytesToStringify = (endOfHeaders - buffer) + strlen(CRLFx2);
			}

			text = text.append(buffer, bytesToStringify);

			this->iostream->read((uint8_t*)buffer, bytesToStringify);
		}
	}

	return text;
}

HttpRequestMsg HttpProtocol::readRequest()
{
	/*
	https://stackoverflow.com/questions/17290144/stdio-vs-iostream
	http://www.cplusplus.com/doc/tutorial/basic_io/
	 
	output stream in c++
	https://www.cprogramming.com/tutorial/c++-iostreams.html
	https://stackoverflow.com/questions/13703823/a-custom-ostream
	
	input streams in c++
	https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
	http://www.cplusplus.com/reference/sstream/stringbuf/
	https://stackoverflow.com/questions/772355/how-to-inherit-from-stdostream
	*/

	HttpRequestMsg msg = HttpRequestMsg();
	string text = this->getHeadersFromStream();
	
	std::vector<string> lines = split(text, '\n');
	std::vector<string> parts = split(lines[0], ' ');

	// more than 3 parts is protocolo error, we don't care
	if (parts.size() >= 3) {
		msg.method = parts[0];
		msg.uri = parts[1];
	}

	for (int pos = 1; pos < lines.size(); pos++) {
		vector<string> headerParts = split(lines[pos], ':');
		if (headerParts.size() == 2) {
			string key = headerParts[0];
			string val = headerParts[1];
			toLower(key);
			trim(key);
			trim(val);
			msg.headers.insert_or_assign(key, val);
		}
	}

	return msg;
}


// -------------- HttpProtocol -----------------------------


void HttpProtocol::sendResponse(HttpResponseMsg& msg)
{
	char temporaryLine[BUFFER_LEN];
	uint32_t contentLength;
	uint8_t* contentBuffer = msg._getBuffer(&contentLength);

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

	this->iostream->write("\r\n");
	this->iostream->write(contentBuffer, contentLength);
}


// -------------- MemBuffer -----------------------------


MemBuffer::MemBuffer()
{
	this->buffer = (uint8_t*)malloc(BUFFER_LEN);
	this->buffer[0] = 0;
	this->buffSize = BUFFER_LEN;
}

MemBuffer::MemBuffer(MemBuffer &original)
{
	this->buffSize = original.buffSize;
	this->buffer = (uint8_t*)malloc(this->buffSize);

	memcpy(this->buffer, original.buffer, original.buffSize);
}

MemBuffer::~MemBuffer()
{
	free(this->buffer);
}

bool MemBuffer::ensureEnoughSpace(uint32_t desiredSize)
{
	if (this->buffSize < desiredSize) {
		void* ptr = realloc(this->buffer, this->buffSize * 2);
		if (!ptr) {
			return false;
		}
		this->buffer = (uint8_t*)ptr;
		this->buffSize *= 2;
	}
	return true;
}

bool MemBuffer::write(const char * str)
{
	int len = strlen(str);
	return this->write((uint8_t*) str, len);
}

bool MemBuffer::write(uint8_t * buffer, uint32_t len)
{
	uint32_t spaceNeeded = this->buffUsed + len;
	bool hasEnoughSpace = this->ensureEnoughSpace(spaceNeeded);
	if (!hasEnoughSpace) {
		return false;
	}

	uint32_t available = this->buffSize - this->buffUsed;
	uint8_t* ptrStart = this->buffer + this->buffUsed;

	memcpy_s(ptrStart, available, buffer, len);
	this->buffUsed += len;
}

uint32_t MemBuffer::size()
{
	return this->buffUsed;
}

uint8_t * MemBuffer::_getBuffer(uint32_t * outSize)
{
	*outSize = this->buffUsed;
	return this->buffer;
}

void HttpResponseMsg::setHeader(const char* key, const char* value)
{
	this->headers[key] = value;
}
