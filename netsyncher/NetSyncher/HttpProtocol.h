#pragma once

#include <vector>
#include <map>
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ctype.h>
#include <thread>
#include <chrono>

#include "MemBuffer.h"

using namespace std;


// --------------------------------------------
//                  classes
// --------------------------------------------


class IOStream {
public:
	/// Peek into the stream without advancing the buffer and deleting the information
	virtual uint32_t peek(uint8_t* buffer, uint32_t len) = 0;

	/// Reads up to len bytes into buffer.
	/// Returns the number of read bytes
	virtual uint32_t read(uint8_t* buffer, uint32_t len) = 0;

	/// Attempts to write as much as len bytes to the stream.
	/// Returns the number of bytes actually written
	virtual uint32_t write(uint8_t* buffer, uint32_t len) = 0;

	/// Attempts to write string to the stream
	/// returns the number of bytes actually written
	virtual uint32_t write(const char* string);

	/// closes the stream
	virtual void close() = 0;
};


/// This stream allows reading of files from a multipart/form-data request
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
	virtual void close() override;
};


/// A request message as parsed by HttpProtocol
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


/// Http response message
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

class HttpServer
{
public:
	/// An upgrade handler interface
	class IUpgradeHandler
	{
		public:
			virtual shared_ptr<HttpResponseMsg> AcceptUpgrade(HttpRequestMsg* req) = 0;
			virtual void Upgrade(IOStream* stream) = 0;
	};

	/// Network interface
	class INetwork {
		public:
			virtual shared_ptr<IOStream> Accept() = 0;
			virtual void Listen(int port) = 0;
	};

	/// Route handler interface
	class IRouteHandler {
		public:
			virtual shared_ptr<HttpResponseMsg> onRequest(HttpRequestMsg* req) = 0;
	};

	HttpServer(INetwork* network, IRouteHandler* handler);
	~HttpServer();

	void Listen(int port);
	void registerUpdateHandler(const string& key, IUpgradeHandler* handler);
	IUpgradeHandler* getUpdateHandler(shared_ptr<string> key);

private:
	void AcceptLoop();

	INetwork* network;
	IRouteHandler* handler;

	map<string, IUpgradeHandler*> upgradeHandlers;
	std::thread* serverThread;
	bool isServerThreadAlive;
};


// --------------------------------------------
//                  functions
// --------------------------------------------
// shared_ptr<string> getMapValue(const map<string, string>& headers, string key);
// shared_ptr<int> getHeaderAsInt(const map<string, string>& headers, string key);


/// Log timestamped messages to VisualStudio debug log
void DebugLog(const char *format...);

/// consumes from the stream until the position returned by the lambda function,
/// or until there is no more bytes available
std::shared_ptr<MemBuffer> readUntilPosition(IOStream* iostream, const std::function<int(Buffer)>& getEndPos);

/// reads from the stream until the endSequence is found, or until there is no more in the stream
std::shared_ptr<MemBuffer> readUntil(IOStream* iostream, uint8_t* endSequence, uint32_t endLength);

/// reads a block of headers out of the stream
string readHeadersBlock(IOStream* iostream);

/// Takes a list of lines representing http headers
/// and puts them as key/value pairs in the headers map
void parseHeadersIntoMap(vector<string>& lines, map<string, string>& headers);


/// gets a header from a key/value map
/// returns nullptr if the header is not found
/// unlike [] operator, does not create an empty value if there is none
shared_ptr<string> getMapValue(const map<string, string>& headers, string key);

/// returns a multivalue header value as a key/value dictionary
/// example, the following header
///   Content-Type: multipart/form-data; boundary=49cda107-3fbb-4edf-a726-7da8756f5a4b
/// is converted to
///   "value" => "multipart/form-data"
///   "boundary" => "49cda107-3fbb-4edf-a726-7da8756f5a4b"
map<string, string> getAttributeMap(string stringValue);

/// gets a header from a key/value map as integer
/// returns nullptr if the header is not found
shared_ptr<int> getHeaderAsInt(const map<string, string> headers, string key);
