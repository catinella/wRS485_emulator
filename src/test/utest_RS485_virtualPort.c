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
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <glib.h>
#include <libForTests.h>
#include <debugTools.h>
#include <minute.h>
#include <RS485_emulator.h>
#include <RS485_portsDB.h>
#include <RS485_virtualPort.h>

struct qRetData {
	GPtrArray         *list;
	unsigned int      itsNumber;
	RS485emErrorCodes_t err;
};

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
			g_ptr_array_add(qrd->list, (gpointer)strdup(data[0]));
			//DBGTRACE
		}
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

	{
		FILE  *fh = NULL;
		char  procFile[PATH_MAX];
		char  line[1024];
		pid_t tPid = getpid();

		sprintf(procFile, "/proc/%d/task/%d/children", tPid, tPid);
		if ((fh = fopen(procFile, "r")) == NULL) {
			// ERROR!
			fprintf (
				stderr, "ERROR(%d)! I cannot open the \"/proc/%d/task/%d/children\" file (errno=%d)\n",
				__LINE__, tPid, tPid, errno
			);
		
		} else if (fgets(line, sizeof(line), fh) == NULL) {
			// ERROR!
			fprintf (
				stderr, "ERROR(%d)! I cannot read the \"/proc/%d/task/%d/children\" file (errno=%d)\n",
				__LINE__, tPid, tPid, errno
			);

		} else {
			GPtrArray    *pidsList = g_ptr_array_new_with_free_func(g_free);
				
			//
			// Because the test has created one port, this process should have a single child-process where socat
			// is running
			//
			ASSERT_EQ (1, split_stringList(pidsList, line, " \t"));

			if (pidsList->len == 1) {
				tPid = atoi((char*)g_ptr_array_index(pidsList, 0));
	
				if (tPid == 0) {
					// ERROR
					fprintf (stderr, "ERROR(%d)! Internal error in the testing code\n", __LINE__);
				
				} else {
					FILE *pFH = NULL;
					sprintf(procFile, "/proc/%d/cmdline", tPid);
					if ((pFH = fopen(procFile, "r")) == NULL) {
						// ERROR!
						fprintf (
							stderr, "ERROR(%d)! I cannot open the \"/proc/%d/cmdline\" file (errno=%d)\n",
							__LINE__, tPid, errno
						);

					} else if (fgets(line, sizeof(line), pFH) == NULL) {
						// ERROR!
						fprintf (
							stderr, "ERROR(%d)! I cannot read the \"/proc/%d/cmdline\" file (errno=%d)\n",
							__LINE__, tPid, errno
						);
						fclose(pFH);

					} else {
						char *file = NULL;
						chomp(line);
						if ((file = strrchr(line, '/')) == NULL)
							file = line;
						else
							file++;

						if (fileArgumentsDb_get("verbose", NULL)) 
							printf("Child process: \"%s\"\n", file);
							
						// Checking for the child process' executable file
						ASSERT_EQ (0, strcmp(file, "socat"));
					}
					
					if (pFH != NULL) fclose(pFH);
				}			
			}
			fclose(fh);

			g_ptr_array_free(pidsList, TRUE);
		}
	}

	free_virtualPort(&newport);

	// Ports-database closing
	close_portsDB();
	return;
}


TEST (RS485_virtualPort_testingSuite, dataExchange) {
	/*
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
	
	*/
	return;
}

#include "utest_RS485_virtualPort__main.sgc"
