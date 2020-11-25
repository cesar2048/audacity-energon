#pragma once
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


class ConnectionWindow : public wxFrame {
public:
	ConnectionWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size, const char* qrData);

private:

	void OnExit(wxCommandEvent& event);
	const char* data;

	wxDECLARE_EVENT_TABLE();
};