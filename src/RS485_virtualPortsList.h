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

typedef struct virtualPortsListItem virtualPortsListItem_t;

typedef struct virtualPortsListItem {
	virtualPort_t          vpObj;
	virtualPortsListItem_t *next;
	virtualPortsListItem_t *prev;
} virtualPortsListItem_t;

typedef struct {
	virtualPortsListItem_t *head;
	virtualPortsListItem_t *tail;
	virtualPortsListItem_t *it;
} virtualPortsList_t;

void              init_virtualPortsListItem (virtualPortsListItem_t *item);

void              init_virtualPortsList     (virtualPortsList_t *list);
RS485emErrorCodes add_virtualPortsList      (virtualPortsList_t *list);
RS485emErrorCodes update_virtualPortsList   (virtualPortsList_t *list, const char **busports);
RS485emErrorCodes pidsChk_virtualPortsList  (virtualPortsList_t *list);
virtualPort_t*    next_virtualPortsList     (virtualPortsList_t *list);
void              resetIT_virtualPortsList  (virtualPortsList_t *list);
void              free_virtualPortsList     (virtualPortsList_t *list);
void              print_virtualPortsList    (virtualPortsList_t *list);


#endif

