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
// File:     fakeMaster1.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This software is used to test the RS485 software emulatorhas with one or many fakeSerialDev1 istances. 
//	When this fake master device is connected to the RS485 bus emulator, it starts to send foo data packages in broadcast to
//	every slaves, for a given time. Every slave virtual device will store the data-package in a file file.
//
//	use: fakeMaster1 [--time=<seconds>] [--loopSleep=<milliseconds>]
//
//
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485emulatorAPI.h>
#include <fakeMaster1.h>

#ifndef DBGTRACE
#define DBGTRACE ;
#endif

#define FMASTER_TIME      10
#define FMASTER_LOOPSLEEP 100


struct option myopts[] = {
	 {"time",      required_argument, 0, 't'},
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
	fprintf(stderr, "Use: %s [--time=<seconds>] [--loopSleep=<milliseconds>]\n", file);

	return;
}

void fooData(char *data, uint16_t size) {
	static uint16_t idx = 0;
	
	data[idx] = '-';
	idx++;
	if (idx == (size-1)) idx = 0;
	data[idx] = '#';
	return;
}

//------------------------------------------------------------------------------------------------------------------------------
//                                                         M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	rs485emule_portsNum_type err       = RS485EMULE_SUCCESS;
	uint16_t                 execTime  = FMASTER_TIME;
	uint16_t                 loopSleep = FMASTER_LOOPSLEEP;
	int                      sfd       = 0;
	char                     sport[PATH_MAX];

	
	//
	// File's arguments reading...
	//
	if (argc > 1) {
		int option_index = 0;
		int c = 0;
		while (c >= 0) {
			c = getopt_long(argc, argv, "p:c:r:", myopts, &option_index);
			if (c > 0) {
				if      (c == 't') execTime  = atoi(optarg);
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
	if (execTime == 0 || loopSleep == 0) {
		fprintf(stderr, "[ERROR!] wrong defined values\n");
		summary(argv[0]);
		exit(RS485EMULE_ERROR_UNKNOWNARG);
	}


	//
	//  Database initialization
	//
	if ((err = init_RS485emulatorAPI()) && err != RS485EMULE_SUCCESS) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot open the virtual ports DB\n");
		exit(err);
	}
	
				
	//
	// Serial port file name acknowledge
	//
	err = getMPort_RS485emulatorAPI(sport);
	if (err == RS485EMULE_WARNING_NOTHINGTODO) {
		fprintf(stderr, "[ERROR!] BUG in RS485emulatorAPI module\n");
		exit(RS485EMULE_ERROR_INTERNAL);
		
	} else if (err == RS485EMULE_ERROR_FORBIDDENOP) {
		fprintf(stderr, "[ERROR!] The Device Master's port is already in use\n");
		exit(err);
		
	} else if (err != RS485EMULE_SUCCESS) {
		fprintf(stderr, "[ERROR!] I cannot retrive the RS485 emulator's port\n");
		exit(RS485EMULE_ERROR_IOFAILED);
	}
	printf("Serial port: %s\n", sport);
		

	//
	// Serial port opening
	//
//	if ((sfd = open(sport, O_RDWR|O_NOCTTY|O_SYNC)) && sfd < 0) {
	if ((sfd = open(sport, O_RDWR|O_NOCTTY)) && sfd < 0) {
		// ERROR!
		fprintf(stderr, "[ERROR!] I cannot open the \"%s\" serial port\n", sport);
		exit(RS485EMULE_ERROR_IOFAILED);
	}

	{
		char     data[FMASTER_DATASIZE];
		uint16_t counter   = 0;
		time_t   startTime = time(NULL);
		time_t   timeNow   = startTime;

		memset((void*)data, '-', ((sizeof(char)*FMASTER_DATASIZE)-1));
		data[FMASTER_DATASIZE] = '\0';
		
		while (loop && (timeNow-startTime) < execTime) {
			fooData(data, FMASTER_DATASIZE);
			printf("\r%ld: %s", (timeNow-startTime), data); fflush(stdout);
			timeNow = time(NULL);
			
			if (RS485emu_dataSend(sfd, data, FMASTER_DATASIZE) != FMASTER_DATASIZE) {
				fprintf(stderr, "[ERROR!] Data sending operation failed: %s\n", strerror(errno));
				loop = false;
			}

			counter++;
			usleep(loopSleep*1000);
		}
		printf("\n\n%d data packages sent\n", counter);

		close(sfd);
	}
	
	err = release_RS485emulatorAPI(sport);
	if (err > 64) fprintf(stderr, "[ERROR!] I cannot relese the used port\n");
	close_RS485emulatorAPI();
	
	return(RS485emu_bashErrorCode(err));
}
