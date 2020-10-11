#pragma once
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif


class ConnectionWindow : public wxFrame {
public:
	ConnectionWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size);

private:

	void OnExit(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};