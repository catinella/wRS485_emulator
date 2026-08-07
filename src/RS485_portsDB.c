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
//	This module provides functions to manage the ports-list database file and its concurrent access too.
//	
//	Database structure:
//		+----------+----------+----------+-----------+
//		|   role   | busPort  |  devPort |    pid    |
//		| (string) | (string) | (string) | (integer) |
//		+----------+----------+----------+-----------+
//	
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
//#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485_portsDB.h>


#ifndef DBGTRACE
#define DBGTRACE ;
#endif


struct queryRetData {
	char              **list;
	RS485emErrorCodes err;
};

static sqlite3 *portsDB = NULL;


static int pidByPort_callback (void *pid, int count, char **data, char **columns) {
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


static int usedPorts_callback (void *psl, int count, char **data, char **columns) {
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
	static portsDBindexType numups = 0;

	if (psl == NULL)
		// Function initialization
		numups = 0;

	else {
		struct queryRetData *qrd = (struct queryRetData*)psl;
		
		if (count != 1 || strcmp(columns[0], "busPort") != 0) {
			// ERROR! (You should never get this error)
			qrd->err = RS485EMULE_ERROR_INTERNAL;
		} else {
			qrd->list[numups] = strdup(data[0]);
			//BGTRACE
		}
		numups++;
		qrd->list[numups] = NULL;
		
	}
	return(0);
}
//------------------------------------------------------------------------------------------------------------------------------
//                                             P U B L I C   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------

RS485emErrorCodes init_portsDB() {
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
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	int               rc  = 0;

	unlink(RS485_PORTSDBFILE);
	rc = sqlite3_open(RS485_PORTSDBFILE, &portsDB);
	if (rc == SQLITE_IOERR || rc == SQLITE_PERM)
		err = RS485EMULE_ERROR_IOFAILED;
	else if (rc != SQLITE_OK)
		err = RS485EMULE_ERROR_INTERNAL;
	else {
		char *sqlStatement = "CREATE TABLE portsDB( \
			\"role\"    TEXT,                     \
			\"busPort\" TEXT NOT NULL UNIQUE,     \
			\"devPort\" TEXT NOT NULL UNIQUE,     \
			\"pid\"     INTEGER NOT NULL          \
		);";
		
		// SQL statement execution
		if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) != SQLITE_OK) {
			err = RS485EMULE_ERROR_INTERNAL;
			sqlite3_close(portsDB);
			portsDB = NULL;
		}
	}
	
	if (
		getuid() == 0 && err == RS485EMULE_SUCCESS && (
			chmod(RS485_PORTSDBFILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0 ||
			chown(RS485_PORTSDBFILE, 0, RS485EMULE_GROUP)             != 0
		)
	)
		// ERROR!
		err = RS485EMULE_ERROR_FORBIDDENOP;

	return(err);
}


void close_portsDB() {
	//
	// Description:
	//	This function closes the internal SQLite DB
	//
	if (portsDB != NULL) {
		sqlite3_close(portsDB);
		unlink(RS485_PORTSDBFILE);
	}
	return;
}


RS485emErrorCodes push_portsDB(const char* busport, const char* devport, virtualPortRole role) {
	//
	// Description:
	//	It allows you to add a new virtual serial port to the in-process DB
	//	[!] It is used by the BUS emulator to build the ports DB at the beginning status
	//
	// Returne value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_INTERNAL
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	char              sqlStatement[256];
	char              roleChar = 'S';
	
	if (role == vPortMaster) roleChar = 'M';
	sprintf(sqlStatement, "INSERT INTO portsDB VALUES(\"%c\", \"%s\", \"%s\", 0);", roleChar, busport, devport);
	if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) != SQLITE_OK) 
		err = RS485EMULE_ERROR_INTERNAL;

	return(err);
}


RS485emErrorCodes usedPorts_portsDB (char **portsList) {
	//
	// Description:
	//	It allows the emulator's process to retrive the list of all in-use ports
	//	[!] The memory are used by portsList MUST be allocated by the function caller. The memory used to store every item is
	//	    allocated by the callback, dinamically.
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_INTERNAL
	//
	RS485emErrorCodes   err = RS485EMULE_SUCCESS;
	char                *sqlStatement = "SELECT busPort FROM portsDB WHERE pid>0 and role=\"S\";";
	int                 rc;
	struct queryRetData qrd;

	// Initialization...
	usedPorts_callback(NULL, 0, NULL, NULL);
	qrd.list = portsList;
	qrd.err  = RS485EMULE_SUCCESS;
	portsList[0] = '\0';
	
	rc = sqlite3_exec(portsDB, sqlStatement, usedPorts_callback, (void*)&qrd, NULL);
	if (rc != SQLITE_OK)
		err = RS485EMULE_ERROR_INTERNAL;
	else 
		err = qrd.err;
		
	return(err);
}


void print_portsDB() {
	/*
	portsDBindexType  t;
	for (t=0; t<lastRecord; t++) 
		printf("%s (%d)\n", portsDB[t].port, portsDB[t].client);
	*/
	return;
}


RS485emErrorCodes pidChk_portsDB(const char *port) {
	//
	// Description:
	//	It marks the argument defined port as an available one
	//	
	// Returned value:
	//	RS485EMULE_SUCCESS              The port has been released
	//	RS485EMULE_WARNING_NOTHINGTODO  The process is still running
	//	RS485EMULE_ERROR_INTERNAL       SQL query returned error
	//
	RS485emErrorCodes err = RS485EMULE_SUCCESS;
	char              sqlStatement[256];
	pid_t             pid;
	
	sprintf(sqlStatement, "SELECT \"pid\" FROM portsDB WHERE busPort==\"%s\";", port);
	if (sqlite3_exec(portsDB, sqlStatement, pidByPort_callback, (void*)&pid, NULL) != SQLITE_OK) {
		err = RS485EMULE_ERROR_INTERNAL;

	} else if (pid > 0) {
		struct stat buff;
		char        folder[PATH_MAX];
		sprintf(folder, "/proc/%d", pid);
		
		if (stat(folder, &buff) < 0) {
			sprintf(sqlStatement, "UPDATE portsDB SET pid=0 WHERE busPort=\"%s\";", port);
			if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) != SQLITE_OK)
				err = RS485EMULE_ERROR_INTERNAL;
		} else
			// The process is still running
			err = RS485EMULE_WARNING_NOTHINGTODO;
	} else
		// No process to check for
		err = RS485EMULE_WARNING_NOTHINGTODO;
			
	return(err);
}

