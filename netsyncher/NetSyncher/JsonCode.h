#pragma once
#include <vector>
#include <string>

using namespace std;

class JsonObject {
public:
	virtual string toJsonString() = 0;
	virtual const char* toCString() = 0;
};

class JsonConnectInfo : public JsonObject {
	vector<string> addrList;

public:
	JsonConnectInfo(vector<string> list);

	// Heredado vía JsonObject
	virtual string toJsonString() override;
	virtual const char* toCString() override;
};
