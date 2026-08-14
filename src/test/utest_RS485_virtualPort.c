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
// File:     utest_RS485_virtualPort.c
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
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sqlite3.h>
#include <debugTools.h>
#include <minute.h>
#include <RS485_emulator.h>
#include <RS485_portsDB.h>
#include <RS485_virtualPort.h>

struct qRetData {
	char              **list;
	unsigned int      itsNumber;
	RS485emErrorCodes_t err;
};

//static bool initFlag = false;

/*
static void _sigHandler (int signum) {
	printf("%d-signal received\n", signum);
	return;
}

static void _init() {
	if (initFlag == false) {
		printf("PID: %d\n", getpid());
		signal(SIGHUP,  _sigHandler);
		signal(SIGTTIN, _sigHandler);
		signal(SIGTTOU, _sigHandler);
		initFlag = true;
	}
	return;
}
*/

static int _myCB (void *psl, int count, char **data, char **columns) {
	//
	// Description
	//	This callback is called by sqlite3_exec() function for every item in the query.
	//
	//	[!] It allocates new memory areas in the heap, you will have to release them explicity
	//
	// Arguments:
	//	psl     - Pointer to an array of names
	//	count   - The number of columns in the result set
	//	data    - The row's data
	//	columns - The column names
	//
	static portsDBindexType idx = 0;

	if (psl == NULL)
		// Function initialization
		idx = 0;

	else {
		struct qRetData *qrd = (struct qRetData*)psl;
		
		if (strcmp(columns[0], "busPort") != 0) {
			// ERROR! (You should never get this error)
			qrd->err = RS485EMULE_ERROR_INTERNAL;
		} else {
			qrd->list[idx] = strdup(data[0]);
			qrd->itsNumber = idx;
			//DBGTRACE
		}
		idx++;
		qrd->list[idx] = NULL;
		
	}
	return(0);
}

//------------------------------------------------------------------------------------------------------------------------------
//                                      T E S T I N G   P R O C E D U R E S
//------------------------------------------------------------------------------------------------------------------------------
TEST (RS485_virtualPort_testingSuite, newPortCreation) {
	virtualPort_t      newport;
	RS485emErrorCodes_t  err = RS485EMULE_SUCCESS;

	init_virtualPort(&newport);

	err = create_virtualPort(&newport, RS485EMULE_PORTSLAVE);
	ASSERT_EQ (err, RS485EMULE_SUCCESS);

	if (err == RS485EMULE_SUCCESS) {
		sqlite3         *portsDB = NULL;
		char            sqlStatement[1024];
		struct qRetData qrd;
		char            *sqliteErrMsg = NULL;
		int             rc;

		if ((rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB)) != SQLITE_OK)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);

		else {
		 	sprintf("SELECT devPort FROM portsDB WHERE busport=\"%s\";", newport.port);
			if ((rc = sqlite3_exec(portsDB, sqlStatement, _myCB, (void*)&qrd, &sqliteErrMsg)) != SQLITE_OK) 
				// ERROR!
				printf ("ERROR(%d)! Test procedure intermnally failed (retcode=%d): %s\n", __LINE__, rc, sqliteErrMsg);
	
			else if (qrd.err != RS485EMULE_SUCCESS)
				// ERROR!
				printf ("ERROR(%d)! Test procedure intermnally failed (retcode=%d)\n", __LINE__, qrd.err);
			
			else {
				DBGTRACE
				// TODO: getting the d-port

				// TODO: checking for the files

			}
		}
	}
	
	free_virtualPort(&newport);

	// Ports-database closing
	close_portsDB();
	return;
}


TEST (RS485_virtualPort_testingSuite, dataExchange) {
	/*
	struct virtualPort_t newport;
	RS485emErrorCodes_t  err    = RS485EMULE_SUCCESS;
	pid_t              pid    = 0;
	char               *data  = "qwertyuiop1234567890";
	
	_init();
	
	init_virtualPort(&newport);
	err = create_virtualPort(&newport, RS485EMULE_PORTSLAVE);
	
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
	*/
	return;
}

#include "utest_RS485_virtualPort__main.sgc"
