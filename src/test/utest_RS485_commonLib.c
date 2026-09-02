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
// File:     utest_RS485_commonLib.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	Unit tests for RS485_commonLib library
//	
//	
//	
// Eritor parameters: cols=128 tab-space=6
-------------------------------------------------------------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <minute.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485_emulator.h>

#define PIDFILE "/tmp/myTestPidFile"
#define FAKEPID 4096

TEST (RS485_commonLib_testingSuite, checkForProcStatusTest) {
	char  status;
	pid_t pid;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS) 

	//
	// Check for running process
	//
	err = RS485emu_checkForProcStatus(getpid(), &status);
	ASSERT_TRUE (WERROR_ISSUCCESS(err));
	ASSERT_EQ   (toupper(status), 'R');

	pid = fork();
	if (pid < 0) {
	} else if (pid == 0) {
		sleep(10);
		exit(0);
	} else {
		kill(pid, SIGTERM);
		usleep(1000);
	}

	//
	// Check for zombie process
	//
	err = RS485emu_checkForProcStatus(pid, &status);
	ASSERT_TRUE (WERROR_ISSUCCESS(err));
	ASSERT_EQ (toupper(status), 'Z');

	wait(NULL);
	usleep(1000);

	//
	// Check for dead process
	//
	err = RS485emu_checkForProcStatus(pid, &status);
	ASSERT_TRUE (WERROR_ISSUCCESS(err));
	ASSERT_EQ (toupper(status), 'X');
	
	return;
}

TEST (RS485_commonLib_testingSuite, pidfileTest) {
	pid_t pid;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS) 
	
	unlink(PIDFILE);
	err = RS485emu_writePidFile(FAKEPID, PIDFILE);
	ASSERT_TRUE (WERROR_ISSUCCESS(err));

	err = RS485emu_readPidFile(&pid, PIDFILE);
	ASSERT_TRUE (WERROR_ISSUCCESS(err));
	
	ASSERT_EQ (pid, FAKEPID);

	return;
}

TEST (RS485_commonLib_testingSuite, stringSplitterTest) {
	char wa[64];
	char wb[64];
	char *sstring = "bmngbknmgrblkgnbl:lvjfhvkjvnkgvjng";
	memset(wa, 0, 64*sizeof(char));
	memset(wb, 0, 64*sizeof(char));
	ASSERT_EQ (RS485emu_stringSplitter(sstring, '$', NULL, NULL), false);
	ASSERT_EQ (RS485emu_stringSplitter(sstring, ':', wa, wb), true);
	ASSERT_EQ (strcmp(wa, "bmngbknmgrblkgnbl"), 0);
	ASSERT_EQ (strcmp(wb, "lvjfhvkjvnkgvjng"), 0);

	return;
}

#include "utest_RS485_commonLib__main.sgc"

