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
#	Settings:
#		PREFIX     Set your customized path where you want to install the RS485 emulator
#		TESTMODE   Set it to execute the unit tests as unprivileged user
#		GDB        It enables the debugger support
#
#-------------------------------------------------------------------------------------------------------------------------------
MAKEFLAGS += --no-builtin-rules

CPP      = g++
CC       = gcc

prjComponents     = RS485_emulator libRS485emulatorAPI.so RS485_updateSignal 

RS485EMULE_GROUP := $(shell sed -n 's/^tty:[^:]*:\([0-9]\+\):.*/\1/p' /etc/group)

VERSION          ?= "unknown"
PREFIX           ?= /usr/local
TESTMODE         ?= 0 
GDB              ?= 0
DEBUG            ?= 0

PRJHOME          := $(shell pwd)/..


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

LIBS =                                    \
	$(shell pkg-config --libs sqlite3)  \
	$(shell pkg-config --libs glib-2.0) \
	$(shell pkg-config --libs wError)

INCLUDE  =                                    \
	-I/usr/include -I/usr/local/include -I. \
	$(shell pkg-config --cflags sqlite3)    \
	$(shell pkg-config --cflags glib-2.0)   \
	$(shell pkg-config --cflags wError)

.PHONY: emulator clean test cleanall install uninstall help _help 

#-------------------------------------------------------------------------------------------------------------------------------
#                                              C O M P I L I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------
all:				$(prjComponents)

%.o:				%.c %.h 
				@echo "[ CC ] $@"
				@$(CC) $(GCCOPTS) -c $(INCLUDE) -o $@ $(SYMBOLS) $<

%-sh.o:			%.c %.h
				@echo "[CC-SH] $@"
				@$(CC) $(GCCOPTS) -Wall -Wextra -pedantic -fPIC -shared -c $(INCLUDE) -o $@ $(SYMBOLS) $<

RS485_emulator:		RS485_emulator.o RS485_commonLib.o RS485_portsDB.o RS485_virtualPort.o RS485_virtualPortsList.o
				@echo "[LD C1] $@"
				@$(CC) -Wall -o $@ $^ $(LIBS)
	
RS485_updateSignal:	RS485_updateSignal.o RS485_commonLib.o
				@echo "[LD C4] $@"
				@$(CC) -Wall -o $@ $^ -lwError

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
				@rm -fv $(prjComponents)

