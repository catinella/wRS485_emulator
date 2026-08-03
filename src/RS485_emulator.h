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

#ifndef RS485EMULATOR

#define RS485EMULATOR

#include <stdint.h>
#include <signal.h>
#include <termios.h>

#define RS485EMULE_PORTSNUM       16
#define RS485EMULE_BAUDRATES      B115200
#define RS485EMULE_PORTSFOLDER    "/dev/pts"
#define RS485EMULE_LOGFACILITY    LOG_LOCAL1
#define RS485EMULE_UPDATESIGN     SIGHUP

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

//
// Error Codes
//
typedef enum _RS485emErrorCodes {
	//-------------------------------------------
	//         S U C C E S S   C O D E S
	//                 [1-32]
	//-------------------------------------------
	RS485EMULE_SUCCESS              = 1,
	RS485EMULE_INFO_AVAILABLEPORT   = 3,
	//-------------------------------------------
	//        W A R N I N G   C O D E S
	//                 [33-64]
	//-------------------------------------------
	RS485EMULE_WARNING_GENERIC      = 33,
	RS485EMULE_WARNING_ITEMNOTFOUND = 35,
	RS485EMULE_WARNING_NOTHINGTODO  = 37,
	RS485EMULE_WARNING_TIMEOUT      = 39,
	//-------------------------------------------
	//       B U S   E R R O R   C O D E S
	//                  [65-128]
	//-------------------------------------------
	RS485EMULE_ERROR_GENERIC        = 65,
	RS485EMULE_ERROR_UNKNOWNARG     = 67,
	RS485EMULE_ERROR_IOFAILED       = 69,
	RS485EMULE_ERROR_INTERNAL       = 71,
	RS485EMULE_ERROR_FILENOTFOUND   = 73,
	RS485EMULE_ERROR_ILLEGALSYNTAX  = 75,
	RS485EMULE_ERROR_CORRUPTEDDATA  = 77,
	RS485EMULE_ERROR_DATAOVERFLOW   = 79,
	RS485EMULE_ERROR_NOSYSRESOURCE  = 81,
	RS485EMULE_ERROR_MISSINGDATA    = 83,
	RS485EMULE_ERROR_EXTPROCFAILED  = 85,
	RS485EMULE_ERROR_NOSPACE        = 87,
	RS485EMULE_ERROR_ITEMNOTFOUND   = 89,
	RS485EMULE_ERROR_FORBIDDENOP    = 91,
	RS485EMULE_ERROR_EXTTOOLFAILURE = 93,
	RS485EMULE_ERROR_ILLEGALDATA    = 95,
	RS485EMULE_ERROR_UNKNOWN        = 97,
	RS485EMULE_ERROR_UNAVAILRES     = 99
	//-------------------------------------------
	// F A K E   D E V S   E R R O R   C O D E S
	//                  [129-255]
	//-------------------------------------------
	
} RS485emErrorCodes;




#endif
