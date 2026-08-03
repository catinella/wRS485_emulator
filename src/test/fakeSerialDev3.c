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
// File:     fakeSerialDev3.c
// 
// Authour:  Silvano Catinella
// 
// Language: C
// 
// Description:
//	It is software allows you to create one or many  simple modbus compliant fake slave devices. It has been developed to be
//	used with fakeMaster3 software master fake device.
//
//		use: fakeSerialDev2 --id=<1-64> regsAddress=<0-15> regsNumber=<1-255> fooOffset=<10-15>	
//	
//	
//	
// Editor params: cols=128, tab-size=6
-------------------------------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <debugTools.h>
#include <RS485_commonLib.h>
#include <RS485emulatorAPI.h>
#include <fakeMaster1.h>


#ifndef DBGTRACE
#define DBGTRACE ;
#endif

#define FS2_MAXREGSNUMBER    127
#define FS2_LASTREGADDRESS   16
#define FS2_MAXFOOOFFSET     16

#define FS2_REGADDR          40000



struct option myopts[] = {
	 {"id",          required_argument, 0, 'i'},
	 {"regsAddress", required_argument, 0, 'a'},
	 {"regsNumber",  required_argument, 0, 'n'},
	 {"fooOffset",   required_argument, 0, 'f'},
	 {"help",        required_argument, 0, 'h'},
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
	fprintf(stderr,
		"Use: %s --id=<1-16> --regsAddress=<1-%d> --regsNumber=<1-%d> --fooOffset=<1-%d>\n",
		file, FS2_LASTREGADDRESS, FS2_MAXREGSNUMBER, FS2_MAXFOOOFFSET
	);
	return;
}

//------------------------------------------------------------------------------------------------------------------------------
//                                                         M A I N 
//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	RS485emErrorCodes err        = RS485EMULE_SUCCESS;
	uint8_t           myID       = 0;
	uint16_t          regsaddr   = 0;
	uint16_t          regsnum    = 0;
	uint16_t          foooffset  = 0;
	modbus_mapping_t  *registers = NULL;

	//
	// File's arguments
	//
	{
		int c = 0;
		int option_index = 0;
		
		if (argc > 1) {
			c = 0;
			while (c >= 0) {
				c = getopt_long(argc, argv, "a:n:i:h", myopts, &option_index);
				if (c > 0) {
					if (c == 'i') {
						// Slave ID
						myID = atoi(optarg);

					} else if (c == 'a') {
						// First resgister's address
						regsaddr = atoi(optarg);
						
					} else if (c == 'n') {
						// Number of registers
						regsnum = atoi(optarg);
						
					} else if (c == 'o') {
						// Max foo offset
						foooffset = atoi(optarg);
						
					} else if (c == 'h') {
						// Help
						summary(argv[0]);
						exit(0);

					} else {
						summary(argv[0]);
						exit(RS485EMULE_ERROR_UNKNOWNARG);
					}
				}
			}
		}

		if (
			myID == 0 || regsnum == 0 || regsaddr == 0 ||
			regsnum > FS2_MAXREGSNUMBER || regsaddr > FS2_LASTREGADDRESS || foooffset > FS2_MAXFOOOFFSET 
		) {
			summary(argv[0]);
			exit(RS485EMULE_ERROR_ILLEGALSYNTAX);
		}
	}

	signal(SIGTERM, sigHandler);
	signal(SIGINT,  sigHandler);

	// Internal registers allocation;
	registers = modbus_mapping_new(0, 0, FS2_REGADDR+32, 0);

	
	if (registers == NULL) {
		// ERROR!
		fprintf(stderr, "ERROR! I cannot create the device's data registers: %s\n", modbus_strerror(errno));
		err = RS485EMULE_ERROR_NOSYSRESOURCE;
	
	} else {
		pid_t    empid = 0;
		int      sfd;
		char     myport[PATH_MAX];
		modbus_t *ctx;
		uint16_t tab_reg[10];

			
		if ((err = init_RS485emulatorAPI()) && err != RS485EMULE_SUCCESS) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot open the virtual ports DB\n");
				
		} else if ((err = takePort_RS485emulatorAPI(myport)) && err != RS485EMULE_SUCCESS) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot retrive the port I have to use\n");

		} else if ((ctx = modbus_new_rtu(myport, RS485EMULE_BAUDRATES, 'N', 8, 1)) && ctx == NULL) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot open the \"%s\" serial port\n", myport);
			err = RS485EMULE_ERROR_IOFAILED;
	
		} else if (modbus_set_slave(ctx, myID) != 0) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot set my slave-device ID: %s\n", modbus_strerror(errno));
			err = RS485EMULE_ERROR_EXTTOOLFAILURE;

		} else if (modbus_rtu_set_serial_mode(ctx, MODBUS_RTU_RS485) != 0) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot set the serial type: %s\n", modbus_strerror(errno));
			err = RS485EMULE_ERROR_EXTTOOLFAILURE;

		} else if (modbus_rtu_set_rts(ctx, MODBUS_RTU_RTS_NONE) != 0) {
			// ERROR!
			fprintf(stderr, "ERROR! I cannot set the Request/Send mode: %s\n", modbus_strerror(errno));
			err = RS485EMULE_ERROR_EXTTOOLFAILURE;

		} else {
			int ret;
			uint8_t req[MODBUS_RTU_MAX_ADU_LENGTH];
			//struct timeval response_timeout;

			/*
			//
			// Timeout setting
			//
			response_timeout.tv_sec = 10;
			response_timeout.tv_usec = 0;
			modbus_set_response_timeout(ctx, &response_timeout);
			*/
			 
			// Verbose messages
			modbus_set_debug(ctx, TRUE);

			printf("The ModBUS %d-fakeSlavedevice is ready\n", myID);

			while (loop && err < 64) {
				 ret =  modbus_receive(ctx, req);

				if (ret < 0) {
				 	// ERROR!
					fprintf(stderr, "ERROR! Wrong request received: %s\n", modbus_strerror(errno));
					err = RS485EMULE_ERROR_EXTTOOLFAILURE;

				} else if (ret == 0) {
					// WARNING! nothing to do


				} else {
					// A new request has been received
					// +-------+-----------+-------------+
					// | FUNC  | First reg | Regs number |
					// |(1byte)| (2 bytes) |  (2 bytes)  |
					// +-------+-----------+-------------+
					//
					if (req[0] == MODBUS_FC_READ_INPUT_REGISTERS) {
						if (ret == 5) {
							uint16_t reqAddr = *(req+1);
							uint16_t regsNum = *(req+3);
							if (modbus_reply(ctx, req, ret, registers) < 0) {
								// ERROR!
								fprintf(stderr, "ERROR! I cannot reply\n");
								err = RS485EMULE_ERROR_IOFAILED;
							}
						} else {
							// ERROR!
							fprintf(stderr, "ERROR! malformed request received\n");
							err = RS485EMULE_ERROR_ILLEGALDATA;
						}
						
					} else if (req[0] == MODBUS_FC_WRITE_SINGLE_REGISTER) {

					} else {
						// ERROR!
						fprintf(stderr, "ERROR! Unknown request\n");
						err = RS485EMULE_ERROR_INTERNAL;
					}
				}
			}
			modbus_close(ctx);
		}

		if (ctx != NULL) modbus_free(ctx);
		modbus_mapping_free(registers);
	}

	exit(RS485emu_bashErrorCode(err));
}
