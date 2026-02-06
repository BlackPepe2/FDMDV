/*--------------------------------------------------------------------

        GRAPHWIN.C

  --------------------------------------------------------------------*/

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include "graphwin.h"           

/*--------------------------------------------------------------------
        FUNCTION PROTOTYPES
  --------------------------------------------------------------------*/

LRESULT WINAPI Graph_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam );
static void Graph_OnPaint( HWND hWnd );

/*--------------------------------------------------------------------
        STATIC VARIABLES
  --------------------------------------------------------------------*/

static char szGraphClass[] = "GraphClass";

extern int disptype = 5;

// 700hz default, at 960 fft len
// fft base is 12.5 hz, spacing is 75 hz, 6 bins / spacing
// 700 hz is bin 56, line 112
// center at 1225hz, -> bin 98
// reso is 2 -> center line at 2*98 = 196;
// factor is 6.25 or 0.16 !!
int flinec = 196; 

/*--------------------------------------------------------------------
        MACROS
  --------------------------------------------------------------------*/

#define RECTWIDTH( r )  ((r.right) - (r.left) + 1)
#define RECTHEIGHT( r ) ((r.bottom) - (r.top) + 1)

/*--------------------------------------------------------------------
        REGISTER GRAPH CLASS
        Register the window class for the dialog box's wave graph
        control window.  The main dialog box's resource template
        names this window class for one of its controls.  This
        procedure must be called before CreateDialog.
  --------------------------------------------------------------------*/

BOOL RegisterGraphClass ( HINSTANCE hInstance )
{
    WNDCLASS wc;

    wc.style         = 0;
    wc.lpfnWndProc   = Graph_WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor( NULL, IDC_CROSS );
    wc.hbrBackground = GetStockBrush( BLACK_BRUSH );
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szGraphClass;

    return( RegisterClass(&wc) );
}

static void Graph_OnMouse (HWND hWnd, int x, int y);

/*--------------------------------------------------------------------
        GRAPH WNDPROC
        Receive messages for the main dialog box's sound graph window.
  --------------------------------------------------------------------*/

LRESULT WINAPI Graph_WndProc (
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
        /* paint the green sound graph line */
        HANDLE_MSG( hWnd, WM_PAINT, Graph_OnPaint );

        case WM_CREATE:
			SetCapture(hWnd);
			return 0L;
		case WM_LBUTTONDOWN:
			Graph_OnMouse (hWnd,LOWORD (lParam),HIWORD (lParam));
			return 0L;

        default:
            return( DefWindowProc(hWnd, uMsg, wParam, lParam) );
    }
}


HPEN  hPenB,hPenG,hPenR,hPenW;      
RECT  rClient; 
int nacq = 0;
int nacqpos = 196;

void Graph_Init ( HWND hWnd )
{
    hPenW = CreatePen( PS_SOLID, 1, RGB_WHITE );    // color display
    hPenB = CreatePen( PS_SOLID, 1, RGB_BLUE );     // color display
    hPenG = CreatePen( PS_SOLID, 1, RGB_GREEN );    // color display
    hPenR = CreatePen( PS_SOLID, 1, RGB_RED );      // color display
}

static void Graph_OnMouse (HWND hWnd, int x, int y)
{
	//if ((x <= 300) && (x >= 100)) freq = (x - 84) * 6.25;
	nacq = 1;
}

/*--------------------------------------------------------------------
        GRAPH_ONPAINT
        Handle WM_PAINT messages for the Graph window.  Draw
        the green wave graph.
  --------------------------------------------------------------------*/

int outlinept = 0;

static void Graph_OnPaint ( HWND hWnd )
{
    PAINTSTRUCT ps;
    HDC   hDC;
	int ix,sv=0,hv=0;
	int mid,bot;

    /* begin paint processing */
    hDC = BeginPaint( hWnd, &ps );
    GetClientRect( hWnd, &rClient );  //260 / 401
	flinec = (int)(freq * 0.16) + 86; 
	
	if (disptype == 1) // Scope
	{
		SelectPen( hDC, hPenG );
		mid = (rClient.bottom + rClient.top) / 2;
		sv = mid + (int)(rfbuf[0] / 256.0);
		MoveToEx(hDC,rClient.left,sv,NULL);
		for (ix=1;ix<specbuffsize;ix++)
		{
			sv = mid + (int)(rfbuf[ix] / 256.0);
			hv = ix;
			LineTo(hDC,hv,sv);
		}
	}
	else if (disptype == 2) // Spectrum
	{
		SelectPen( hDC, hPenG );
		bot = rClient.bottom - 1;
		sv = bot - (int)(ispecbuf[0]*0.3f);
		MoveToEx(hDC,rClient.left,sv,NULL);
		for (ix=1;ix<specbuffsize;ix++)
		{
			sv = bot - (int)(ispecbuf[ix]*0.3f);
			hv = ix;
			LineTo(hDC,hv,sv);
		}
		SelectPen( hDC, hPenR );
		MoveToEx(hDC,flinec - 86,bot,NULL);
		LineTo(hDC  ,flinec - 86,rClient.top);
		MoveToEx(hDC,flinec + 86,bot,NULL);
		LineTo(hDC  ,flinec + 86,rClient.top);
	}
	else if (disptype == 3) // Spectrum freq acq
	{
		SelectPen( hDC, hPenG );
		bot = rClient.bottom - 1;
		sv = bot - (int)(ispecbuf[0]*0.3f);
		MoveToEx(hDC,rClient.left,sv,NULL);
		for (ix=1;ix<specbuffsize;ix++)
		{
			sv = bot - (int)(ispecbuf[ix]*0.3f);
			hv = ix;
			LineTo(hDC,hv,sv);
		}
		SelectPen( hDC, hPenR );
		MoveToEx(hDC,nacqpos,bot,NULL);
		LineTo(hDC  ,nacqpos,rClient.top);
	}
	else if (disptype == 4) // demod i and q buffers
	{
		COLORREF col;
		int midh;
		mid = (rClient.bottom + rClient.top) / 2;
		midh = (rClient.right + rClient.left) / 2;
		col = RGB(200,200,200);
		SetPixel(hDC, midh, mid, col);
		col = RGB(000,200,000);
		for (ix=1;ix<32;ix++)
		{
			sv = mid  + (int)(fiibuf[ix] / 80.0);
			hv = midh + (int)(fqqbuf[ix] / 80.0);
			SetPixel(hDC, hv, sv, col);
		}
	}
	else if (disptype == 5) // Waterfall
	{
		int i;
		COLORREF col;
		int btmp=0,gtmp=0,rtmp=0;
		{
			bot = rClient.left;
			for (i=0;i<specbuffsize;i++)
			{
				btmp = (int)(ispecbuf[i] * 2.0);
				if (btmp < 0 ) btmp = 0;
				gtmp=0; rtmp=0;
				if (btmp >= 255) { gtmp = btmp - 255; btmp = 255; }
				if (gtmp >= 255) { rtmp = gtmp - 255; gtmp = 255; }
				if (rtmp >= 255) rtmp = 255;
				//         R   G   B
				col = RGB(rtmp,gtmp,btmp);

				SetPixel(hDC, bot + i,    outlinept, col);	
			}
			if (outlinept < rClient.top) outlinept = rClient.bottom;
			else outlinept--;
		}
		SelectPen( hDC, hPenB );
		MoveToEx(hDC,rClient.left,outlinept,NULL);
		LineTo(hDC,rClient.right,outlinept);
		SelectPen( hDC, hPenR );
		MoveToEx(hDC,flinec - 86,outlinept,NULL);
		LineTo(hDC  ,flinec + 86,outlinept);
	}

	/* end paint processing */
    EndPaint( hWnd, &ps );
    return;
}
