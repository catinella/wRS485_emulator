![Project's banner](../../../images/wRS485_emulator.png)

# wRS485_emulator - RS485 BUS emulator for test and development environment 

## 1.0 Files

|     Files/Dirs    |               Description              |
|-------------------|----------------------------------------|
| fakeMaster2.c     | Source code of the master's process    |
| fakeSerialDev2.c  | Source code for the slave processes    |
| Makefile          | Set of rules to build the binary files |

## 2.0 Description

## 3.0 Binary file building
To build the files required by the test, type the following command:
	
	[TESTMODE=1] [GDB=1] make 

To clean the tree, execute the next command:
	make cleanall
