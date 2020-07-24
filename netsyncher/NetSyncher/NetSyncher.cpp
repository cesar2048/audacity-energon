#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "NetSyncher.h"

NetSyncher::NetSyncher() {
	wxIPV4address addr;
	addr.AnyAddress();
	addr.Service(8080);
	
	this->server = new wxSocketServer(addr, wxSOCKET_BLOCK);
	wxSocketBase *socket = this->server->Accept(true);

	char buffer[1024];
	socket->Read(buffer, 1024);

	int i = socket->LastReadCount();
	wxString Foobar;
	Foobar.Printf(wxT("Hello I have %d cookies."), i);
	wxMessageBox(Foobar);
}

void NetSyncher::Listen() {

}
