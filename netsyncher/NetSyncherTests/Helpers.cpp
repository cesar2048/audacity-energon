#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

void compareBuffers(uint8_t* expected, uint32_t expectedLen, uint8_t* actual, uint32_t actualLen) {
	Assert::AreEqual(expectedLen, actualLen, L"Did not write correct amount of bytes");
	for (uint32_t i = 0; i < actualLen; i++) {
		Assert::AreEqual(expected[i], actual[i]);
	}
}

namespace NetSyncherTests
{

	// ------------------- StrInputStream -------------------


	/* Auxiliary class to test to provide an in-memory IOStream */
	StrInputStream::StrInputStream(const char *str) {
		size_t bufferSize = strlen(str) + 1;

		this->inCursor = 0;
		this->inputBuffer = (char*)malloc(bufferSize);

		strcpy_s(this->inputBuffer, bufferSize, str);
	}

	StrInputStream::~StrInputStream() {
		if (this->inputBuffer) {
			free(this->inputBuffer);
		}
	}

	// Imlements the peek interface
	uint32_t StrInputStream::peek(uint8_t * buffer, uint32_t len)
	{
		char* str = this->inputBuffer + this->inCursor;

		uint32_t strLen = strlen(str);
		uint32_t readableLen = (len > strLen) ? strLen : len;

		if (readableLen > 0) {
			memcpy(buffer, str, readableLen);
		}

		return readableLen;
	}

	// Imlements the read interface
	uint32_t StrInputStream::read(uint8_t * buffer, uint32_t len)
	{
		uint32_t bytesRead = this->peek(buffer, len);
		this->inCursor += bytesRead;
		return bytesRead;
	}

	// Implements write interface
	uint32_t StrInputStream::write(uint8_t* buffer, uint32_t len)
	{
		bool result = this->outBuffer.write(buffer, len);
		if (result) {
			return len;
		}
		return 0;
	}


	// ------------------- TransmissionReader -------------------


	TransmissionReader::TransmissionReader(char *fileName) {
		this->f = fopen(fileName, "rb");
		this->frameLength = -1;
	}

	uint32_t TransmissionReader::peek(uint8_t * buffer, uint32_t len)
	{
		long pos = ftell(this->f);

		uint32_t bytesRead = this->read(buffer, len);

		fseek(this->f, pos, SEEK_SET);
		return bytesRead;
	}

	uint32_t TransmissionReader::read(uint8_t * buffer, uint32_t len)
	{
		if (this->f == NULL) {
			return 0;
		}

		if (this->frameLength == -1) {
			fread(&this->frameLength, sizeof(uint32_t), 1, this->f);
		}
		
		uint32_t bytesToRead = len < this->frameLength ? len : this->frameLength;
		uint32_t bytesRead = fread(buffer, sizeof(uint8_t), bytesToRead, f);

		if (bytesToRead != bytesRead) {
			// due to the recording file format, this condition should only be due to error
			throw string("Mismatch between amount to read and actually read");
		}

		if (feof(this->f)) {
			fclose(this->f);
			this->f = NULL;
		}

		this->frameLength -= bytesRead;
		if (this->frameLength == 0) {
			this->frameLength = -1;
		}

		return bytesRead;
	}

	uint32_t TransmissionReader::write(uint8_t * buffer, uint32_t len)
	{
		return 0;
	}

}

