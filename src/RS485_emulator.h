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
// File:     RS485_emulator.h
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
#pragma once

#include <stdint.h>
#include <signal.h>
#include <termios.h>

#define RS485EMULE_PORTSNUM       16
#define RS485EMULE_BAUDRATES      B115200
#define RS485EMULE_PORTSFOLDER    "/dev/pts"
#define RS485EMULE_LOGFACILITY    LOG_LOCAL1
#define RS485EMULE_UPDATESIGN     SIGHUP
#define RS485EMULE_BROKENSIGN     SIGUSR1 

#if TESTMODE == 1
#define RS485EMULE_CONFIG_FILE    "/tmp/RS485_emulator.cfg"
#define RS485EMULE_PIDFILE        "/tmp/RS485_emulator.pid"
#else
#define RS485EMULE_CONFIG_FILE    "/etc/RS485_emulator.cfg"
#define RS485EMULE_PIDFILE        "/var/run/RS485_emulator.pid"
#endif

// TTY group ID, usually
#ifndef RS485EMULE_GROUP
#define RS485EMULE_GROUP          5
#endif

typedef uint8_t rs485emule_portsNum_type;


