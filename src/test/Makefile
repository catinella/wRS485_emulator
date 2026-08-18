#-------------------------------------------------------------------------------------------------------------------------------
#
#                                                 ____  ____  _  _    ___ ____  
#                                                |  _ \/ ___|| || |  ( _ ) ___| 
#                                                | |_) \___ \| || |_ / _ \___ \ 
#                                                |  _ < ___) |__   _| (_) |__) |
#                                                |_| \_\____/   |_|  \___/____/ 
#                                                           Emulator         
#
# 
# File:     Makefile
# 
# Authour:  Silvano Catinella
# 
# Language: make
# 
# Description:
#	This file contains the set of rules you need to build the RS485 emulator software and its testing modules.
#	
#	Multiplatform:
#	==============
#	This is an emulator so you don't need to install it in your target industrial device, usually. But, in some case you
#	could need it, for example to evaluate the performance lack between the emulator's environment and the real one. 
#	In these cases, to compile the project for ARM targets you have to set the ARCH and BHOST symbols.
#		Eg. make BHOST=../buildroot-fimer/output/host ARCH=arm
#
#	Settings:
#		ARCH       {x86|arm}
#		BHOST      Buildroot's host folder
#		TESTMODE   Set it to execute the unit tests as unprivileged user
#		GDB        It enables the debugger support
#		NOMODBUS   It prevent the building of all tests where they include MODBUS support
#
#-------------------------------------------------------------------------------------------------------------------------------
MAKEFLAGS += -rR

ARCH    ?= x86

ifeq ($(ARCH), arm)
	CPP      = $(BHOST)/bin/arm-none-linux-gnueabihf-g++
	CC       = $(BHOST)/bin/arm-none-linux-gnueabihf-gcc
	SLIBPATH = -L$(BHOST)/usr/lib
	INCLPATH = -I$(BHOST)/usr/include -I. $(shell pkg-config --cflags glib-2.0)
	CCLAB    = CC/ARM
	DLIBPATH = $(BHOST)/usr/lib

else
	CPP      = g++
	CC       = gcc
	SLIBPATH = -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib/x86_64-linux-gnu
	INCLPATH := -I/usr/include -I/usr/local/include -I. $(shell pkg-config --cflags glib-2.0)
	CCLAB    = CC/X86
	DLIBPATH = /usr/local/lib/x86_64-linux-gnu
endif

testSrcsList     := $(shell ls utest_*.c)
testsList        := $(testSrcsList:%.c=%)
fDevSrcsList     := $(shell ls fakeSerialDev*.c)
fakeDevsList     := $(fDevSrcsList:%.c=%)
fMastersSrcsList := $(shell ls fakeMaster*.c)
fakeMastersList  := $(fMastersSrcsList:%.c=%)
RS485EMULE_GROUP := $(shell sed -n 's/^tty:[^:]*:\([0-9]\+\):.*/\1/p' /etc/group)
PRJHOME          := $(shell pwd)

#
# Default configurable values
#
VERSION          ?= unknown
TESTMODE         ?= 0 
GDB              ?= 0
NOMODBUS         ?= 1
DEBUG            ?= 0

SYMBOLS = \
	"-DTESTMODE=$(TESTMODE)"                  \
	"-DRS485EMULE_GROUP=$(RS485EMULE_GROUP)"  \
	"-DPREFIX=\"$(PREFIX)\""                  \
	"-DPRJHOME=\"$(PRJHOME)\""                \
	"-DVERSION=\"$(VERSION)\""                \
	"-D__DEBUG__=$(DEBUG)"                    \
	"-DGDB=$(GDB)"

ifeq ($(GDB), 1)
	gopt = -o0 -g
else
	gopt = -o2 
endif

ifeq ($(NOMODBUS), 1)
	mbLinkOpt =
else
	mbLinkOpt = -lmodbus
endif

GCCOPTS     = -Wall $(gopt) -std=c11 -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
PRJLIB_L    = -Wl,-rpath,../ -L../ -lRS485emulatorAPI
MINUTE_SCG  = /usr/local/share/minute/minute_cCodeGenerator.py

.PHONY: emulator clean test cleanall install uninstall PRJ

#-------------------------------------------------------------------------------------------------------------------------------
#                                                 M A I N  R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

all:				PRJ $(testsList) $(fakeDevsList) $(fakeMastersList)

clean:
				@echo "[CLEAN]"
				@rm -fv *.o *.sgc


cleanall:			clean
				@echo "[CLEANALL]"
				@rm -fv $(testsList) $(fakeDevsList) $(fakeMastersList)
				@make -C../ cleanall

PRJ:
				@make -C ../ all

#-------------------------------------------------------------------------------------------------------------------------------
#                                              C O M P I L I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

test_%.o:			test_%.c ../%.c
				@echo "[$(CCLAB) 1] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I../ "-DTESTMODE=$(TESTMODE)" -o $@ 
			

utest_%.o:			utest_%.c utest_%__main.sgc
				@echo "[$(CCLAB) 2] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I../ "-DTESTMODE=$(TESTMODE)" -o $@ 


utest_%__main.sgc:	utest_%.c
				@echo "[SCRIPT] $@"
				@$(MINUTE_SCG) --srcFilename $< --tgtFilename $@ --verbose


fakeSerialDev%.o:		fakeSerialDev%.c
				@echo "[$(CCLAB) 3] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I../ "-DTESTMODE=$(TESTMODE)" -o $@


libForTests.o:		libForTests.c
				@echo "[$(CCLAB) C1] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I.. "-DTESTMODE=$(TESTMODE)" -o $@


fakeMaster%.o:		fakeMaster%.c
				@echo "[$(CCLAB) 4] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I../ "-DTESTMODE=$(TESTMODE)" -o $@


%.o:				../%.c ../%.h
				@echo "[$(CCLAB) 5] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I. -I../ "-DTESTMODE=$(TESTMODE)" -o $@


#-------------------------------------------------------------------------------------------------------------------------------
#                                               L I N K I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

utest_RS485_virtualPort:	utest_RS485_virtualPort.o RS485_virtualPort.o RS485_commonLib.o RS485_portsDB.o libForTests.o
					@echo "[LD C3] $@"
					@$(CC) -Wall -o $@ $^ -lm -lMinuteC -lsqlite3 $(shell pkg-config --static --libs glib-2.0)

ifeq ($(NOMODBUS), 1)
fakeSerialDev3:		
				@echo "[SKIPPED] $@"
endif


fakeMaster%:		fakeMaster%.o libForTests.o
				@echo "[LD 1] $@"
				$(CC) -Wall -o $@ $^ -lm $(PRJLIB_L) $(shell pkg-config --static --libs glib-2.0)


fakeSerialDev%:		fakeSerialDev%.o libForTests.o 
				@echo "[LD 2] $@"
				$(CC) -Wall -o $@ $^ -lm $(mbLinkOpt) $(PRJLIB_L) $(shell pkg-config --static --libs glib-2.0)


utest_%:			utest_%.o libForTests.o %.o RS485_commonLib.o
				@echo "[LD 4] $@"
				@$(CC) -Wall -o $@ $^ -lm -lMinuteC -lsqlite3 $(shell pkg-config --static --libs glib-2.0)


utest_%:			utest_%.o libForTests.o RS485_commonLib.o
				@echo "[LD 5] $@"
				@$(CC) -Wall -o $@ $^ -lm -lMinuteC -lsqlite3


