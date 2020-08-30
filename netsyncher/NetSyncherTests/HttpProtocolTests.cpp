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
			HttpRequestMsg req = http.readRequest(sampleGetReq);
			
			// assert
			map<string, string>::iterator it = req.headers.find("accept");
			Assert::AreNotEqual(it, req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("application/json"));

			map<string, string>::iterator it = req.headers.find("host");
			Assert::AreNotEqual(it, req.headers.end(), L"\"Accept\" header not found");
			Assert::AreEqual(it->second, string("www.w3.org"));
		}
	};
}
