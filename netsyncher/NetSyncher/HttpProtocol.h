#pragma once

#include <vector>
#include <map>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ctype.h>

#include "MemBuffer.h"

using namespace std;

// shared_ptr<string> getMapValue(const map<string, string>& headers, string key);
// shared_ptr<int> getHeaderAsInt(const map<string, string>& headers, string key);

/*
This stream allows reading of files from a multipart/form-data request
*/
class MultipartStream : public IOStream
{
	map<string, string> mimeHeaders;
	IOStream* stream;
	string boundary;
	int available;
	int length;

public:
	MultipartStream(IOStream* stream, string boundary, int contentLength);

	int getLength();

	// Heredado v�a IOStream
	virtual uint32_t peek(uint8_t * buffer, uint32_t len) override;
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override;
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override;
};

/**
* A request message as parsed by HttpProtocol
*/
class HttpRequestMsg
{
	IOStream* stream;
	string multipartBoundary;
	map<string, string> readMimePartHeaders();

public:
	HttpRequestMsg(IOStream *stream);

	map<string, string> headers;
	string uri;
	string method;

	void setMultipartBoundary(string boundary);
	shared_ptr<string> getHeader(string key);

	shared_ptr<MultipartStream> readFile(const char* name);
};


/*
Http response message
*/
class HttpResponseMsg : public MemBuffer
{
public:
	map<string, string> headers;
	int statusCode = 200;

	void setHeader(const char* key, const char* value);
};

/**
The main class responsible for interacting with application code
This implementation does not support transfer encoding chunked,
requiring to have the entire response in memory before sending it back to the client.
*/
class HttpProtocol
{
	IOStream *iostream;

public:

	HttpProtocol(IOStream *iStream);

	/*
	Reads an object representing the request
	*/
	HttpRequestMsg readRequest();

	/*
	Writes the response object
	*/
	void sendResponse(HttpResponseMsg &msg);

};



