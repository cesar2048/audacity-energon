#include "MemBuffer.h"


Buffer::Buffer(uint32_t len, uint8_t * buffer)
	:len(len), buffer(buffer), selfAllocated(false)
{
}

Buffer::Buffer(const char * str)
{
	this->len = strlen(str);
	this->buffer = (uint8_t*) str;
	this->selfAllocated = false;
}

Buffer::Buffer(uint32_t len)
{
	this->selfAllocated = true;
	this->len = len;
	this->buffer = (uint8_t*)malloc(len);
}

Buffer::~Buffer()
{
	if (this->selfAllocated) {
		free(this->buffer);
	}
}

Buffer Buffer::fromString(const char* str)
{
	return Buffer(str);
}

Buffer Buffer::operator+(unsigned int offset)
{
	if (offset > this->len) {
		throw std::string("Can't advance buffer more than it's size");
	}

	return Buffer(this->len - offset, this->buffer + offset);
}


// -------------- MemBuffer -------------------------

MemBuffer::MemBuffer() : MemBuffer(BUFFER_LEN) {
}

MemBuffer::MemBuffer(uint32_t size)
{
	this->buffer = (uint8_t*)malloc(size);
	this->buffer[0] = 0;
	this->buffSize = size;
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

uint8_t * MemBuffer::getBuffer()
{
	return this->buffer;
}

uint8_t * MemBuffer::_getBuffer(uint32_t * outSize)
{
	*outSize = this->buffUsed;
	return this->buffer;
}



// -------------- util methods -------------------------


int findSequenceInBuffer(uint8_t* buffer, uint32_t len, uint8_t* bytesToFind, uint32_t bytesLength)
{
	int found = -1;
	for (int i = 0; found == -1 && i <= len - bytesLength; i++) {
		found = i;
		for (int j = 0; found != -1 && j < bytesLength; j++) {
			if (buffer[i + j] != bytesToFind[j]) {
				found = -1;
			}
		}
	}

	return found;
}

int findSequenceInBuffer(Buffer have, Buffer sequenceToFind)
{
	return findSequenceInBuffer(have.buffer, have.len, sequenceToFind.buffer, sequenceToFind.len);
}

int findByteSpan(Buffer have, Buffer bytes)
{
	int lastPos = 0;
	bool byteInSet = true;

	for (int i = 0; byteInSet && i < have.len; i++) {
		byteInSet = false;
		for (int j = 0; !byteInSet && j < bytes.len; j++) {
			byteInSet = byteInSet || have.buffer[i] == bytes.buffer[j];
		}
		if (byteInSet) {
			lastPos = i + 1;
		}
	}

	return lastPos;
}


