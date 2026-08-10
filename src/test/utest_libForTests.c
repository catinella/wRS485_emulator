/*-------------------------------------------------------------------------------------------------------------------------------
//
//                                                 ____  ____  _  _    ___ ____  
//                                                |  _ \/ ___|| || |  ( _ ) ___| 
//                                                | |_) \___ \| || |_ / _ \___ \
//                                                |  _ < ___) |__   _| (_) |__) |
//                                                |_| \_\____/   |_|  \___/____/ 
//                                                           Emulator         
//
// 
// File:     utest_libForTests.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	Unit tests for libForTests.c
//
//
//
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <minute.h>
#include <libForTests.h>


TEST (libForTests_testingSuite, randomString_test) {
	char    data1[256];
	char    data2[256];
	uint8_t size1 = 127;
	uint8_t size2 = 127;
	
	randomString (data1, &size1);
	randomString (data2, &size2);
	printf("[%s]\n[%s]\n\n", data1, data2);
	ASSERT_NE (strcmp(data1, data2), 0);

	return;
}

TEST (libForTests_testingSuite, stringInversion_test) {
	//
	// Description:
	//
	char *oddString  = "123456789";
	char *iodString  = "987654321";
	char *evenString = "abcdef";
	char *ievString  = "fedcba";
	char result[32];

	strcpy(result, oddString);
	stringInversion(result);
	ASSERT_EQ (strcmp(result, iodString), 0);

	stringInversion(result);
	ASSERT_EQ (strcmp(result, oddString), 0);

	strcpy(result, evenString);
	stringInversion(result);
	ASSERT_EQ (strcmp(result, ievString), 0);

	stringInversion(result);
	ASSERT_EQ (strcmp(result, evenString), 0);

	{
		char data1[256];
		char data2[256];
		randomString (data1, NULL);
		strcpy(data2, data1);
		stringInversion(data1);
		ASSERT_NE (strcmp(data1, data2), 0);
		stringInversion(data1);
		ASSERT_EQ (strcmp(data1, data2), 0);
	}
}

TEST (libForTests_testingSuite, randomInt_test) {
	uint8_t w, k, x;
	bool    flag = false;
	
	for (w=0; w<10; w++) {
		k = randomInt(255);
		if (flag){
			ASSERT_NE (x, k);
		} else {
			flag = true;
		}
		x = k;
	}
	
	return;
}

#include "utest_libForTests__main.sgc"
