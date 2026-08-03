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
// File:     RS485emulatorAPI.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This module provides the functions used by BUS emulator's clients to requires a virtual serial port or to release a
//	used one
//	
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#ifndef RS485EMULEAPI
#define RS485EMULEAPI

#include <RS485_emulator.h>

#define RS485EMULE_UPDATECMD "RS485_updateSignal"

RS485emErrorCodes init_RS485emulatorAPI     (void);
void              close_RS485emulatorAPI    (void);
RS485emErrorCodes release_RS485emulatorAPI  (const char *serialport);
RS485emErrorCodes takePort_RS485emulatorAPI (char *port);
RS485emErrorCodes getMPort_RS485emulatorAPI (char *fpname);

#endif
