#include "HttpProtocol.h"
#include <algorithm> 
#include <cctype>
#include <locale>
#include <ctype.h>


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
	uint32_t lineLength = 0, consumed = 0;
	std::vector<string> lines;
	std::string remainder(s);

	while (remainder.length()) {
		lineLength = remainder.find(byChar);
		if (lineLength == string::npos) {
			lineLength = remainder.length() - 1;
		}
		string part = remainder.substr(0, lineLength);

		remainder = remainder.substr(lineLength + 1);
		consumed += lineLength + 1;

		if (part.length() > 0) {
			lines.push_back(part);
		}
	}
	return lines;
}


HttpRequestMsg HttpProtocol::readRequest(InputStream *iStream)
{
	/*
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
	#define BUFFER_LEN 32

	HttpRequestMsg msg = HttpRequestMsg();

	char buffer[BUFFER_LEN];
	string text;
	uint32_t read = 1;

	while (read != 0) {
		read = iStream->read((uint8_t*)buffer, BUFFER_LEN);
		if (read) {
			text = text.append(buffer, read);
		}
	}

	std::vector<string> lines = split(text, '\n');
	std::vector<string> parts = split(lines[0], ' ');

	if (parts.size() >= 3) { // more than 3 is protocolo error, we don't care
		msg.method = parts[0];
		msg.uri = parts[1];
	}

	for (int pos = 1; pos < lines.size(); pos++) {
		vector<string> headerParts = split(lines[pos], ':');
		if (headerParts.size() == 2) {
			string key = headerParts[0];
			string val = headerParts[1];
			toLower(key);
			trim(key);
			trim(val);
			msg.headers.insert_or_assign(key, val);
		}
	}



	return msg;
}
