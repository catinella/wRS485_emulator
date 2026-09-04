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
//	connecte to the BUS using the virual available slaves ports
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
//	| under test |                  +----+-------------------+--+--+--+--+------------------+
//	|            |                       |                   |  |  |  |  |
//	+------+-----+                       |   +---------------+  |  |  |  +---------------+
//	       |               +-------------+   |                  |  |  |                  |
//	       |               |                 |          +-------+  |  +-------+          |
//	       |          +----+---+             |          |          |          |          |
//	       +--------->| Master |         +---+---+  +---+---+  +---+---+  +---+---+  +---+---+
//	                  |  port  |         | slave |  | slave |  | slave |  | slave |  | slave | 
//	                  +--------+         | port  |  | port  |  | port  |  | port  |  | port  | 
//	                                     +---+---+  +---+---+  +---+---+  +---+---+  +---+---+
//	                                         |          |          |          |          |
//	                                         |          |          |          |          |
//	                                      +--+---+   +--+---+   +--+---+   +--+---+   +--+---+
//	                                      | DEV1 |   | DEV2 |   | DEV3 |   | DEV4 |   | DEVn |  
//	                                      +------+   +------+   +------+   +------+   +------+
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
#include <glib.h>
#include <errno.h>
#include <syslog.h>
#include <wError.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485_portsDB.h>
#include <RS485_virtualPort.h>
#include <RS485_virtualPortsList.h>
#include <RS485_emulator.h>

#define RS485_PKGDATA_SIZE     192

struct option myopts[] = {
	 {"foreground", 0,                 0, 'f'},
	 {"help",       0,                 0, 'h'},
	 {"version",    0,                 0, 'v'},
	 {0,            0,                 0, 0  }
};

bool loop           = true;
bool configRequest  = false;
bool foreground     = false;
bool interruptedBus = false;
//------------------------------------------------------------------------------------------------------------------------------
//                                                  F U N C T I O N S 
//------------------------------------------------------------------------------------------------------------------------------
void wrpLogMsg (const char *message, uint8_t ec);


void sigHandler (int signum) {
	if      (signum == SIGTERM || signum == SIGINT) loop = false;
	else if (signum == RS485EMULE_UPDATESIGN)       configRequest = true;
	else if (signum == RS485EMULE_BROKENSIGN) {
		if (interruptedBus == false) {
			interruptedBus = true;
			wrpLogMsg("RS485-BUS broken", RS485EMULE_SUCCESS);
		} else {
			interruptedBus = false;
			wrpLogMsg("RS485-BUS fixed", RS485EMULE_SUCCESS);
		}
	}
	return;
}

void summary (const char *execfile) {
	//
	// Description:
	//	Common help (howto use) message
	//
	fprintf(stderr, "Use: %s [--foreground] [--help] [--version]\n", execfile);
}

void logMsg (const char *message, wError_t ec) {
	//
	// Description:
	//	Centralized log message
	//
	if (foreground) {
		char symbol[32];
		if      (WERROR_ISSUCCESS(ec)) strcpy(symbol, "[  INFO  ]"); 
		else if (WERROR_ISWARNING(ec)) strcpy(symbol, "[WARNING!]");
		else                           strcpy(symbol, "[ ERROR! ]");
		if (WERROR_ISERROR(ec)) {fprintf(stderr, "%s %s\n", symbol, message); fflush(stderr);}
		else                    {fprintf(stdout, "%s %s\n", symbol, message); fflush(stdout);}
	} else {
		int priority;
		if      (WERROR_ISSUCCESS(ec)) priority = LOG_INFO;
		else if (WERROR_ISWARNING(ec)) priority = LOG_WARNING;
		else                           priority = LOG_ERR;
		syslog(priority, message);
	}
	return;
}

void wrpLogMsg (const char *message, uint8_t ec) {
	WERROR_DECLARATION(err, WERROR_JUSTCODE, ec);
	logMsg (message, err);
	return;
}

void wrpExit (uint8_t ec) {
	WERROR_DECLARATION(err, WERROR_JUSTCODE, ec);
	exit(wError_shellCode(err));
	return;
}

//------------------------------------------------------------------------------------------------------------------------------
//                                                       M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	char               serialPort[PATH_MAX];
	virtualPort_t      masterPort;
	virtualPortsList_t vplist;
	WERROR_DECLARATION(err, WERROR_JUSTCODE, RS485EMULE_SUCCESS);
	
	serialPort[0] = '\0';
	
	//
	// Arguments reading...
	//
	if (argc > 1) {
		int option_index = 0;
		int c            = 0;
		
		while (c >= 0) {
			c = getopt_long(argc, argv, "c:fhv", myopts, &option_index);
			if (c == 'f')
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
				wrpExit(RS485EMULE_ERROR_UNKNOWNARG);
			}
		}
	}

	// Signal settings
	signal(SIGTERM,                sigHandler);
	signal(SIGINT,                 sigHandler);
	signal(RS485EMULE_UPDATESIGN,  sigHandler);
	signal(RS485EMULE_BROKENSIGN,  sigHandler);

	// Initialization
	init_portsDB();
	init_virtualPortsList(&vplist);
	init_virtualPort(&masterPort);

	// Master port creation
	if ((err = create_virtualPort(&masterPort, RS485EMULE_PORTMASTER)), WERROR_ISERROR(err)) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot create the virtual serial port for the master device\n");
		
	// Master port opening
	} else if ((err = open_virtualPort(&masterPort)), WERROR_ISERROR(err)) { 
		// ERROR!
		fprintf(stderr, "ERROR! I cannot open the \"%s\" file\n", serialPort);

	} else {
		struct pollfd           fds[RS485EMULE_PORTSNUM+1];
		int ret;
		void                     *chunk = malloc(RS485_PKGDATA_SIZE);
		GPtrArray                *busports = g_ptr_array_new_with_free_func(g_free);
		virtualPort_t            *virtualport = NULL;
		rs485emule_portsNum_type t;
		rs485emule_portsNum_type fdsNum;
		struct timespec          pollTimeout;
		chunkDataSize_type       trs = 0;

		printf("DEBUG: Master's port: \"%s\"\n", masterPort.port);

		
		//
		// Initialization....
		//
		pollTimeout.tv_sec = 0;
		pollTimeout.tv_nsec = 100 * 1000 * 1000;
		configRequest = true;


		//
		// Virtual ports list population
		//
		for (t=0; t<RS485EMULE_PORTSNUM; t++) {
			err = add_virtualPortsList(&vplist);
			if (WERROR_ISERROR(err)) {
				fprintf(stderr, "ERROR! I cannot cretes the required virtual ports\n");
				exit(wError_shellCode(err));
			}
		}


		//
		// Demonizing
		//
		if (foreground == false) {
			openlog(argv[0], LOG_NDELAY|LOG_PID, RS485EMULE_LOGFACILITY);

			if (daemon(0,0) < 0) {
				fprintf(stderr, "ERROR! daemon() syscall failed\n");
				wrpExit(RS485EMULE_ERROR_NOSYSRESOURCE);
			}
		}

		
		//
		// PID file creation
		//
		err = RS485emu_writePidFile(getpid(), RS485EMULE_PIDFILE);
		if (WERROR_ISERROR(err)) {
			fprintf(stderr, "ERROR! I cannot cretes the \"%s\" pid-file\n", RS485EMULE_PIDFILE);
			exit(wError_shellCode(err));
		}


		wrpLogMsg("RS485 emulator started", RS485EMULE_SUCCESS);
		
		//
		// MAIN LOOP
		//
		while (loop) {
			
			if (configRequest) {
				wrpLogMsg("Configuration request detected", RS485EMULE_SUCCESS);
				
				if ((err = usedPorts_portsDB(busports)), WERROR_ISSUCCESS(err)) {
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
					
					if ((err = update_virtualPortsList(&vplist, (const GPtrArray*)busports)), WERROR_ISSUCCESS(err)) {
						//DBGTRACE
						resetIT_virtualPortsList(&vplist);
	
						// Master's port is always the first
						fds[0].fd = masterPort.fd;
						fds[0].events = POLLIN;
						t = 1;
						
						while((virtualport = next_virtualPortsList(&vplist)) != NULL) {
							fds[t].fd = virtualport->fd;
							fds[t].events = POLLIN;
							t++;
						}
						fdsNum = t;
					} else
						// ERROR!
						logMsg("I cannot update the connected ports list", err);
				} else
					// ERROR!
					logMsg("I cannot retrive the list of the connected ports to slave devices", err);
				
				configRequest = false;
				//DBGTRACE
			}

			// ******* POLL *******
			ret = ppoll(fds, (fdsNum+1), &pollTimeout, NULL);

			if (ret < 0 && errno != EINTR) {
				// ERROR!
				wrpLogMsg("ppoll() failed", RS485EMULE_ERROR_IOFAILED);
				
			} else if (ret == 0) {
				// TIMEOUT
				//printf("Timeout\n");
				
				if ((err = pidsChk_virtualPortsList(&vplist)), WERROR_ISSUCCESS(err)) 
					// The list has been changed
					configRequest = true;
				
				if (WERROR_ISERROR(err))
					// ERROR!
					logMsg("I cannot check for dead processes", err);
				
			} else {
				if (ret > 1) {
					//
					// === WARNING! ===
					// Mome slave devices are sending data at the same time
					//
					wrpLogMsg("Concurrent BUS access", RS485EMULE_WARNING_GENERIC);
				}
				
				if (fds[0].revents & POLLIN) {
			//		printf("Data from BUS master has been detected\n");

					// Data reading from the master
					trs = read(fds[0].fd, chunk, RS485_PKGDATA_SIZE);
					
					if (trs < 0) {
						// ERROR!
						wrpLogMsg("I cannot read data from the serial port", RS485EMULE_ERROR_IOFAILED);
						
					} else if (trs > 0 && interruptedBus == false) {
						//
						// Data writing to slave devices
						//
						resetIT_virtualPortsList(&vplist);
						while((virtualport = next_virtualPortsList(&vplist)) != NULL) {
							if (virtualport->fd > 0) {
								err = send_virtualPort (*virtualport, chunk, trs);
								if (WERROR_ISERROR(err)) {
									logMsg("I cannot send data to the fake devices", err);
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
							if (trs > 0 && interruptedBus == false)
								err = send_virtualPort(masterPort, chunk, trs);
							else
								WERROR_GETCODE(err) = RS485EMULE_ERROR_IOFAILED;
						}
					}
				}
			}
			usleep(10); // 10us = 100KHz --> troughput = 100 000 * RS485_PKGDATA_SIZE / 1024
		} // --- main loop ---


		//
		// Resources releasing...
		//
		wrpLogMsg("Shouting down procedure....", RS485EMULE_SUCCESS);
		if (foreground == false) closelog();
		free(chunk);
		g_ptr_array_free(busports, TRUE);
		close_virtualPort(&masterPort);
		free_virtualPort(&masterPort);
		free_virtualPortsList(&vplist);
	}

	return(wError_shellCode(err));
}
