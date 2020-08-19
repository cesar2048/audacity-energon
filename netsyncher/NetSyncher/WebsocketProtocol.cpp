#include <stdlib.h>
#include <string.h>

#include "WebsocketProtocol.h"

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

