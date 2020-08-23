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

		TEST_METHOD(TestWriteFrame_LenLessThan125)
		{
			static const size_t LEN = 100;
			static const size_t BYTES_HEADER = 2 + 0;

			// build message
			uint8_t inBuffer[LEN];
			for (int i = 0; i < LEN; i++) {
				inBuffer[i] = i & 0xFF;
			}

			// write frame
			WebsocketProtocol ws = WebsocketProtocol();
			uint8_t outBuffer[256];
			unsigned int written = ws.WriteFrameToBuffer(WSOpcode::BinaryFrame, inBuffer, LEN, outBuffer, 256, false);

			// build expected frame
			uint8_t expected[BYTES_HEADER + LEN];
			expected[0] = 0x82; // fin  + binary opcode
			expected[1] = 100;  // mask + length
			for (int i = 0; i < LEN; i++) {
				expected[BYTES_HEADER + i] = i & 0xFF;
			}

			Assert::AreEqual(BYTES_HEADER + LEN, written, L"Did not write correct amount of bytes");
			for (int i = 0; i < written; i++) {
				Assert::AreEqual(expected[i], outBuffer[i]);
			}
		}

		TEST_METHOD(TestWriteFrame_LenAbove125)
		{
			static const size_t LEN = 130;
			static const size_t BYTES_HEADER = 2 + 2;

			// build message
			uint8_t inBuffer[LEN];
			for (int i = 0; i < LEN; i++) {
				inBuffer[i] = i & 0xFF;
			}

			// write frame
			WebsocketProtocol ws = WebsocketProtocol();
			uint8_t outBuffer[256];
			unsigned int written = ws.WriteFrameToBuffer(WSOpcode::BinaryFrame, inBuffer, LEN, outBuffer, 256, false);

			// build expected frame
			uint8_t expected[BYTES_HEADER + LEN];
			expected[0] = 0x82;              // fin + binary opcode
			expected[1] = 0 + 126;           // mask + length(126), cause length fits in 16 bit
			expected[2] = LEN & 0xFF;        // length, 8 bits
			expected[3] = (LEN << 8) & 0xFF; // length, 8 bits
			for (int i = 0; i < LEN; i++) {
				expected[BYTES_HEADER + i] = i & 0xFF;
			}

			Assert::AreEqual(BYTES_HEADER + LEN, written, L"Did not write correct amount of bytes");
			for (int i = 0; i < written; i++) {
				Assert::AreEqual(expected[i], outBuffer[i]);
			}
		}

		TEST_METHOD(TestWriteFrame_LenAbove16bit)
		{
			static const size_t OUT_LEN = UINT16_MAX + 200;
			static const size_t LEN = UINT16_MAX + 100;
			static const size_t BYTES_HEADER = 2 + 8;

			// build message
			uint8_t inBuffer[LEN];
			for (int i = 0; i < LEN; i++) {
				inBuffer[i] = i & 0xFF;
			}

			// build expected frame
			uint8_t expected[BYTES_HEADER + LEN];
			expected[0] = 0x82;               // fin + binary opcode
			expected[1] = 0 + 127;            // mask + length(127), cause length needs more than 16 bit
			expected[2] = LEN & 0xFF;         // length, 8 bits
			expected[3] = (LEN << 8) & 0xFF;  // length, 8 bits
			expected[4] = (LEN << 16) & 0xFF; // length, 8 bits
			expected[5] = (LEN << 24) & 0xFF; // length, 8 bits
			expected[6] = 0;
			expected[7] = 0;
			expected[8] = 0;
			expected[9] = 0;
			for (int i = 0; i < LEN; i++) {
				expected[BYTES_HEADER + i] = i & 0xFF;
			}

			// Execute, write frame
			WebsocketProtocol ws = WebsocketProtocol();
			uint8_t outBuffer[OUT_LEN];
			unsigned int written = ws.WriteFrameToBuffer(WSOpcode::BinaryFrame, inBuffer, LEN, outBuffer, OUT_LEN, false);


			Assert::AreEqual(BYTES_HEADER + LEN, written, L"Did not write correct amount of bytes");
			for (int i = 0; i < written; i++) {
				Assert::AreEqual(expected[i], outBuffer[i]);
			}
		}
	};
}
