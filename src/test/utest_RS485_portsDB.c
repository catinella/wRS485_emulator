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
#include <string.h>
#include <unistd.h>
#include <debugTools.h>
#include <RS485_emulator.h>
#include <RS485_portsDB.h>
#include <sqlite3.h>
#include <minute.h>
#include <debugTools.h>
#include <math.h>
#include <libForTests.h>


#define PREFIX        "/dev/Port-"
#define TESTNUMPORTS  16
#define TESTUSEDPORTS 3

bool signalFlag = false;

struct qRetData {
	GPtrArray         *list;
	unsigned int      itsNumber;
	RS485emErrorCodes_t err;
};

#define PIDLIMIT 72000

//------------------------------------------------------------------------------------------------------------------------------
//                                         P R I V A T E   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------


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


RS485emErrorCodes_t _fillDB(unsigned int noi, const char *prefix) {
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
	char              bport[PATH_MAX];
	char              dport[PATH_MAX];
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	
	for (unsigned int t=0; t<noi; t++) {
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
	if (psl != NULL) {
		struct qRetData *qrd = (struct qRetData*)psl;
		
		if (strcmp(columns[0], "busPort") != 0) {
			// ERROR! (You should never get this error)
			qrd->err = RS485EMULE_ERROR_INTERNAL;
		} else {
			g_ptr_array_add(qrd->list, (gpointer)strdup(*data));
			//DBGTRACE
		}
		
	}
	return(0);
}


bool _getFooPorts (GPtrArray *portsList) {
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
	GPtrArray       *myList = NULL;;
	bool            out = true;

	// Initialization
	myList = g_ptr_array_new_with_free_func(g_free);

	out = _getFooPorts(myList);
	
	if (out) {
		unsigned int step = trunc(myList->len / noi);
		div_t        x;
		int          rc;
		char         *sqliteErrMsg = NULL;

		if ((rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB)) != SQLITE_OK)
			// ERROR!
			printf ("ERROR(%d)! Test procedure intermnally failed\n", __LINE__);

		else {
			for (unsigned int t = 0; t < (step * noi); t++) {
				x = div(t, step);
				if (x.rem == 0) {
					sprintf(
						sqlStatement, 
						"UPDATE portsDB SET pid=%d WHERE busPort=\"%s\";", 
						pid, 
						(char*)g_ptr_array_index(myList, t)
					);
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
	
	g_ptr_array_free(myList, TRUE);

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
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	
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
	//	This test performs the following steps:
	//	 1) It writes a foo-data set in the DB (as in the previous test)
	//	 2) Using the SQL API, it loads the foo-data and checks for it size
	//	 3) It marks some port as used, with a not-used PID
	//	 4) It tests the usedPorts_portsDB() function
	//	 5) It checks for the list size
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;

	// Module initialization
	init_portsDB();

	if ((err = _fillDB(TESTNUMPORTS, PREFIX)) != RS485EMULE_SUCCESS) {
		// ERROR!
		printf ("ERROR(%d)! push_portsDB() call failed (retcode=%d)\n", __LINE__, err);
	
	} else {
		GPtrArray *pList = g_ptr_array_new_with_free_func(g_free);

		if (_getFooPorts(pList)) {
		
			if (fileArgumentsDb_get("verbose", NULL)) {
				printf("List of registered ports:\n");
				print_stringList((const GPtrArray*)pList);
				printf("\n");
			}

			// It checks for the number of recorded items
			ASSERT_EQ (TESTNUMPORTS, pList->len);
			
			// List cleaning...
			g_ptr_array_set_size(pList, 0);

			// Marking some port as used one
			if (_setFooPortAsUsed(TESTUSEDPORTS, PIDLIMIT) == false) {
				// ERROR!
				printf("ERROR! I cannot marks the required ports as used ones\n");

			} else {

				// Checking for used ports
				err = usedPorts_portsDB(pList);
				ASSERT_EQ (err, RS485EMULE_SUCCESS);
			
				if (fileArgumentsDb_get("verbose", NULL)) {
					printf("List of used ports:\n");
					print_stringList((const GPtrArray *)pList);
					printf("\n");
				}
				ASSERT_EQ (TESTUSEDPORTS, pList->len);
			}
		}
		g_ptr_array_free(pList, TRUE);
	}
	
	close_portsDB();
	
	return;
}


TEST (RS485_portsDB_testingSuite, pidChk_portsDB) {
	//
	// Description:
	//	It performs the following steps:
	//	 1) It writes a foo-data set in the DB (as in the previous test)
	//	 2) It marks some port as used, with a not-used PID and marks two port with the current process ID
	//	 3) It tests the usedPorts_portsDB() function
	//	 4) It checks for the list size
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;

	init_portsDB();
	
	if (_fillDB(TESTNUMPORTS, PREFIX) != RS485EMULE_SUCCESS) {
		// ERROR!
		printf("ERROR(%d)! Test skipped for internal errors\n", __LINE__);

	} else if (
		_setFooPortAsUsed(TESTUSEDPORTS, PIDLIMIT) == false ||
		_setFooPortAsUsed(2, getpid()) == false
	) {
		// ERROR!
		printf("ERROR! I cannot marks the required ports as used ones\n");

	} else {
		GPtrArray *pList = g_ptr_array_new_with_free_func(g_free);

		// All ports
		_getFooPorts(pList);

		for (unsigned int t = 0; t < pList->len; t++) {
			const char *port = g_ptr_array_index(pList, t);

			err = pidChk_portsDB(port);
			
			if (fileArgumentsDb_get("verbose", NULL)) {
				if (err == RS485EMULE_SUCCESS)
					printf("%s released\n", port);

				else if (err == RS485EMULE_WARNING_UNAVAILRES)
					printf("%s still in-use\n", port);

				else
					printf("%s is not in-use\n", port);
			}
			t++;
		}
	
		// List cleaning...
		g_ptr_array_set_size(pList, 0);

		err = usedPorts_portsDB(pList);
		ASSERT_EQ (err, RS485EMULE_SUCCESS);
		
		if (fileArgumentsDb_get("verbose", NULL)) {
			printf("List of used ports:\n");
			print_stringList((const GPtrArray*)pList);
			printf("\n");
		}
		ASSERT_EQ (3, pList->len);
	
		g_ptr_array_free(pList, TRUE);
	}

	close_portsDB();

	return;
}

#include "utest_RS485_portsDB__main.sgc"
