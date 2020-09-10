#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void compareBuffers(uint8_t* expected, uint32_t expectedLen, uint8_t* actual, uint32_t actualLen) {
	Assert::AreEqual(expectedLen, actualLen, L"Did not write correct amount of bytes");
	for (uint32_t i = 0; i < actualLen; i++) {
		Assert::AreEqual(expected[i], actual[i]);
	}
}