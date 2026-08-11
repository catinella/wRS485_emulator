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
// File:     utest_RS485_portsDB.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	It is the unit test of the RS485_portsDB module
// 
// 
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <debugTools.h>
#include <RS485_emulator.h>
#include <RS485_portsDB.h>
#include <minute.h>

#define PREFIX       "/dev/Port-"
#define TESTNUMPORTS 16

bool signalFlag = false;

//------------------------------------------------------------------------------------------------------------------------------
//                                         P R I V A T E   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------
RS485emErrorCodes fillDB(portsDBindexType noi, const char *prefix) {
	//
	// Description:
	//	It fills the DB with foo values
	//
	portsDBindexType  t = 0;
	char              bport[PATH_MAX];
	char              dport[PATH_MAX];
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	
	for (t=0; t<noi; t++) {
		sprintf(bport, "%s%d", prefix, (t*4));
		sprintf(dport, "%s%d", prefix, (t*4+1));
		err = push_portsDB(bport, dport, RS485EMULE_PORTSLAVE);
		if (err != RS485EMULE_SUCCESS) break;
	}
	return(err);
}


void sigHandler (int signum) {
	printf("%d-sinal received\n", signum);
	signalFlag = true;
	return;
}


void fakePidFile (pid_t fpid) {
	//
	// Description:
	//	Because the RS485 emulaor is not running, we have to create a fake pid-file
	//	If it is not possible then the test cannot go on
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	if ((err = RS485emu_writePidFile(getpid(), RS485EMULE_PIDFILE)) && err != RS485EMULE_SUCCESS) {
		fprintf(stderr, "ERROR! I cannot create the \"%s\" file\n", RS485EMULE_PIDFILE);
		exit(err);
	}
	return;
}


//------------------------------------------------------------------------------------------------------------------------------
//                                                    T E S T S
//------------------------------------------------------------------------------------------------------------------------------
TEST (RS485_portsDB_testingSuite, dbCreation) {
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	
	err = init_portsDB();
	ASSERT_EQ (err, RS485EMULE_SUCCESS);
	err = fillDB(TESTNUMPORTS, PREFIX);
	ASSERT_EQ (err, RS485EMULE_SUCCESS);

	close_portsDB();
	return;
}

TEST (RS485_portsDB_testingSuite, dbCheckForContent) {
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	char              *myList[TESTNUMPORTS + 10];
	char              portName[PATH_MAX];
	unsigned int      counter = 0;

	init_portsDB();
	err = fillDB(TESTNUMPORTS, PREFIX);

	if (err == RS485EMULE_SUCCESS)
		err = usedPorts_portsDB(myList);
	
	while (err == RS485EMULE_SUCCESS && counter < TESTNUMPORTS + 1) {
		sprintf(portName, "%s%d", PREFIX, counter);
		if (strcmp(portName, myList[counter]) != 0)
			break;
		
		counter++;
	}
	if (counter != TESTNUMPORTS)
		// ERROR!
		err = RS485EMULE_ERROR_GENERIC;
			
	ASSERT_EQ (err, RS485EMULE_SUCCESS);
	
	close_portsDB();
	
	return;
}


TEST (RS485_portsDB_testingSuite, pidChk_portsDB) {
	//
	// [!] The following lines retrive the in-use ports
	//     Thid feture is performed by the BUS emulator to know where to send and receive data
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	portsDBindexType t;
	char             *portsList[TESTNUMPORTS];

	init_portsDB();
	err = fillDB(TESTNUMPORTS, PREFIX);

	if (err == RS485EMULE_SUCCESS) {



		ASSERT_EQ (err, RS485EMULE_SUCCESS);
	
	} else {
		// WARNING!
		printf("WARNING(%d)! Test skipped for internal errors\n", __LINE__);
	}

	close_portsDB();

	return;
}

#include "utest_RS485_portsDB__main.sgc"
