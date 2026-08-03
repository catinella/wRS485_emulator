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
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#ifndef RS485COMMONLIB
#define RS485COMMONLIB 1

#include <stdint.h>
#include <stdbool.h>
#include <RS485_emulator.h>

rs485emule_portsNum_type RS485emu_checkForProcStatus (pid_t pid, char *status);

rs485emule_portsNum_type RS485emu_readPidFile        (pid_t *pid, const char *file);

rs485emule_portsNum_type RS485emu_writePidFile       (pid_t pid, const char *file);

int                      RS485emu_bashErrorCode      (rs485emule_portsNum_type ec);

bool                     RS485emu_stringSplitter     (const char *src, char splitter, char *firstField, char *secondField);

bool                     RS485emu_chomp              (char *text);

unsigned int             RS485emu_dataReceive        (int fd, void *buffer, unsigned int size);

unsigned int             RS485emu_dataSend           (int fd, const void *buffer, unsigned int size);

#endif
