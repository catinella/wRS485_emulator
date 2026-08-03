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
// File:     test_RS485_virtualPort.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	It is the unit test of the RS485_virtualPort module
// 
// 
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <debugTools.h>
#include <RS485_emulator.h>
#include <RS485_portsDB.h>
#include <RS485emulatorAPI.h>
#include <RS485_virtualPort.h>

void sigHandler (int signum) {
	printf("%d-signal received\n", signum);
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
//                                      T E S T I N G   P R O C E D U R E S
//------------------------------------------------------------------------------------------------------------------------------
TEST (RS485_virtualPort_testingSuite, newPortCreation) {
	struct virtualPort newport;
	RS485emErrorCodes  err = RS485EMULE_SUCCESS;
	pid_t              pid;

	fakePidFile(getpid());
	init_virtualPort(&newport);
	init_portsDB();

	err = create_virtualPort(&newport, vPortSlave);
	ASSERT_EQ (err, RS485EMULE_SUCCESS);

	
	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "ERROR! I cannot create sub-processes\n");

	} else if (pid == 0) {
		char        port[PATH_MAX];
		struct stat statbuff;
		bool        ok = false;
		RS485emErrorCodes err = RS485EMULE_SUCCESS;
		
		init_RS485emulatorAPI();
		err = takePort_RS485emulatorAPI(port);
		//printf("Sub-process get the \"%s\" port\n", port);
		ASSERT_EQ (err, RS485EMULE_SUCCESS);
		
		if (err == RS485EMULE_SUCCESS) {
			if (stat(port, &statbuff) == 0 && S_ISCHR(statbuff.st_mode)) ok = true;
			ASSERT_EQ (ok, true);
		}
			
		close_RS485emulatorAPI();
		exit(err);
	} else
		usleep(50000);
			

	{
		pid_t tpid = newport.pid;
		err = free_virtualPort(&newport);
		if (err != RS485EMULE_SUCCESS)
			fprintf(stderr, "WARNING! (%d) It has been impossible to terminate the %d-process in correct way\n", err, tpid);
	}
	
	// Ports-database closing
	close_portsDB();
	return;
}


TEST (RS485_virtualPort_testingSuite, dataExchange) {
	struct virtualPort newport;
	RS485emErrorCodes  err    = RS485EMULE_SUCCESS;
	pid_t              pid    = 0;
	char               *data  = "qwertyuiop1234567890";
	
	fakePidFile(getpid());
	init_virtualPort(&newport);
	init_portsDB();
	err = create_virtualPort(&newport, vPortSlave);
	pid = fork();

	if (pid < 0) {
		fprintf(stderr, "ERROR! I cannot create sub-processes\n");

	} else if (pid == 0) {
		char port[PATH_MAX];
		int  fd;
		
		init_RS485emulatorAPI();
		err = takePort_RS485emulatorAPI(port);
		//printf("Sub-process get the \"%s\" port\n", port);
		if (err == RS485EMULE_SUCCESS) {
			uint8_t dataSize = sizeof(char)*(1 + strlen(data));
			fd = open(port, O_RDWR|O_NOCTTY);
			if (fd < 0)
				fprintf(stderr, "ERROR! I cannot open the \"%s\" port\n", port);
			else {
				uint8_t ts=0, ps=1;
				if (write(fd, (void*)&dataSize, sizeof(dataSize)) == sizeof(dataSize)) {
					//printf("Sent data: %s\n", data); fflush(stdout);

					while (ts < dataSize && ps > 0) {
						ps = write(fd, (void*)(data+ts), (dataSize-ts));
						if (ps > 0) ts = ts + ps;
					}
					if (ps < 0)
						fprintf(stderr, "ERROR! IO operations failed (line=%d)\n", __LINE__);

					sleep(1);
				} else
					fprintf(stderr, "ERROR! IO operations failed (line=%d)\n", __LINE__);
			}
			close(fd);
		} else 
			fprintf(stderr, "ERROR! (%d) I cannot assign the port to my sub-process\n", err);
				
		close_RS485emulatorAPI();
		exit(err);
			
	} else {
		uint8_t size = 0;
		char    recData[256];
		
		err = open_virtualPort(&newport);
		ASSERT_EQ (err, RS485EMULE_SUCCESS);
		
		err = recv_virtualPort(newport, &size, sizeof(size)); 
		ASSERT_EQ (err, RS485EMULE_SUCCESS);

		//printf("Data size: %d\n", (int)size);
		//fflush(stdout);

		err = recv_virtualPort(newport, recData, size); 
		ASSERT_EQ (err, RS485EMULE_SUCCESS);

		ASSERT_EQ (strcmp(recData, data), 0);
	}		

	free_virtualPort(&newport);
	
	// Ports-database closing
	close_portsDB();
	return;
}


//------------------------------------------------------------------------------------------------------------------------------
//                                                     M A I N
//------------------------------------------------------------------------------------------------------------------------------

int main() {
	printf("PID: %d\n", getpid());
	signal(SIGHUP,  sigHandler);
	signal(SIGTTIN, sigHandler);
	signal(SIGTTOU, sigHandler);
	
	newPortCreation();
	dataExchange();

	
	return(0);
}
