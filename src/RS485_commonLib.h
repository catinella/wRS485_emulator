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
// File:     RS485_commonLib.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This is a library of generic function used by the other project's module
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <RS485_emulator.h>
#include <RS485_errorCodes.h>
#include <wError.h>

wError_t     RS485emu_checkForProcStatus (pid_t pid, char *status);
wError_t     RS485emu_readPidFile        (pid_t *pid, const char *file);
wError_t     RS485emu_writePidFile       (pid_t pid, const char *file);
bool         RS485emu_stringSplitter     (const char *src, char splitter, char *firstField, char *secondField);
bool         RS485emu_chomp              (char *text);
unsigned int RS485emu_dataReceive        (int fd, void *buffer, unsigned int size);
unsigned int RS485emu_dataSend           (int fd, const void *buffer, unsigned int size);
