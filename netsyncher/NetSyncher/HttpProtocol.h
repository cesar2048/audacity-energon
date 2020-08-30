#pragma once

#include <vector>
#include <map>

using namespace std;

class HttpRequestMsg
{
public:
	map<string, string> headers;
	string host;
	string uri;
};

class HttpProtocol
{
public:
	/*
	https://stackoverflow.com/questions/14544043/operand-types-are-incompatible-char-and-const-char
	*/
	HttpRequestMsg readRequest(const char* request);
};

