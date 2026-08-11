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
// File:     RS485_virtualPortsList.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This module provides useful functions to handle a list of virtual ports
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <debugTools.h>
#include <RS485_emulator.h>
#include <RS485_virtualPortsList.h>
#include <RS485_portsDB.h>

static bool initDBflag = false;

void init_virtualPortsListItem (virtualPortsListItem_t *item) {
	//
	// Description:
	//	It initializes the argument defined struct virtualPortsListItem object
	//
	init_virtualPort(&(item->vpObj));
	item->next = NULL;
	item->prev = NULL;
	return;
}


void init_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	It initializes the argument defined struct virtualPortsList object
	//
	list->head = NULL;
	list->tail = NULL;
	list->it   = NULL;

	if (initDBflag == false) {
		init_portsDB();
		initDBflag = true;
	}
	
	return;
}

void print_virtualPortsList (virtualPortsList_t *list) {
	virtualPortsListItem_t *ptr = NULL;
	for (ptr = list->head; ptr != NULL; ptr = ptr->next)
		printf("%p: %s    (prev=%p  next=%p)\n", ptr, ptr->vpObj.port, ptr->prev, ptr->next);
	return;
}


RS485emErrorCodes add_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	It creates a new emply virtual port, it uses the port to make a list item, and add the item to the list tail
	//
	// Returned code:
	//	see the create_virtualPort() documentation
	//
	virtualPortsListItem_t *item = NULL;
	RS485emErrorCodes       err   = RS485EMULE_SUCCESS;

	// List item creation
	item = (virtualPortsListItem_t*)malloc(sizeof(virtualPortsListItem_t));
	init_virtualPortsListItem(item);
	err = create_virtualPort(&(item->vpObj), RS485EMULE_PORTSLAVE);

	// Item linking to the list
	if (list->tail == NULL) {
		list->head = item;
		list->tail = item;
	} else {
		list->tail->next = item;
		item->prev = list->tail;
		list->tail = item;
	}
	
	return(err);
}


RS485emErrorCodes update_virtualPortsList (virtualPortsList_t *list, const char **busports) {
	//
	// Description:
	//	It accepts a list of file names of assigned ports and set the argument defined virtual ports list considering the
	//	files names set. To make the iteration faster, all not-yet-assigned ports are stored in the bottom side of the list.
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_INTERNAL
	//
	virtualPortsListItem_t *item = NULL;
	virtualPort_t          *myport = NULL;
	int                    i     = 0;
	RS485emErrorCodes      err   = RS485EMULE_SUCCESS;
	
	if (list->head == NULL || list->tail == NULL)
		// ERROR! (the list is empty)
		err = RS485EMULE_ERROR_INTERNAL;
		
	else {
		bool found;
		resetIT_virtualPortsList(list);
		myport = list->head ? &(list->head->vpObj) : NULL;
		
		while (err == RS485EMULE_SUCCESS) {
			i     = 0;
			found = false;

			item = list->it;
			myport = next_virtualPortsList(list);
			if (myport == NULL) break;
			
			//printf("Checking for %s port..\n", myport->port);
			while (found == false && busports[i] != NULL) {
				//DBGTRACE
				if (strcmp(busports[i], myport->port) == 0) {
					found = true;;
					break;
				}
				i++;
			}
			

			if (found && myport->fd <= 0) {
				//
				// Virtual port has been assigned to
				//
				err = open_virtualPort(myport);
				

			} else if (found == false && myport->fd > 0) {
				//
				// Virtual port released by the user
				//
				close_virtualPort(myport);

				// In the "item == tail" case there is nothing to do
				if (item->next != NULL) {
					item->next->prev = item->prev;
				
					if (item->prev == NULL)
						// Item == list->head
						list->head = item->next;
					else
						item->prev->next = item->next;
				
					list->tail->next = item;
					item->prev = list->tail;
					item->next = NULL;
					list->tail = item;
				}
			}	
		}	
	}	
	return(err);
}


virtualPort_t* next_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	This function allows you to iterate among the ports. All available ones are stored in the bottom side of
	//	the list
	//
	virtualPort_t *obj = NULL;
	if (list->it != NULL) {
		obj = &(list->it->vpObj);
		list->it = list->it->next;
	}
	return(obj);
}


void resetIT_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	It resets the internal iterator pointer
	//
	list->it = list->head;
	return;
}


void free_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	It release all memory areas allocated for the list's items, kill all processes that keep alive the virtual ports,
	//	
	//
	
	if (list->head != NULL) {
		while (list->head->next != NULL) {
			if (list->head->prev != NULL) {
				free_virtualPort(&(list->head->prev->vpObj));
				free(list->head->prev);
			}
			list->head = list->head->next;
		}

		free_virtualPort(&(list->head->prev->vpObj));
		free(list->head->prev);
		free_virtualPort(&(list->head->vpObj));
		free(list->head);
	}
	
	if (initDBflag) {
		close_portsDB();
		initDBflag = false;
	}
	return;
}


RS485emErrorCodes pidsChk_virtualPortsList (virtualPortsList_t *list) {
	//
	// Description:
	//	It checks for ports owned by dead processes and releases the ports 
	//
	// Returned value:
	//	RS485EMULE_SUCCESS              The list has been modified
	//	RS485EMULE_WARNING_NOTHINGTODO  No modifications have been made
	//	pidChk_portsDB() exit codes
	//
	virtualPortsListItem_t *ptr  = NULL;
	RS485emErrorCodes      err   = RS485EMULE_WARNING_NOTHINGTODO;
	bool                   flag  = false;
		
	for (ptr = list->head; ptr != NULL; ptr = ptr->next) {
		if (ptr->vpObj.fd > 0) {
			err = pidChk_portsDB(ptr->vpObj.port);
			if (err > 64)
				break;
				
			else if (err == RS485EMULE_SUCCESS)
				flag = true;
		}
	}

	if (err <= 64) {
		if (flag)
			// SUCCESS!
			err = RS485EMULE_SUCCESS;
		else
			// ERROR!
			err = RS485EMULE_WARNING_NOTHINGTODO;
	}
	
	return(err);
}
