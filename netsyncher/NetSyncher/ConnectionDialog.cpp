#include "ConnectionDialog.h"
#include <sstream>

#include "../qrCodeGen/cpp/QrCode.hpp"
#include "osinterface.h"

using namespace qrcodegen;

// ------------ functions ---------------------

string vectorToJson(vector<string> list) {
	ostringstream ss;
	ss << "[";

	auto end = list.end();
	bool isFirst = true;
	for (auto it = list.begin(); it != end; it++) {
		if (!isFirst) {
			ss << ",";
		}
		ss << "\"" << (*it) << "\"";
		isFirst = false;
	}

	ss << "]";

	return ss.str();
}

vector<string> filter(vector<string> list, const std::function<bool(string)>& filterFn) {
	vector<string> result;
	for (auto& v : list)
		if (filterFn(v))
			result.push_back(v);

	return result;
}

// ------------ JsonConnectInfo ------------

class JsonObject {
public:
	virtual string toJsonString() = 0;
	virtual const char* toCString() = 0;
};


class JsonConnectInfo : public JsonObject {
	vector<string> addrList;

public:
	JsonConnectInfo(vector<string> ipAddrList) {
		this->addrList = filter(ipAddrList, [=](string item) {
			// ignoring localhost and link-local (no dhcp) addresses
			return item.find("127.") != 0 && item.find("169") != 0;
		});
	}

	// Heredado vía JsonObject
	virtual string toJsonString() override
	{
		string addresses = vectorToJson(this->addrList);
		return "{\"version\": \"1.0\", \"addresses\":"+addresses+"}";
	}

	virtual const char* toCString() override
	{
		return this->toJsonString().c_str();
	}
};


// ------------ QrVisor ---------------------

class QrVisor : public wxScrolledWindow
{
	qrcodegen::QrCode code;
	int moduleSize;


public:
	QrVisor(wxFrame* parent, qrcodegen::QrCode code, int moduleSize = 10)
		: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL | wxNO | wxFULL_REPAINT_ON_RESIZE),
		code(code),
		moduleSize(moduleSize)
	{
		int size = code.getSize() * moduleSize;
		this->SetMinSize(wxSize(size, size + 40));
	}

	void OnPaint(wxPaintEvent &event) {
		wxPaintDC pdc(this);
		wxDC &dc = pdc;

		dc.SetBackgroundMode(wxSOLID);
		dc.SetPen(wxPen(*wxBLACK, 0));
		dc.SetBrush(*wxBLACK_BRUSH);

		const int moduleWidth = this->moduleSize;
		for (int y = 0; y < code.getSize(); y++) {
			for (int x = 0; x < code.getSize(); x++) {
				if (code.getModule(x, y)) {
					dc.DrawRectangle(x * moduleWidth, y * moduleWidth, moduleWidth, moduleWidth);
				}
			}
		}
	}

	wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(QrVisor, wxScrolledWindow)
	EVT_PAINT(QrVisor::OnPaint)
wxEND_EVENT_TABLE()


wxBEGIN_EVENT_TABLE(ConnectionWindow, wxFrame)
wxEND_EVENT_TABLE()


ConnectionWindow::ConnectionWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(parent, wxID_ANY, title, pos, size)
{
	{ // content block
		vector<string> addresses = FindOSInterfaces(4);
		JsonConnectInfo info(addresses);
		
		qrcodegen::QrCode qr0 = qrcodegen::QrCode::encodeText(info.toJsonString().c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
		wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

		QrVisor *canvas = new QrVisor(this, qr0);
		topSizer->Add(canvas, 1, wxEXPAND | wxALL, 1);

		// main panel
		this->SetSizerAndFit(topSizer);
	}

}