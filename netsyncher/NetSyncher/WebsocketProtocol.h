#pragma once

#include <stdio.h>

/**
* https://arne-mertz.de/2018/10/calling-cpp-code-from-c-with-extern-c/
*
*/
extern "C" {
	#include "sha1.h"
	#include "base64.h"
}
// SHA length = 168 bit = 21 byte
#define SHA1_RESULT_IN_BYTES 20

// GUID defined in RFC6455 for websocket
const char wsGUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// https://www.codeproject.com/articles/13853/secure-coding-best-practices-for-memory-allocation
// https://stackoverflow.com/a/30595226
// https://stackoverflow.com/questions/7739323/freeing-memory-in-caller-or-callee

enum WSOpcode {
	Continuation = 0x0,
	TextFrame = 0x1,
	BinaryFrame = 0x2,
	CloseConnection = 0x8,
	Ping = 0x9,
	Pong = 0xA,
};

class WebsocketProtocol
{
public:
	int TestMethod();

	// Generates the server key signature
	// returns a pointer to a buffer with the signature
	char* CalculateSignature(char* clientKey);
	
	// Encodes a websocket frame and writes it into the provided buffer
	// returns how many bytes were actually written
	uint32_t WriteFrameToBuffer(WSOpcode opcode, uint8_t* inBuffer, uint32_t inSize, uint8_t* outBuffer, uint32_t outSize, bool mask);
};

