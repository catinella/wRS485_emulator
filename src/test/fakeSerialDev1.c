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
// File:     fakeSerialDev1.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	It is the simplest fake device you want to use and it has developed JUST to test the RS485 emulator or its modules.
//	The goal of this slave device is to log in the argument defined file all data received by the master device
//
//		use: fakeSerialDev1 --repofile=<file name>
//	
//	
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <poll.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485emulatorAPI.h>
#include <fakeMaster1.h>

#ifndef DBGTRACE
#define DBGTRACE ;
#endif

struct option myopts[] = {
	 {"repoFile", required_argument, 0, 'r'},
	 {0, 0, 0, 0}
};

bool loop = true;

//------------------------------------------------------------------------------------------------------------------------------
//                                     P R I V A T E   F U N C T I O N S
//------------------------------------------------------------------------------------------------------------------------------

void sigHandler (int signum) {
	DBGTRACE
	if (signum == SIGTERM || signum == SIGINT) loop = false;
	return;
}

void summary (char *file) {
	fprintf(stderr, "Use: %s --repoFile=<file name>\n", file);
	return;
}

double getTimeStamp() {
	//
	// Description:
	//	It returns a milliseconds timestamp
	//
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	return((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}

//------------------------------------------------------------------------------------------------------------------------------
//                                                         M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	int               c = 0;
	char              file[PATH_MAX];
	int               option_index = 0;
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;

	DBGTRACE
	file[0] = '\0';
	//for (c=0; c<argc; c++) printf("%d: %s\n", c, argv[c]);

	if (argc > 1) {
		c = 0;
		while (c >= 0) {
			c = getopt_long(argc, argv, "r:", myopts, &option_index);
			if (c > 0) {
				if (c == 'r')
					strcpy(file, optarg);
				else {
					summary(argv[0]);
					exit(RS485EMULE_ERROR_UNKNOWNARG);
				}
			}
		}
	} else {
		summary(argv[0]);
		exit(RS485EMULE_ERROR_ILLEGALSYNTAX);
	}

	if (file[0] != '\0') {
		FILE *fh = fopen(file, "w");
		if (fh != NULL) {
			int   sfd;
			char  myport[PATH_MAX];

			signal(SIGTERM, sigHandler);
			signal(SIGINT,  sigHandler);
			
			if ((err = init_RS485emulatorAPI()) && err != RS485EMULE_SUCCESS) {
				// ERROR!
				fprintf(stderr, "ERROR! I cannot open the virtual ports DB\n");
				
			} else if ((err = takePort_RS485emulatorAPI(myport)) && err != RS485EMULE_SUCCESS) {
				// ERROR!
				fprintf(stderr, "ERROR! I cannot retrive the port I have to use\n");

			} else if ((sfd = open(myport, O_RDWR|O_NOCTTY)) && sfd < 0) {
				// ERROR!
				fprintf(stderr, "ERROR! I cannot open the \"%s\" serial port\n", myport);
				err = RS485EMULE_ERROR_IOFAILED;
	
			} else {
				char            data[1024];
				struct pollfd   fds[2];
				int             ret;
				struct timespec pollTimeout;

				//
				// Polling mechanism settings...
				//
				fds[0].fd           = sfd;
				fds[0].events       = POLLIN;
				pollTimeout.tv_sec  = 0;
				pollTimeout.tv_nsec = 100 * 1000 * 1000;
				
				printf("%s is ready to log data from %s (fd=%d)\n", argv[0], myport, sfd);
				fprintf(fh, "%s (pid=%d)\n", argv[0], getpid());
			
				while (loop && err < 64) {
					ret = ppoll(fds, 2, &pollTimeout, NULL);

					if (ret < 0 && errno != EINTR) {
						// ERROR!
						fprintf(stderr, "ppoll() failed: %s\n", strerror(errno));
						err = RS485EMULE_ERROR_IOFAILED;
					
					} else if (ret == 0) {
						// TIMEOUT
						//printf("Timeout\n");
				
					} else if (fds[0].revents & POLLIN) {

						if (RS485emu_dataReceive(sfd, (void*)data, sizeof(char)*FMASTER_DATASIZE) == FMASTER_DATASIZE) {
							fprintf(fh, "%f: %s\n", getTimeStamp(), data); fflush(fh);
							printf("\r%s", data); fflush(stdout);
						} else {
							// ERROR!
							fprintf(
								stderr, "ERROR! I cannot read data from \"%s\" serial port: %s\n",
								myport, strerror(errno)
							);
							err = RS485EMULE_ERROR_IOFAILED;
						}
					}
				}

				printf("Shouting down peocedure....\n");
				
				if ((err = release_RS485emulatorAPI(myport)) && err != RS485EMULE_SUCCESS)
					// ERROR!
					fprintf(stderr, "ERROR! I cannot release the port I used\n");
					
				close_RS485emulatorAPI();
			}
			fclose(fh);
		}
	} else {
		// ERROR!
		fprintf(stderr, "ERROR! You have to specify the activity report filename\n");
		summary(argv[0]);
		exit(RS485EMULE_ERROR_ILLEGALSYNTAX);
	}

	exit(RS485emu_bashErrorCode(err));
}	

