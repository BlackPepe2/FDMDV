/*--------------------------------------------------------------------

        GRAPHWIN.H

  --------------------------------------------------------------------*/

         
/*--------------------------------------------------------------------
        PROGRAM CONSTANTS
  --------------------------------------------------------------------*/

#define RGB_WHITE       RGB( 0xFF, 0xFF, 0xFF )
#define RGB_GREEN       RGB( 0x00, 0xFF, 0x00 )
#define RGB_BLUE        RGB( 0x00, 0x00, 0xFF )
#define RGB_RED         RGB( 0xFF, 0x00, 0x00 )

/*--------------------------------------------------------------------
        PROGRAM FUNCTIONS
  --------------------------------------------------------------------*/

BOOL RegisterGraphClass ( HINSTANCE hInstance );

LRESULT WINAPI Graph_WndProc (
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

void Graph_Init ( HWND hWnd );

extern int disptype;
extern double freq;
extern int nacq;
extern int nacqpos;

#define audbuffsize   1920
#define specbuffsize  400

extern double rfbuf[audbuffsize];
extern double ispecbuf[specbuffsize];
extern double fibuf[32];
extern double fqbuf[32];
extern double fiibuf[32];
extern double fqqbuf[32];

// in codectest
void setfreq();

