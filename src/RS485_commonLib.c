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
// File:     RS485_commonLib.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	
//	
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <RS485_emulator.h>
#include <RS485_commonLib.h>

wError_t RS485emu_checkForProcStatus (pid_t pid, char *status) {
	//
	// Descfription:
	//	It checks for the status of the process associated to the given PID. The status will be written on the memory defined
	//	by the "status" argument. Please, read proc(5) manpage for further details
	//
	// Returned value:
	//	RS485EMULE_SUCCESS               The process is up and running
	//	RS485EMULE_WARNING_ITEMNOTFOUND  The process is not currently running
	//	RS485EMULE_ERROR_CORRUPTEDDATA   Unexpected data in /proc filesystem
	//	RS485EMULE_ERROR_IOFAILED        File reading operation failed on /proc/<pid>/stat file
	//
	struct stat statbuf;
	char        procPid[PATH_MAX];
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS)

	*status = '\0';
	sprintf(procPid, "/proc/%d", pid);
	if (stat(procPid, &statbuf) == 0) {
		if (S_ISDIR(statbuf.st_mode)) {
			sprintf(procPid, "/proc/%d/stat", pid);
			if (stat(procPid, &statbuf) == 0) {
				FILE *fh = fopen(procPid, "r");
				if (fh != NULL) {
					int  tpid;
					char timg[PATH_MAX];
					char tstat;
					
					if (fscanf(fh, "%d %s %c", &tpid, timg, &tstat) == 3)
						*status = tstat;
					else
						// ERROR! Unexpected data format
						WERROR_GETCODE(err) = RS485EMULE_ERROR_CORRUPTEDDATA;
						
					fclose(fh);
				} else
					// ERROR! (I cannot open the /proc/<pid>/stat file)
					WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
			} else
				// WARNING!
				WERROR_GETCODE(err) = RS485EMULE_WARNING_ITEMNOTFOUND;
		} else
			// ERROR! (You should never get this error)
			WERROR_GETCODE(err) = RS485EMULE_ERROR_CORRUPTEDDATA;
	} else
		// The process no more exists
		*status = 'X';
		
	return(err);
}

wError_t RS485emu_readPidFile (pid_t *pid, const char *file) {
	//
	// Descfription:
	//	It retrives the pid information by the argument defined pid-file
	//
	// Returned value:
	//	RS485EMULE_SUCCESS               PID has been correctly read       
	//	RS485EMULE_ERROR_CORRUPTEDDATA   The pid-file does not contain numeric PID
	//	RS485EMULE_ERROR_IOFAILED        I cannot open the pid-file
	//
	FILE      *fh = fopen(file, "r");
	size_t    msz = 16 * sizeof(char);
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS)
	
	if (fh != NULL) {
		pid_t tmp  = 0;
		char  *line = (char*)malloc(msz);
		if (line != NULL) {
			memset(line, 0, msz);
			if (getline(&line, &msz, fh) > 0) {
				tmp = atoi(line);
				if (tmp > 0)
					*pid = tmp;
				else
					// ERROR!
					WERROR_GETCODE(err) = RS485EMULE_ERROR_CORRUPTEDDATA;
			} else {
				// ERROR!
				WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
			}
			free(line);
		} else
			// ERROR!
			WERROR_GETCODE(err) = RS485EMULE_ERROR_NOSYSRESOURCE;
			
		fclose(fh);
	} else {
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
	}
	
	return(err);
}

wError_t RS485emu_writePidFile (pid_t pid, const char *file) {
	//
	// Descfription:
	//	It writes the given pid in the argument defined file
	//
	// Returned value:
	//	RS485EMULE_SUCCESS           PID has been correctly read       
	//	RS485EMULE_ERROR_IOFAILED    I cannot open the pid-file
	//
	// TODO: strerror(errno) message management adding
	//
	FILE *fh = fopen(file, "w");
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS)
	
	if (fh != NULL) {
		fprintf(fh, "%d", pid);
		fclose(fh);
		
		// The BUS is running as root, but the user must be able to read the pid-file
		if (chmod(file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) != 0)
			// ERROR!
			WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
	} else
		// ERROR!
		WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
	
	return(err);
}


bool RS485emu_stringSplitter (const char *src, char splitter, char *firstField, char *secondField) {
	//
	// Descfription:
	//	This function allows you to split a string in two parts where their boundaries are defined by the argument defined
	//
	// Returned value:
	//	true    The splitter char has been found
	//	false   No splitter char in the defined chgaracters string
	//
	bool flag = false;
	uint16_t si = 0, fi = 0;
	uint8_t st = 0;
	
	for(si = 0; src[si] != '\0'; si++) {
		if (src[si] != ' ' && src[si] != '\t') {
			if (st == 0) {
				if (src[si] == splitter) {
					st = 1;
					flag = true;
					if (firstField != NULL) {
						firstField[fi] = '\0';
						fi = 0;
					} else if (secondField == NULL) {
						break;
					}
				} else if (firstField != NULL) {
					firstField[fi] = src[si];
					fi++;
				}
			} else if (st == 1 && secondField != NULL) {
				secondField[fi] = src[si];
				fi++;
				
			} else
				break;
		}
	}
	if (secondField != NULL && flag)          secondField[fi] = '\0';
	if (firstField  != NULL && flag == false) firstField[0] = '\0';

	return(flag);
}


bool RS485emu_chomp (char *text) {
	//
	// Description:
	//	It removes all characters, in the argument defined string, from the carriage-return (<CR>) character to the end
	//
	// Returned value:
	//	true    The characters string has been reduced
	//	false   No <CR> found in the string
	//
	unsigned int x = 0;
	bool         flag = false;
	while (text[x] != '\0' && text[x] != '\n') x++;
	if (text[x] == '\n') {
		text[x] = '\0';
		flag = true;
	}
	return(flag);
}


unsigned int RS485emu_dataReceive (int fd, void *buffer, unsigned int size) {
	//
	// Description:
	//	It reads the argument defined number of bytes, and write then into the memory area pointed by buff argument
	//	This function has been developed for unbuffered channels
	//
	//	[!] If the argument defined buffer is NULL, then the function unfill the channel
	//
	// Returned value:
	//	<n>   The number of received bytes
	//
	uint8_t      tmpBuff[64];
	unsigned int ts      = 0;
	int          ps      = 1;
	uint8_t      tmpSize = 0;
	uint8_t      *ptr    = NULL;
	
	while (ts < size && ps > 0) {
		if (buffer != NULL) {
			ptr = (uint8_t*)buffer+ts;               // Aritmetic operations on void* produces GCC Warning 
			ps = read(fd, (void*)ptr, (size-ts));
		} else {
			ptr = (uint8_t*)tmpBuff+ts;
			if ((size-ts) > 64) tmpSize = 64;
			else                tmpSize = (size-ts);
			ps = read(fd, (void*)ptr, tmpSize);
		}
		if (ps > 0) ts = ts + ps;
	}
	return(ts);
}


unsigned int RS485emu_dataSend (int fd, const void *buffer, unsigned int size) {
	//
	// Description:
	//	It writes the argument defined number of bytes in the specified (fd) channel
	//	It has been developed for unbuffered channels
	//
	// Returned value:
	//	<n>   The number of sent bytes
	//
	unsigned int ts = 0;
	int          ps = 1;
	uint8_t      *ptr    = NULL;

	while (ts < size && ps > 0) {
		ptr = (uint8_t*)buffer+ts;
		ps = write(fd, (void*)ptr, (size-ts));
		if (ps > 0) ts = ts + ps;
	}
	return(ts);
}
