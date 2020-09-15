#include "HttpProtocol.h"
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ctype.h>

// -------------- functions -------------------------


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
	int partLength = 0, consumed = 0, spliterLen = 1;
	std::vector<string> lines;
	std::string remainder(s);

	while (remainder.length()) {
		partLength = remainder.find(byChar);
		if (partLength == string::npos) {
			partLength = remainder.length();
			spliterLen = 0;
		}
		string part = remainder.substr(0, partLength);

		remainder = remainder.substr(partLength + spliterLen);
		consumed += partLength + spliterLen;

		if (part.length() > 0) {
			lines.push_back(part);
		}
	}
	return lines;
}


string readHeadersBlock(IOStream* iostream)
{
	const char CRLFx2[] = "\r\n\r\n";
	char buffer[BUFFER_LEN];
	string text;
	uint32_t read = 1;
	char* endOfHeaders = NULL;

	while (read != 0 && endOfHeaders == NULL) {
		read = iostream->peek((uint8_t*)buffer, BUFFER_LEN - 1);
		if (read) {
			// null terminate our buffer
			buffer[read] = '\0';

			uint32_t bytesToStringify = read;
			uint32_t blankChars = strspn(buffer, "\t\r\n ");

			endOfHeaders = strstr(buffer + blankChars, CRLFx2);
			if (endOfHeaders != NULL) {
				bytesToStringify = (endOfHeaders - buffer);
			}

			text = text.append(buffer, bytesToStringify);

			iostream->read((uint8_t*)buffer, bytesToStringify + strlen(CRLFx2));
		}
	}

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

// -------------- HttpRequestMsg -----------------------------

HttpRequestMsg::HttpRequestMsg(IOStream * stream)
	: stream(stream)
{
}

shared_ptr<MultipartStream> HttpRequestMsg::readFile(const char * name)
{
	shared_ptr<MultipartStream> stream(new MultipartStream(this->stream, "nada"));
	return stream;
}


// -------------- HttpResponseMsg -----------------------------


void HttpResponseMsg::setHeader(const char* key, const char* value)
{
	this->headers[key] = value;
}


// -------------- HttpProtocol -----------------------------


HttpProtocol::HttpProtocol(IOStream *iostream) :iostream(iostream) {
}


HttpRequestMsg HttpProtocol::readRequest()
{
	/*
	Pass the argument by reference
	https://stackoverflow.com/a/14548993

	Basic IO
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


	return msg;
}


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

	// content
	this->iostream->write("\r\n");
	this->iostream->write(contentBuffer, contentLength);
}



// ----------------------- MultipartStream -----------------------


MultipartStream::MultipartStream(IOStream* stream, string boundary)
	: stream(stream), consumed(0), boundary(boundary), length(-1), isValid(true)
{
}

uint32_t MultipartStream::peek(uint8_t * buffer, uint32_t len)
{
	throw string("Not implemented");
}

uint32_t MultipartStream::read(uint8_t * buffer, uint32_t len)
{
	if (!this->isValid) {
		return -1;
	}

	if (this->length == -1) {
		string mimeHeadersText = readHeadersBlock(this->stream);
		std::vector<string> lines = split(mimeHeadersText, '\n');
		
		parseHeadersIntoMap(lines, this->mimeHeaders);

		if (this->mimeHeaders.find("content-length") != this->mimeHeaders.end()) {
			try {
				int contentLength = stoi(this->mimeHeaders["content-length"]);
				this->length = contentLength;
			}
			catch (const std::invalid_argument& ia) {
				this->isValid = false;
				ia.what();
			}
		}
	}


	uint32_t available = this->length - this->consumed;
	uint32_t readable  = available < len ? available : len;
	if (readable == 0) {
		return 0;
	}

	uint32_t bytesRead = this->stream->read(buffer, readable);
	this->consumed += bytesRead;

	if (this->consumed == this->length) {
		// read the ending boundary from the stream
		uint8_t endLine[2048];
		this->stream->read(endLine, 2048);
	}

	return bytesRead;
}

uint32_t MultipartStream::write(uint8_t * buffer, uint32_t len)
{
	return uint32_t();
}
