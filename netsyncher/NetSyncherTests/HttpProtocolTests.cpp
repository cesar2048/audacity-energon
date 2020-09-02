#include "pch.h"
#include "CppUnitTest.h"

#include "../NetSyncher/HttpProtocol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// "HTTP/1.1 200 ok\r\nContent-Type: application/json\r\n\r\n{\"Hello\":\"world\"}"
#define CR_LF "\r\n"


// https://tools.ietf.org/html/rfc2616#section-4.1
static const char* sampleGetToTestWellBehavedServers =
CR_LF CR_LF
"GET /pub/WWW/TheProject.html HTTP/1.1" CR_LF
"Host: www.w3.org"						CR_LF
"Accept: application/json"				CR_LF
CR_LF
;

namespace NetSyncherTests
{
	class StrInputStream : public InputStream {
	private:
		char* str;
		int pos;
	public:
		StrInputStream(const char *str) {
			size_t bufferSize = strlen(str) + 1;

			this->pos = 0;
			this->str = (char*)malloc(bufferSize);

			strcpy_s(this->str, bufferSize, str);
		}
		StrInputStream() {
			if (this->str) free(this->str);
		}
		// Heredado vía InputStream
		virtual uint32_t read(uint8_t * buffer, uint32_t len) override
		{
			char* str = this->str + this->pos;

			uint32_t strLen = strlen(str);
			uint32_t min = (len > strLen) ? strLen : len;

			if (min > 0) {
				memcpy(buffer, str, min);
				this->pos += min;
			}
			
			return min;
		}
	};

	TEST_CLASS(HttpProtocolTests)
	{
	public:
		TEST_METHOD(TestServerKey)
		{
			// prepare
			static const char* sampleGetReq =
				"GET /pub/WWW/TheProject.html HTTP/1.1" CR_LF
				"Host: www.w3.org"						CR_LF
				"Accept: application/json"				CR_LF
				CR_LF;
			HttpProtocol http = HttpProtocol();

			// execute
			StrInputStream iss(sampleGetReq);
			HttpRequestMsg req = http.readRequest(&iss);
			
			// assert
			map<string, string>::iterator it;

			it = req.headers.find("accept");
			Assert::IsFalse(it == req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("application/json"));
			
			it = req.headers.find("host");
			Assert::IsFalse(it == req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("www.w3.org"));
		}
	};
}
