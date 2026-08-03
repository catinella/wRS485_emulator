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
// File:     RS485_emulator.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	This software provides a fake RS485 serial network. All the executable files that implement the fake serial devices can be
//	Aconnecte to the BUS using the virual available slaves ports
//
//	Communication protocol:
//	=======================
//	In order to use the RS485 multipoint serial communication bus, you must implement a master-slaves protocol, usually.
//	It is necessary because the protocol is not provided by hardware, it means, every data package sent by the master is
//	received by all slave devices, and everyone of them must know how to recognize its own addres by itself.
//	RS485 serial bus cannpot handle unicast communications, just broadcast are allowed.
//
//
//	How this software function:
//	===========================
//	Using a virtual serial port, the master can send data packages to the RS485 emulator. Every package is forwarded to all 
//	processes that reppresent the fake-devices. Everyone of them will process the data package.
//
//	+------------+
//	|            |                  +-------------------------------------------------------+
//	|  Software  |                  |                      RS485 Emulator                   |
//	| under test |                  +----+------------------+--+--+--+--+-------------------+
//	|            |                       |                  |  |  |  |  |
//	+------+-----+                       |    +-------------+  |  |  |  +-------------+
//	       |                 +-----------+    |                |  |  |                |
//           |                 |                |         +------+  |  +------+         |
//	       |          +------+-----+          |         |         |         |         |
//	       +--------->|  Master's  |       +--+---+  +--+---+  +--+---+  +--+---+  +--+---+
//	                  |  fake port |       | fake |  | fake |  | fake |  | fake |  | fake | 
//	                  +------------+       | port |  | port |  | port |  | port |  | port | 
//	                                       +--+---+  +--+---+  +--+---+  +--+---+  +--+---+
//	                                          |         |         |         |         |
//	                                          |         |         |         |         |
//	                                       +--+---+  +--+---+  +--+---+  +--+---+  +--+---+
//	                                       | DEV1 |  | DEV2 |  | DEV3 |  | DEV4 |  | DEVn |  
//	                                       +------+  +------+  +------+  +------+  +------+
//	
//	Ports DB:
//	=========
//	To maitain data on the virtual ports and its assignment, the BUS emulator uses SQLite.
//	All the features required by BUS emulator are provided by RS485_portsDB module. The BUS' clients can require or release
//	ports using the function provided by the RS485_portsDB_client module.
//
-------------------------------------------------------------------------------------------------------------------------------*/
 #define _GNU_SOURCE         /* See feature_test_macros(7) */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <syslog.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485_portsDB.h>
#include <RS485_virtualPort.h>
#include <RS485_virtualPortsList.h>
#include <RS485_emulator.h>

#define RS485_PKGDATA_SIZE     192

struct option myopts[] = {
	 {"configFile", required_argument, 0, 'c'},
	 {"foreground", 0,                 0, 'f'},
	 {"help",       0,                 0, 'h'},
	 {"version",    0,                 0, 'v'},
	 {0,            0,                 0, 0  }
};

bool loop          = true;
bool configRequest = false;
bool foreground    = false;

//------------------------------------------------------------------------------------------------------------------------------
//                                                  F U N C T I O N S 
//------------------------------------------------------------------------------------------------------------------------------

void sigHandler (int signum) {
	if      (signum == SIGTERM || signum == SIGINT) loop = false;
	else if (signum == RS485EMULE_UPDATESIGN)       configRequest = true;
	
	return;
}


void summary (const char *execfile) {
	fprintf(stderr, "Use: %s [--configFile=<filename>] [--foreground] [--help] [--version]\n", execfile);
}


void printErr (const char *message, rs485emule_portsNum_type ec) {
	if (foreground) {
		char symbol[32];
		if      (ec <= 32)           strcpy(symbol, "[  INFO  ]"); 
		else if (ec >32 && ec <= 64) strcpy(symbol, "[WARNING!]");
		else                         strcpy(symbol, "[ ERROR! ]");
		if (ec <= 64) {fprintf(stdout, "%s %s\n", symbol, message); fflush(stdout);}
		else          {fprintf(stderr, "%s %s\n", symbol, message); fflush(stderr);}
	} else {
		int priority;
		if      (ec <= 32)           priority = LOG_INFO;
		else if (ec >32 && ec <= 64) priority = LOG_WARNING;
		else                         priority = LOG_ERR;
		syslog(priority, message);
	}
	return;
}


//------------------------------------------------------------------------------------------------------------------------------
//                                                       M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	char                    configFile[PATH_MAX];
	char                    serialPort[PATH_MAX];
	uint8_t                 err = RS485EMULE_SUCCESS;
	struct virtualPort      masterPort;
//	char                    bufferForMessages[1024];
	struct virtualPortsList vplist;
	
	configFile[0] = '\0';
	serialPort[0] = '\0';
	
	//
	// Arguments reading...
	//
	if (argc > 1) {
		int option_index = 0;
		int c            = 0;
		
		while (c >= 0) {
			c = getopt_long(argc, argv, "c:fhv", myopts, &option_index);
			if (c == 'c')
				strcpy(configFile, optarg);

			else if (c == 'f')
				foreground = true;
				
			else if (c == 'h') {
				summary(argv[0]);
				exit(0);
				
			} else if (c == 'v') {
				printf("%s %s\n", VERSION, GDB ? "(gdb)" : "");
				exit(0);

			} else if (c > 0) {
				fprintf(stderr, "ERROR! unknown argument\n");
				summary(argv[0]);
				exit(RS485emu_bashErrorCode(RS485EMULE_ERROR_UNKNOWNARG));
			}
		}
	}

	// Defaults applying
	if (configFile[0] == '\0') strcpy(configFile, RS485EMULE_CONFIG_FILE);

	// Signal settings....	
	signal(SIGTERM,                sigHandler);
	signal(SIGINT,                 sigHandler);
	signal(RS485EMULE_UPDATESIGN,  sigHandler);

	init_virtualPortsList(&vplist);
	init_virtualPort(&masterPort);

	if ((err = create_virtualPort(&masterPort, vPortMaster)) && err > 64) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot create the virtual serial port for the master device\n");
		
	} else if ((err = open_virtualPort(&masterPort)) && err > 64) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot open the \"%s\" file\n", serialPort);

	} else {
		struct pollfd           fds[RS485EMULE_PORTSNUM+1];
		int ret;
		void                    *chunk = malloc(RS485_PKGDATA_SIZE);
		char                    *busports[RS485EMULE_PORTSNUM+1];
		struct virtualPort      *virtualport = NULL;
		rs485emule_portsNum_type t;
		rs485emule_portsNum_type fdsNum;
		struct timespec         pollTimeout;
		chunkDataSize_type      trs = 0;
		
		//
		// Initialization....
		//
		for (t=0; t<RS485EMULE_PORTSNUM; t++) busports[t] = NULL;
		pollTimeout.tv_sec = 0;
		pollTimeout.tv_nsec = 100 * 1000 * 1000;
		configRequest = true;
		

		//
		// Virtual ports list population
		//
		for (t=0; t<RS485EMULE_PORTSNUM; t++) {
			err = add_virtualPortsList(&vplist);
			if (err != RS485EMULE_SUCCESS) {
				fprintf(stderr, "ERROR! I cannot cretes the required virtual ports\n");
				exit(err);
			}
		}
		

		//
		// Demonizing
		//
		if (foreground == false) {
			openlog(argv[0], LOG_NDELAY|LOG_PID, RS485EMULE_LOGFACILITY);

			if (daemon(0,0) < 0) {
				fprintf(stderr, "ERROR! daemon() syscall failed\n");
				exit(RS485EMULE_ERROR_NOSYSRESOURCE);
			}
		}

		
		//
		// PID file creation
		//
		err = RS485emu_writePidFile(getpid(), RS485EMULE_PIDFILE);
		if (err != RS485EMULE_SUCCESS) {
			fprintf(stderr, "ERROR! I cannot cretes the \"%s\" pid-file\n", RS485EMULE_PIDFILE);
			exit(err);
		}


		printErr("RS485 emulator started", RS485EMULE_SUCCESS);
		
		//
		// MAIN LOOP
		//
		while (loop) {
			
			if (configRequest) {
				printErr("Configuration request detected", RS485EMULE_SUCCESS);
				
				if ((err = usedPorts_portsDB(busports)) && err == RS485EMULE_SUCCESS) {
					/*
					//
					// Used ports list on screen
					//
					{
						uint16_t t = 0;
						printf("Used ports:\n");
						while (busports[t]  != NULL) {
							printf("%p: \t%s\n", busports[t], busports[t]);
							t++;
						}
					}
					*/
					
					if ((err = update_virtualPortsList(&vplist, (const char**)busports)) && err == RS485EMULE_SUCCESS) {
						//DBGTRACE
						resetIT_virtualPortsList(&vplist);
	
						// Master's port is always the first
						fds[0].fd = masterPort.fd;
						fds[0].events = POLLIN;
						t = 1;
						
						while((virtualport = next_virtualPortsList(&vplist)) && virtualport != NULL) {
							fds[t].fd = virtualport->fd;
							fds[t].events = POLLIN;
							t++;
						}
						fdsNum = t;
					} else
						printErr("I cannot update the connected ports list", err);
				} else
					printErr("I cannot retrive the list of the connected ports to slave devices", err);
				
				configRequest = false;
				//DBGTRACE
			}

			// ******* POLL *******
			ret = ppoll(fds, (fdsNum+1), &pollTimeout, NULL);

			if (ret < 0 && errno != EINTR) {
				// ERROR!
				printErr("ppoll() failed", RS485EMULE_ERROR_IOFAILED);
				
			} else if (ret == 0) {
				// TIMEOUT
				//printf("Timeout\n");
				err = pidsChk_virtualPortsList(&vplist);
				
				if (err == RS485EMULE_SUCCESS)
					// The list has been changed
					configRequest = true;
				
				if (err > 64)
					// ERROR!
					printErr("I cannot check for dead processes", err);
				
			} else {
				if (ret > 1) {
					printErr("Concurrent BUS access", RS485EMULE_WARNING_GENERIC);
				}
				
				if (fds[0].revents & POLLIN) {
			//		printf("Data from BUS master has been detected\n");

					// Data reading from the master
					trs = read(fds[0].fd, chunk, RS485_PKGDATA_SIZE);
					
					if (trs < 0) {
						// ERROR!
						printErr("I cannot read data from the serial port", RS485EMULE_ERROR_IOFAILED);
						
					} else if (trs > 0) {
						//
						// Data writing to slave devices
						//
						resetIT_virtualPortsList(&vplist);
						while((virtualport = next_virtualPortsList(&vplist)) && virtualport != NULL) {
							if (virtualport->fd > 0) {
								err = send_virtualPort (*virtualport, chunk, trs);
								if (err != RS485EMULE_SUCCESS) {
									printErr("I cannot send data to the fake devices", err);
									break;
								}
							}
						}
					}
					
				} else {
			//		printf("Data from a slave device has been detected\n");
					//fdsNum = t;
					for (t=1; t<fdsNum; t++) {
						if (fds[t].revents & POLLIN) {
							// Slave to master data exchange
							trs = read(fds[t].fd, chunk, RS485_PKGDATA_SIZE);
							if (trs > 0)    err = send_virtualPort(masterPort, chunk, trs);
							else            err = RS485EMULE_ERROR_IOFAILED;
						}
					}
				}
			}
			usleep(10); // 10us = 100KHz --> troughput = 100 000 * RS485_PKGDATA_SIZE / 1024
		} // --- main loop ---


		//
		// Resources releasing...
		//
		printErr("Shouting down procedure....", RS485EMULE_SUCCESS);
		if (foreground == false) closelog();
		free(chunk);
		close_virtualPort(&masterPort);
		free_virtualPort(&masterPort);
		free_virtualPortsList(&vplist);
	}

	return(RS485emu_bashErrorCode(err));
}
