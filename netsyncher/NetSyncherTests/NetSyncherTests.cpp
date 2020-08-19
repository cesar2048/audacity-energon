#include "pch.h"
#include "CppUnitTest.h"

#include "../NetSyncher/WebsocketProtocol.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NetSyncherTests
{
	TEST_CLASS(NetSyncherTests)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			int expected = 5;
			WebsocketProtocol ws = WebsocketProtocol();
			int actual = ws.TestMethod();

			Assert::AreEqual(expected, actual);
		}

		TEST_METHOD(TestServerKey)
		{
			WebsocketProtocol ws = WebsocketProtocol();
			char* key = ws.CalculateSignature("dGhlIHNhbXBsZSBub25jZQ==");
			char expected[] = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

			Assert::AreEqual(expected, key);
		}
	};
}
