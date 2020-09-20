#pragma once

#include <memory>
#include <functional>

#define BUFFER_LEN 2048

class Buffer {
	bool selfAllocated;

public:
	uint32_t len;
	uint8_t* buffer;

	Buffer(uint32_t len, uint8_t* buffer);
	Buffer(const char* str);
	Buffer(uint32_t len);
	~Buffer();

	static Buffer fromString(const char* str);
	Buffer operator+(unsigned int offset);
};

class IOStream {
public:
	/*
	Peek into the stream without advancing the buffer and deleting the information
	*/
	virtual uint32_t peek(uint8_t* buffer, uint32_t len) = 0;

	/*
	Reads up to len bytes into buffer.
	Returns the number of read bytes
	*/
	virtual uint32_t read(uint8_t* buffer, uint32_t len) = 0;

	/**
	Attempts to write as much as len bytes to the stream.
	Returns the number of bytes actually written
	*/
	virtual uint32_t write(uint8_t* buffer, uint32_t len) = 0;

	/**
	Attempts to write string to the stream
	returns the number of bytes actually written
	*/
	virtual uint32_t write(const char* string);
};

/**
MemBuffer
*/
class MemBuffer
{
	uint8_t* buffer;
	uint32_t buffSize;
	uint32_t buffUsed;

	bool ensureEnoughSpace(uint32_t desiredLength);

public:
	MemBuffer();
	MemBuffer(uint32_t size);
	MemBuffer(MemBuffer&);
	~MemBuffer();

	// adds a zero terminated string to the response (buffered)
	// returns true if the operation succeded
	bool write(const char *str);

	// adds a byte array to the response (buffered)
	bool write(uint8_t* buffer, uint32_t len);

	uint32_t size();

	uint8_t* getBuffer();

	uint8_t* _getBuffer(uint32_t *outSize);
};

// ---- 

// Returns the position where the first ocurrence of the sequence bytesToFind is found in buffer
// -1 if nothing is found
int findSequenceInBuffer(uint8_t* buffer, uint32_t len, uint8_t* bytesToFind, uint32_t bytesLength);
int findSequenceInBuffer(Buffer have, Buffer sequenceToFind);

int findByteSpan(Buffer have, Buffer bytes);

// consumes from the stream until the position returned by the lambda function,
// or until there is no more bytes available
std::shared_ptr<MemBuffer> readUntilPosition(IOStream* iostream, const std::function<int(Buffer)>& getEndPos);

// reads from the stream until the endSequence is found, or until there is no more in the stream
std::shared_ptr<MemBuffer> readUntil(IOStream* iostream, uint8_t* endSequence, uint32_t endLength);