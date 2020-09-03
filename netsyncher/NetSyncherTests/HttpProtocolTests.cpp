#include "pch.h"
#include "CppUnitTest.h"

#include "../NetSyncher/HttpProtocol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define CR_LF "\r\n"

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
		void runTestAsserts(HttpRequestMsg req) {
			// assert
			map<string, string>::iterator it;

			it = req.headers.find("accept");
			Assert::IsFalse(it == req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("application/json"));

			it = req.headers.find("host");
			Assert::IsFalse(it == req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("www.w3.org"));
		}

	public:
		TEST_METHOD(Test_ReqStandard)
		{
			// prepare
			static const char* sampleGetReq =
				"GET /pub/WWW/TheProject.html HTTP/1.1" CR_LF
				"Host: www.w3.org"						CR_LF
				"Accept: application/json"				CR_LF
				CR_LF;
			
			// execute
			StrInputStream iss(sampleGetReq);
			HttpProtocol http = HttpProtocol();
			HttpRequestMsg req = http.readRequest(&iss);

			// assert
			this->runTestAsserts(req);
		}

		TEST_METHOD(Test_ReqWithInitialSapces)
		{

			// https://tools.ietf.org/html/rfc2616#section-4.1
			static const char* sampleGetToTestWellBehavedServers =
				CR_LF CR_LF
				"GET /pub/WWW/TheProject.html HTTP/1.1" CR_LF
				"Host: www.w3.org"						CR_LF
				"Accept: application/json"				CR_LF
				CR_LF
			;

			// execute
			StrInputStream iss(sampleGetToTestWellBehavedServers);
			HttpProtocol http = HttpProtocol();
			HttpRequestMsg req = http.readRequest(&iss);

			// assert
			this->runTestAsserts(req);
		}

		int generateResponse(char *buffer, int32_t bufferLength, const char *body) {
			static const char* resTemplate = "HTTP/1.1 200 ok" CR_LF
				"Content-Type: application/json" CR_LF
				"Content-Length: %i" CR_LF
				CR_LF
				"%s"
				;
			
			return sprintf_s(buffer, bufferLength, resTemplate, strlen(body), body);
		}

		TEST_METHOD(Test_ResDefault)
		{
			char buffer[2048];
			this->generateResponse(buffer, 2048, "{\"Hello\":\"world\"}");

			HttpResponseMsg res;
			res.write("{\"Hello\":\"world\"}");

			HttpProtocol http = HttpProtocol();
		}
	};
}
