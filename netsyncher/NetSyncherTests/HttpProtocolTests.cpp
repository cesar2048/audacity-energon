#include "pch.h"
#include "CppUnitTest.h"

#include "../NetSyncher/HttpProtocol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define CR_LF "\r\n"

namespace NetSyncherTests
{
	/* Auxiliary class to test to provide an in-memory IOStream */
	class StrInputStream : public IOStream {
	public:
		char* inputBuffer;
		int inCursor;
		char *outputBuffer;
		int outCursor;

		StrInputStream(const char *str) {
			size_t bufferSize = strlen(str) + 1;

			this->inCursor = 0;
			this->outCursor = 0;
			this->inputBuffer = (char*)malloc(bufferSize);

			strcpy_s(this->inputBuffer, bufferSize, str);
		}

		~StrInputStream() {
			if (this->inputBuffer) {
				free(this->inputBuffer);
			}
		}

		// Imlements the read interface
		virtual uint32_t read(uint8_t * buffer, uint32_t len) override
		{
			char* str = this->inputBuffer + this->inCursor;

			uint32_t strLen = strlen(str);
			uint32_t min = (len > strLen) ? strLen : len;

			if (min > 0) {
				memcpy(buffer, str, min);
				this->inCursor += min;
			}
			
			return min;
		}

		// Implements write interface
		virtual uint32_t write(uint8_t* buffer, uint32_t len) override
		{
			if (this->outCursor == 0) {
				this->outputBuffer = (char*)malloc(len);
				memcpy_s(this->outputBuffer, len, buffer, len);
				this->outCursor = len;
				return len;
			} else {
				this->outputBuffer = (char*) realloc(this->outputBuffer, this->outCursor + len);
				memcpy_s(this->outputBuffer + this->outCursor, len, buffer, len);
				this->outCursor += len;
			}
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

		void compareBuffers(uint8_t* expected, uint32_t expectedLen, uint8_t* actual, uint32_t actualLen) {
			Assert::AreEqual(expectedLen, actualLen, L"Did not write correct amount of bytes");
			for (uint32_t i = 0; i < actualLen; i++) {
				Assert::AreEqual(expected[i], actual[i]);
			}
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
			int expectedLen = this->generateResponse(buffer, 2048, "{\"Hello\":\"world\"}");
			StrInputStream iss("");

			// execute
			HttpProtocol http = HttpProtocol(&iss);
			HttpResponseMsg res;
			res.write("{\"Hello\":\"world\"}");
			http.sendResponse(res);

			compareBuffers((uint8_t*) buffer, expectedLen, (uint8_t*) iss.outputBuffer, iss.outCursor);
		}
	};
}
