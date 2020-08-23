#include <stdlib.h>
#include <string.h>

#include "WebsocketProtocol.h"

uint32_t writeWord(uint8_t* outBuffer, uint32_t word, uint32_t pos) {
	outBuffer[pos++] = word & 0xFF;
	outBuffer[pos++] = (word << 8) & 0xFF;
	outBuffer[pos++] = (word << 16) & 0xFF;
	outBuffer[pos++] = (word << 24) & 0xFF;
	return pos;
}

int WebsocketProtocol::TestMethod() {
	return 5;
}

char* WebsocketProtocol::CalculateSignature(char* clientKey) {
	char stringToHash[128];
	char hashResult[SHA1_RESULT_IN_BYTES + 1];

	// generate string to SHA
	stringToHash[0] = NULL;
	strcat(stringToHash, clientKey);
	strcat(stringToHash, wsGUID);

	// SHA the string
	SHA1(hashResult, stringToHash, strlen(stringToHash));

	// format the hash for printing
	char *result = (char*)malloc(41);
	size_t offset;
	for (offset = 0; offset < 20; offset++) {
		sprintf((result + (2 * offset)), "%02x", hashResult[offset] & 0xff);
	}

	// convert to base64
	int b64OutSize = b64e_size(SHA1_RESULT_IN_BYTES + 1);
	char* b64String = (char*) malloc(sizeof(char) * b64OutSize);
	b64_encode((unsigned char*) hashResult, SHA1_RESULT_IN_BYTES, (unsigned char*) b64String);

	return b64String;
}

uint32_t WebsocketProtocol::WriteFrameToBuffer(WSOpcode opcode, uint8_t* inBuffer, uint32_t inSize, uint8_t* outBuffer, uint32_t outSize,bool mask)
{
/*
	  0               1               2               3
	  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	 +-+-+-+-+-------+-+-------------+-------------------------------+
	 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
	 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
	 | |1|2|3|       |K|             |                               |
	 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	 |     Extended payload length continued, if payload len == 127  |
	 + - - - - - - - - - - - - - - - +-------------------------------+
	 |                               |Masking-key, if MASK set to 1  |
	 +-------------------------------+-------------------------------+
	 | Masking-key (continued)       |          Payload Data         |
	 +-------------------------------- - - - - - - - - - - - - - - - +
	 :                     Payload Data continued ...                :
	 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	 |                     Payload Data continued ...                |
	 +---------------------------------------------------------------+
*/
	// verify min buffer length
	if (inSize < 16) {
		return -1;
	}

	// 7bit [+ 16 bit | + 64 bit]
	int bytesForLength = 1;
	if (inSize > UINT16_MAX) {
		bytesForLength = 1 + 8;
	} else if (inSize > 125) {
		bytesForLength = 1 + 2;
	}

	// opcode [+ mask]
	int bytesOverhead = 1;
	if (mask) {
		bytesOverhead = 1 + 4;
	}

	uint8_t isFinal = 0;
	if (outSize >= bytesOverhead + bytesForLength + inSize) {
		isFinal = 1;
	}

	int pos = 0;
	outBuffer[pos++] = opcode | (isFinal << 7);
	outBuffer[pos++] = (mask << 7) | (inSize <= 125 ? inSize : (inSize < UINT16_MAX ? 126 : 127));

	if (inSize > 125) {
		outBuffer[pos++] = inSize & 0xFF;
		outBuffer[pos++] = (inSize << 8) & 0xFF;
		if (inSize > UINT16_MAX) {
			outBuffer[pos++] = (inSize << 16) & 0xFF;
			outBuffer[pos++] = (inSize << 24) & 0xFF;
			// zeroing the last 4 bytes as we are only creating
			// frames whose length needs as much as 32 bit
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
			outBuffer[pos++] = 0;
		}
	}

	if (mask) {
		uint32_t maskValue = (uint32_t)rand();
		pos = writeWord(outBuffer, maskValue, pos);
	}

	for (int i = 0; i < inSize; i++) {
		outBuffer[pos++] = inBuffer[i];
	}

	return pos;
}


