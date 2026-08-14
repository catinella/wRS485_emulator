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
// File:     libForTests.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	It is a simple set of general purples functions used just by testing modules
//
//
//
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <libForTests.h>

#define FIRSTASCIICHAR 60
#define LASTASCIICHAR  124

static bool randInitFlag = false;

//------------------------------------------------------------------------------------------------------------------------------
//                                           P R I V A T E   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------
void srandInit() {
	if (randInitFlag == false) {
		// srand() should only be called one time only.
		srand(time(NULL));
		randInitFlag = true;
	}
	return;
}

//------------------------------------------------------------------------------------------------------------------------------
//                                             P U B L I C   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------
void stringInversion (char *data) {
	//
	// Description:
	//	It inverts the characters order inside the argument defined string
	//
	uint16_t x     = 0;
	uint16_t psize = strlen(data);
	div_t    hs    = div(psize, 2);
	char     tmp;
				
	for (x=0; x<hs.quot; x++) {
		tmp = data[x];
		data[x] = data[psize-1-x];
		data[psize-1-x] = tmp;
	}
	return;
}


void randomString (char *data, uint8_t *size) {
	//
	// Description:
	//	It calculates a randomic string. The string size must be from 4 to 255
	//
	uint8_t c = 0;
	uint8_t ts = 0;

	srandInit();
	
	if (size == NULL)
		ts = (uint8_t)( + ((float)rand()/RAND_MAX)*251);
	else
		ts = *size;
		
	for (c=0; c<ts; c++) 
		data[c] = (int8_t)(roundf(FIRSTASCIICHAR + ((float)rand()/RAND_MAX) * (LASTASCIICHAR - FIRSTASCIICHAR)));
	
	data[c] = '\0';
	//printf("Randomic string: %s\n", sdata);

	return;
}


unsigned int randomInt (unsigned int scale) {
	//
	// Description:
	//	Randomic integer number generator
	//
	srandInit(); 
	return((unsigned int)roundf(((float)rand()/RAND_MAX)*scale));
}


void init_stringList (char **pList, unsigned int noi) {
	for (unsigned int t = 0; t < noi; t++) pList[t] = NULL;
	return;
}


unsigned int getSize_stringList (const char **list) {
	//
	// Description:
	//	It returns the size of argument defined ports list 
	//
	unsigned int t = 0;
	while (list[t] != NULL) t++;
	return(t);
}


void free_stringList (char **list) {
	//
	// Description:
	//	It releases the memory resources used by the chars-string items
	//
	unsigned int t = 0;
	while (list[t] != NULL) {
		free(list[t]);
		list[t] = NULL;
		t++;
	}
	return;
}


void print_stringList (const char **list) {
	unsigned int t = 0;
	while (list[t] != NULL) {
		printf("%s\n", list[t]);
		t++;
	}
	return;
}
