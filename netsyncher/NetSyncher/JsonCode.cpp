#include "JsonCode.h"
#include <sstream>
#include <thread>


using namespace std;

// ------------ functions ---------------------

vector<string> filterList(vector<string> list, const std::function<bool(string)>& filterFn) {
	vector<string> result;
	for (auto& v : list)
		if (filterFn(v))
			result.push_back(v);

	return result;
}

string vectorToJson(vector<string> list) {
	ostringstream ss;
	ss << "[";

	auto end = list.end();
	bool isFirst = true;
	for (auto it = list.begin(); it != end; it++) {
		if (!isFirst) {
			ss << ",";
		}
		ss << "\"" << (*it) << "\"";
		isFirst = false;
	}

	ss << "]";
	return ss.str();
}

// ------------ JsonConnectInfo ------------

JsonConnectInfo::JsonConnectInfo(vector<string> ipAddrList) {
	this->addrList = filterList(ipAddrList, [=](string item) {
		// ignoring localhost and link-local (no dhcp) addresses
		return (bool)(item.find("127.") != 0 && item.find("169") != 0);
	});
}

string JsonConnectInfo::toJsonString() {
	string addresses = vectorToJson(this->addrList);
	return "{\"version\": \"1.0\", \"addresses\":" + addresses + "}";
}

const char* JsonConnectInfo::toCString()
{
	return this->toJsonString().c_str();
}
