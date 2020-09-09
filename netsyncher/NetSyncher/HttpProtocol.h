#pragma once

#include <vector>
#include <map>

using namespace std;

#define BUFFER_LEN 2048

class IOStream {
public:
	/*
	Reads up to len bytes into buffer.
	Returns the number of read bytes
	*/
	virtual uint32_t read(uint8_t* buffer, uint32_t len) = 0;

	/**
	Attempts to write as much as len bytes to the stream.
	Returns the number of bytes actually writte.
	*/
	virtual uint32_t write(uint8_t* buffer, uint32_t len) = 0;
};



/**
* A request message as parsed by HttpProtocol
*/
class HttpRequestMsg
{
public:
	map<string, string> headers;
	string uri;
	string method;
};

/**
MemBuffer
*/
class MemBuffer
{
	uint8_t* buffer;
	uint32_t buffSize;
	uint32_t buffUsed;

	bool ensureEnoughSpace(uint32_t desiredLength);

public:
	MemBuffer();
	MemBuffer(MemBuffer&);
	~MemBuffer();

	// adds a string to the response (buffered)
	// void write(string str);
	
	// adds a zero terminated string to the response (buffered)
	// returns true if the operation succeded
	bool write(const char *str);

	// adds a byte array to the response (buffered)
	bool write(uint8_t* buffer, uint32_t len);

	uint32_t size();

	uint8_t* _getBuffer(uint32_t *outSize);
};

/*
Http response message
*/
class HttpResponseMsg
{
public:
	MemBuffer buffer;
	map<string, string> headers;
	int statusCode = 200;

	bool write(char *str);
	void setHeader(char* key, char* value);
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
	Pass the argument by reference
	https://stackoverflow.com/a/14548993
	*/
	HttpRequestMsg readRequest();

	/*
	Writes the response object
	*/
	void sendResponse(HttpResponseMsg &msg);

};

