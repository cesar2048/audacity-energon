#include "MemBuffer.h"

// -------------- MemBuffer -------------------------

MemBuffer::MemBuffer()
{
	this->buffer = (uint8_t*)malloc(BUFFER_LEN);
	this->buffer[0] = 0;
	this->buffSize = BUFFER_LEN;
}

MemBuffer::MemBuffer(MemBuffer &original)
{
	this->buffSize = original.buffSize;
	this->buffer = (uint8_t*)malloc(this->buffSize);

	memcpy(this->buffer, original.buffer, original.buffSize);
}

MemBuffer::~MemBuffer()
{
	free(this->buffer);
}

bool MemBuffer::ensureEnoughSpace(uint32_t desiredSize)
{
	if (this->buffSize < desiredSize) {
		void* ptr = realloc(this->buffer, this->buffSize * 2);
		if (!ptr) {
			return false;
		}
		this->buffer = (uint8_t*)ptr;
		this->buffSize *= 2;
	}
	return true;
}

bool MemBuffer::write(const char * str)
{
	int len = strlen(str);
	return this->write((uint8_t*)str, len);
}

bool MemBuffer::write(uint8_t * buffer, uint32_t len)
{
	uint32_t spaceNeeded = this->buffUsed + len;
	bool hasEnoughSpace = this->ensureEnoughSpace(spaceNeeded);
	if (!hasEnoughSpace) {
		return false;
	}

	uint32_t available = this->buffSize - this->buffUsed;
	uint8_t* ptrStart = this->buffer + this->buffUsed;

	memcpy_s(ptrStart, available, buffer, len);
	this->buffUsed += len;
}

uint32_t MemBuffer::size()
{
	return this->buffUsed;
}

uint8_t * MemBuffer::_getBuffer(uint32_t * outSize)
{
	*outSize = this->buffUsed;
	return this->buffer;
}


// -------------- IOStream -------------------------


uint32_t IOStream::write(const char * string)
{
	uint32_t len = strlen(string);
	uint32_t res = this->write((uint8_t*)string, len);
	return res;
}
