//------------------------------------------------------------------------------------------------------------------------------
//
// Authour:   Silvano Catinella
//
// Description
//	This file contains useful macro to be used in coide debug session
//
//	Macro:
//		DBGTRACE
//			It prints a message to show you where is the currently executed instruction. It allow you to avoid the common
//			printf("pippo\n);
//
//		ERRORBANNER(<error code>) 
//			It prints an error banner with the function-name and the the row number
//
//
// Editor parameters: 128 cols, ts=6
//------------------------------------------------------------------------------------------------------------------------------
#ifndef DEBUGTOOLS
#define DEBUGTOOLS

#define DBGTRACE \
    fprintf(stdout, "---> %s::%s() pid=%d line=%d\n", __FILE__, __func__, getpid(), __LINE__); fflush(stdout);

#define ERRORBANNER(x)                                                                           \
	fprintf(stderr, "\n********************  \033[1;31mERROR\033[0m  ********************\n"); \
	fprintf(stderr, "File:      %s\n",   __FILE__);                                            \
	fprintf(stderr, "Funtion:   %s()\n", __func__);                                            \
	fprintf(stderr, "Line:      %d\n",   __LINE__);                                            \
	fprintf(stderr, "PID:       %d\n",  getpid());                                             \
	fprintf(stderr, "Exit code: %d\n",   x);                                                   \
	fprintf(stderr, "\n");                                                                     \
	fflush(stderr);


#define DBGBREAK \
	{printf("\nPress [RETURN] to continue\n\n"); fflush(stdout); getchar();}

#endif 
