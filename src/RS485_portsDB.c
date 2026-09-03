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
// File:     RS485_portsDB.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This module is used just by the RS485_virtualPortsList one, and it provides functions to manage the ports-list stored in
//    the database file and its concurrent access too.
//
//	+-------------------+             +---------------+
//	|                   |             |               |             +--------+
//	| RS485_virtualPort +---(uses)--->| RS485_portsDB +---(uses)--->| SQLite |
//	|                   |             |               |             +----+---+
//	+-------------------+             +---------------+                  |
//	                                                                     |
//	                                                                 /---+---\
//	                                                                 |  file |
//	                                                                 \-------/
//
//
//	
//	Database structure:
//		+----------+----------+----------+-----------+
//		|   role   | busPort  |  devPort |    pid    |
//		| (string) | (string) | (string) | (integer) |
//		+----------+----------+----------+-----------+
//	                                              ^
//	                                              |
//	                                   (it is used just by the APIs)
//	
//	Rules:
//		- All unused records must have the "port" fieeld set to '\0'
//		- Records cannot be removed
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sqlite3.h>
#include <wError.h>
//#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485_portsDB.h>


#ifndef DBGTRACE
#define DBGTRACE ;
#endif

#define SQLWERR(VAR, RC)                                                                                                        \
	wErrorWithMessage_set(&VAR, "SQLITE3(%d): %s", RC, sqlite3_errstr(RC));
	
#define SQLWERR2(VAR_A, RC, VAR_B)                                                                                              \
	wErrorWithMessage_set(&VAR_A, "SQLITE3(%d): %s", RC, (VAR_B == NULL) ? sqlite3_errstr(RC) : VAR_B);

struct queryRetData {
	GPtrArray *list;
	wError_t  err;
};

static sqlite3 *portsDB = NULL;


static int _pidByPort_callback (void *pid, int count, char **data, char **columns) {
	//
	// Description
	//	This is the callback called to retrive the ID of the process who owned the port
	//

	if (count != 1 || strcmp(columns[0], "pid") != 0) 
		// ERROR!
		*(pid_t*)pid = 0;
	else
		*(pid_t*)pid = atoi(data[0]);
		
	return(0);
}


static int _usedPorts_callback (void *psl, int count, char **data, char **columns) {
	//
	// Description
	//	This callback is called by sqlite3_exec() function for every item in the busPorts query. The goal is to obtains a
	//	list of all in-use-ports names
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
		struct queryRetData *qrd = (struct queryRetData*)psl;
		wError_init (&(qrd->err), WERROR_JUSTCODE);
		
		if (count != 1 || strcmp(columns[0], "busPort") != 0) {
			// ERROR! (You should never get this error)
			WERROR_GETCODE(qrd->err) = RS485EMULE_ERROR_INTERNAL;
		} else {
			g_ptr_array_add (qrd->list, (gpointer)strdup(data[0]));
			WERROR_GETCODE(qrd->err) = RS485EMULE_SUCCESS;
			//BGTRACE
		}
	}
	return(0);
}
//------------------------------------------------------------------------------------------------------------------------------
//                                             P U B L I C   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------

wError_t init_portsDB() {
	//
	// Description:
	//	It initializes the SQLite DB, and sets internal data used by many functions
	//
	// Returne value:
	//	RS485EMULE_SUCCESS              The DB has been correctly created
	//	RS485EMULE_ERROR_IOFAILED       Impossible to create/write the DB file (RS485_PORTSDBFILE)
	//	RS485EMULE_ERROR_INTERNAL       Unknown error
	//	RS485EMULE_ERROR_FORBIDDENOP    File permission changing forbidden (when I am running as root!!!!)
	//
	int rc = 0;
	WERROR_DECLARATION(err, WERROR_WITHMESSAGE, RS485EMULE_SUCCESS)
	
	unlink(RS485_PORTSDBFILE);
	rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB);
	if (rc == SQLITE_IOERR || rc == SQLITE_PERM) {
		// ERROR!
		SQLWERR(err, rc)
		WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
		
	} else if (rc != SQLITE_OK) {
		// ERROR!
		SQLWERR(err, rc)
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		
	} else {
		char *sqlStatement = "CREATE TABLE portsDB( \
			\"role\"    TEXT,                     \
			\"busPort\" TEXT NOT NULL UNIQUE,     \
			\"devPort\" TEXT NOT NULL UNIQUE,     \
			\"pid\"     INTEGER NOT NULL          \
		);";
		
		// SQL statement execution
		if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) != SQLITE_OK) {
			// ERROR!
			SQLWERR(err, rc)
			WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
			sqlite3_close(portsDB);
			portsDB = NULL;
		}
	}
	
	if (
		getuid() == 0 && WERROR_ISERROR(err) == false && (
			chmod(RS485_PORTSDBFILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0 ||
			chown(RS485_PORTSDBFILE, 0, RS485EMULE_GROUP)             != 0
		)
	)
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_FORBIDDENOP;

	return(err);
}


wError_t close_portsDB() {
	//
	// Description:
	//	This function closes the internal SQLite DB
	//
	int rc = 0;
	WERROR_DECLARATION(err, WERROR_WITHMESSAGE, RS485EMULE_SUCCESS)
	if (portsDB != NULL) {
		rc = sqlite3_close(portsDB);
		SQLWERR(err, rc)
		unlink(RS485_PORTSDBFILE);
		portsDB = NULL;
	}
	return(err);
}


wError_t push_portsDB(const char* busport, const char* devport, virtualPortRole_t role) {
	//
	// Description:
	//	It allows you to add a new virtual serial port to the in-process DB
	//	[!] It is used by the BUS emulator to build the ports DB at the beginning status
	//
	// Returne value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_INTERNAL
	//
	char sqlStatement[256];
	char *sqliteErrMsg = NULL;
	char roleChar = 'S';
	int  rc = 0;
	WERROR_DECLARATION(err, WERROR_WITHMESSAGE, RS485EMULE_SUCCESS)
	
	if (role == RS485EMULE_PORTMASTER) roleChar = 'M';
	sprintf(sqlStatement, "INSERT INTO portsDB VALUES(\"%c\", \"%s\", \"%s\", 0);", roleChar, busport, devport);
	rc = sqlite3_exec(portsDB, sqlStatement, NULL, NULL, &sqliteErrMsg);
	if (rc != SQLITE_OK) {
		// ERROR!
		SQLWERR2(err, rc, sqliteErrMsg)
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
	}
	
	if (sqliteErrMsg != NULL) sqlite3_free(sqliteErrMsg);

	return(err);
}


wError_t usedPorts_portsDB (GPtrArray *portsList) {
	//
	// Description:
	//	It allows the emulator's process to retrive the list of all in-use ports
	//	[!] The memory are used by portsList MUST be allocated by the function caller. The memory used to store every item
	//	    is allocated by the callback, dinamically.
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_INTERNAL
	//
	char                *sqlStatement = "SELECT busPort FROM portsDB WHERE pid>0 and role=\"S\";";
	int                 rc;
	struct queryRetData qrd;
	char                *sqliteErrMsg = NULL;

	// Initialization...
	qrd.list = portsList;
	wError_init(&qrd.err, WERROR_WITHMESSAGE);
	WERROR_GETCODE(qrd.err) = RS485EMULE_SUCCESS;
	
	rc = sqlite3_exec(portsDB, sqlStatement, _usedPorts_callback, (void*)&qrd, &sqliteErrMsg);
	if (rc != SQLITE_OK) {
		// ERROR!
		WERROR_GETCODE(qrd.err) = RS485EMULE_ERROR_INTERNAL;
		SQLWERR2(qrd.err, rc, sqliteErrMsg)
	}
		
	if (sqliteErrMsg != NULL) sqlite3_free(sqliteErrMsg);
	
	return(qrd.err);
}


void print_portsDB() {
	/*
	portsDBindexType  t;
	for (t=0; t<lastRecord; t++) 
		printf("%s (%d)\n", portsDB[t].port, portsDB[t].client);
	*/
	return;
}


wError_t pidChk_portsDB(const char *port) {
	//
	// Description:
	//	It marks the argument defined port as an available one
	//	
	// Returned value:
	//	RS485EMULE_SUCCESS              The port has been released
	//	RS485EMULE_WARNING_UNAVAILRES   The process is still running
	//	RS485EMULE_WARNING_NOTHINGTODO  The port was already available
	//	RS485EMULE_ERROR_INTERNAL       SQL query returned error
	//
	char  sqlStatement[256];
	pid_t pid;
	char  *sqliteErrMsg = NULL;
	int   rc = 0;
	WERROR_DECLARATION(err, WERROR_WITHMESSAGE, RS485EMULE_SUCCESS)
	
	sprintf(sqlStatement, "SELECT \"pid\" FROM portsDB WHERE busPort=\"%s\";", port);
	if ((rc = sqlite3_exec(portsDB, sqlStatement, _pidByPort_callback, (void*)&pid, &sqliteErrMsg)) != SQLITE_OK) {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		SQLWERR2(err, rc, sqliteErrMsg)
		
	} else if (pid > 0) {
		struct stat buff;
		char        folder[PATH_MAX];
		sprintf(folder, "/proc/%d", pid);

		if (stat(folder, &buff) < 0) {
			// The port-owner process is no more running...
			sprintf(sqlStatement, "UPDATE portsDB SET pid=0 WHERE busPort=\"%s\";", port);
			if ((rc = sqlite3_exec(portsDB, sqlStatement, NULL, NULL, &sqliteErrMsg)) != SQLITE_OK) {
				// ERROR!
				SQLWERR2(err, rc, sqliteErrMsg)
				WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
			}
		} else
			// WARNING! the process is running
			WERROR_GETCODE(err) = RS485EMULE_WARNING_UNAVAILRES;
	} else
		// No process to check for
		WERROR_GETCODE(err) = RS485EMULE_WARNING_NOTHINGTODO;
			
	if (sqliteErrMsg != NULL) sqlite3_free(sqliteErrMsg);

	return(err);
}

