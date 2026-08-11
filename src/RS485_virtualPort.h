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
// File:     RS485_virtualPort.h
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	
//	
//	chunkDataSize_type
//		This data type is used to define the single data slice to send/receive to/from the virtual serial port. The maximum
//		number you can reppresent using the data type must be equal or greather then the serial port's buffer size
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/

#ifndef VIRTUALPORTS

#define VIRTUALPORTS

#include <limits.h>
#include <sys/types.h>
#include <stdint.h>
#include <RS485_emulator.h>
#include <RS485_commonLib.h>

typedef uint16_t chunkDataSize_type;

#define RS485EMULE_PORTSMAKER_CMD  "/usr/bin/socat"
#define RS485EMULE_PORTSMAKER_ARG1 "-d"
#define RS485EMULE_PORTSMAKER_ARG2 "pty,raw,echo=0" 
#define RS485EMULE_PORTSMAKER_ARG3 "pty,raw,echo=0"

typedef enum {
	RS485EMULE_PORTMASTER,
	RS485EMULE_PORTSLAVE
} virtualPortRole_t;

typedef struct {
	pid_t   pid;
	char    port[PATH_MAX];
	int     fd;
} virtualPort_t;


void                init_virtualPort    (virtualPort_t *item);
virtualPort_t*      new_virtualPort     ();
RS485emErrorCodes   free_virtualPort    (virtualPort_t *item);
RS485emErrorCodes   create_virtualPort  (virtualPort_t *item, virtualPortRole_t role);
RS485emErrorCodes   open_virtualPort    (virtualPort_t *item);
void                print_virtualPort   (virtualPort_t item);
RS485emErrorCodes   send_virtualPort    (virtualPort_t item, const void *data, chunkDataSize_type size);
RS485emErrorCodes   recv_virtualPort    (virtualPort_t item, void *data,       chunkDataSize_type size);
RS485emErrorCodes   close_virtualPort   (virtualPort_t *item);
#endif
