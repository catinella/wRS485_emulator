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

#include <RS485_emulator.h>
#include <RS485_virtualPort.h>

#define RS485_PORTSDBFILE   "/tmp/RS485_emulator.db"

typedef uint8_t portsDBindexType;


//
// Used by the BUS emulator
//
RS485emErrorCodes init_portsDB      ();
void              close_portsDB     ();
RS485emErrorCodes push_portsDB      (const char *busport, const char *devport, virtualPortRole role);
RS485emErrorCodes usedPorts_portsDB (char **portsList);
void              print_portsDB     ();
RS485emErrorCodes pidChk_portsDB    (const char *port);


#endif
