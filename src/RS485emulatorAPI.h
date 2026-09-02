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
#pragma once

#include <RS485_emulator.h>
#include <RS485_errorCodes.h>
#include <wError.h>

#define RS485EMULE_UPDATECMD "RS485_updateSignal"

wError_t init_RS485emulatorAPI     (void);
void     close_RS485emulatorAPI    (void);
wError_t release_RS485emulatorAPI  (const char *serialport);
wError_t takePort_RS485emulatorAPI (char *port);
wError_t getMPort_RS485emulatorAPI (char *fpname);
