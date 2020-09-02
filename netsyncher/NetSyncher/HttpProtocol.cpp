#include "HttpProtocol.h"



HttpRequestMsg HttpProtocol::readRequest(InputStream *iStream)
{
	/*
	https://stackoverflow.com/questions/17290144/stdio-vs-iostream
	http://www.cplusplus.com/doc/tutorial/basic_io/
	 
	output stream in c++
	https://www.cprogramming.com/tutorial/c++-iostreams.html
	https://stackoverflow.com/questions/13703823/a-custom-ostream
	
	input streams in c++
	https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
	http://www.cplusplus.com/reference/sstream/stringbuf/
	https://stackoverflow.com/questions/772355/how-to-inherit-from-stdostream
	*/
	#define BUFFER_LEN 32

	HttpRequestMsg msg = HttpRequestMsg();
	
	uint32_t read = 1;
	uint32_t bytesUsed = 0;
	uint32_t bufferLength = BUFFER_LEN;
	char *str = (char*) malloc(bufferLength * sizeof(char));
	
	while (read != 0) {
		uint32_t available = bufferLength - bytesUsed;

		if (available == 0) {
			bufferLength = bufferLength + BUFFER_LEN;
			str = (char*)realloc(str, bufferLength);
			available = bufferLength - bytesUsed;
		}
	
		char *ptr = str + bytesUsed;
		read = iStream->read((uint8_t*) ptr, available);
		bytesUsed += read;
	};

	return msg;
}
