/******************************************************************************\
 * Description:
 * Sound card interface for Windows operating systems
\******************************************************************************/

#include <stdio.h>
#include "Sound.h"

/* Implementation *************************************************************/
/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/

boolean Read(short * psData)
{
	int			i;
	boolean		bError;
	int			iNumInBufDone = 0;

	/* Check if device must be opened or reinitialized */
	if (bChangDevIn == TRUE)
	{
		OpenInDevice();

		/* Reinit sound interface */
		InitRecording(iBufferSizeIn,iSRateIn);

		/* Reset flag */
		bChangDevIn = FALSE;
	}

	/* Wait until data is available */
	if (!(m_WaveInHeader[iWhichBufferIn].dwFlags & WHDR_DONE))
	{
		if (bBlockingRec == TRUE)
			WaitForSingleObject(m_WaveInEvent, INFINITE);
		else
			return FALSE;
	}

	/* Check if buffers got lost */
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		if (m_WaveInHeader[i].dwFlags & WHDR_DONE)
			iNumInBufDone++;
	}

	/* If the number of done buffers equals the total number of buffers, it is
	   very likely that a buffer got lost -> set error flag */
	if (iNumInBufDone == NUM_SOUND_BUFFERS_IN)
		bError = TRUE;
	else
		bError = FALSE;

	/* Copy data from sound card in output buffer */
	for (i = 0; i < iBufferSizeIn; i++)
		psData[i] = psSoundcardBuffer[iWhichBufferIn][i];

	/* Add the buffer so that it can be filled with new samples */
	AddInBuffer();

	/* In case more than one buffer was ready, reset event */
	ResetEvent(m_WaveInEvent);

	return bError;
}

void AddInBuffer()
{
	/* Unprepare old wave-header */
	waveInUnprepareHeader(
		m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

	/* Prepare buffers for sending to sound interface */
	PrepareInBuffer(iWhichBufferIn);

	/* Send buffer to driver for filling with new data */
	waveInAddBuffer(m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

	/* Toggle buffers */
	iWhichBufferIn++;
	if (iWhichBufferIn == NUM_SOUND_BUFFERS_IN)
		iWhichBufferIn = 0;
}

void PrepareInBuffer(int iBufNum)
{
	/* Set struct entries */
	m_WaveInHeader[iBufNum].lpData = (LPSTR) &psSoundcardBuffer[iBufNum][0];
	m_WaveInHeader[iBufNum].dwBufferLength = iBufferSizeIn * BYTES_PER_SAMPLE;
	m_WaveInHeader[iBufNum].dwFlags = 0;

	/* Prepare wave-header */
	waveInPrepareHeader(m_WaveIn, &m_WaveInHeader[iBufNum], sizeof(WAVEHDR));
}

void InitRecording(int iNewBufferSize, int iSampleRate)
{
	int i;

	iSRateIn = iSampleRate;

	/* Check if device must be opened or reinitialized */
	if (bChangDevIn == TRUE)
	{
		OpenInDevice();

		/* Reset flag */
		bChangDevIn = FALSE;
	}

	/* Set internal parameter */
	iBufferSizeIn = iNewBufferSize;

	/* Reset interface so that all buffers are returned from the interface */
	waveInReset(m_WaveIn);
	waveInStop(m_WaveIn);
	
	/* Reset current buffer ID (it is important to do this BEFORE calling
	   "AddInBuffer()" */
	iWhichBufferIn = 0;

	/* Create memory for sound card buffer */
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		/* Unprepare old wave-header in case that we "re-initialized" this
		   module. Calling "waveInUnprepareHeader()" with an unprepared
		   buffer (when the module is initialized for the first time) has
		   simply no effect */
		waveInUnprepareHeader(m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

		if (psSoundcardBuffer[i] != NULL)
			free(psSoundcardBuffer[i]);

		psSoundcardBuffer[i] = malloc(2*iBufferSizeIn);

		/* Send all buffers to driver for filling the queue ----------------- */
		/* Prepare buffers before sending them to the sound interface */
		PrepareInBuffer(i);

		AddInBuffer();
	}

	/* Notify that sound capturing can start now */
	waveInStart(m_WaveIn);

	/* This reset event is very important for initialization, otherwise we will
	   get errors! */
	ResetEvent(m_WaveInEvent);
}

void OpenInDevice()
{
	MMRESULT result;
	/* Open wave-input and set call-back mechanism to event handle */
	if (m_WaveIn != NULL)
	{
		waveInReset(m_WaveIn);
		waveInClose(m_WaveIn);
	}

	sWaveFormatEx.nSamplesPerSec = iSRateIn;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign * sWaveFormatEx.nSamplesPerSec;

	result = waveInOpen(&m_WaveIn, iCurInDev, &sWaveFormatEx,
		(DWORD) m_WaveInEvent, (DWORD)NULL, CALLBACK_EVENT);
	if (result != MMSYSERR_NOERROR)
		printf("Sound Interface Start, waveInOpen() failed.");
}

void SetInDev(int iNewDev, BOOLEAN bBlocking)
{
	/* Set device to wave mapper if iNewDev is greater that the number of
	   sound devices in the system */
	if (iNewDev >= (int)iNumDevsIn)
		iNewDev = WAVE_MAPPER;

	bBlockingRec = bBlocking;

	/* Change only in case new device id is not already active */
	iCurInDev = iNewDev;
	bChangDevIn = TRUE;
}


/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
boolean Write(short * psData)
{
	int			i, j;
	int			iCntPrepBuf;
	int			iIndexDoneBuf;
	boolean		bError;

	/* Check if device must be opened or reinitialized */
	if (bChangDevOut == TRUE)
	{
		OpenOutDevice();

		/* Reinit sound interface */
		InitPlayback(iBufferSizeOut,iSRateOut);

		/* Reset flag */
		bChangDevOut = FALSE;
	}

	/* Get number of "done"-buffers and position of one of them */
	GetDoneBuffer(&iCntPrepBuf, &iIndexDoneBuf);

	/* Now check special cases (Buffer is full or empty) */
	if (iCntPrepBuf == 0)
	{
		if (bBlockingPlay == TRUE)
		{
			/* Blocking wave out routine. Needed for transmitter. Always
			   ensure that the buffer is completely filled to avoid buffer
			   underruns */
			while (iCntPrepBuf == 0)
			{
				WaitForSingleObject(m_WaveOutEvent, INFINITE);

				GetDoneBuffer(&iCntPrepBuf, &iIndexDoneBuf);
			}
			bError = FALSE;
		}
		else
		{
			/* All buffers are filled, dump new block ----------------------- */
			// It would be better to kill half of the buffer blocks to set the start
			// back to the middle: TODO
			return TRUE; /* An error occurred */
		}
	}
	else if (iCntPrepBuf == NUM_SOUND_BUFFERS_OUT)
	{
		/* ---------------------------------------------------------------------
		   Buffer is empty -> send as many cleared blocks to the sound-
		   interface until half of the buffer size is reached */
		/* Send half of the buffer size blocks to the sound-interface */
		for (j = 0; j < NUM_SOUND_BUFFERS_OUT / 2; j++)
		{
			/* First, clear these buffers */
			for (i = 0; i < iBufferSizeOut; i++)
				psPlaybackBuffer[j][i] = 0;

			/* Then send them to the interface */
			AddOutBuffer(j);
		}

		/* Set index for done buffer */
		iIndexDoneBuf = NUM_SOUND_BUFFERS_OUT / 2;

		bError = TRUE;
	}
	else
		bError = FALSE;

	/* Copy stereo data from input in soundcard buffer */
	for (i = 0; i < iBufferSizeOut; i++)
		psPlaybackBuffer[iIndexDoneBuf][i] = psData[i];

	/* Now, send the current block */
	AddOutBuffer(iIndexDoneBuf);

	return bError;
}

void GetDoneBuffer(int * iCntPrepBuf, int * iIndexDoneBuf)
{
	int i;
	/* Get number of "done"-buffers and position of one of them */
	*iCntPrepBuf = 0;
	for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		if (m_WaveOutHeader[i].dwFlags & WHDR_DONE)
		{
			*iCntPrepBuf += 1;
			*iIndexDoneBuf = i;
		}
	}
}

void AddOutBuffer(int iBufNum)
{
	/* Unprepare old wave-header */
	waveOutUnprepareHeader(
		m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));

	/* Prepare buffers for sending to sound interface */
	PrepareOutBuffer(iBufNum);

	/* Send buffer to driver for filling with new data */
	waveOutWrite(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void PrepareOutBuffer(int iBufNum)
{
	/* Set Header data */
	m_WaveOutHeader[iBufNum].lpData = (LPSTR) &psPlaybackBuffer[iBufNum][0];
	m_WaveOutHeader[iBufNum].dwBufferLength = iBufferSizeOut * BYTES_PER_SAMPLE;
	m_WaveOutHeader[iBufNum].dwFlags = 0;

	/* Prepare wave-header */
	waveOutPrepareHeader(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void InitPlayback(int iNewBufferSize, int iSampleRate)
{
	int	i, j;

	iSRateOut = iSampleRate;

	/* Check if device must be opened or reinitialized */
	OpenOutDevice();

	/* Reset flag */
	bChangDevOut = FALSE;

	/* Set internal parameters */
	iBufferSizeOut = iNewBufferSize;

	/* Reset interface */
	waveOutReset(m_WaveOut);

	for (j = 0; j < NUM_SOUND_BUFFERS_OUT; j++)
	{
		/* Unprepare old wave-header (in case header was not prepared before,
		   simply nothing happens with this function call */
		waveOutUnprepareHeader(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));

		/* Create memory for playback buffer */

		if (psPlaybackBuffer[j] != NULL)
			free(psPlaybackBuffer[j]);

		psPlaybackBuffer[j] = malloc(2*iBufferSizeOut);

		/* Clear new buffer */
		for (i = 0; i < iBufferSizeOut; i++)
			psPlaybackBuffer[j][i] = 0;

		/* Prepare buffer for sending to the sound interface */
		PrepareOutBuffer(j);

		/* Initially, send all buffers to the interface */
		AddOutBuffer(j);
	}
}

void OpenOutDevice()
{
	MMRESULT result;
	if (m_WaveOut != NULL)
	{
		waveOutReset(m_WaveOut);
		waveOutClose(m_WaveOut);
	}

	sWaveFormatEx.nSamplesPerSec = iSRateOut;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign * sWaveFormatEx.nSamplesPerSec;

	result = waveOutOpen(&m_WaveOut, iCurOutDev, &sWaveFormatEx,
		(DWORD) m_WaveOutEvent, (DWORD)NULL, CALLBACK_EVENT);

	if (result != MMSYSERR_NOERROR)
		printf("Sound Interface Start, waveOutOpen() failed.");
}

void SetOutDev(int iNewDev, BOOLEAN bBlocking)
{
	/* Set device to wave mapper if iNewDev is greater that the number of
	   sound devices in the system */
	if (iNewDev >= (int)iNumDevsOut)
		iNewDev = WAVE_MAPPER;

	bBlockingPlay = bBlocking;

	/* Change only in case new device id is not already active */
	iCurOutDev = iNewDev;
	bChangDevOut = TRUE;
}


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
void Close()
{
	int			i;
	MMRESULT	result;

	/* Reset audio driver */
	if (m_WaveOut != NULL)
	{
		result = waveOutReset(m_WaveOut);
		if (result != MMSYSERR_NOERROR)
			printf("Sound Interface, waveOutReset() failed.");
	}

	if (m_WaveIn != NULL)
	{
		result = waveInReset(m_WaveIn);
		if (result != MMSYSERR_NOERROR)
			printf("Sound Interface, waveInReset() failed.");
	}

	/* Set event to ensure that thread leaves the waiting function */
	if (m_WaveInEvent != NULL)
		SetEvent(m_WaveInEvent);

	/* Wait for the thread to terminate */
	Sleep(500);

	/* Unprepare wave-headers */
	if (m_WaveIn != NULL)
	{
		for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
		{
			result = waveInUnprepareHeader(
				m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

			if (result != MMSYSERR_NOERROR)
				printf("Sound Interface, waveInUnprepareHeader() failed.");
		}

		/* Close the sound in device */
		result = waveInClose(m_WaveIn);
		if (result != MMSYSERR_NOERROR)
			printf("Sound Interface, waveInClose() failed.");
	}

	if (m_WaveOut != NULL)
	{
		for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
		{
			result = waveOutUnprepareHeader(
				m_WaveOut, &m_WaveOutHeader[i], sizeof(WAVEHDR));

			if (result != MMSYSERR_NOERROR)
				printf("Sound Interface, waveOutUnprepareHeader() failed.");
		}

		/* Close the sound out device */
		result = waveOutClose(m_WaveOut);
		if (result != MMSYSERR_NOERROR)
			printf("Sound Interface, waveOutClose() failed.");
	}

	/* Set flag to open devices the next time it is initialized */
	bChangDevIn = TRUE;
	bChangDevOut = TRUE;
}

int	GetNumDevIn() 
{
	return iNumDevsIn;
}
int	GetNumDevOut() 
{
	return iNumDevsOut;
}

char * GetDeviceNameIn(int iDiD)
{
	return pstrDevicesIn[iDiD];
}
char * GetDeviceNameOut(int iDiD)
{
	return pstrDevicesOut[iDiD];
}

void SoundInit()
{
	int i;

	/* Should be initialized because an error can occur during init */
	m_WaveInEvent = NULL;
	m_WaveOutEvent = NULL;
	m_WaveIn = NULL;
	m_WaveOut = NULL;

	/* Init buffer pointer to zero */
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		memset(&m_WaveInHeader[i], 0, sizeof(WAVEHDR));
		psSoundcardBuffer[i] = NULL;
	}

	for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		memset(&m_WaveOutHeader[i], 0, sizeof(WAVEHDR));
		psPlaybackBuffer[i] = NULL;
	}

	/* Init wave-format structure */
	sWaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormatEx.nChannels = NUM_IN_OUT_CHANNELS;
	sWaveFormatEx.wBitsPerSample = BITS_PER_SAMPLE;
	sWaveFormatEx.nSamplesPerSec = 48000;
	sWaveFormatEx.nBlockAlign = sWaveFormatEx.nChannels *
		sWaveFormatEx.wBitsPerSample / 8;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign *
		sWaveFormatEx.nSamplesPerSec;
	sWaveFormatEx.cbSize = 0;

	/* Get the number of digital audio devices in this computer, check range */
	iNumDevsIn = waveInGetNumDevs();
	iNumDevsOut = waveOutGetNumDevs();

	if (iNumDevsIn > MAX_NUMBER_SOUND_CARDS)
		iNumDevsIn = MAX_NUMBER_SOUND_CARDS;
	if (iNumDevsOut > MAX_NUMBER_SOUND_CARDS)
		iNumDevsOut = MAX_NUMBER_SOUND_CARDS;

	/* At least one device must exist in the system */
	if ((iNumDevsIn == 0) || (iNumDevsOut == 0))
		printf("No audio device found.");

	/* Get info about the devices and store the names */
	for (i = 0; i < (int)iNumDevsIn; i++)
		if (!waveInGetDevCaps(i, &m_WaveInDevCaps, sizeof(WAVEINCAPS)))
		{
			strcpy(pstrDevicesIn[i],m_WaveInDevCaps.szPname);
		}
	for (i = 0; i < (int)iNumDevsOut; i++)
		if (!waveOutGetDevCaps(i, &m_WaveOutDevCaps, sizeof(WAVEOUTCAPS)))
		{
			strcpy(pstrDevicesOut[i],m_WaveOutDevCaps.szPname);
		}

	/* We use an event controlled wave-in (wave-out) structure */
	/* Create events */
	m_WaveInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_WaveOutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Set flag to open devices */
	bChangDevIn = TRUE;
	bChangDevOut = TRUE;

	/* Default device number (first device in system) */
	iCurInDev = 0;
	iCurOutDev = 0;

	/* Non-blocking wave out is default */
	bBlockingPlay = FALSE;

	/* Blocking wave in is default */
	bBlockingRec = TRUE;
}

void SoundDestroy()
{
	int i;

	/* Delete allocated memory */
	for (i = 0; i < NUM_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			free(psSoundcardBuffer[i]);
	}

	for (i = 0; i < NUM_SOUND_BUFFERS_OUT; i++)
	{
		if (psPlaybackBuffer[i] != NULL)
			free(psPlaybackBuffer[i]);
	}

	/* Close the handle for the events */
	if (m_WaveInEvent != NULL)
		CloseHandle(m_WaveInEvent);

	if (m_WaveOutEvent != NULL)
		CloseHandle(m_WaveOutEvent);
}
