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


const char wsGUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

class WebsocketProtocol
{
public:
	int TestMethod();
	char* CalculateSignature(char* clientKey);
};

// SHA length = 168 bit = 21 byte
#define SHA1_RESULT_IN_BYTES 20