
#include <stdio.h>
#include <math.h>
#include "fft.h"


//----------------------------------------------

const double fftdpi = 3.1415926535897932;
const double fftdpi2 = 2.0 * 3.1415926535897932;


double sinbuf[fftcarr][fftlen];
double cosbuf[fftcarr][fftlen];
double hann[fftlen];
double flattop[fftlen];

void fft(double * usefuldata, double * sumval)
{
	int k,i;
	double sumi,sumq,sample;
	double ival,qval;
	*(sumval+0) = 0.0;
	for (k=1;k<fftcarr;k++)	// for all carriers
	{
		sumi = 0.0;
		sumq = 0.0;
		for (i=0;i<fftlen;i++)	
		{
			sample = (double)*(usefuldata + i);
			sumi += sinbuf[k][i] * sample;
			sumq += cosbuf[k][i] * sample;
		}
		ival = sumi/fftlen;
		qval = sumq/fftlen;
		*(sumval+k) = sqrt(ival*ival + qval*qval);
	}
	return;
}

void ffthann(double * usefuldata, double * sumval)
{
	int k,i;
	double sumi,sumq,sample;
	double ival,qval;
	*(sumval+0) = 0.0;
	for (k=1;k<fftcarr;k++)	// for all carriers
	{
		sumi = 0.0;
		sumq = 0.0;
		for (i=0;i<fftlen;i++)	
		{
			sample = hann[i] * (double)*(usefuldata + i);
			//sample = flattop[i] * (double)*(usefuldata + i);
			sumi += sinbuf[k][i] * sample;
			sumq += cosbuf[k][i] * sample;
		}
		ival = sumi/fftlen;
		qval = sumq/fftlen;
		*(sumval+k) = sqrt(ival*ival + qval*qval);
	}
	return;
}

void fftinit()
{
	double step,ang = 0.0;
	double a1 = 1.93;
	double a2 = 1.29;
	double a3 = 0.388;
	double a4 = 0.032;
	int k,i;
	for (k=0;k<fftcarr;k++)	// for all carriers
	{
		step = (double)(k) * (fftdpi2/fftlen);	// init step size (carrier freq.)
		ang = 0.0;
		for (i=0;i<fftlen;i++)	
		{
			sinbuf[k][i] = sin(ang);
			cosbuf[k][i] = cos(ang);
			ang += step;						// next angle
			if (ang >= fftdpi) ang -= fftdpi2;	// close circle
		}
	}
	step = fftdpi2/fftlen;	// init step size (carrier freq.)
	ang = 0.0;
	for (i=0;i<fftlen;i++)	
	{
		hann[i] = 0.5 * (1.0-cos(ang));
		flattop[i] = 1.0 - a1*cos(ang) + a2*cos(2.0*ang) - a3*cos(3.0*ang) + a4*cos(4.0*ang);
		ang += step;						// next angle
		if (ang >= fftdpi) ang -= fftdpi2;	// close circle
	}
}

