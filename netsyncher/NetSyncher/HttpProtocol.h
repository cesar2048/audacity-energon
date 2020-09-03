#pragma once

#include <vector>
#include <map>

using namespace std;

class InputStream {
public:
	/*
	Reads up to len bytes into buffer. Returns the number of read bytes
	*/
	virtual uint32_t read(uint8_t* buffer, uint32_t len) = 0;
};

class HttpRequestMsg
{
public:
	map<string, string> headers;
	string uri;
	string method;
};

class HttpResponseMsg
{

};

class HttpProtocol
{
public:
	/*
	Pass the argument by reference
	https://stackoverflow.com/a/14548993
	*/
	HttpRequestMsg readRequest(InputStream *iStream);
};

