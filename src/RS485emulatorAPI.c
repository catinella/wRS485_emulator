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
// File:     RS485emulatorAPI.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This module provides the functions used by BUS emulator's clients to requires a virtual serial port or to release a
//	used one
//	
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sqlite3.h>
#include <RS485_emulator.h>
#include <RS485_commonLib.h>
#include <RS485_portsDB.h>
#include <RS485emulatorAPI.h>

#if __DEBUG__ > 0
#include <debugTools.h>
#else
#define DBGTRACE       ;
#define ERRORBANNER(x) ;
#endif

static sqlite3 *portsDB = NULL;
static char    rs485_updateTool[PATH_MAX];

static int _getMasterPortCB (void *portname, int count, char **data, char **columns) {
	//
	// Description
	//	This callback is executed to read the name of the port usdd by the software fake-master

	if (count == 1 && strcmp(columns[0], "devPort") == 0)
		strcpy((char*)portname, data[0]);
	else
		*(char*)portname = '\0';
		
	return(0);
}

static int _getPortStatusCB (void *err_a, int count, char **data, char **columns) {
	//
	// Description
	//	This callback is executed to check the port availability.
	//	It is deveoped to be used just by release_RS485emulatorAPI() function
	//
	// Arguments:
	//	<custom> - Used to exchange data from/to the caller code
	//	count    - The number of columns in the result set
	//	data     - The row's data
	//	columns  - The column names
	//
	// Returned code:
	//	RS485EMULE_SUCCESS              The port is already assigned to the client
	//	RS485EMULE_INFO_AVAILABLEPORT   The port is available
	//	RS485EMULE_ERROR_INTERNAL       BUG
	//	RS485EMULE_ERROR_FORBIDDENOP    The port is already used by another (client) process
	//
	pid_t myPid = getpid();
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS);

	if (count != 1 || strcmp(columns[0], "pid") != 0) {
		// ERROR! (You should never get this error)
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		ERRORBANNER(WERROR_GETCODE(err))
	
	} else if (strcmp(data[0], "0") == 0) {
		// WARNING!
		WERROR_GETCODE(err) = RS485EMULE_INFO_AVAILABLEPORT;
		
	} else if (atoi(data[0]) != myPid) {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_FORBIDDENOP;
		ERRORBANNER(WERROR_GETCODE(err))
		//printf("The %d-proc is requiring a port used by %d-proc\n",myPid, atoi(data[0]));
	}
	

	if (err_a != NULL)
		memcpy(err_a, &err, sizeof(err));

	return(0);
}


int _sqlTransaction (sqlite3 *db, const char *op) {
	//
	// Description:
	//	This function is used to open or close a sqlite-transaction
	//
	// Returned value:
	//	The SQLite3 exit code
	//
	uint16_t tout = 2000;
	int      rc;
	while ((rc = sqlite3_exec(db, op, NULL, NULL, NULL)) && rc == SQLITE_BUSY && tout > 0) {
		usleep(1000); // 1ms
		tout--;
	}
	#if __DEBUG__ > 0
	printf("sqlTransaction(): RC=%d\n", rc);
	#endif
	return(rc);
}

//------------------------------------------------------------------------------------------------------------------------------
//                                        P U B L I C   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------
wError_t init_RS485emulatorAPI(void) {
	//
	// Description:
	//	It initializes the module's static values. This function must be called before then any other one belongs to
	//	this module
	//
	//	[!] rs485_updateTool
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_IOFAILED
	//
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS)

	#if TESTMODE > 0
	sprintf(rs485_updateTool, "%s/src/%s", PRJHOME, RS485EMULE_UPDATECMD);
	#else
	sprintf(rs485_updateTool, "%s/bin/%s", PREFIX, RS485EMULE_UPDATECMD);
	#endif
	
	if (sqlite3_open(RS485_PORTSDBFILE, &portsDB) != SQLITE_OK) {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
		ERRORBANNER(RS485EMULE_ERROR_IOFAILED)
	}
	
	return(err);
}

	
void close_RS485emulatorAPI(void) {
	//
	// Description:
	//	This function closes the internal SQLite DB
	//
	if (portsDB != NULL) sqlite3_close(portsDB);
	return;
}


wError_t getMPort_RS485emulatorAPI (char *fpname) {
	//
	// Description:
	//	This function is used by the process with master role to get the the serial port name it has to use.
	//	If the name is an empty string then it means the RS485 emulator has not yet created the virtual port
	//
	// Returned value:
	//	RS485EMULE_SUCCESS                The master port has been correctly assigned to the process
	//	RS485EMULE_WARNING_NOTHINGTODO    The port is already assigned to the process
	//	RS485EMULE_WARNING_TIMEOUT        The database is temporary unavailable
	//	RS485EMULE_ERROR_INTERNAL         Unknown (and unexpected) error
	//	RS485EMULE_ERROR_ITEMNOTFOUND     No port available
	//	RS485EMULE_ERROR_EXTTOOLFAILURE   rs485_updateTool returned an error
	//
	char sqlStatement[PATH_MAX+64];
	char port[PATH_MAX];
	int  rc;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_ERROR_ITEMNOTFOUND)

	*port = '\0';

	rc = _sqlTransaction(portsDB, "BEGIN;");
	if (rc == SQLITE_OK) {

		sprintf(sqlStatement, "SELECT devPort FROM portsDB WHERE role=\"M\";");
		rc = sqlite3_exec(portsDB, sqlStatement, _getMasterPortCB, (void*)port, NULL);
		if (rc != SQLITE_OK) {
			// ERROR!
			WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
			ERRORBANNER(WERROR_GETCODE(err))
			
		} else if (strlen(port) > 0) {
			sprintf(sqlStatement, "SELECT pid FROM portsDB WHERE devPort=\"%s\";", port);
			rc = sqlite3_exec(portsDB, sqlStatement, _getPortStatusCB, (void*)&err, NULL);
			
			if (rc != SQLITE_OK) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
				ERRORBANNER(WERROR_GETCODE(err))
		
			} else if (WERROR_GETCODE(err) == RS485EMULE_INFO_AVAILABLEPORT) {
				sprintf(sqlStatement, "UPDATE portsDB SET pid=%d WHERE devPort=\"%s\";", getpid(), port); 
				if (sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL) == SQLITE_OK) {
					WERROR_GETCODE(err) = RS485EMULE_SUCCESS;
					strcpy(fpname, port);

					if (system(rs485_updateTool) != 0) {
						// ERROR!
						WERROR_GETCODE(err) = RS485EMULE_ERROR_EXTTOOLFAILURE;
						ERRORBANNER(WERROR_GETCODE(err))
					}
				} else {
					// ERROR!
					WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
					ERRORBANNER(WERROR_GETCODE(err))
				}
			} else if (WERROR_ISSUCCESS(err)) {
				// The process is already the master's port owner
				WERROR_GETCODE(err) = RS485EMULE_WARNING_NOTHINGTODO;
				strcpy(fpname, port);
			}
		}		
		
		rc = _sqlTransaction(portsDB, "END;");

	} else if (rc == SQLITE_BUSY) {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_WARNING_TIMEOUT;
		ERRORBANNER(WERROR_GETCODE(err))

	} else {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		ERRORBANNER(WERROR_GETCODE(err))
	}	
	return(err);
}


wError_t release_RS485emulatorAPI (const char *serialport) {
	//
	// Description:
	//	It allows the client process to get an available virtual serial port
	//
	// Returne value:
	//	RS485EMULE_SUCCESS                OK, the porst is available now
	//	RS485EMULE_WARNING_NOTHINGTODO    The port was already available
	//	RS485EMULE_ERROR_ITEMNOTFOUND     The required port does not exists
	//	RS485EMULE_ERROR_INTERNAL         Database table has been created in a wrong way
	//	RS485EMULE_ERROR_FORBIDDENOP      The port is aòlready in use by another proc
	//	RS485EMULE_WARNING_TIMEOUT
	//
	char sqlStatement[256];
	int  rc;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_ERROR_ITEMNOTFOUND)
	
	rc = _sqlTransaction(portsDB, "BEGIN;");
	if (rc == SQLITE_OK) {

		sprintf(sqlStatement, "SELECT pid FROM portsDB WHERE devPort=\"%s\";", serialport);
		rc = sqlite3_exec(portsDB, sqlStatement, _getPortStatusCB, (void*)&err, NULL);
	
		if (rc != SQLITE_OK) {
			// ERROR!
			WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
			ERRORBANNER(WERROR_GETCODE(err))
			#if __DEBUG__ > 0
			fprintf(stderr, "SQLite(%d): %s\n", rc, sqlite3_errstr(rc));
			#endif
			
		} else if (WERROR_GETCODE(err) == RS485EMULE_INFO_AVAILABLEPORT) {
			// WARNING!
			WERROR_GETCODE(err) = RS485EMULE_WARNING_NOTHINGTODO;
	
		} else if (WERROR_GETCODE(err) == RS485EMULE_SUCCESS) {
			sprintf(sqlStatement, "UPDATE portsDB SET pid=0 WHERE devPort=\"%s\";", serialport); 
			rc = sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL);
			if (rc != SQLITE_OK) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
				ERRORBANNER(WERROR_GETCODE(err))
				#if __DEBUG__ > 0
				fprintf(stderr, "SQLite(%d): %s\n", rc, sqlite3_errstr(rc));
				#endif
				
			} else if ((rc = _sqlTransaction(portsDB, "END;")) &&rc != SQLITE_OK) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
				ERRORBANNER(WERROR_GETCODE(err))
				#if __DEBUG__ > 0
				fprintf(stderr, "SQLite(%d): %s\n", rc, sqlite3_errstr(rc));
				#endif

			} else if (system(rs485_updateTool) != 0) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_EXTTOOLFAILURE;
				ERRORBANNER(WERROR_GETCODE(err))
			}
		} 
	} else if (rc == SQLITE_BUSY) {
		// WARNING!
		WERROR_GETCODE(err) = RS485EMULE_WARNING_TIMEOUT;
		ERRORBANNER(WERROR_GETCODE(err))
		
	} else {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		ERRORBANNER(WERROR_GETCODE(err))
	}
	return(err);
}


wError_t takePort_RS485emulatorAPI (char *port) {
	//
	// Description:
	//	It allows you to take the first available port
	//	[!] This function is used just by the emulator's client
	//
	// TODO:
	//	Because every process should usually require one or two ports, the pre-compiled query is unuseful
	//	It should be rewritten with the callback method
	//
	// Returne value:
	//	RS485EMULE_SUCCESS               The port has been reserved
	//	RS485EMULE_ERROR_ITEMNOTFOUND    No available ports
	//	RS485EMULE_ERROR_FORBIDDENOP     You attemt to write into a read-only DB
	//	RS485EMULE_ERROR_UNKNOWN         Unknown error
	//	RS485EMULE_WARNING_TIMEOUT       Reached timeout. But the function could be re-executed with success
	//	RS485EMULE_ERROR_UNAVAILRES      The function failed, the resource was unexpectly unavailable
	//
	char         *sqlStatement = "SELECT devPort FROM portsDB WHERE pid=0 and role=\"S\";";
	sqlite3_stmt *stmt         = NULL;
	int          rc;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_ERROR_ITEMNOTFOUND)
	
	DBGTRACE
	rc = _sqlTransaction(portsDB, "BEGIN;");
	if (rc == SQLITE_OK) {

		if (sqlite3_prepare(portsDB, sqlStatement, -1, &stmt, NULL) == SQLITE_OK) {
		
			DBGTRACE
			while (WERROR_GETCODE(err) == RS485EMULE_ERROR_ITEMNOTFOUND && (rc = sqlite3_step(stmt)) && rc != SQLITE_DONE) {
				if (rc == SQLITE_BUSY) {
					usleep(50000);
					printf("DB busy\n");
					
				} else if (rc == SQLITE_ERROR) {
					// ERROR!
					printf("rc = %d\n", rc);
					WERROR_GETCODE(err) = RS485EMULE_ERROR_EXTPROCFAILED;
					ERRORBANNER(WERROR_GETCODE(err))
					break;
	
				} else if (rc == SQLITE_ROW) {
					// SUCCESS!
					strcpy(port, (const char*)sqlite3_column_text(stmt, 0));
					WERROR_GETCODE(err) = RS485EMULE_SUCCESS;
					break;
				}
			}
		
			sqlite3_finalize(stmt);
		
		} else {
			// ERROR!
			WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
			ERRORBANNER(WERROR_GETCODE(err))
		}

		if (WERROR_ISERROR(err) == false) {
			char sqlStatement[256];
			
			sprintf(sqlStatement, "UPDATE portsDB SET pid=%d WHERE devPort=\"%s\";", getpid(), port);
			
			{
				uint16_t tout = 1000;
				rc = SQLITE_BUSY;
				while (rc == SQLITE_BUSY && tout > 0) {
					rc = sqlite3_exec(portsDB, sqlStatement, NULL, NULL, NULL);
					tout--;
					if (rc == SQLITE_BUSY) usleep(1000);
				}
			}

			// ERRORS!!
			{
				bool ef = true;
				if      (rc == SQLITE_READONLY)  WERROR_GETCODE(err) = RS485EMULE_ERROR_FORBIDDENOP;
				else if (rc == SQLITE_BUSY)      WERROR_GETCODE(err) = RS485EMULE_ERROR_UNAVAILRES;
				else if (rc != SQLITE_OK)        WERROR_GETCODE(err) = RS485EMULE_ERROR_UNKNOWN;
				else                             ef = false;
			
				if (ef) {ERRORBANNER(WERROR_GETCODE(err))}
			}
		}
	
		if (WERROR_ISERROR(err) == false) {
			rc = _sqlTransaction(portsDB, "END;");
			if (rc != SQLITE_OK) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_UNKNOWN;
				ERRORBANNER(WERROR_GETCODE(err))
				
			} else if (system(rs485_updateTool) != 0) {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_EXTTOOLFAILURE;
				ERRORBANNER(WERROR_GETCODE(err))
			}
		}
		
		if (WERROR_ISERROR(err)) *port = '\0';
		
	} else if (rc == SQLITE_BUSY) {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_WARNING_TIMEOUT;
		ERRORBANNER(WERROR_GETCODE(err))
		
	} else {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_INTERNAL;
		ERRORBANNER(WERROR_GETCODE(err))
	}

	return(err);
}
