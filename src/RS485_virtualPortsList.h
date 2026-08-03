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
// File:     RS485_virtualPortsList.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/

#ifndef VIRTUALPORTSLIST

#define VIRTUALPORTSLIST

#include <RS485_virtualPort.h>

struct virtualPortsListItem {
	struct virtualPort          vpObj;
	struct virtualPortsListItem *next;
	struct virtualPortsListItem *prev;
};

struct virtualPortsList {
	struct virtualPortsListItem *head;
	struct virtualPortsListItem *tail;
	struct virtualPortsListItem *it;
};

void   init_virtualPortsListItem (struct virtualPortsListItem *item);

void                init_virtualPortsList     (struct virtualPortsList *list);
RS485emErrorCodes   add_virtualPortsList      (struct virtualPortsList *list);
RS485emErrorCodes   update_virtualPortsList   (struct virtualPortsList *list, const char **busports);
RS485emErrorCodes   pidsChk_virtualPortsList  (struct virtualPortsList *list);
struct virtualPort* next_virtualPortsList     (struct virtualPortsList *list);
void                resetIT_virtualPortsList  (struct virtualPortsList *list);
void                free_virtualPortsList     (struct virtualPortsList *list);
void                print_virtualPortsList    (struct virtualPortsList *list);


#endif

