#include "WebApp.h"

shared_ptr<HttpResponseMsg> WebApp::onRequest(HttpRequestMsg req)
{
	char tempLine[2048];
	shared_ptr<MultipartStream> file = req.readFile("filename");

	int totalRead = 0;
	int totalExpected = 0;
	if (file != nullptr) {
		int read = -1;
		totalExpected = file->getLength();

		uint8_t buffer[4096];
		time_t ts = time(NULL);
		sprintf(tempLine, "video-%d.mp4", ts);
		FILE* f = fopen(tempLine, "wb");
		do {
			read = file->read(buffer, 2048);
			totalRead += read;
			fwrite(buffer, sizeof(uint8_t), read, f);
		} while (read != 0);
		fclose(f);
	}

	HttpResponseMsg* res = new HttpResponseMsg();

	res->setHeader("Content-Type", "application/json");
	sprintf_s(tempLine, 2048, "{\"Hello\":\"world\",\n"
		"\"Received\":%d,"
		"\"Expected\":%d"
		"\n}", totalRead, totalExpected);
	res->write(tempLine);

	return shared_ptr<HttpResponseMsg>(res);
}
