![Project's banner](../images/wRS485_emulator.png)

# wRS485_emulator - RS485 BUS emulator for test and development environment 

## 1.0 Files

|         Files/Dirs       |                             Description                              |
|--------------------------|----------------------------------------------------------------------|
| debugTools.h             | Simple macros used to debug the project's code                       |
| Makefile                 | Set of rules to build the project's source code                      |
| RS485_commonLib.c        | Set of generic functions used the project's modules                  |
| RS485_commonLib.h        |  ""                                                                  |
| RS485emulatorAPI.c       | Emulator's API                                                       |
| RS485emulatorAPI.h       |  ""                                                                  |
| RS485_emulator.c         | Executable file that implements the virtual bus                      |
| RS485_emulator.h         |  ""                                                                  |
| RS485_errorCodes.h       | All error codes used by this project                                 |
| RS485_portsDB.c          | Low-level management of the virtual ports                            |
| RS485_portsDB.h          |  ""                                                                  |
| RS485_updateSignal.c     | It produces the executable file used to send signals to the emulator |
| RS485_updateSignal.h     |  ""                                                                  |
| RS485_virtualPort.c      | High-level management of a virtual port                              |
| RS485_virtualPort.h      |  ""                                                                  |
| RS485_virtualPortsList.c | It manages lists of virtual-ports and operations on the whoole list  |
| RS485_virtualPortsList.h |  ""                                                                  |
| test                     | This folder contains all project's tests                             |
| winstall_bin.conf        | Winstall configuration file                                          |
| winstall_sbin.conf       | Winstall configuration file                                          |

## 2.0 Description
This folder contains the source code of the wRS485_emulator project. In order to build the files needed by the project type the
following command:
	
	[TESTMODE=1] [GDB=1] make 

If you want to install directly just the binary files, you can use the following command

	[PREFIX=<folder>] make install

For further details on the available options, please read the Makefile header.
