
//#define STRICT
//#define _MT


#include <process.h>            // 
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>             
#include <math.h>             
#include <commctrl.h>           // prototypes and defs for common controls 
#include "codectest.h"              // program variables and functions
#include "graphwin.h"              // program variables and functions
#include "resource.h"           // program resource ID definitions
#include "fft.h"
#include "sound.h"
#include "melp.h"
#include "ptt.h"

/*--------------------------------------------------------------------
        PROTOTYPES FOR PRIVATE FUNCTIONS
  --------------------------------------------------------------------*/

static BOOL my_OnInitDialog( HWND hDlg, HWND hWndFocus, LPARAM lParam );
static void my_OnCommand( HWND hDlg, int iCmd, HWND hCtl, UINT uCode );
static void my_OnDestroy( HWND hDlg );
static void my_OnTimer ( HWND hWnd, UINT id );

BOOL CALLBACK SoundCardDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam);

/*--------------------------------------------------------------------
        STATIC VARIABLES
  --------------------------------------------------------------------*/


/*--------------------------------------------------------------------
        SHOWWAVE DIALOG PROCEDURE
        Sort out messages for the program's main window, the ShowWave
        dialog box.
  --------------------------------------------------------------------*/

LRESULT WINAPI my_WndProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
        HANDLE_MSG( hDlg, WM_INITDIALOG, my_OnInitDialog );

        HANDLE_MSG( hDlg, WM_COMMAND, my_OnCommand );

        HANDLE_MSG( hDlg, WM_DESTROY, my_OnDestroy );

        HANDLE_MSG( hDlg, WM_TIMER, my_OnTimer );

        default:
            return( DefWindowProc(hDlg, uMsg, wParam, lParam) );
     }
}

/*--------------------------------------------------------------------
        TEST
  --------------------------------------------------------------------*/

int iNumDevOut;
int scoutdevtx = 0;
int scoutdevrx = 0;
int iNumDevIn;
int scindevtx = 0;
int scindevrx = 0;
char comport = '0';

HWND progbar;
FILE * set = NULL;
char callsign[50];

//FILE * logf = NULL;

/*--------------------------------------------------------------------
        PAINT WAVE DATA
        Repaint the dialog box's GraphClass display control
  --------------------------------------------------------------------*/

static void PaintWaveData ( HWND hDlg )
{
    HWND hwndShowWave = GetDlgItem( hDlg, IDS_WAVE_PANE );
    InvalidateRect( hwndShowWave, NULL, (disptype <= 4) );
    UpdateWindow( hwndShowWave );
    return;
}
/*--------------------------------------------------------------------
        modem stuff 
  --------------------------------------------------------------------*/

#define	  pi		 3.14159265 			
#define	  pi2		 6.28318531			
#define   nbitfreq   2.0/15.00
#define	  ntones	 15			// Number of tones of 20ms / 50 baud
#define	  puretone	 7			
#define   spacing    75.0		// 1.5 times symbolrate
#define   basefreq   700.0		// bottom tone frequency Hz
#define   samplerate 12000
#define   rxsamplerate 6000
#define   txsamplerate 12000
#define   rolloff    0.5		// RRC filter width 1.5 times 75 = 112.5
#define   rrcsize    128
#define   bufsize    240
#define   rrcsizetx  8*bufsize

int curcar = puretone;
double freq;

double fil81[81] = //first filter
{
		0.000605, -0.000124, -0.000716, -0.000050, 0.000890, 0.000323, 
		-0.001102, -0.000764, 0.001286, 0.001423, -0.001339, -0.002320, 
		0.001128, 0.003420, -0.000508, -0.004632, -0.000661, 0.005797, 
		0.002492, -0.006691, -0.005050, 0.007033, 0.008335, -0.006493, 
		-0.012275, 0.004694, 0.016718, -0.001199, -0.021446, -0.004546, 
		0.026185, 0.013394, -0.030632, -0.027011, 0.034483, 0.049682, 
		-0.037460, -0.097362, 0.039341, 0.315235, 0.459830, 0.315235, 
		0.039341, -0.097362, -0.037460, 0.049682, 0.034483, -0.027011, 
		-0.030632, 0.013394, 0.026185, -0.004546, -0.021446, -0.001199, 
		0.016718, 0.004694, -0.012275, -0.006493, 0.008335, 0.007033, 
		-0.005050, -0.006691, 0.002492, 0.005797, -0.000661, -0.004632, 
		-0.000508, 0.003420, 0.001128, -0.002320, -0.001339, 0.001423, 
		0.001286, -0.000764, -0.001102, 0.000323, 0.000890, -0.000050, 
		-0.000716, -0.000124, 0.000605, 
};

double fil48[48] = //second filter
{
    -8.638680e-004, -5.889242e-004, -6.695990e-004, -6.345310e-004,
    -4.192960e-004, 4.721631e-005, 8.403140e-004, 2.034841e-003,
    3.696014e-003, 5.873224e-003, 8.599516e-003, 1.187555e-002,
    1.567687e-002, 1.994355e-002, 2.458330e-002, 2.947266e-002,
    3.446187e-002, 3.938090e-002, 4.404875e-002, 4.828422e-002,
    5.191347e-002, 5.478422e-002, 5.677157e-002, 5.778786e-002,
    5.778786e-002, 5.677157e-002, 5.478422e-002, 5.191347e-002,
    4.828422e-002, 4.404875e-002, 3.938090e-002, 3.446187e-002,
    2.947266e-002, 2.458330e-002, 1.994355e-002, 1.567687e-002,
    1.187555e-002, 8.599516e-003, 5.873224e-003, 3.696014e-003,
    2.034841e-003, 8.403140e-004, 4.721631e-005, -4.192960e-004,
    -6.345310e-004, -6.695990e-004, -5.889242e-004, -8.638680e-004
};

double fdmbuftx[2*bufsize];	// tone multiplex buffer
double RRCcofstx[8*bufsize];
double Sfiltertx[8*bufsize];
int Sptrtx = 0;

double freqstx[ntones];
double phasestx[ntones];
double Pstepstx[ntones];

double phase;
  
void MakeRRCtx(double r, double cofs[])
{
	int i;
	double t,max,act;
	max = 0.0;
	for (i=0;i<rrcsizetx;i++)
	{
		t = ((double)i - rrcsizetx / 2 - 0.5) / bufsize;
		cofs[i] = (4 * r * t * cos(pi * (1 + r) * t) + sin(pi * (1 - r) * t)) /
			((1 - (4 * r * t) * (4 * r * t)) * pi * t);
		act = cofs[i];
		if (act < 0.0) act = -act;
		if (act > max) max = act;
	}
	for (i=0;i<rrcsizetx;i++)
		cofs[i] = cofs[i] / max;  // normalise to unity at centre
}

void MakeRRC(double r, double cofs[])
{
	int i;
	double t, sum;
	sum = 0.0;
	for (i=0;i<rrcsize;i++)
	{
		t   = (i - rrcsize / 2 - 0.5) / rrcsize * 8;
		cofs[i] = (4 * r * t * cos(pi * (1 + r) * t) + sin(pi * (1 - r) * t)) /
		  ((1 - (4 * r * t) * (4 * r * t)) * pi * t);
		sum = sum + cofs[i];
	}
	for (i=0;i<rrcsize;i++)
		cofs[i] = cofs[i] / sum * 5;
}
double frac(double in)
{
	while (in >= 1.0) in -= 1.0;
	while (in < 0.0) in += 1.0;
	return in;
}

int bitarr[100];
int bitct;
int msgpart = 0;
void setbitct(int in)
{
	bitct = in;
}
void store2bit(int in)
{
	bitarr[bitct++] = in & 0x01;
	bitarr[bitct++] = (in >> 1) & 0x01;
}
void store6bit(int in)
{
	bitarr[bitct++] = in & 0x01;
	bitarr[bitct++] = (in >> 1) & 0x01;
	bitarr[bitct++] = (in >> 2) & 0x01;
	bitarr[bitct++] = (in >> 3) & 0x01;
	bitarr[bitct++] = (in >> 4) & 0x01;
	bitarr[bitct++] = (in >> 5) & 0x01;
}
int get1bit()
{
	return bitarr[bitct++]; 
}
int get2bit()
{
	int ret;
	ret = get1bit(); 
	ret = ret | (get1bit() << 1);
	return ret; 
}
int get6bit()
{
	int i,ret;
	ret = get1bit(); 
	for (i=1;i<6;i++)
		ret = ret | (get1bit() << i); 
	return ret;
}

BYTE tosend[ntones];
int calltwobitarr[100];
int callslen = 0;
int callspt = 0;

void storeword(BYTE dat[9])
{
	int i;
	setbitct(0);
	for (i=0;i<9;i++) store6bit(dat[i]);
	setbitct(0);
	msgpart = 0;
}

double GetTxPhase(int in)
{
	int val;
	if (in == puretone)	val = msgpart;
	else				val = get2bit();
	if (in >= ntones-1) msgpart += 2;
	return val * 0.25;
}

double outbuf[bufsize];
double usbuf[81];
int usptr = 0;

void makesound(short datout[])
{
	int i,j,val;
	double magnitude = 1000.0;
	for (j=0;j<2*bufsize;j++)	fdmbuftx[j] = 0.0;
	
	for (i=0;i<ntones;i++)	//add in waveforms 
	{
		phasestx[i] = frac(phasestx[i] + GetTxPhase(i) + Pstepstx[i]);	//advance phase by data value
		phase = phasestx[i];
		if (i == puretone)
			for (j=0;j<2*bufsize;j++)	//compute and ADD the waveform into fdmbuf
			{
			  fdmbuftx[j] += 1.3 * sin(pi2 * phase);
			  phase = frac(phase + freqstx[i]);
			}
		else
			for (j=0;j<2*bufsize;j++)	//compute and ADD the waveform into fdmbuf
			{
			  fdmbuftx[j] += sin(pi2 * phase);
			  phase = frac(phase + freqstx[i]);
			}
	}

	// now filter fdmbuftx over 107mS interval
	for (i=0;i<bufsize;i++)		// oldest 13.33mS: add previous and send to soundcard
	{
		outbuf[i] = magnitude * (Sfiltertx[Sptrtx] + RRCcofstx[i] * fdmbuftx[i]);
		Sptrtx = (Sptrtx + 1) % (7*bufsize);
	}
	for (i=bufsize;i<7*bufsize;i++)	// middle 80mS: add into Sfiltertx
	{
		j = i % (2*bufsize);
		Sfiltertx[Sptrtx] = Sfiltertx[Sptrtx] + RRCcofstx[i] * fdmbuftx[j];
		Sptrtx = (Sptrtx + 1) % (7*bufsize);
	}
	for (i=7*bufsize;i<8*bufsize;i++)	// last 13.33mS: store it in new part of Sfiltertx
	{
		j = i % (2*bufsize);
		Sfiltertx[Sptrtx] = RRCcofstx[i] * fdmbuftx[j];
		Sptrtx = (Sptrtx + 1) % (7*bufsize);
	}

	for (i=0;i<bufsize;i++)		
	{
		val = (int)outbuf[i]; //v;
		if (val > 32767)		val = 32767;
		else if (val < -32767)  val = -32767;
		datout[i] = (short)val;
	}
}

//-------------------------------

double rxtolerance = 1.000;
double rrccofs[rrcsize];
int maxlevel;
double rxphases[ntones];
double rxfreqs[ntones];
int fil1ptr = 0;
double I1bufs[ntones][48];
double Q1bufs[ntones][48];
int fil2ptr = 0;
double I2bufs[ntones][128];
double Q2bufs[ntones][128];
double bitphase = 0.0;
double bitfreq;
double early = 0.0;
double late = 0.0;
double timcorr = 0.0;

double AFCsum = 0.0;
double pafcsum = 0.0;
double totAFCsum = 0.0;
double difAFCsum = 0.0;

BOOL asql,dsql,sql;

int round(double in)
{
	if (in >= 0.0)	return (int)(in + 0.5);
	else			return (int)(in - 0.5);
}

void setfreq()
{
	int i;
	for (i=0;i<ntones;i++)
		rxfreqs[i] = rxtolerance * (freq + spacing * i) / (double)rxsamplerate;
}


void DoAFC(double phase2, int tnr)	//process the differential phase residue
{
	double ph;
	ph = phase2 - round(phase2);
	// phase2 is -2 .. +2
	// ph is -0.5 .. +0.5
	// AFCsum is nocarr*ph = -7.5 .. +7.5
	AFCsum += ph;
	if (ph >= 0.0)	pafcsum += ph;
	else			pafcsum -= ph;
	if (tnr == ntones - 1)
	{
		freq += AFCsum * 0.01; //0.003;
		setfreq();
		totAFCsum = pafcsum;
		if (AFCsum >= 0.0)		difAFCsum = pafcsum - AFCsum;
		else					difAFCsum = pafcsum + AFCsum;
		AFCsum = 0.0;
		pafcsum = 0.0;
	}
}

int rxedarr[9];
int rxedc;
int rxedpt = 0;
int rxed[ntones];

BYTE isq = 0;

void doword(void)
{
	int i;
	int pct = 0;
	int tmp;
	isq = isq << 1;
	if (rxed[puretone] == 0) // copy 28 bits
	{
		isq &= 0xFE;
		setbitct(0);
		for (i=0;i<15;i++)
		{
			tmp = rxed[pct++];
			if (i != puretone) store2bit(tmp);
		}
	}
	else
	{
		isq |= 0x01;
		setbitct(28);
		for (i=0;i<15;i++)
		{
			tmp = rxed[pct++];
			if (i != puretone) store2bit(tmp);
		}
		setbitct(0);
		for (i=0;i<9;i++)
			rxedarr[i] = get6bit();
		//rxedc = get2bit();
		rxedpt++;
		dsql = (isq == 0x55);
	}
}

double lastphases[ntones];

void dosymbol(int tnr, double i, double q)
{
	int P,P1;
	double phase1, phase2, dphase, cphase;
	phase1 = atan2(i, q);
	phase2 = lastphases[tnr];
	dphase = phase1 - phase2;
	if (dphase >= pi)
		dphase -=  pi2;
	else if (dphase <= -pi)
		dphase += pi2;
	lastphases[tnr] = phase1;
	cphase = 2.0 * dphase / pi;
	DoAFC(cphase, tnr);
	if (tnr == puretone)
		P1 = round(0.5*cphase); // range -1 .. +1 -> 0 .. 1
	else
		P1 = round(cphase);		// range -2 .. +2 -> 0 .. 3
	P = P1 & 0x03;				//P now 0,1,2,3
	rxed[tnr] = P;
	if (tnr == ntones-1) doword();
}

double fibuf[32];
double fqbuf[32];
double fiibuf[32];
double fqqbuf[32];
int aind = 0;

double dsbuf[81];
int dsptr = 0;
int dsct = 0;

void rxFDM(short audio)
{
	double faudio;
	double i, q, s, c;
	int tnr, tap, ind;
	int level;
	// 12000 samp/sec
	// one symbol is 240 samples long
	level = abs(audio) / 320;
	if (level > maxlevel)	maxlevel = level; //for level meter
	// downsample to 6000
	// one symbol is 120 samples long
	dsbuf[dsptr] = (double)audio;
	dsptr  = (dsptr + 1) % 81;
	if (dsct == 1)
	{
		dsct = 0;
		return;
	}
	else dsct++;
	i = 0.0;
	ind = dsptr;
	for (tap=0;tap<81;tap++) 
	{
		i += dsbuf[ind] * fil81[tap];
		ind = (ind + 1) % 81;
	}
	faudio = i;

	for (tnr=0;tnr<ntones;tnr++)  //down-mix channels to baseband
	{
		rxphases[tnr] = frac(rxphases[tnr] + rxfreqs[tnr]);
		s = sin(2 * pi * rxphases[tnr]);
		c = cos(2 * pi * rxphases[tnr]);
		I1bufs[tnr][fil1ptr] = faudio * c;
		Q1bufs[tnr][fil1ptr] = faudio * s;
	}
	fil1ptr  = (fil1ptr + 1) % 48;
	bitphase = bitphase + bitfreq;
	// bitfreq = 0.13333 => 6000 / 7.5 = 800 samples / sec
	// one symbol is 16 samples long now !
	if (bitphase > 1.0) 
	{
		int ind1 = fil1ptr;
		bitphase = bitphase - 1.0;
		for (tnr=0;tnr<ntones;tnr++)
		{
			i = 0.0;
			q = 0.0;
			for (tap=0;tap<48;tap++) // first downsample filter
			{
				i = i + I1bufs[tnr][ind1] * fil48[tap];
				q = q + Q1bufs[tnr][ind1] * fil48[tap];
				ind1 = (ind1 + 1) % 48;
			}
			I2bufs[tnr][fil2ptr] = i;
			Q2bufs[tnr][fil2ptr] = q;
		}
		fil2ptr = (fil2ptr + 1) % 128;
		// 200 samples / sec
		// one symbol is 4 samples long now !
		if ((fil2ptr & 3) == 3) 
		{
			for (tnr=0;tnr<ntones;tnr++)
			{
				int ind2 = fil2ptr;
				i = 0.0;
				q = 0.0;

				for (tap=0;tap<128;tap++)	// main RRC filter
				{
					i = i + I2bufs[tnr][ind2] * rrccofs[tap];
					q = q + Q2bufs[tnr][ind2] * rrccofs[tap];
					ind2 = (ind2 + 1) % 128;
				}
				
				if ((fil2ptr & 15) == 3)
					early += i * i + q * q;	// first quarter of symbol
				else if ((fil2ptr & 15) == 7)
					dosymbol(tnr, i, q);			// at 44Hz: middle of symbol
				else if ((fil2ptr & 15) == 11)
					late += i * i + q * q;	// third quarter of symbol
				if (tnr == curcar)
				{
					int ind = (fil2ptr >> 2);
					fibuf[ind] = i;
					fqbuf[ind] = q;
					if ((fil2ptr& 15) == 7)
					{
						fiibuf[aind] = i;
						fqqbuf[aind] = q;
						aind = (aind + 1) % 32;
					}
				}
			}
			if ((fil2ptr& 15) == 15)				// symbol boundary
			{
				if ((early + late) > 0)
				{
					timcorr = (early - late) / (early + late); // adjust timing
					//fprintf(logf,"\n %f %f ",timcorr,bitphase);
					bitphase = bitphase + timcorr; //0.1 * timcorr
					// timcorr = 1.0  means timing correction of 1/16 symbol length
					// rxfreqs[i] has increment for 6000 samp/sec, 120 samp symb len.
					// 1/16 symb len -> 7.5 * rxfreqs[i].
					//lastphases[tnr] = timcorr * freq[tnr];
				}
				early = 0.0;
				late = 0.0;
			}
 		}
	}
}

//-------------------------------

void initme()
{
	int i;
	MakeRRCtx(0.5, RRCcofstx);
	MakeRRC(rolloff, rrccofs);
	for (i=0;i<ntones;i++)
	{
		freqstx[i]  = (basefreq + spacing * (double)i) / (double)txsamplerate;	// set up tone frequencies
		phasestx[i] = frac(i * i / ntones / 2.0);				// phase spreading for minimum crest factor
		Pstepstx[i] = frac(bufsize * freqstx[i]);				// phase step for each symbol
		rxfreqs[i]  = rxtolerance * (basefreq + spacing * i) / (double)rxsamplerate;
	}
	freq = basefreq;
	bitfreq = nbitfreq * rxtolerance;
}

/*--------------------------------------------------------------------
        SHOWWAVE_ONTIMER
        Handle WM_TIMER here. 
  --------------------------------------------------------------------*/

short rbuf[audbuffsize];
short tbuf[audbuffsize];

double rfbuf[audbuffsize];
double ibuf[specbuffsize];
double ispecbuf[specbuffsize];
double tmpbuf[specbuffsize];

BOOL rxrunning = TRUE;
BOOL txrunning = FALSE;

char rxcalls[20] = "           ";

static void my_OnTimer ( HWND hWnd, UINT id )
{
	int i;
	double mu;
	int maxi;

	for (i=0;i<audbuffsize;i++) rfbuf[i] = rbuf[i];
	ffthann(rfbuf,ibuf);
	
	for (i=100;i<specbuffsize-100;i++)
	{
		mu = (ibuf[i-2]+ibuf[i-1]) * (ibuf[i+1]+ibuf[i+2]);
		tmpbuf[i] = (tmpbuf[i] * 4.0f + (mu / 400.0)) * 0.2f;
	}
	if (nacq == 1)
	{
		mu = 0.0;
		for (i=100;i<specbuffsize-100;i++)
			if (tmpbuf[i] >= mu) { mu = tmpbuf[i]; maxi = i; }
		nacqpos = maxi;
		freq = (maxi - 84) * 6.25;
		setfreq();
		nacq = 0;
	}
	
	if (disptype == 2)
	{
		for (i=0;i<specbuffsize;i++)
			ispecbuf[i] = (ispecbuf[i] * 4.0f + ibuf[i]) * 0.2f;
	}
	if (disptype == 3)
	{
		for (i=0;i<specbuffsize;i++)
			ispecbuf[i] = tmpbuf[i];
	}
	if (disptype == 5)
	{
		for (i=0;i<specbuffsize;i++)
			ispecbuf[i] = ibuf[i];
	}
       
	PaintWaveData( hWnd );

	SendMessage(progbar, PBM_SETPOS, maxlevel, 0);
	maxlevel = 0;
	//SendMessage(progbar, PBM_SETPOS, (int)(10.0*difAFCsum), 0);

	if (dsql)	SendMessage (GetDlgItem (hWnd, IDC_RADIO1), BM_SETCHECK, (WPARAM)1, 0);
	else		SendMessage (GetDlgItem (hWnd, IDC_RADIO1), BM_SETCHECK, (WPARAM)0, 0);
	if (asql)	SendMessage (GetDlgItem (hWnd, IDC_RADIO2), BM_SETCHECK, (WPARAM)1, 0);
	else		SendMessage (GetDlgItem (hWnd, IDC_RADIO2), BM_SETCHECK, (WPARAM)0, 0);
	 
	//SetDlgItemInt( hWnd, IDC_EDIT1, (int)(100.0*timcorr), TRUE);

	return;
}

/*--------------------------------------------------------------------
        THREADS
        tx and rx thread.
  --------------------------------------------------------------------*/

#undef usefile
//#define usefile

BYTE sendstr[2*ntones];
BYTE instr[9];
FILE * audin;

WORD rxc;
int rxcct = 0;

struct melp_param melp_par_syn;
struct melp_param melp_par_ana;
unsigned int chbuf[9];
float fvoicerbuf[320];
short voicerbuf[audbuffsize];
voicebufct = 0;

void ThreadProc( HWND hWnd )
{
	int i,j;
	int indi,indo;
	int level;

	melp_ana_init();
	melp_syn_init();

	InitPlayback(audbuffsize,samplerate);
	InitRecording(audbuffsize,samplerate);

	while (TRUE)
	{
		SetOutDev(scoutdevrx,FALSE);
		SetInDev(scindevrx,TRUE);
		Sleep(100);
		while (rxrunning) 
		{
			Read(rbuf);
			for (j=0;j<8;j++)
			{
				for (i=0;i<bufsize;i++)	rxFDM(rbuf[j*bufsize+i]);
				asql = (difAFCsum < 1.5);
				sql = dsql | asql;
				//if (rxedpt != 0)
				if ((rxedpt != 0) && sql)
				{
					// process rxedarr[lpt]
					for (i=0;i<9;i++) chbuf[i] = rxedarr[i];
					melp_par_syn.chptr = chbuf;
					melp_par_syn.chbit = 0;
					melp_syn(&melp_par_syn,fvoicerbuf);
					indi = 0;
					indo = 2*bufsize*voicebufct;
					for (i=0;i<160;i++) 
					{
						double d1,d2;
						d1 = fvoicerbuf[indi++];
						d2 = fvoicerbuf[indi++];
						voicerbuf[indo++] = (short)d1;
						voicerbuf[indo++] = (short)((d1+d2)*0.5);
						voicerbuf[indo++] = (short)d2;
					}
					voicebufct++; 
					if (voicebufct >= 4)
					{
						Write(voicerbuf);
						voicebufct = 0;
					}
					rxedpt = 0;
				}
			}
		}
		SetOutDev(scoutdevtx,TRUE);
		SetInDev(scindevtx,FALSE);
		Sleep(100);
		if (txrunning) dotx(); 
		while (txrunning) 
		{
			Read(rbuf);

			for (j=0;j<4;j++)
			{
				
				indi = 480*j;
				indo = 0;
				for (i=0;i<160;i++) 
				{
					float d1,d2,d3;
					d1 = rbuf[indi++];
					d2 = rbuf[indi++];
					d3 = rbuf[indi++];
					fvoicerbuf[indo++] = d1;
					fvoicerbuf[indo++] = (float)((d2 + d3) * 0.5);
				}
				for (i=0;i<320;i++) 
				{
					level = abs((int)fvoicerbuf[i]) / 320;
					if (level > maxlevel)	maxlevel = level; //for level meter
				}
				melp_par_ana.chptr = chbuf;
				melp_par_ana.chbit = 0;
				melp_ana(fvoicerbuf, &melp_par_ana);
#ifdef usefile
				for (i=0;i<9;i++)	instr[i] = fgetc(audin);
#else
				for (i=0;i<9;i++) instr[i] = (BYTE)(chbuf[i]& 0x3f);
#endif
				storeword(instr);
				makesound(&tbuf[2*j*bufsize]);
				makesound(&tbuf[2*j*bufsize+bufsize]);
			}

			Write(tbuf);
		}
		endtx();
	}
}

/*--------------------------------------------------------------------
        SHOWWAVE_ONINITDIALOG
        Handle WM_INITDIALOG here. Initialize program settings.
  --------------------------------------------------------------------*/

HFONT   myfont;


static BOOL my_OnInitDialog (HWND hDlg, HWND hwndFocus, LPARAM lParam )
{

	wsprintf(callsign,"NOCALL");

	progbar = GetDlgItem(hDlg,IDC_PROGRESS1);

	set = fopen("sc.txt","rt");
	if (set != NULL)
	{
		char rubbish[100];
		fscanf(set,"%d %s",&scoutdevtx,&rubbish);
		fscanf(set,"%d %s",&scindevtx, &rubbish);
		fscanf(set,"%d %s",&scoutdevrx,&rubbish);
		fscanf(set,"%d %s",&scindevrx, &rubbish);
		fscanf(set,"%s %s",callsign, &rubbish);
	}

	SoundInit();
	SetOutDev(scoutdevrx,FALSE);
	SetInDev(scindevrx,TRUE);

	comport = gettxport();
	comtx(comport);

	Graph_Init(hDlg);
	fftinit();
	initme();

	SetTimer(hDlg,1,100,NULL);
	Sleep(1000);

    _beginthread(ThreadProc, 0, hDlg );

	SetDlgItemInt( hDlg, IDC_EDIT3, curcar, FALSE );

	return( TRUE );
    UNREFERENCED_PARAMETER( hwndFocus );
    UNREFERENCED_PARAMETER( lParam );
}

/*--------------------------------------------------------------------
        SHOWWAVE_ONCOMMAND
        Handle WM_COMMAND messages here.  Respond to actions from
        each dialog control and from the menu.
  --------------------------------------------------------------------*/


static void my_OnCommand (
    HWND hDlg,
    int iCmd,
    HWND hCtl,
    UINT uCode )
{
    switch (iCmd)
    {
		case IDM_EXIT:                      // end program
            FORWARD_WM_CLOSE( hDlg, PostMessage );
            break;

		case ID_SOUNDCARD_SOUNDCARD:
			DialogBox(TheInstance, MAKEINTRESOURCE (DLG_SOUNDCARDSEL), hDlg, SoundCardDlgProc);
			break;

		case ID_DISPLAY_SCOPE:
			disptype = 1;
			break;
		case ID_DISPLAY_SPECTRUM:
			disptype = 2;
			break;
		case ID_DISPLAY_IQ1:
			disptype = 3;
			break;
		case ID_DISPLAY_IQ2:
			disptype = 4;
			break;
		case ID_DISPLAY_WATERFALL:
			disptype = 5;
			break;

		case ID_COMPORT_NONE:
			comport = '0';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM1:
			comport = '1';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM2:
			comport = '2';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM3:
			comport = '3';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM4:
			comport = '4';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM5:
			comport = '5';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM6:
			comport = '6';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM7:
			comport = '7';	settxport(comport);	comtx(comport);
			break;
		case ID_COMPORT_COM8:
			comport = '8';	settxport(comport);	comtx(comport);
			break;

		case IDC_UPC:
			if (curcar <= ntones-2)	curcar++;
			SetDlgItemInt( hDlg, IDC_EDIT3, curcar, FALSE );
			break;
		case IDC_DNC:
			if (curcar >= 1)	curcar--;
			SetDlgItemInt( hDlg, IDC_EDIT3, curcar, FALSE );
			break;

		case IDC_TX:
			if (rxrunning)
			{
#ifdef usefile
				audin = fopen("audio.mlp","r+b");
#endif
				rxrunning = FALSE;
				txrunning = TRUE;
				SetDlgItemText( hDlg, IDC_TX, "RX" );
			}
			else
			{
				txrunning = FALSE;
				rxrunning = TRUE;
				SetDlgItemText( hDlg, IDC_TX, "TX" );
#ifdef usefile
				if (audin != NULL) fclose(audin);
#endif
			}
			break;

        default:
            break;
    }
    
    return;
    UNREFERENCED_PARAMETER( hCtl );
    UNREFERENCED_PARAMETER( uCode );
}

BOOL CALLBACK SoundCardDlgProc(HWND hwnd, UINT message, UINT wParam, LPARAM lParam)
{
	int i;
	HANDLE hlb;

    switch (message)
    {
    case WM_INITDIALOG:

		hlb = GetDlgItem(hwnd, IDC_LISTINTX);
		iNumDevOut = GetNumDevIn();
		ListBox_ResetContent(hlb);
		if (iNumDevIn >= 10) iNumDevIn = 10;
		for (i=0;i<iNumDevOut;i++) ListBox_AddString(hlb,GetDeviceNameIn(i));
		ListBox_SetCurSel(hlb,scindevtx);

		hlb = GetDlgItem(hwnd, IDC_LISTOUTTX);
		iNumDevOut = GetNumDevOut();
		ListBox_ResetContent(hlb);
		if (iNumDevOut >= 10) iNumDevOut = 10;
		for (i=0;i<iNumDevOut;i++) ListBox_AddString(hlb,GetDeviceNameOut(i));
		ListBox_SetCurSel(hlb,scoutdevtx);

		hlb = GetDlgItem(hwnd, IDC_LISTINRX);
		iNumDevIn = GetNumDevIn();
		ListBox_ResetContent(hlb);
		if (iNumDevIn >= 10) iNumDevIn = 10;
		for (i=0;i<iNumDevIn;i++) ListBox_AddString(hlb,GetDeviceNameIn(i));
		ListBox_SetCurSel(hlb,scindevrx);

		hlb = GetDlgItem(hwnd, IDC_LISTOUTRX);
		iNumDevOut = GetNumDevOut();
		ListBox_ResetContent(hlb);
		if (iNumDevOut >= 10) iNumDevOut = 10;
		for (i=0;i<iNumDevOut;i++) ListBox_AddString(hlb,GetDeviceNameOut(i));
		ListBox_SetCurSel(hlb,scoutdevrx);

		break;

    case WM_COMMAND:
		switch (wParam)
		{
		    case IDOK:
				scindevtx = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTINTX),LB_GETCURSEL, 0, 0); 
				scoutdevtx = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTOUTTX),LB_GETCURSEL, 0, 0); 
				scindevrx = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTINRX),LB_GETCURSEL, 0, 0);
				scoutdevrx = (int)SendMessage(GetDlgItem(hwnd,IDC_LISTOUTRX),LB_GETCURSEL, 0, 0);

				if (txrunning)
				{
					SetOutDev(scoutdevtx,TRUE);
					SetInDev(scindevtx,FALSE);
				}
				else
				{
					SetOutDev(scoutdevrx,FALSE);
					SetInDev(scindevrx,TRUE);
				}

				set = fopen("sc.txt","wt");
				if (set != NULL)
				{
					fprintf(set,"%d SCouttx\n",scoutdevtx);
					fprintf(set,"%d SCintx\n",scindevtx);
					fprintf(set,"%d SCoutrx\n",scoutdevrx);
					fprintf(set,"%d SCintx\n",scindevrx);
					fprintf(set,"%s Callsign\n",callsign);
				}
	 			EndDialog (hwnd, 0);
				return TRUE;
		}
		break;

    case WM_CLOSE:
 		EndDialog (hwnd, 0);
		return TRUE;
		break;

	default: 
		return FALSE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------
        SHOWWAVE_ONDESTROY
        Handle WM_DESTROY message here.  Close all devices.
  --------------------------------------------------------------------*/

static void my_OnDestroy ( HWND hDlg )
{
    PostQuitMessage( 0 );                           // send WM_QUIT
    return;
}



