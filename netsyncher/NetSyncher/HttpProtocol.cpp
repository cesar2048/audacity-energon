#include "HttpProtocol.h"

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

	string text((char*) bytes->getBuffer(), bytes->size());
	trim(text);
	return text;
}

/// Takes a list of lines representing http headers
/// and puts them as key/value pairs in the headers map
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

/// gets a header from a key/value map
/// returns nullptr if the header is not found
/// unlike [] operator, does not create an empty value if there is none
shared_ptr<string> getMapValue(const map<string, string>& headers, string key)
{
	auto it = headers.find(key);
	if (it != headers.end()) {
		return shared_ptr<string>(new string(it->second));
	}
	return nullptr;
}

/// gets a header from a key/value map as integer
/// returns nullptr if the header is not found
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

/// returns a multivalue header value as a key/value dictionary
/// example, the following header
///   Content-Type: multipart/form-data; boundary=49cda107-3fbb-4edf-a726-7da8756f5a4b
/// is converted to
///   "value" => "multipart/form-data"
///   "boundary" => "49cda107-3fbb-4edf-a726-7da8756f5a4b"
map<string, string> getAttributeMap(string stringValue) {
	map<string, string> attributes;

	auto parts = split(stringValue, ';');
	for (int i = 0; i < parts.size(); i++) {
		string part = parts[i];
		trim(part);

		std::vector<string> keyValuePair = split(part, '=');
		if (keyValuePair.size() == 1) {
			attributes["value"] = part;
		} else if (keyValuePair.size() == 2) {
			string key = keyValuePair[0];
			string val = keyValuePair[1];

			attributes[key] = val;
		}
	}

	return attributes;
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


// -------------- HttpRequestMsg -----------------------------

HttpRequestMsg::HttpRequestMsg(IOStream * stream)
	: stream(stream)
{
}

void HttpRequestMsg::setMultipartBoundary(string boundary)
{
	this->multipartBoundary = boundary;
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
	auto mimeHeaders = this->readMimePartHeaders();
	auto contentLength = getHeaderAsInt(mimeHeaders, "content-length");

	if (this->method == "POST" && this->multipartBoundary.size() > 0) {
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
