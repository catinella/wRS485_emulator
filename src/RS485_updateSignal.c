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
// File:     RS485_updateSignal.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	The goal of this executable file is just to send a wakeup-signal to the bus emulator manager. In fact, the client process
//	that usually runs as unpriviledged user CANNOT senf signals too the BUS manager while it runs as root user's process.
//	For this reason I wrote this simple executable file where its SUID bit will be set during the installation process.
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <wError.h>
#include <RS485_emulator.h>
#include <RS485_commonLib.h>


int main() {
	pid_t    busemPid = 0;
	wError_t err      =  RS485emu_readPidFile(&busemPid, RS485EMULE_PIDFILE);

	if (WERROR_ISERROR(err) == false) kill(busemPid, RS485EMULE_UPDATESIGN);

	return(wError_shellCode(err));
}
