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

	/// returns true if the stream is able to read/write
	virtual bool isconnected() =0;
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

	// Heredado vía IOStream
	virtual uint32_t peek(uint8_t * buffer, uint32_t len) override;
	virtual uint32_t read(uint8_t * buffer, uint32_t len) override;
	virtual uint32_t write(uint8_t * buffer, uint32_t len) override;
	virtual void close() override;
	virtual bool isconnected() override;
};


/// A request message as parsed by HttpProtocol
class HttpRequestMsg
{
public:
	/// returns a header, or nullptr if the header is not found
	virtual shared_ptr<string> getHeader(string key) =0;

	/// returns a MultipartStream, or null if there is no files to read
	virtual shared_ptr<MultipartStream> readFile(const char* name) =0;
};

typedef shared_ptr<HttpRequestMsg> HttpReqPtr;


/// Http response message
class HttpResponseMsg : public MemBuffer
{
public:

	/// sets an output header
	virtual void setHeader(const char* key, const char* value) =0;

	/// returns a copy of the headers
	virtual map<string, string> getHeaders() =0;

	virtual void setStatus(int status) = 0;
	virtual int getStatus() = 0;
};

typedef shared_ptr<HttpResponseMsg> HttpResPtr;


/// This is the main class that implements the http protocol
/// This implementation does not support transfer encoding chunked,
/// requiring to have the entire response in memory before sending it back to the client.
class HttpProtocol
{
	IOStream *iostream;

public:
	/// constructor
	HttpProtocol(IOStream *iostream);

	/// Reads an object representing the request
	HttpReqPtr readRequest();

	/// Writes the response object
	void sendResponse(HttpResponseMsg* msg);
};


class HttpServer
{
public:
	/// An upgrade handler interface
	class IUpgradeHandler
	{
		public:
			virtual shared_ptr<HttpResponseMsg> AcceptUpgrade(HttpRequestMsg* req) = 0;
			virtual void HandleStream(shared_ptr<IOStream> stream) = 0;
	};

	/// Network interface
	class INetwork {
		public:
			virtual shared_ptr<IOStream> Accept() = 0;
			virtual void Listen(int port) = 0;
			virtual void Close() = 0;
	};

	/// Route handler interface
	class IRouteHandler {
		public:
			virtual shared_ptr<HttpResponseMsg> OnRequest(HttpRequestMsg* req) = 0;
	};

	~HttpServer();
	void SetHandlers(INetwork* network);
	void SetRouteHandler(IRouteHandler* handler);
	void Listen(int port);
	void RegisterUpdateHandler(const string& key, IUpgradeHandler* handler);
	IUpgradeHandler* getUpdateHandler(shared_ptr<string> key);

	static shared_ptr<HttpResponseMsg> createResponse();

private:
	void HandleLoop();

	INetwork* network;
	IRouteHandler* handler;

	map<string, IUpgradeHandler*> upgradeHandlers;
	std::thread* serverThread;
	bool isServerThreadAlive;
};


// --------------------------------------------
//                  functions
// --------------------------------------------


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
