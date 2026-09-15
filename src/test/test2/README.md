![Project's banner](../../../images/wRS485_emulator.png)

# wRS485_emulator - RS485 BUS emulator for test and development environment 

## 1.0 Files

|     Files/Dirs    |               Description              |
|-------------------|----------------------------------------|
| fakeMaster2.c     | Source code of the master's process    |
| fakeSerialDev2.c  | Source code for the slave processes    |
| Makefile          | Set of rules to build the binary files |

## 2.0 Description
In this scenario the virtual master device accepts a list of IDs as command line argument. Thens, for every defined ID, the
master will send a package with it. The slave with that ID will recognize it, will read data, then will reply to master. The
other slaves, whose IDs were not specified in the master's arguments, will receive the data but will not process it.

## 2.1 Test preparation
1) you need to build the binary files. To achieve the result, please, follow the section #3.

2) start the RS485 bus emulator

	../../RS485_emulator --foreground

3) Start as many slaves as you want with the following command:

	./fakeSerialDev2 --id=<n>

4) Start the master typing the following command: 

	./fakeMaster2 --ids=<n1>[,<n2>]...


## 3.0 Binary file building
To build the files required by the test, type the following command:
	
	[TESTMODE=1] [GDB=1] make 

To clean the tree, execute the next command:
	make cleanall
