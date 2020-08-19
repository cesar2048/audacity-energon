#include "WebsocketProtocol.h"
#include <stdlib.h>
#include <string.h>

int WebsocketProtocol::TestMethod() {
	return 5;
}

char* WebsocketProtocol::CalculateSignature(char* clientKey) {
	char* buffer = (char*) malloc(128);
	buffer[0] = 0;
	strcpy(buffer, clientKey);
	strcat(buffer, wsGUID);
	return buffer;
}

