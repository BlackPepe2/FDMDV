/******************************************************************************\
 * Description:
 *	See Sound.c
\******************************************************************************/


#include <windows.h>
#include <mmsystem.h>


/* Definitions ****************************************************************/

#define	NUM_IN_OUT_CHANNELS		1		/* Mono recording */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NUM_SOUND_BUFFERS_IN	4		/* Number of sound card buffers */

#define NUM_SOUND_BUFFERS_OUT	4		/* Number of sound card buffers */

/* Maximum number of recognized sound cards installed in the system */
#define MAX_NUMBER_SOUND_CARDS	8


/********************************************************************/
	void SoundInit(void);
	void SoundDestroy(void);

	void		InitRecording(int iNewBufferSize, int iSampleRate);
	void		InitPlayback(int iNewBufferSize, int iSampleRate);
	boolean		Read(short * psData);
	boolean		Write(short * psData);

	int			GetNumDevIn();
	char *		GetDeviceNameIn(int iDiD);
	void		SetInDev(int iNewDev, BOOLEAN bBlocking);
	int			GetNumDevOut();
	char *		GetDeviceNameOut(int iDiD);
	void		SetOutDev(int iNewDev, BOOLEAN bBlocking);

	void		Close();

/********************************************************************/
	void		OpenInDevice();
	void		OpenOutDevice();
	void		PrepareInBuffer(int iBufNum);
	void		PrepareOutBuffer(int iBufNum);
	void		AddInBuffer();
	void		AddOutBuffer(int iBufNum);
	void		GetDoneBuffer(int * iCntPrepBuf, int * iIndexDoneBuf);

	WAVEFORMATEX	sWaveFormatEx;
	UINT			iNumDevsIn;
	UINT			iNumDevsOut;
	char			pstrDevicesIn[MAX_NUMBER_SOUND_CARDS][100];
	char			pstrDevicesOut[MAX_NUMBER_SOUND_CARDS][100];
	UINT			iCurInDev;
	UINT			iCurOutDev;
	BOOLEAN			bChangDevIn;
	BOOLEAN			bChangDevOut;

	/* Wave in */
	WAVEINCAPS		m_WaveInDevCaps;
	HWAVEIN			m_WaveIn;
	HANDLE			m_WaveInEvent;
	WAVEHDR			m_WaveInHeader[NUM_SOUND_BUFFERS_IN];
	int				iBufferSizeIn;
	int				iWhichBufferIn;
	short*			psSoundcardBuffer[NUM_SOUND_BUFFERS_IN];
	boolean			bBlockingRec;
	int				iSRateIn;

	/* Wave out */
	WAVEOUTCAPS		m_WaveOutDevCaps;
	int				iBufferSizeOut;
	HWAVEOUT		m_WaveOut;
	short*			psPlaybackBuffer[NUM_SOUND_BUFFERS_OUT];
	WAVEHDR			m_WaveOutHeader[NUM_SOUND_BUFFERS_OUT];
	HANDLE			m_WaveOutEvent;
	boolean			bBlockingPlay;
	int				iSRateOut;

/********************************************************************/


