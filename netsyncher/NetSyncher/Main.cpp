// NetSyncher.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>
#include "NetSyncher.h"
#include "wx/wxhtml.h"

#include "WebApp.h"
#include "WxHttpServer.h"
#include "../qrCodeGen/cpp/QrCode.hpp"
#include "osinterface.h"

using namespace qrcodegen;

// ------------ MyApp ---------------------

class MyApp : public wxApp {
public:
	virtual bool OnInit();
};

enum {
	ID_Hello = 1,
	ID_Record
};

// ------------ QrVisor ---------------------

class QrVisor : public wxScrolledWindow
{
	qrcodegen::QrCode code;
	int moduleSize;


public:
	QrVisor(wxFrame* parent, qrcodegen::QrCode code, int moduleSize = 20)
		: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL | wxNO | wxFULL_REPAINT_ON_RESIZE),
		  code(code),
		  moduleSize(moduleSize)
	{
		int size = code.getSize() * moduleSize;
		this->SetMinSize(wxSize(size, size+ 40));
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


// ------------ MyFrame ---------------------


class MyFrame : public wxFrame {
	public:
		MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	private:
		shared_ptr<HttpServer> server;
		shared_ptr<WebsocketServer> wsServer;

		NetSynch::NetSyncher* ns;
		WebApp* app;

		wxButton *btnRecord;
		bool isRecording;

		void OnStartServer(wxCommandEvent& event);
		void OnExit(wxCommandEvent& event);
		void OnAbout(wxCommandEvent& event);
		void OnRecord(wxCommandEvent& event);

		wxDECLARE_EVENT_TABLE();
};


wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
	EVT_MENU(ID_Hello, MyFrame::OnStartServer)
	EVT_MENU(wxID_EXIT, MyFrame::OnExit)
	EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);



bool MyApp::OnInit() {
	MyFrame *frame = new MyFrame("Hello world", wxPoint(50, 50), wxSize(450, 340));
	frame->Show(true);
	return true;
}


MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size),
	wsServer(NULL),
	server(NULL)
{
	this->ns = new NetSynch::NetSyncher();
	this->app = new WebApp(this->ns);

	this->isRecording = false;

	{ // menu block
		wxMenu *menuFile = new wxMenu;
		menuFile->Append(ID_Hello, "&Start... \tCtrl-H", "Help string shown in status bar from this menu item");
		menuFile->AppendSeparator();
		menuFile->Append(wxID_EXIT);

		wxMenu *menuHelp = new wxMenu;
		menuHelp->Append(wxID_EXIT);

		wxMenuBar *menuBar = new wxMenuBar;
		menuBar->Append(menuFile, "&File");
		menuBar->Append(menuHelp, "&Help");

		SetMenuBar(menuBar);
	}

	{ // content block
		vector<string> addresses = FindOSInterfaces(4);
		string msg = "";
		auto end = addresses.end();
		for (auto it = addresses.begin(); it != end; it++) {
			msg += *it + ",";
		}
		
		qrcodegen::QrCode qr0 = qrcodegen::QrCode::encodeText(msg.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
		wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

		// record button
		this->btnRecord = new wxButton(this, ID_Record, "Record");
		Bind(wxEVT_BUTTON, &MyFrame::OnRecord, this, ID_Record);
		topSizer->Add(this->btnRecord, 0, wxALL, 1);

		QrVisor *canvas = new QrVisor(this, qr0);
		// canvas->SetMinSize(wxSize(300, 300));
		topSizer->Add(canvas, 1, wxEXPAND | wxALL, 1);

		// main panel
		this->SetSizerAndFit(topSizer);

	}

	CreateStatusBar();
	SetStatusText("Welcome to wxWidgets!");
}

void MyFrame::OnExit(wxCommandEvent& event) {
	Close(true);
	delete this->app;
	delete this->ns;
}

void MyFrame::OnAbout(wxCommandEvent& event) {
	wxMessageBox("This is a wxWidgets' Hello world sample",
		"About Hello World", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnRecord(wxCommandEvent & event)
{
	if (!this->isRecording) {
		this->ns->StartRecording();
		this->btnRecord->SetLabel("Stop");
	} else {
		this->ns->StopRecording();
		this->btnRecord->SetLabel("Record");
	}
	this->isRecording = !this->isRecording;
}

void MyFrame::OnStartServer(wxCommandEvent& event) {
	if (!this->wsServer) {
		this->wsServer = WebsocketServer::CreateWebsocketserver(this->app);

		this->server = AllocateWebServer();
		this->server->SetRouteHandler(this->app);
		this->server->RegisterUpdateHandler("websocket", this->wsServer.get());
		this->server->Listen(8080);

		wxLogMessage("Server started!");
	}
};

