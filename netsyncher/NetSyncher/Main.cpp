// NetSyncher.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>

#include "NetSyncher.h"
#include "WebApp.h"
#include "WxHttpServer.h"


// ------------ MyApp ---------------------

class MyApp : public wxApp {
	public:
		virtual bool OnInit();
	};

	enum {
		ID_Hello = 1,
		ID_Record
};

// ------------ MyFrame ---------------------


class MyFrame : public wxFrame {
	public:
		MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	private:
		NetSynch::NetSyncher* ns;
		WebApp* app;

		HttpServer* server;
		shared_ptr<WebsocketServer> wsServer;

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
		wxPanel* mainPanel = new wxPanel(this);
		this->btnRecord = new wxButton(mainPanel, ID_Record, "Record");

		Bind(wxEVT_BUTTON, &MyFrame::OnRecord, this, ID_Record);
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

