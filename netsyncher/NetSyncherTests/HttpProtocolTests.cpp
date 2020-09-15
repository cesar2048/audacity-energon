#include "pch.h"
#include "CppUnitTest.h"
#include <windows.h>

#include "../NetSyncher/HttpProtocol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define CR_LF "\r\n"

namespace NetSyncherTests
{

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
			HttpProtocol http = HttpProtocol(&iss);
			HttpRequestMsg req = http.readRequest();

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
			HttpProtocol http = HttpProtocol(&iss);
			HttpRequestMsg req = http.readRequest();

			// assert
			this->runTestAsserts(req);
		}

		int generateResponse(char *buffer, int32_t bufferLength, const char *body) {
			static const char* resTemplate = "HTTP/1.1 200 OK" CR_LF
				"Content-Length: %i" CR_LF
				"Content-Type: application/json" CR_LF
				CR_LF
				"%s"
				;
			
			uint32_t length = sprintf_s(buffer, bufferLength, resTemplate, strlen(body), body);
			return length;
		}

		TEST_METHOD(Test_ResDefault)
		{
			char buffer[2048];
			int expectedLen = this->generateResponse(buffer, 2048, "{\"Hello\":\"world\"}");
			StrInputStream iss("");

			// execute
			HttpProtocol http = HttpProtocol(&iss);
			HttpResponseMsg res;
			res.setHeader("Content-Type", "application/json");
			res.write("{\"Hello\":\"world\"}");
			http.sendResponse(res);

			uint32_t outLen;
			uint8_t* outBuffer = iss.outBuffer._getBuffer(&outLen);

			compareBuffers((uint8_t*)buffer, expectedLen, outBuffer, outLen);
		}

		TEST_METHOD(Test_Multipart)
		{
			TransmissionReader tr("sample-multipart-request.bin");

			HttpProtocol http = HttpProtocol(&tr);
			HttpRequestMsg req = http.readRequest();

			int position = tr.position();
			const int headerBytes = 230;

			Assert::AreEqual(headerBytes, position);

			shared_ptr<MultipartStream> file = req.readFile("theFile");
			Assert::IsTrue(file != nullptr);

			int read = -1;
			uint8_t buffer[4096];
			FILE* f = fopen("video.mp4", "wb");
			do {
				read = file->read(buffer, 2048);
				fwrite(buffer, sizeof(uint8_t), read, f);
			} while (read != 0);
			fclose(f);
		}
	};
}
