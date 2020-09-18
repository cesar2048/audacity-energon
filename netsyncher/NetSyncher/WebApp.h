#pragma once
#include "HttpProtocol.h"

class WebApp
{
public:
	shared_ptr<HttpResponseMsg> onRequest(HttpRequestMsg req);
};

