#pragma once

#include <memory>
#define BUFFER_LEN 2048

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
	MemBuffer(MemBuffer&);
	~MemBuffer();

	// adds a zero terminated string to the response (buffered)
	// returns true if the operation succeded
	bool write(const char *str);

	// adds a byte array to the response (buffered)
	bool write(uint8_t* buffer, uint32_t len);

	uint32_t size();

	uint8_t* _getBuffer(uint32_t *outSize);
};
