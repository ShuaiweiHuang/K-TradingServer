#include <cstdio>
#include <iostream>
#include <ctime>
#include <sys/time.h>
#define COLOR_BLACK      "\033[1;30m"
#define COLOR_RED        "\033[1;31m"
#define COLOR_GREEN      "\033[1;32m"
#define COLOR_YELLOW     "\033[1;33m"
#define COLOR_BLUE       "\033[1;34m"
#define COLOR_MANGENTA   "\033[1;35m"
#define COLOR_CYAN       "\033[1;36m"
#define COLOR_WHITE      "\033[1;37m"
#define COLOR_REST       "\033[m"
#define COLOR_REST_N     "\033[m\n"

void FprintfStderrLog(const char* pCause, int nError, unsigned char* pMessage1, int nMessage1Length, unsigned char* pMessage2 = NULL, int nMessage2Length = 0)
{
    struct timeval tv;
	//time_t t = time(NULL);
	gettimeofday(&tv,NULL);
	struct tm tm = *localtime(&tv.tv_sec);

	fprintf(stderr, "[%02d:%02d:%02d'%06d]", tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);

	fprintf(stderr, "[%s][%d]", pCause, nError);
	fprintf(stderr, "[");

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

	fprintf(stderr, "]\n");
}
