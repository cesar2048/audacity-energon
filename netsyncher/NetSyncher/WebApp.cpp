#include "WebApp.h"
#include "NetSyncher.h"

class ClientObject : public NetSynch::IClientDevice, public WebsocketServer::IMessageHandler {

	// Heredado vía IMessageHandler
	virtual void OnMessage(uint8_t * buffer, uint32_t len) override
	{
		string str = string((char*)buffer, len);
		DebugLog("WS> %s\n", str.c_str());
	}

	// Heredado vía IClientDevice
	virtual void onStartRecording() override
	{
		Buffer msg = Buffer::fromString("Start recording");
		Send(msg.buffer, msg.len);
	}

	virtual void onStopRecording() override
	{
		Buffer msg = Buffer::fromString("Stop recording");
		Send(msg.buffer, msg.len);
	}
};


// ------------ WebApplication ---------------------

WebApp::WebApp(NetSynch::NetSyncher * syncher)
	:syncher(syncher)
{
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

	HttpResponseMsg* res = new HttpResponseMsg();
	res->setHeader("Content-Type", "application/json");
	sprintf_s(tempLine, 2048, "{\n\"Hello\":\"world\",\n"
		"\"Received\":%d,\n"
		"\"Expected\":%d\n}", totalRead, totalExpected);
	res->write(tempLine);

	return shared_ptr<HttpResponseMsg>(res);
}

shared_ptr<WebsocketServer::IMessageHandler> WebApp::CreateHandler()
{
	IMessageHandlerPtr ptr(new ClientObject());
	shared_ptr<NetSynch::IClientDevice> dev = std::dynamic_pointer_cast<NetSynch::IClientDevice>(ptr);
	this->syncher->acceptClient(dev);
	return ptr;
}

