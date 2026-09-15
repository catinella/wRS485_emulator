![Project's banner](../../images/wRS485_emulator.png)

# wRS485_emulator - RS485 BUS emulator for test and development environment 

## 1.0 Files

|           Files/Dirs        |                             Description                              |
|-----------------------------|----------------------------------------------------------------------|
| broadcastingComm_test       | Simple test with broadcasting communication scenario                 |
| unicastComm_test            | Test with unicast communication between master and slaves            |
| libForTests.c               | Set of simple functions used by many tests                           |
| libForTests.h               | Header file                                                          |
| Makefile                    | Rules to build the tests                                             |
| utest_libForTests.c         | Unit tests for libForTests.o module                                  |
| utest_RS485_commonLib.c     | Unit tests for RS485_commonLib.o module                              |
| utest_RS485_portsDB.c       | Unit tests for RS485_portsDB.o                                       |
| utest_RS485_virtualPort.c   | Unit tests for RS485_virtualPort.o                                   |

## 2.0 Description
This folder contains the unit tests and higher-level tests for the project.

# 3.0 Tests building
In order to compile the unit tests, the minute tool must be installed in the system. For further details, please, read the
[minute hompage](https://github.com/catinella/minute).

In order to build the test executable files, type the following command:
	
	[TESTMODE=1] [GDB=1] make 

If you want to install directly just the binary files, you can use the following command

	[PREFIX=<folder>] make install

For further details on the available options, please read the Makefile header.
