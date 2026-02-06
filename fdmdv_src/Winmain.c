/*--------------------------------------------------------------------

        WINMAIN.C


  --------------------------------------------------------------------*/

//======================================================================//
//																	
//				NOTE: For a Microsoft project, include winmm.lib	
//						under Build / Settings / Link				
//																					
//======================================================================//


#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>
#include "codectest.h"           
#include "resource.h"           
#include "graphwin.h"            

/*--------------------------------------------------------------------
        FUNCTION PROTOTYPE
  --------------------------------------------------------------------*/

int WINAPI WinMain( HINSTANCE hinstThis, HINSTANCE hinstPrev,
    LPSTR lpszCmdLine, int iCmdShow );
BOOL RegisterShowWaveClass( HINSTANCE hInstance );

/*--------------------------------------------------------------------
        GLOBAL VARIABLES
  --------------------------------------------------------------------*/

HINSTANCE TheInstance = 0;

/*--------------------------------------------------------------------
        STATIC VARIABLE
  --------------------------------------------------------------------*/

static char szShowWaveClass[] = "ShowWaveClass";

/*--------------------------------------------------------------------
        WINMAIN
  --------------------------------------------------------------------*/

int WINAPI WinMain (
    HINSTANCE hinstThis,
    HINSTANCE hinstPrev,
    LPSTR lpszCmdLine,
    int iCmdShow )
{
    TheInstance = hinstThis;

 	InitCommonControls();

    if (!RegisterGraphClass( hinstThis ))       // wave graph 
    {
        return( 0 );
    }

	if (!RegisterShowWaveClass( hinstThis ))    // program window
    {
        return( 0 );
    }

    /* create and display the main program window */
    
	DialogBox( hinstThis,
        MAKEINTRESOURCE(DLG_MAIN),
        NULL,
        (DLGPROC)my_WndProc );
	

    return( 0 );
    UNREFERENCED_PARAMETER( hinstPrev );
    UNREFERENCED_PARAMETER( iCmdShow );
    UNREFERENCED_PARAMETER( lpszCmdLine );
}

/*--------------------------------------------------------------------
        REGISTER SHOWWAVE CLASS
        Register a window class for the main dialog box window.
  --------------------------------------------------------------------*/

BOOL RegisterShowWaveClass ( HINSTANCE hInstance )
{
    WNDCLASS WndClass;

    WndClass.style          = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc    = my_WndProc;
    WndClass.cbClsExtra     = 0;
    WndClass.cbWndExtra     = DLGWINDOWEXTRA;
    WndClass.hInstance      = hInstance;
    WndClass.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_SCOPE) );
    WndClass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    WndClass.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    WndClass.lpszMenuName   = MAKEINTRESOURCE( IDM_MAIN );
    WndClass.lpszClassName  = szShowWaveClass;

    return( RegisterClass(&WndClass) );
}


