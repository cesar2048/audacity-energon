#include "pch.h"
#include "CppUnitTest.h"

#include "../NetSyncher/MemBuffer.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NetSyncherTests
{
	TEST_CLASS(MemBufferTests)
	{
	public:
		TEST_METHOD(findBytesInBuffer_findsBytesWithin)
		{
			uint8_t buffer1[8] = {'a', 'b', 'c', 0, '\r', '\n', 'd', 'e'};
			uint8_t buffer2[2] = {'c', 0};

			int pos = findSequenceInBuffer(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
			Assert::AreEqual(2, pos);
		}

		TEST_METHOD(findBytesInBuffer_notFound)
		{
			uint8_t buffer1[8] = { 'a', 'b', 'c', 0, '\r', '\n', 'd', 'e' };
			uint8_t buffer2[2] = { 'z', 'b' };

			int pos = findSequenceInBuffer(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
			Assert::AreEqual(-1, pos);
		}

		TEST_METHOD(findBytesInBuffer_findBytesEnding)
		{
			uint8_t buffer1[8] = { 'a', 'd', 'c', 0, '\r', '\n', 'd', 'e' };
			uint8_t buffer2[2] = { 'd', 'e' };

			int pos = findSequenceInBuffer(buffer1, sizeof(buffer1), buffer2, sizeof(buffer2));
			Assert::AreEqual(6, pos);
		}

		TEST_METHOD(findByteSpan_happyPath)
		{
			uint8_t buffer1[8] = { 'a', 'b', 'c', 0, '\r', '\n', 'd', 'e' };
			uint8_t buffer2[3] = { 'b', 'c', 'a' };

			int pos = findByteSpan({ sizeof(buffer1), buffer1 }, { sizeof(buffer2), buffer2 });
			Assert::AreEqual(3, pos);

		}

		TEST_METHOD(findByteSpan_allIn)
		{
			uint8_t buffer1[8] = { 'a', 'b', 'c', 'b', 'a', 'c', 'b', 'a' };
			uint8_t buffer2[3] = { 'b', 'c', 'a' };

			int pos = findByteSpan({ sizeof(buffer1), buffer1 }, { sizeof(buffer2), buffer2 });
			Assert::AreEqual((int) sizeof(buffer1), pos);

		}
	};
}

