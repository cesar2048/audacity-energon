#include "ConnectionDialog.h"

#include "../qrCodeGen/cpp/QrCode.hpp"
#include "osinterface.h"


using namespace qrcodegen;


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
		string msg = "";
		auto end = addresses.end();
		for (auto it = addresses.begin(); it != end; it++) {
			msg += *it + ",";
		}

		qrcodegen::QrCode qr0 = qrcodegen::QrCode::encodeText(msg.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
		wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

		QrVisor *canvas = new QrVisor(this, qr0);
		topSizer->Add(canvas, 1, wxEXPAND | wxALL, 1);

		// main panel
		this->SetSizerAndFit(topSizer);
	}

}