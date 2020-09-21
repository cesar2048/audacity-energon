#include <vector>
#include <algorithm> 
#include <cctype>
#include <string>


// https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
// trim from Listen (in place)
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

static inline std::vector<std::string> split(std::string &s, char byChar) {
	int partLength = 0, consumed = 0, spliterLen = 1;
	std::vector<std::string> lines;
	std::string remainder(s);

	while (remainder.length()) {
		partLength = remainder.find(byChar);
		if (partLength == std::string::npos) {
			partLength = remainder.length();
			spliterLen = 0;
		}
		std::string part = remainder.substr(0, partLength);

		remainder = remainder.substr(partLength + spliterLen);
		consumed += partLength + spliterLen;

		if (part.length() > 0) {
			lines.push_back(part);
		}
	}
	return lines;
}
