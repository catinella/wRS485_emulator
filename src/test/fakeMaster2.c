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
// File:     fakeMaster2.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This software is used to test the RS485 software emulatorhas with one or many fakeSerialDev2 istances. 
//	When this fake master device is connected to the RS485 bus emulator, it starts to send foo data packages to the slaves
//	where their IDs are equals to the argument defined ones. Every slave will reply with its datapackage that allows the
//	master to verify the content.
//
//	use: fakeMaster1 --ids=<1-255>[,<1-255>][...]  [--loopSleep=<milliseconds>]
//
//	Maximum allowed ID
//	==================
//	In order to provide a simple test, the exchanged data has 2bytes size (1byte for ID, 1 for value). The data size would
//	allow wide range (1-255) for ID and for data too. But, because the reply data is calculated to molltiply the two fields,
//	and the replied data size has 1byte size too, the max allowed ID and sent data must be less then 16 (2^4)!
//
//
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <debugTools.h>
#include <libForTests.h>
#include <RS485_commonLib.h>
#include <RS485emulatorAPI.h>
#include <fakeMaster1.h>

#ifndef DBGTRACE
#define DBGTRACE ;
#endif

//#define FMASTER_
#define FMASTER_LOOPSLEEP 50


struct option myopts[] = {
	 {"ids",       required_argument, 0, 'i'},
	 {"loopSleep", required_argument, 0, 'l'},
	 {0,           0,                 0,  0 }
};

bool loop = true;


void sigHandler (int signum) {
	DBGTRACE
	if (signum == SIGTERM || signum == SIGINT) loop = false;
	return;
}

void summary (char *file) {
	fprintf(stderr, "Use: %s ids=<1-255>[,<1-255>][...] [--loopSleep=<milliseconds>]\n", file);

	return;
}

uint8_t listConverter (uint8_t *list, char *str) {
	//
	// Description:
	//	This function converts the items list string in an IDs list
	//	Every ID must be unique and its value must be less the 16
	//
	char    *start = str;
	char    *end = NULL;
	char    *idx = NULL;
	uint8_t listIdx = 0;
	char    tmpStr[4];
	
	for (idx = str; *idx != '\0'; idx++) {
		if (*idx == ',') {
			start = idx+1;
			
		} else if (*(idx+1) == ',' || *(idx+1) == '\0') {
			end = idx;
			memset(tmpStr, '\0', 4);
			strncpy(tmpStr, start, (end-start+1));
			list[listIdx] = atoi(tmpStr);
			if (list[listIdx] > 15) {
				printf("ERROR! every ID must be less then 16\n");
				listIdx = 0;
			} else
				listIdx++;
		}
	}
	return(listIdx);		
}


int getIndexByID (uint8_t *list, uint8_t id) {
	int  idx = 0;
	bool flag = false;
	
	while (list[idx] != 0 && flag == false) {
		if (list[idx] == id)
			flag = true;
		else
			idx++;
	}

	if (flag == false) idx = -1;

	return(idx);
}
//------------------------------------------------------------------------------------------------------------------------------
//                                                         M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	RS485emErrorCodes err       = RS485EMULE_SUCCESS;
	uint16_t          loopSleep = FMASTER_LOOPSLEEP;
	int               sfd       = 0;
	char              sport[PATH_MAX];
	char              idsStr[256];
	uint8_t           idsList[16];
	uint8_t           valuesList[16];
	uint8_t           idsListSize = 0;

	memset(idsList,    0,    16);
	memset(valuesList, 0,    16);
	memset(idsStr,    '\0',  256*sizeof(char));
	
	//
	// File's arguments reading...
	//
	if (argc > 1) {
		int option_index = 0;
		int c = 0;
		while (c >= 0) {
			c = getopt_long(argc, argv, "p:c:r:", myopts, &option_index);
			if (c > 0) {
				if      (c == 'i') strcpy(idsStr, optarg);
				else if (c == 'l') loopSleep = atoi(optarg);
				else {
					fprintf(stderr, "[ERROR!] Unknown argument\n");
					summary(argv[0]);
					exit(RS485EMULE_ERROR_UNKNOWNARG);
				}
			}
		}
	}
	
	//
	// Arguments checking
	//
	idsListSize = listConverter(idsList, idsStr);
	if (idsListSize == 0 || loopSleep == 0) {
		fprintf(stderr, "[ERROR!] wrong defined values\n");
		summary(argv[0]);
		exit(RS485EMULE_ERROR_UNKNOWNARG);
	}


	//
	// Foo data generating...
	//
	for (uint8_t t = 0; idsList[t] != 0; t++) {
		valuesList[t] = randomInt(16);
		//printf("%d: %d\n", (int)idsList[t], (int)valuesList[t]);
	}


	//
	//  Database initialization
	//
	if ((err = init_RS485emulatorAPI()) && err != RS485EMULE_SUCCESS) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot open the virtual ports DB\n");
		exit(err);
	}
	
	// UNIX Signals
	signal(SIGTERM, sigHandler);
	signal(SIGINT,  sigHandler);
			
				
	// Serial port file name acknowledge
	err = getMPort_RS485emulatorAPI(sport);
	if (err == RS485EMULE_WARNING_NOTHINGTODO) {
		fprintf(stderr, "[ERROR!] BUG in RS485emulatorAPI module\n");
		err = RS485EMULE_ERROR_INTERNAL;
		
	} else if (err == RS485EMULE_ERROR_FORBIDDENOP) {
		fprintf(stderr, "[ERROR!] The Device Master's port is already in use\n");
		
	} else if (err != RS485EMULE_SUCCESS) {
		fprintf(stderr, "[ERROR!] I cannot retrive the RS485 emulator's port\n");

	// Serial port opening
	} else if ((sfd = open(sport, O_RDWR|O_NOCTTY)) && sfd < 0) {
		// ERROR!
		fprintf(stderr, "[ERROR!] I cannot open the \"%s\" serial port\n", sport);
		err = RS485EMULE_ERROR_IOFAILED;

	} else {
		uint8_t         datapkg[2];
		struct pollfd   fds[2];
		int             ret;
		struct timespec pollTimeout;

		printf("Serial port: %s\n", sport);

		//
		// Polling mechanism settings...
		//
		fds[0].fd           = sfd;
		fds[0].events       = POLLIN;
		pollTimeout.tv_sec  = 1;
		pollTimeout.tv_nsec = 0;

			
		//
		// Data sending to slave devices
		//
		for (uint8_t t = 0; t < idsListSize; t++) {
			datapkg[0] = idsList[t];
			datapkg[1] = valuesList[t];
			if (write(sfd, datapkg, 2) != 2) {
				// ERROR!
				fprintf(stderr, "[ERROR!] Data sending operation failed: %s\n", strerror(errno));
				err = RS485EMULE_ERROR_IOFAILED;
			} else {
				//printf("Data-packages has been sent\n");

				ret = ppoll(fds, 2, &pollTimeout, NULL);

				if (ret < 0 && errno != EINTR) {
					// ERROR!
					fprintf(stderr, "ERROR! ppoll() failed: %s\n", strerror(errno));
					err = RS485EMULE_ERROR_IOFAILED;
					break;
				
				} else if (ret == 0) {
					// TIMEOUT
					printf("Timeout\n");
			
				} else if (fds[0].revents & POLLIN) {
					int ts = 0;
					//printf("Data acknowledge...\n");
					ts = read(sfd, datapkg, 2);
					if (ts == 2) {
					//	printf("ID=%d Recived=%d Sent=%d\n", datapkg[0], datapkg[1],
					//		valuesList[getIndexByID(idsList, datapkg[0])]
					//	);
						if ((datapkg[1]/datapkg[0]) == valuesList[getIndexByID(idsList, datapkg[0])]) 
							printf("[%d] OK!\n", datapkg[0]);
						else 
							printf("[%d] ERROR!\n", datapkg[0]);
						
					} else {
						fprintf(stderr, "ERROR! Corrupted data\n");
						printf("ts=%d, id=%d, value=%d\n", ts, datapkg[0], datapkg[1]);
					}
				}
				usleep(loopSleep*1000);
			}
		} // --- For-loop ...
		
		close(sfd);
	}

	if (*sport != '\0') {
		err = release_RS485emulatorAPI(sport);
		if (err > 64) fprintf(stderr, "[ERROR!] I cannot relese the used port\n");
	}
	close_RS485emulatorAPI();

	return(RS485emu_bashErrorCode(err));
}
