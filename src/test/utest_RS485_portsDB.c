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
// File:     test_RS485_portsDB.c
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
#include <RS485emulatorAPI.h>

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
		err = push_portsDB(bport, dport, vPortSlave);
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
	portsDBindexType  t = 0;
	char              portfn[PATH_MAX];

	fakePidFile(getpid());
	init_portsDB();
	init_RS485emulatorAPI();
	err = fillDB(TESTNUMPORTS, PREFIX);

	//
	// [!] It puts all the ports in busy state. Enabling the print_portsDB() call you should see all the ports with the
	//     associated pid
	//
	for (t=0; t<TESTNUMPORTS; t++) err = takePort_RS485emulatorAPI(portfn);
	ASSERT_EQ (err, RS485EMULE_SUCCESS);


	//
	// [!] The following lines, release some port
	//
	for (t=0; t<(TESTNUMPORTS/2); t++) {
		sprintf(portfn, "%s%d", PREFIX, (t*8));

		// Because it is a bus-port, the function should return the proper error code
		err = release_RS485emulatorAPI(portfn);
		if (err != RS485EMULE_ERROR_ITEMNOTFOUND) {
			DBGTRACE
			break;
		}

		// It is the dev-port, so it should returns a successfull code
		sprintf(portfn, "%s%d", PREFIX, (t*8)+1);
		err = release_RS485emulatorAPI(portfn);
		if (err != RS485EMULE_SUCCESS) {
			DBGTRACE
			break;
		}
	}
	ASSERT_EQ (err, RS485EMULE_SUCCESS);


	//
	// [!] The following lines retrive the in-use ports
	//     Thid feture is performed by the BUS emulator to know where to send and receive data
	//
	{
		portsDBindexType t;
		char             *portsList[TESTNUMPORTS];

		for (t=0; t<TESTNUMPORTS; t++) portsList[t] = NULL;
		err = usedPorts_portsDB((char**)portsList);
		ASSERT_EQ (err, RS485EMULE_SUCCESS);

		t = 0;
		while (portsList[t] != NULL && t<TESTNUMPORTS) {
			//printf("Bus port %d: %s\n", t, portsList[t]);
			free(portsList[t]);
			portsList[t] = NULL;
			t++;
		}
		ASSERT_EQ (t, (TESTNUMPORTS/2));
	}

	close_RS485emulatorAPI();
	close_portsDB();

	return;
}


TEST (RS485_portsDB_testingSuite, securityFeature) {
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	char              portfn[PATH_MAX];
	
	fakePidFile(getpid());
	init_portsDB();
	err = fillDB(TESTNUMPORTS, PREFIX);

	//
	// [!] Another process will try to take the first port (port-1)
	//
	{
		pid_t childPid = fork();
		
		if (childPid < 0)
			// ERROR!
			err = RS485EMULE_ERROR_NOSYSRESOURCE;

		else if (childPid == 0) {
			init_RS485emulatorAPI();
			err = takePort_RS485emulatorAPI(portfn);
			close_RS485emulatorAPI();
			exit(err);
			
		} else {
			wait(NULL);
			init_RS485emulatorAPI();
			sprintf(portfn, "%s%d", PREFIX, 1);
			//printf("I am trying to reserve the %s port\n", portfn);
			err = release_RS485emulatorAPI(portfn);
			ASSERT_EQ (err, RS485EMULE_ERROR_FORBIDDENOP);
			close_RS485emulatorAPI();
		}
	}

	close_portsDB();

	return;
}


TEST (RS485_portsDB_testingSuite, deviceMasterPort) {
	RS485emErrorCodes err         = RS485EMULE_SUCCESS;
	char              *busport    = "/tmp/bus-port";
	char              *masterport = "/tmp/master-port";
	
	fakePidFile(getpid());
	init_portsDB();
	err = push_portsDB(busport, masterport, vPortMaster);

	//
	// [!] The fake master-device process will take the master-port
	//
	{
		pid_t childPid = fork();
		
		if (childPid < 0)
			// ERROR!
			err = RS485EMULE_ERROR_NOSYSRESOURCE;

		else if (childPid == 0) {
			// I am the Master
			char portfn[PATH_MAX];

			init_RS485emulatorAPI();
			err = getMPort_RS485emulatorAPI(portfn);
			if (strcmp(portfn, masterport) != 0) err = RS485EMULE_ERROR_GENERIC;
			close_RS485emulatorAPI();
			//printf("exit-code: %d\n", bashErrorCode(err));
			exit(RS485emu_bashErrorCode(err));
			
		} else {
			int wstatus = 0;
			if (wait(&wstatus) != childPid)
				fprintf(stderr, "ERROR! The child process' soul did not pass to here\n");
			else if (!WIFEXITED(wstatus))
				fprintf(stderr, "ERROR! The child process crashed, unecpectedly\n");
			else {
				ASSERT_EQ (WEXITSTATUS(wstatus), 0);
			}
		}
	}
	
	close_portsDB();

	return;
}

//------------------------------------------------------------------------------------------------------------------------------
//                                                     M A I N
//------------------------------------------------------------------------------------------------------------------------------

int main() {
	printf("PID: %d\n", getpid());
	
	signal(SIGHUP, sigHandler);
	
	dbCreation();
	dbCheckForContent();
	securityFeature();
	deviceMasterPort();
	
	return(0);
}
