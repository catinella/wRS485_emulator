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
//		ASSERT_EQ()
//			If you are using google tests suite then this simbol is already set and nothing will be changed. But if the
//			symbol is not set then the Google ASSERT_<op> symbols will be substituted with the custom versions defined
//			in this header file. In this case also the GTEST_DISABLED symbol will be defined, and you will be able to use
//			it to verify which library is running effectively.
//			This custom version prints a coloured message for every ASSERT_*
//			It is useful when you compile your test with GDB support, but you don't want to rebuild libgtest.so
//
//
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

//
// Google Tests library mocking
//
#ifndef ASSERT_EQ

	#define OKSYMB  "[\033[1;32m  OK  \e[0m]"
	#define ERRSYMB "[\033[1;31mERROR!\e[0m]"

	#define ASSERT_EQ(x, y) \
		if (x == y) printf("%s", OKSYMB); else  printf("%s", ERRSYMB); \
		printf(" %s(%d)\n", __func__, __LINE__);

	#define ASSERT_NE(x, y) \
		if (x != y) printf("%s", OKSYMB); else  printf("%s", ERRSYMB); \
		printf(" %s(%d)\n", __func__, __LINE__);


	#define TEST(x, y)  void y() 

	#define GTEST_DISABLED 1

#endif


#define DBGBREAK \
	{printf("\nPress [RETURN] to continue\n\n"); fflush(stdout); getchar();}

#endif 
