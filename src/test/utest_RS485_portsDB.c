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
#include <debugTools.h>

#define PREFIX        "/dev/Port-"
#define TESTNUMPORTS  16
#define TESTUSEDPORTS 3

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

unsigned int _getSize_portsList (const char **list) {
	//
	// Description:
	//	It returns the size of argument defined ports list 
	//
	unsigned int t = 0;
	while (list[t] != NULL) t++;
	return(t);
}


void _free_portsList (char **list) {
	//
	// Description:
	//	It releases the memory resources used by the chars-string items
	//
	unsigned int t = 0;
	while (list[t] != NULL) {
		free(list[t]);
		list[t] = NULL;
		t++;
	}
	return;
}


void _print_portsList (char **list) {
	unsigned int t = 0;
	while (list[t] != NULL) {
		printf("%s\n", list[t]);
		t++;
	}
	return;
}


/*
bool _search_portsList(const char *port, const char **list) {
	//
	// Description:
	//	It looks for the argument defined port in the ports list
	//
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
*/


RS485emErrorCodes _fillDB(portsDBindexType noi, const char *prefix) {
	//
	// Description:
	//	It fills the DB with foo values composed bt the argument defined prefix and a numerical suffix
	//
	// Arguments:
	//	noi     Number of items
	//	prefix  Port name prefix
	//
	// Returned code:
	//	Please, read the push_portsDB() documentation
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


bool _getFooPorts (char **portsList) {
	//
	// Description:
	//	It writes all busPort names in the argument defined char-strings list
	//
	// Returned code:
	//	true    The operation has conluded with success
	//	false   Some (SQLite) error occurred
	//
	sqlite3         *portsDB = NULL;
	char            *sqlStatement = "SELECT busPort FROM portsDB;";
	struct qRetData qrd;
	int             rc;
	char            *sqliteErrMsg = NULL;
	bool            out = false;

	// Initialization
	_myCB(NULL, 0, NULL, NULL);
	qrd.list      = portsList;
	qrd.err       = RS485EMULE_SUCCESS;
	qrd.itsNumber = 0;

	if ((rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB)) != SQLITE_OK)
		// ERROR!
		printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);

	else {
		if ((rc = sqlite3_exec(portsDB, sqlStatement, _myCB, (void*)&qrd, &sqliteErrMsg)) != SQLITE_OK) 
		// ERROR!
		printf ("ERROR(%d)! Test procedure intermnally failed (retcode=%d): %s\n", __LINE__, rc, sqliteErrMsg);
	
		else if (qrd.err != RS485EMULE_SUCCESS)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed (retcode=%d)\n", __LINE__, qrd.err);
	
		else
			// SUCCESS
			out = true;

		sqlite3_close(portsDB);
	}

	return(out);
}


bool _setFooPortAsUsed (uint8_t noi, unsigned int pid) {
	sqlite3         *portsDB = NULL;
	char            sqlStatement[1024];
	char            *myList[TESTNUMPORTS+2];
	bool            out = true;

	// Initialization
	for (unsigned int t = 0; t < TESTNUMPORTS+2; t++) myList[t] = NULL;

	out = _getFooPorts(myList);
	
	if (out) {
		unsigned int listSize = _getSize_portsList((const char**)myList);
		unsigned int step = (listSize / noi);
		div_t        x;
		int          rc;
		char         *sqliteErrMsg = NULL;

		if ((rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB)) != SQLITE_OK)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);

		else {
			for (unsigned int t = 0; t < listSize; t++) {
				x = div(t, step);
				if (x.rem == 0) {
					sprintf(sqlStatement, "UPDATE portsDB SET pid=%d WHERE devPort=\"%s\";", pid, myList[t]);
					if ((rc = sqlite3_exec(portsDB, sqlStatement, NULL, NULL, &sqliteErrMsg)) != SQLITE_OK) {
						// ERROR!
						printf (
							"ERROR(%d)! Test procedure intermnally failed (retcode=%d): %s\n",
						__LINE__, rc, sqliteErrMsg
						);

						out = false;
						break;
					}
				}
			}
			sqlite3_close(portsDB);
		}
	}

	return(out);
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
	err = _fillDB(TESTNUMPORTS, PREFIX);
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

	if ((err = _fillDB(TESTNUMPORTS, PREFIX)) != RS485EMULE_SUCCESS) {
		// ERROR!
		printf ("ERROR(%d)! push_portsDB() call failed (retcode=%d)\n", __LINE__, err);
	
	} else {
		char *pList[TESTNUMPORTS+2]; 
		
		// Initialization...
		for (unsigned int t = 0; t < TESTNUMPORTS; t++) pList[t] = NULL;
		
		if (_getFooPorts(pList)) {
		
			// It checks for the number of recorded items
			ASSERT_EQ (TESTNUMPORTS, _getSize_portsList((const char**)pList));
			
			// List cleaning...
			_free_portsList(pList);

			// Marking some port as used one
			if (_setFooPortAsUsed(TESTUSEDPORTS, PIDLIMIT) == false) {
				// ERROR!
				printf("ERROR! I cannot marks the required ports as used ones\n");

			} else {
				// Checking for used ports
				err = usedPorts_portsDB(pList);
				ASSERT_EQ (err, RS485EMULE_SUCCESS);
				_print_portsList(pList);
				ASSERT_EQ (TESTUSEDPORTS, _getSize_portsList((const char**)pList));
			}
			
			_free_portsList(pList);
		}
	}
	
	return;
}


TEST (RS485_portsDB_testingSuite, pidChk_portsDB) {
	//
	// Description:
	//	It writes foo-data in the database, and associates every port (data) to not existent PIDs. So, when pidChk_portsDB()
	//	function is called it will re-set those port as available ones (pid=0). The procedure will test this operation.
	//
	/*
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	portsDBindexType t;
	char             *portsList[TESTNUMPORTS];

	init_portsDB();
	err = _fillDB(TESTNUMPORTS, PREFIX);

	if (err == RS485EMULE_SUCCESS) {



		ASSERT_EQ (err, RS485EMULE_SUCCESS);
	
	} else {
		// WARNING!
		printf("WARNING(%d)! Test skipped for internal errors\n", __LINE__);
	}

	close_portsDB();
	*/

	return;
}

#include "utest_RS485_portsDB__main.sgc"
