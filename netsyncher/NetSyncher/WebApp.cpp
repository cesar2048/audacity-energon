#include "WebApp.h"
#include "NetSyncher.h"

class ClientObject : public NetSynch::IClientDevice {


	// Heredado vía IClientDevice
	virtual void onStartRecording() override
	{
		// Buffer msg = Buffer::fromString("Start recording");
		// Send(msg.buffer, msg.len);
	}

	virtual void onStopRecording() override
	{
		// Buffer msg = Buffer::fromString("Stop recording");
		// Send(msg.buffer, msg.len);
	}
};


// ------------ WebApplication ---------------------

WebApp::WebApp(NetSynch::NetSyncher * syncher)
	:syncher(syncher)
{
}

void WebApp::onOpen(WebSocketBase * conn)
{
	DebugLog("WS: opened connection\n");
}

void WebApp::onMessage(WebSocketBase * conn, string message)
{
	DebugLog("WS: onmessage: %s\n", message.c_str());
}

void WebApp::onClose(WebSocketBase * conn)
{
	DebugLog("WS: closed connection\n");
}

shared_ptr<HttpResponseMsg> WebApp::OnRequest(HttpRequestMsg* req)
{
	char tempLine[2048];
	shared_ptr<MultipartStream> file = req->readFile("filename");

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

	auto res = HttpServer::createResponse();
	res->setHeader("Content-Type", "application/json");
	sprintf_s(tempLine, 2048, "{\n\"Hello\":\"world\",\n"
		"\"Received\":%d,\n"
		"\"Expe||2cted\":%d\n}", totalRead, totalExpected);
	res->write(tempLine);

	return res;
}

/*
shared_ptr<WebsocketServer::IMessageHandler> WebApp::CreateHandler()
{
	IMessageHandlerPtr ptr(new ClientObject());
	shared_ptr<NetSynch::IClientDevice> dev = std::dynamic_pointer_cast<NetSynch::IClientDevice>(ptr);
	this->syncher->acceptClient(dev);
	return ptr;
}
*/
