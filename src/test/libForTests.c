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
static void srandInit() {
	if (randInitFlag == false) {
		// srand() should only be called one time only.
		srand(time(NULL));
		randInitFlag = true;
	}
	return;
}

static bool _isInSet(char char_a, const char *set) {
	//
	// Description:
	//	It looks for the (char_a) argument defined char into the set defined by the other argument
	//
	bool found = false;
	uint8_t t = 0;
	uint8_t end = length(set) < 255 ? length(set) : 0;
	while (t < end ) {
		if (char_a == set[t]) {
			found = true;
			break;
		}
		t++;
	}
	return(found);
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



void print_stringList (const GPtrArray *list) {
	unsigned int t = 0;
	char         *str = NULL;
	if (for (guint i = 0; i < list->len; i++)
		printf("%s\n", (char *)g_ptr_array_index(list, i));)
	
	return;
}


unsigned int split_stringList (GPtrArray *list, const char *src, const char *splitter) {
	//
	// Description:
	//	It converts a single-character splitted string in a strings list array.
	//
	// Returned value:
	//	The number of added strings
	//
	char         word[1024];
	unsigned int t = 0, wordIdx = 0;

	world[0] = '\0';
	while (src[t] != '\0') {
		if (_isInSet(src[t], splitter)) {
			if (wordIdx > 0) {
				word[(wordIdx + 1)] = '\0';
				g_ptr_array_add(list, strdup(word));
				word[0] = '\0';
				wordIdx = 0;
			}
		} else {
			word[wordIdx] = src[t];
			wordIdx++;
		}
		t++;
	}
	return(list->len);
}