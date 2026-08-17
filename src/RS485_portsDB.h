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
// File:     RS485_portsDB.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This module is used just by the RS485_virtualPortsList one, and it provides functions to manage the ports-list tored in
//    the database file and its concurrent access too.
//	
//	Symbols:
//		RS485_PORTSDBFILE   SQLight DB file
//		                    [!] This file has been placed in /tmp folder, because when the process access to a DB file
//		                        (using the qlite3 library), it needs to have write permissions also on the directory the
//		                        file belongs to. If it is unable to get the permissions it will open the DB in read-only
//		                        mode. For this reason you SHOULD NOT change the folder to a system folder (eg. /var)
//
-------------------------------------------------------------------------------------------------------------------------------*/
#ifndef RS485PORTSDB

#define RS485PORTSDB

#include <glib.h>
#include <RS485_emulator.h>
#include <RS485_virtualPort.h>

#if TESTMODE > 0
#define RS485_PORTSDBFILE   "/tmp/RS485_emulator.db"
#else
#define RS485_PORTSDBFILE   "/var/lib/RS485_emulator.db"
#endif


//
// Used by the BUS emulator
//
RS485emErrorCodes_t init_portsDB      ();
void                close_portsDB     ();
RS485emErrorCodes_t push_portsDB      (const char *busport, const char *devport, virtualPortRole_t role);
RS485emErrorCodes_t usedPorts_portsDB (GPtrArray *portsList);
void                print_portsDB     ();
RS485emErrorCodes_t pidChk_portsDB    (const char *port);


#endif
