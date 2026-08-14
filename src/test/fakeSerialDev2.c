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
// File:     fakeSerialDev2.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This fake device accepts datapackages with its ID from the master. The received data is composed by two bytes where the
//	first one is the slave-ID and the second is a number used by the slave's process to calculate the reply to send to the 
//	master.
//
//		use: fakeSerialDev1 --id=<1-255>
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
	 {"id", required_argument, 0, 'i'},
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
	fprintf(stderr, "Use: %s --id=<1-255>\n", file);
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
	RS485emErrorCodes_t err = RS485EMULE_SUCCESS;
	uint8_t           myID = 0;

	if (argc > 1) {
		int c = 0;
		int option_index = 0;
		
		while (c >= 0) {
			c = getopt_long(argc, argv, "i:", myopts, &option_index);
			if (c > 0) {
				if (c == 'i')
					myID = atoi(optarg);
				else {
					summary(argv[0]);
					exit(RS485EMULE_ERROR_UNKNOWNARG);
				}
			}
		}
	}

	if (myID == 0){
		summary(argv[0]);
		exit(RS485EMULE_ERROR_ILLEGALSYNTAX);
	}

	signal(SIGTERM, sigHandler);
	signal(SIGINT,  sigHandler);
			
	{
		int   sfd;
		char  myport[PATH_MAX];

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
			struct pollfd   fds[2];
			int             ret;
			struct timespec pollTimeout;
			uint8_t         datapkg[2];
			
			//
			// Polling mechanism settings...
			//
			fds[0].fd           = sfd;
			fds[0].events       = POLLIN;
			pollTimeout.tv_sec  = 0;
			pollTimeout.tv_nsec = 100 * 1000 * 1000;
			
			printf("%s is listening on %s port (fd=%d)\n", argv[0], myport, sfd);
		
			while (loop && err < 64) {
				ret = ppoll(fds, 2, &pollTimeout, NULL);

				if (ret < 0 && errno != EINTR) {
					// ERROR!
					fprintf(stderr, "ERROR! ppoll() failed: %s\n", strerror(errno));
					err = RS485EMULE_ERROR_IOFAILED;
				
				} else if (ret == 0) {
					// TIMEOUT
					//printf("Timeout\n");
			
				} else if (fds[0].revents & POLLIN) {

					if (read(sfd, datapkg, 2) == 2) {
						printf("Data acknowledge...\n");
						
						if (datapkg[0] == myID) {
							printf("The data has been sent to me\n");
							datapkg[1] = myID * datapkg[1];
							if (write(sfd, &datapkg, 2) != 2) {
								// ERROR!
								fprintf(
									stderr, "ERROR! I cannot send data trough the BUS: %s\n", strerror(errno)
								);
								err = RS485EMULE_ERROR_IOFAILED;
								DBGTRACE
							} else
								printf("The reply has been sent\n");
						}
					} else {
						// ERROR!
						fprintf(stderr, "ERROR! I cannot read data from the BUS: %s\n", strerror(errno));
						err = RS485EMULE_ERROR_IOFAILED;
						DBGTRACE
					}
				}
			}
		}

		printf("Shouting down peocedure....\n");
				
		if ((err = release_RS485emulatorAPI(myport)) && err != RS485EMULE_SUCCESS)
			// ERROR!
			fprintf(stderr, "ERROR! I cannot release the port I used\n");
					
		close_RS485emulatorAPI();
	}

	exit(RS485emu_bashErrorCode(err));
}	

