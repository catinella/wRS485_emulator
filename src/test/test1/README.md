![Project's banner](../../../images/wRS485_emulator.png)

# wRS485_emulator - RS485 BUS emulator for test and development environment 

## 1.0 Files

|     Files/Dirs    |               Description                |
|-------------------|------------------------------------------|
| fakeMaster1.c     | Source code of the master's process      |
| fakeMaster1.h     | Header file with configurable parameters |
| fakeSerialDev1.c  | Source code for the slave processes      |
| Makefile          | Set of rules to build the binary files   |

## 2.0 Description
This is a simple example: the virtual-device master sends a data stream in broadcast. All virtual-device slaves receive the data,
print it, and write it to the file specified as file's argument.

![](./images/overview.svg)

## 2.1 Test preparation
1) you need to build the binary files. To achieve the result, please, follow the section #3.

2) start the RS485 bus emulator

	../../RS485_emulator --foreground

3) Start as many slaves as you want with the following command:

	./fakeSerialDev1 --repoFile=<file name> [--verbose]

4) Start the master typing the following command: 

	./fakeMaster1 [--time=<seconds>] [--loopSleep=<milliseconds>]

**About the order**

As on a real serial bus, the master cannot know whether the transmitted data has been received. For this reason you have to start
all slaves before the master. In this way every slave will initialize itself and will wait for new data from the bus' master.

## 2.2 Test execution
During the test you will see data sent by master flowing to the bus, being received by the slaves, then being written on the slave's
files.

If you want, you can simulate a bus-interruption event sending a **SIGUSR1** signal to the mastre's process, using the following
command:

	kill -10 <master PID>

Then you will see that the master continues sending data, but none of the slaves receives it. If you send the same signal again,
the emulator will virtually restore the bus connection and the slaves will start receiving data again.

## 3.0 Binary file building
To build the files required by the test, type the following command:
	
	[TESTMODE=1] [GDB=1] make 

To clean the tree, execute the next command:
	make cleanall
