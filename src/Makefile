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
#		PREFIX     Set your customized path where you want to install the RS485 emulator
#		TESTMODE   Set it to execute the unit tests as unprivileged user
#		GDB        It enables the debugger support
#
#-------------------------------------------------------------------------------------------------------------------------------
MAKEFLAGS += --no-builtin-rules

ARCH    ?= x86

ifeq ($(ARCH), arm)
	CPP      = $(BHOST)/bin/arm-none-linux-gnueabihf-g++
	CC       = $(BHOST)/bin/arm-none-linux-gnueabihf-gcc
	SLIBPATH = -L$(BHOST)/usr/lib
	INCLPATH := -I$(BHOST)/usr/include  -I/usr/local/include -I. $(shell pkg-config --cflags glib-2.0)
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

prjComponents     = RS485_emulator libRS485emulatorAPI.so RS485_updateSignal 

RS485EMULE_GROUP := $(shell sed -n 's/^tty:[^:]*:\([0-9]\+\):.*/\1/p' /etc/group)
VERSION          := "unknown"

PREFIX           ?= /usr/local
TESTMODE         ?= 0 
GDB              ?= 0
DEBUG            ?= 0

PRJHOME          := $(shell pwd)


ifeq ($(GDB), 1)
	gopt = -o0 -g
else
	gopt = -o2 
endif

GCCOPTS = -Wall $(gopt) -std=c11 -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

SYMBOLS =                                       \
	"-DTESTMODE=$(TESTMODE)"                  \
	"-DRS485EMULE_GROUP=$(RS485EMULE_GROUP)"  \
	"-DPREFIX=\"$(PREFIX)\""                  \
	"-DPRJHOME=\"$(PRJHOME)\""                \
	"-DVERSION=\"$(VERSION)\""                \
	"-D__DEBUG__=$(DEBUG)"                    \
	"-DGDB=$(GDB)"                        

.PHONY: emulator clean test cleanall install uninstall help _help

#-------------------------------------------------------------------------------------------------------------------------------
#                                              C O M P I L I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------
all:				$(prjComponents)

%.o:				%.c %.h
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $(INCLPATH) -o $@ $(SYMBOLS) $<

%-sh.o:			%.c %.h
				@echo "[$(CCLAB)-SH] $@"
				@$(CC) $(GCCOPTS) -Wall -Wextra -pedantic -fPIC -shared -c $(INCLPATH) -o $@ $(SYMBOLS) $<

RS485_emulator:		RS485_emulator.o RS485_commonLib.o RS485_portsDB.o RS485_virtualPort.o RS485_virtualPortsList.o
				@echo "[LD C1] $@"
				@$(CC) -Wall -o $@ $^ -lsqlite3 $(shell pkg-config --static --libs glib-2.0)
	
RS485_updateSignal:	RS485_updateSignal.o RS485_commonLib.o
				@echo "[LD C4] $@"
				@$(CC) -Wall $^ -o $@

libRS485emulatorAPI.so:	RS485emulatorAPI-sh.o RS485_commonLib-sh.o
				@echo "[LD-SO] $@"
				@$(CC) -Wall -shared -o $@ -Wl,-Bdynamic $^ -lsqlite3

#-------------------------------------------------------------------------------------------------------------------------------
#                                                 O T H E R   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

install:			all
				@echo "[INSTALL]"
				@install -g root -o root -m 0750 -v --target-directory=$(PREFIX)/sbin    ./RS485_emulator
				@install -g root -o root -m 4755 -v --target-directory=$(PREFIX)/bin     ./RS485_updateSignal
				@install -g root -o root -m 0755 -v --target-directory=$(PREFIX)/lib     ./libRS485emulatorAPI.so
				@install -g root -o root -m 0644 -v --target-directory=$(PREFIX)/include ./include/RS485_emulator.h
				@install -g root -o root -m 0644 -v --target-directory=$(PREFIX)/include ./include/RS485emulatorAPI.h
				@/usr/sbin/ldconfig --verbose

clean:
				@echo "[CLEAN]"
				@rm -fv *.o

cleanall:			clean
				@echo "[CLEANALL]"
				@rm -fv $(prjComponents)

