#include "WebApp.h"
#include "NetSyncher.h"

class ClientObject : public NetSynch::IClientDevice {
	WebSocketBase* conn;

public:
	ClientObject(WebSocketBase* conn)
		: conn(conn)
	{
	}

	// Heredado vía IClientDevice
	virtual void onStartRecording() override
	{
		conn->Send("Start recording");
	}

	virtual void onStopRecording() override
	{
		conn->Send("Stop recording");
	}
};


// ------------ WebApplication ---------------------

WebApp::WebApp(NetSynch::NetSyncher * syncher)
	:syncher(syncher)
{
}

void WebApp::onOpen(WebSocketBase * conn)
{
	shared_ptr<ClientObject> client(new ClientObject(conn));
	this->syncher->acceptClient(client);
	DebugLog("WS: opened connection\n");
}

void WebApp::onMessage(WebSocketBase * conn, string message)
{
	// nothing to do here
	DebugLog("WS: onmessage: %s\n", message.c_str());
}

void WebApp::onClose(WebSocketBase * conn)
{
	// also, nothing to do here
	DebugLog("WS: closed connection\n");
}


shared_ptr<HttpResponseMsg> WebApp::post_upload(HttpRequestMsg * req)
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
	res->setStatus(204);
	return res;
}

shared_ptr<HttpResponseMsg> WebApp::get(HttpRequestMsg * req)
{
	char tempLine[2048];
	auto res = HttpServer::createResponse();
	res->setHeader("Content-Type", "application/json");
	sprintf_s(tempLine, 2048, "{\"Hello\":\"world\"}");
	res->write(tempLine);

	return res;
}

shared_ptr<HttpResponseMsg> WebApp::not_found(HttpRequestMsg * req)
{
	auto res = HttpServer::createResponse();
	res->setStatus(404);
	res->write("Not found");
	return res;
}

shared_ptr<HttpResponseMsg> WebApp::OnRequest(HttpRequestMsg* req)
{
	const string uri = req->getUrl();
	const string method = req->getMethod();

	if (uri == "/") {
		return this->get(req);
	} else if (uri == "/upload" && method == "POST") {
		return this->post_upload(req);
	} else {
		return this->not_found(req);
	}
}
