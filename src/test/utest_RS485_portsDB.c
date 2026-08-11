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
#include <sqlite3.h>
#include <minute.h>

#define PREFIX       "/dev/Port-"
#define TESTNUMPORTS 16

bool signalFlag = false;

struct qRetData {
	char              **list;
	unsigned int      itsNumber;
	RS485emErrorCodes err;
};

#define PIDLIMIT 7200

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


static int myCB (void *psl, int count, char **data, char **columns) {
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
	static portsDBindexType numups = 0;

	if (psl == NULL)
		// Function initialization
		numups = 0;

	else {
		struct qRetData *qrd = (struct qRetData*)psl;
		
		if (count != 1 || strcmp(columns[0], "busPort") != 0) {
			// ERROR! (You should never get this error)
			qrd->err = RS485EMULE_ERROR_INTERNAL;
		} else {
			qrd->list[numups] = strdup(data[0]);
			qrd->itsNumber = numups;
			//BGTRACE
		}
		numups++;
		qrd->list[numups] = NULL;
		
	}
	return(0);
}

void freeList (char **list) {
	unsigned int t = 0;
	while (list[t] != NULL) {
		free(list[t]);
		list[t] = NULL;
		t++;
	}
	return;
}

bool lookForPort(const char *port, const char **list) {
	unsigned t = 0;
	bool found = false;
	while (list[t] != NULL) {
		if (strcmp(list[t], port) == 0) {
			found = true;
			break;
		}
	}
	return(found);
}
//------------------------------------------------------------------------------------------------------------------------------
//                                                    T E S T S
//------------------------------------------------------------------------------------------------------------------------------
TEST (RS485_portsDB_testingSuite, dbCreation) {
	//
	// Description:
	//	It tests the functions to open the dtatabase, write foo-data inside it, and close it.
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	
	err = init_portsDB();
	ASSERT_EQ (err, RS485EMULE_SUCCESS);
	err = fillDB(TESTNUMPORTS, PREFIX);
	ASSERT_EQ (err, RS485EMULE_SUCCESS);

	close_portsDB();
	return;
}

TEST (RS485_portsDB_testingSuite, dbCheckForContent) {
	//
	// Description:
	//	It writes data into the database, read it, and check for read data.
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;

	init_portsDB();
	err = fillDB(TESTNUMPORTS, PREFIX);

	if (err == RS485EMULE_SUCCESS) {
		char            *myList[TESTNUMPORTS];
		char            *apsList[TESTNUMPORTS];   // all ports list
		//char            portName[PATH_MAX];
		sqlite3         *portsDB = NULL;
		char            sqlStatement[1024];
		struct qRetData qrd;
		int             rc;
		
		//
		// Initialization...
		//
		sprintf(sqlStatement, "SELECT devPort FROM busportDB;");
		for (unsigned int t = 0; t < TESTNUMPORTS; t++) {
			myList[t] = NULL;
			apsList[t] = NULL;
		}
		myCB(NULL, 0, NULL, NULL);
		qrd.list      = apsList;
		qrd.err       = RS485EMULE_SUCCESS;
		qrd.itsNumber = 0;
		
		
		if ((rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB)) != SQLITE_OK)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);
	
		else if ((rc = sqlite3_exec(portsDB, sqlStatement, myCB, (void*)&qrd, NULL)) != SQLITE_OK) 
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);
	
		else if (qrd.err != RS485EMULE_SUCCESS)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);
	
		else {	
			uint8_t t = 0;
			div_t   x;
			
			while (apsList[t] != NULL && err == RS485EMULE_SUCCESS) {
				x = div(t, 4);
				if (x.rem == 0) {
					sprintf(
						sqlStatement, 
						"UPDATE portsDB SET pid=%d WHERE busPort=\"%s\";", 
						(PIDLIMIT + t), apsList[t]
					);
					
					if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) != SQLITE_OK)
						// ERROR!
						err = RS485EMULE_ERROR_INTERNAL;
				}
			}
		
			
			err = usedPorts_portsDB(myList);
			ASSERT_EQ (err, RS485EMULE_SUCCESS);
	
			if (err == RS485EMULE_SUCCESS) {
				bool flag = false;
				t = 0;
				
				while (apsList[t] != NULL && flag == true) {
					x = div(t, 4);
					if (x.rem == 0)
						flag = lookForPort(apsList[t], (const char**)myList);
					t++;
				}
				ASSERT_TRUE((flag == false));
			}
		}
		
		sqlite3_close(portsDB);
		freeList(apsList);
		freeList(myList);
		close_portsDB();
	}
	
	
	return;
}


TEST (RS485_portsDB_testingSuite, pidChk_portsDB) {
	//
	// Description:
	//	It writes foo-data in the database, and associates every port (data) to not existent PIDs. So, when pidChk_portsDB()
	//	function is called it will re-set those port as available ones (pid=0). The procedure will test this operation.
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
