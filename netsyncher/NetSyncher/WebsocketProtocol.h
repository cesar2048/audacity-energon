#pragma once

#include <stdio.h>
#include <memory>

/**
* https://arne-mertz.de/2018/10/calling-cpp-code-from-c-with-extern-c/
*
*/
extern "C" {
	#include "sha1.h"
	#include "base64.h"
}
// SHA length = 168 bit = 21 byte
#define SHA1_LEN_OUT_BYTES 20

// Initial buffer size
#define WS_INITIAL_BUFFER 2048

// GUID defined in RFC6455 for websocket
const char wsGUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// https://www.codeproject.com/articles/13853/secure-coding-best-practices-for-memory-allocation
// https://stackoverflow.com/a/30595226
// https://stackoverflow.com/questions/7739323/freeing-memory-in-caller-or-callee

enum WSOpcode {
	Continuation	= 0x0,
	TextFrame		= 0x1,
	BinaryFrame		= 0x2,
	CloseConnection = 0x8,
	Ping			= 0x9,
	Pong			= 0xA,
};

typedef struct {
	WSOpcode opcode;
	uint8_t* buffer;
	uint32_t length;
} WS_MSG;

/**
Websocket implementation

RFC reference: https://tools.ietf.org/html/rfc6455#section-1.3
*/
class WebsocketProtocol
{
public:
	WebsocketProtocol();
	~WebsocketProtocol();

	// Generates the server key signature
	// returns a pointer to a buffer with the signature
	char* CalculateSignature(const char* clientKey);
	
	// Encodes a websocket frame and writes it into the provided buffer
	// returns how many bytes were actually written
	uint32_t WriteFrame(WSOpcode opcode, uint8_t* inBuffer, uint32_t inSize, uint8_t* outBuffer, uint32_t outSize, bool mask);

	// Decodes a websocket frame and writes the plain message 
	std::shared_ptr<WS_MSG> ReadFrame(uint8_t* buffer, uint32_t length);

private:
	uint8_t* buffer;
	size_t bufferLength;
};

