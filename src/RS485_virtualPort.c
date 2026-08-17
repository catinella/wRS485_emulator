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
// File:     RS485_virtualPort.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This library provides all functions you need to create and use a virual serial port.
//
//	
//	
//	                             +-----------------------------------------+ pid=x
//	       A:B ------------------+                                         |
//	       C:D ------------------+              Main process               | ppid=1
//	       E:F ------------------+                                         |
//	       G:H ------------------+                                         |
//	                             +--------+----+----+----+--+--------------+
//	                                      |    |    |    |  |
//	                      +---------------+    |    |    |  +---------------+
//	                      |                    |    |    |                  |
//                          |             +------+    |    +--------+         |
//	                      |             |           |             |         |
//	                  +---+------+ +----+-----+ +---+------+ +----+-----+ +-+--------+
//	                  | A:B port | | C:D port | | E:F port | | G:H port | | I:L port |
// 	                  | manager  | | manager  | | manager  | | manager  | | manager  |
//	                  +----------+ +----------+ +----------+ +----------+ +----------+
//	                   (ppid=x)      (ppid=x)     (ppid=x)     (ppid=x)     (ppid=x)
//	
//	
//	
//	
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <dirent.h>
#include <RS485_virtualPort.h>
#include <RS485_portsDB.h>
#include <debugTools.h>

#ifndef DBGTRACE
#define DBGTRACE ;
#endif

// Maximum number of files in any list
#define RS485_FILESMAX 1024

typedef struct {
	char *list[RS485_FILESMAX];
} filesList_t;

static bool initFlag = false;

//------------------------------------------------------------------------------------------------------------------------------
//                                    P R I V A T E   F U N C T I O N S 
//------------------------------------------------------------------------------------------------------------------------------

static void _free_FilesList (filesList_t *files) {
	//
	// Desription:
	//	It free al system resources used to store the filenames list. [!] the list must be NULL terminated
	//
	uint16_t t;
	for (t=0; t<RS485_FILESMAX; t++) {
		if (files->list[t] == NULL)
			break;
		else {
			free(files->list[t]);
			files->list[t] = NULL;
		}
	}
	return;	
};


static void _init_FilesList (filesList_t *files) {
	//
	// Description:
	//	Structure initialization
	//
	uint16_t t;
	if (initFlag == false) {
		init_portsDB();
		initFlag = true;
	}
	for (t=0; t<RS485_FILESMAX; t++) {
		files->list[t] = NULL;
	}
	return;
}


static RS485emErrorCodes_t _getDirContent (const char *path, filesList_t *files) {
	//
	// Desription:
	//	It stores the files belong to the "path" argument defined folder in the "filesList" given struct
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_IOFAILED
	//	RS485EMULE_ERROR_DATAOVERFLOW
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	DIR           *dh = NULL;
	struct dirent *entry;
	uint16_t      counter = 0;
	
	_free_FilesList(files);

	if ((dh = opendir(path)) && dh == NULL)
		// ERROR
		err = RS485EMULE_ERROR_IOFAILED;

	else {
		while((entry = readdir(dh)) && entry != NULL && err == RS485EMULE_SUCCESS) {
			if (counter >= (RS485_FILESMAX - 1)) {
				err = RS485EMULE_ERROR_DATAOVERFLOW;
			} else {
				files->list[counter] = strdup(entry->d_name);
				counter++;
			}
		}
		closedir(dh);
	}

	return(err);
}


static RS485emErrorCodes_t _search_FilesList (filesList_t files, const char *target) {
	//
	// Description:
	//	It looks for the "target" argument defined string in the "files" list.
	//
	// Returned value
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_WARNING_ITEMNOTFOUND
	//
	RS485emErrorCodes_t err = RS485EMULE_WARNING_ITEMNOTFOUND;
	uint16_t          t;
	
	for (t=0; t<RS485_FILESMAX; t++) {
		if (files.list[t] == NULL)
			break;
		else if (strcmp(files.list[t], target) == 0) {
			err = RS485EMULE_SUCCESS;
			break;
		}
	}
	return(err);
}


static RS485emErrorCodes_t _push_FilesList (filesList_t *files, const char *filename) {
	//
	// Description:
	//	It adds a new item to the argument defined list
	//
	// Returned value
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_DATAOVERFLOW
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	uint16_t          t;
	
	for (t=0; t<RS485_FILESMAX; t++) {
		if (files->list[t] == NULL) break;
	}
	if (t<RS485_FILESMAX) files->list[t] = strdup(filename);
	else                  err = RS485EMULE_ERROR_DATAOVERFLOW;

	return(err);
}


static uint16_t _size_FilesList (filesList_t files) {
	//
	// Description:
	//	It returns the number of stored items
	//
	uint16_t t;
	
	for (t=0; t<RS485_FILESMAX; t++) {
		if (files.list[t] == NULL) break;
	}
	return(t);
}


//------------------------------------------------------------------------------------------------------------------------------
//                                     P U B L I C   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------
void init_virtualPort (virtualPort_t *item) {
	//
	// Description:
	//	It ititializes the argument defined struct
	//
	item->pid = 0;
	item->fd  = -1;
	memset((void*)item->port, 0, (PATH_MAX * sizeof(char)));
	return;
}


RS485emErrorCodes_t free_virtualPort (virtualPort_t *item) {
	//
	// Description:
	//	It releases the previousely allocated resources used by the argument defined struct. The procedure will kill the
	//	port owner process, too.
	//	[!] This function is called by the emulator process during its shouting-down procedure, only.
	//
	// Returned code:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_WARNING_NOTHINGTODO
	//
	char              status;
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	
	// Port's channel closing
	if (item->fd > 0) close(item->fd);

	//
	// Port manager shuting down
	//
	if (
		item->pid > 0                                                &&
		RS485emu_checkForProcStatus(item->pid, &status) == RS485EMULE_SUCCESS &&
		(toupper(status) == 'R' || toupper(status) == 'S')           
	) {
		//DBGTRACE
		int wstatus = 0;
		//fprintf(stdout, "I am trying to kill the %d-process\n", item->pid); fflush(stdout);
		kill(item->pid, SIGTERM);
		usleep(5000);
	
		if (waitpid(item->pid, &wstatus, 0) < 0) {
			//printf("ERROR!: %s\n", strerror(errno));
			DBGTRACE
			err = RS485EMULE_WARNING_TIMEOUT;
			kill(item->pid, SIGKILL);
			waitpid(item->pid, &wstatus, WNOHANG);
		}
	} else
		// WARNING! (port not in use)
		err = RS485EMULE_WARNING_NOTHINGTODO;
	
	init_virtualPort(item);

	return(err);
}


virtualPort_t* new_virtualPort() {
	//
	// Description:
	//	It creates a new object allocated into the heap memory area
	//
	virtualPort_t *item = (virtualPort_t*)malloc(sizeof(virtualPort_t));
	init_virtualPort(item);

	return(item);
}


RS485emErrorCodes_t open_virtualPort (virtualPort_t* item) {
	//
	// Description:
	//	This function allows you to open the virtual serial port
	//
	// Returned value:
	//	RS485EMULE_SUCCESS              Procedure successfully terminated
	//	RS485EMULE_ERROR_UNAVAILRES     The port is already in use
	//	RS485EMULE_ERROR_IOFAILED       I cannot open the port
	//	RS485EMULE_ERROR_NOSYSRESOURCE  Port setting procedure failed 
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	struct termios    options;

	if (item->fd > 0)
		// ERROR!
		err = RS485EMULE_ERROR_UNAVAILRES;
	
	else if ((item->fd = open(item->port, O_RDWR|O_NOCTTY|O_ASYNC)) < 0)
		// ERROR!
		err = RS485EMULE_ERROR_IOFAILED;
	
	else if (
		tcgetattr(item->fd, &options)               < 0 ||
		cfsetispeed(&options, RS485EMULE_BAUDRATES) < 0 ||
		cfsetospeed(&options, RS485EMULE_BAUDRATES) < 0
	) 
		// ERROR!
		err = RS485EMULE_ERROR_NOSYSRESOURCE;

	else {
		cfmakeraw(&options);
		if (tcsetattr(item->fd, TCSANOW, &options) < 0)
			err = RS485EMULE_ERROR_NOSYSRESOURCE;
	}
	return(err);
}


RS485emErrorCodes_t create_virtualPort (virtualPort_t* item, virtualPortRole_t role) {
	//
	// Description:
	//	This function allows you to create a new virtual serial port based on socat tool.
	//	Every "socat" command call creates 2 linked virtual-ports, and a resident process that will be resposible to move
	//	data from any of the two to the other
	//	
	//	pid=X           ppid=1        file-a           pid=Y   ppid=X         file-b           pid=W    ppid=Z
	//	+--------------------+       +--------+        +------------+       +--------+         +-------------+
	//	| RS485 BUS emulator |<=====>| port-a |<------>| socat proc |<----->| port-b |<=======>| client proc |
	//	+--------------------+       +--------+        +------------+       +--------+         +-------------+
	//	
	// Returned value:
	//	RS485EMULE_SUCCESS               The object has been set correctly
	//	RS485EMULE_ERROR_EXTTOOLFAILURE  The discovery found less or more then 2 ports.
	//	RS485EMULE_ERROR_IOFAILED        The new port opening operation failed
	//	RS485EMULE_ERROR_NOSYSRESOURCE   There are no resources to fork a new process
	//	RS485EMULE_ERROR_FORBIDDENOP     File attributes changing operation failed
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	filesList_t  oldPorts;

	_init_FilesList(&oldPorts);
	err = _getDirContent(RS485EMULE_PORTSFOLDER, &oldPorts);
		
	if (err == RS485EMULE_SUCCESS) {
			
		item->pid = fork();

		if (item->pid == 0) {
			// execv() returns ONLY if there is an error
			//printf("Sub-process: %d\n", getpid());
			execl(
				RS485EMULE_PORTSMAKER_CMD,
				RS485EMULE_PORTSMAKER_CMD,
				RS485EMULE_PORTSMAKER_ARG1,
				RS485EMULE_PORTSMAKER_ARG2,
				RS485EMULE_PORTSMAKER_ARG3,
				(char*)NULL
			);
			exit(RS485EMULE_ERROR_EXTPROCFAILED);
	
		} else if (item->pid > 0) {
			filesList_t newPorts;
			filesList_t tmpPorts;
			
			//
			// The process MUST wait for the port creation procedure. For this reason do not set lower then 10ms sleeping time!
			// It shorter times could create ghost bugs
			//
			usleep(50000); 


			_init_FilesList(&newPorts);
			_init_FilesList(&tmpPorts);
			err = _getDirContent(RS485EMULE_PORTSFOLDER, &newPorts);
	
			if (err == RS485EMULE_SUCCESS) {
				//
				// Discovering the portname
				//
				uint16_t t;
				for (t=0; t<RS485_FILESMAX; t++) {
					if (newPorts.list[t] == NULL)
						break;
					else {
						if (_search_FilesList(oldPorts, newPorts.list[t]) == RS485EMULE_WARNING_ITEMNOTFOUND) {
							//printf("New port: %s\n", newPorts.list[t]);
							err = _push_FilesList(&tmpPorts, newPorts.list[t]);
							if (err != RS485EMULE_SUCCESS) break;
						}
					}
				}

				if (err == RS485EMULE_SUCCESS) {
					if (_size_FilesList(tmpPorts) == 2) {
						// [!] ports[0] is for the user's applications; ports[1] is connected to the BUS
						char bport[PATH_MAX];
						char dport[PATH_MAX];
						
						memset(bport, 0, PATH_MAX * sizeof(char));
						memset(dport, 0, PATH_MAX * sizeof(char));
						strcpy(bport, RS485EMULE_PORTSFOLDER);
						strcat(bport, "/");
						strcpy(dport, bport);

						// port used by BUS emulator
						strcat(bport, tmpPorts.list[1]);
						strcat(item->port, bport);

						// port used by the (master/slave) client
						strcat(dport, tmpPorts.list[0]);
						if (role == RS485EMULE_PORTSLAVE) err = push_portsDB(bport, dport, RS485EMULE_PORTSLAVE);
						else                    err = push_portsDB(bport, dport, RS485EMULE_PORTMASTER);

						//
						// [!] If the BUS emulator is running as root then, in order to allow the user's process to
						//     send and receive data, the assigned port file permissions must be set properly
						//
						if (getuid() == 0) {
							 if (
								chmod(bport, S_IRUSR|S_IWUSR)                  != 0 ||
								chmod(dport, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)  != 0
							) {
								// ERROR!
								err = RS485EMULE_ERROR_FORBIDDENOP;
								DBGTRACE
							} // else
								// printf("%s port: 600\n%s port: 660\n", bport, dport);
						 } else {
							// printf("[!] BUS emulator is running with %d-user priviledges\n", getuid());
						 }
					} else
						// ERROR!
						err = RS485EMULE_ERROR_EXTTOOLFAILURE;
				}
				
				_free_FilesList(&newPorts);
				_free_FilesList(&oldPorts);
				_free_FilesList(&tmpPorts);
			}
	
		} else
			// ERROR!
			err = RS485EMULE_ERROR_NOSYSRESOURCE;
	}
	return(err);
}

	
void print_virtualPort (virtualPort_t item) {
	//
	// Description:
	//	It prints the object's content. It has been developed for debug purples
	//
	printf("Port filename:   %s\n", item.port);
	printf("File-descriptor: %d\n", item.fd);
	printf("Manager's' pid:  %d\n", item.pid);
	printf("\n");

	return;
}


RS485emErrorCodes_t send_virtualPort (virtualPort_t item, const void *data, chunkDataSize_type size) {
	//
	// Description:
	//	It sends the argument defined bytes to the virtual fake device
	//
	// Returned value:
	//	RS485EMULE_SUCCESS
	//	RS485EMULE_ERROR_IOFAILED
	//	RS485EMULE_ERROR_ILLEGALARG
	//
	RS485emErrorCodes_t   err = RS485EMULE_SUCCESS;
	chunkDataSize_type ts  = 0;
	chunkDataSize_type ps  = 0;

	if (item.fd > 0) {
		while (ts < size && err == RS485EMULE_SUCCESS) {
			ps = write(item.fd, (data+ts), size-ts);
			if (ps < 0)
				// ERROR!
				err = RS485EMULE_ERROR_IOFAILED;
			else
				ts = ts + ps;
		}
	} else
		// ERROR!
		err = RS485EMULE_ERROR_ILLEGALARG;
		
	return(err);
}


RS485emErrorCodes_t recv_virtualPort (virtualPort_t item, void *data, chunkDataSize_type size) {
	//
	// Description:
	//	It read from the virtual fake device the argument defined (size) number of bytes
	//	Use this function when you know the expected data size
	//
	// Returned value:
	//	RS485EMULE_SUCCESS           The required data has been succesfully read
	//	RS485EMULE_ERROR_IOFAILED    read() syscall failed
	//	RS485EMULE_ERROR_ILLEGALARG  The port has not been previousely open
	//
	RS485emErrorCodes_t   err = RS485EMULE_SUCCESS;
	chunkDataSize_type ts  = 0;
	chunkDataSize_type ps  = 0;

	if (item.fd > 0) {
		while (ts < size && err == RS485EMULE_SUCCESS) {
			ps = read(item.fd, (data+ts), size-ts);
			if (ps < 0)
				// ERROR!
				err = RS485EMULE_ERROR_IOFAILED;
			else
				ts = ts + ps;
		}
	} else
		// ERROR!
		err = RS485EMULE_ERROR_ILLEGALARG;
		
	return(err);
}


RS485emErrorCodes_t close_virtualPort (virtualPort_t *item) {
	//
	// Description
	//	This function is used by the BUS emulator process to close the file descriptos associated to a port released by the
	//	user
	//
	// Returned values:
	//	RS485EMULE_SUCCESS                The port has been correctly closed
	//	RS485EMULE_WARNING_NOTHINGTODO    The port was already closed
	//
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	if (item->fd < 0)
		err = RS485EMULE_WARNING_NOTHINGTODO;
	else {
		close(item->fd);
		item->fd = -1;
	}
	return(err);
}
