#include <cstdio>
#include <iostream>
#include <ctime>
#include<sys/time.h>

void FprintfStderrLog(const char* pCause, int nError, int nData, const char* pFile = NULL, int nLine = 0,
										unsigned char* pMessage1 = NULL, int nMessage1Length = 0, 
										unsigned char* pMessage2 = NULL, int nMessage2Length = 0)
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	struct tm tm = *localtime(&tv.tv_sec);

	fprintf(stderr, "[%02d:%02d:%02d'%06d]", tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
	fprintf(stderr, "[%s][%d,%d]", pCause, nError, nData);
	fprintf(stderr, "%s:%d", pFile, nLine);
	fprintf(stderr, "{");

	for(int i=0;i<nMessage1Length;i++)
	{
		if(isprint(pMessage1[i]))
		{
			fprintf(stderr, "%c", pMessage1[i]);
		}
		else
		{
			fprintf(stderr, "[%#x]", pMessage1[i]);
		}
	}
	fprintf(stderr, "}");

	fprintf(stderr, "{");
	for(int i=0;i<nMessage2Length;i++)
	{
		if(isprint(pMessage2[i]))
		{
			fprintf(stderr, "%c", pMessage2[i]);
		}
		else
		{
			fprintf(stderr, "[%#x]", pMessage2[i]);
		}
	}

	fprintf(stderr, "}\n");
}
