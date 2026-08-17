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
// File:     libForTests.h
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

#ifndef LIB4TETS

#define LIB4TETS

#include <stdint.h>
#include <glib.h>

void         stringInversion    (char *sentence);
void         randomString       (char *data, uint8_t *size);
unsigned int randomInt          (unsigned int scale);
	
void         print_stringList   (const GPtrArray *list);
unsigned int split_stringList   (GPtrArray *list, const char *src, const char *splitter);


#endif
