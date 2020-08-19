#pragma once

const char wsGUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

class WebsocketProtocol
{
public:
	int TestMethod();
	char* CalculateSignature(char* clientKey);
};

