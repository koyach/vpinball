#include "StdAfx.h"

HINSTANCE g_hinst;
VPinball *g_pvp;
Player *g_pplayer;
HACCEL g_haccel;
BOOL g_fKeepUndoRecords = fTrue;

void ShowError(char *sz)
{
	MessageBox(NULL, sz, "Error", MB_OK | MB_ICONEXCLAMATION);
}

void ExitApp()
{
#ifdef ULTRAPIN
	bool	fe_shutdown_message_sent;
	int		retries;
	HWND	hFrontEndWnd;

	// Check if we need to send a message to the front end.
	fe_shutdown_message_sent = false;
	retries = 0;
	while ( (fe_shutdown_message_sent == false) &&
		    (retries < 100) )
	{
		// Find the front end window.
		hFrontEndWnd = FindWindow( NULL, "Ultrapin (plfe)" );
		if ( hFrontEndWnd != NULL )
		{
			// Send the message to the front end.
			if ( SendMessage( hFrontEndWnd, WM_USER, WINDOWMESSAGE_VPINBALLSHUTDOWN, 0 ) )
			{
				fe_shutdown_message_sent = true;
			}
			else
			{
				// Sleep.
				Sleep ( 500 );
			}
		}

		retries++;
	}
#endif
#ifdef DONGLE_SUPPORT
	// Check if we have a dongle.
	if ( get_dongle_status() == DONGLE_STATUS_OK )
	{
#endif
		// Quit nicely.
		if( g_pvp )
		{
			g_pvp->Quit();
		} 
		else exit(0);
#ifdef DONGLE_SUPPORT
	}
	else
	{
		// Quit hard.
		exit(0);
	}
#endif
}
