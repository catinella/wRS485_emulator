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
#	You can also install the binary files on your target device setting the TGTHOST symbol. You probably need to set the
#	PREFIX symbol too.
#		Eg. make install TGTHOST=perseus-dev-1 PREFIX=/usr
#
#	Settings:
#		ARCH       {x86|arm}
#		BHOST      Buildroot's host folder
#		PREFIX     Set your customized path where you want to install the RS485 emulator
#		TESTMODE   Set it to execute the unit tests as unprivileged user
#		GDB        It enables the debugger support
#		NOMODBUS   It prevent the building of all tests where they include MODBUS support
#
#-------------------------------------------------------------------------------------------------------------------------------
#help-menu-symbol
ARCH    ?= x86
#help-menu-symbol
TGTHOST ?= local

ifeq ($(ARCH), arm)
	CPP      = $(BHOST)/bin/arm-none-linux-gnueabihf-g++
	CC       = $(BHOST)/bin/arm-none-linux-gnueabihf-gcc
	SLIBPATH = -L$(BHOST)/usr/lib
	INCLPATH = -I$(BHOST)/usr/include -I./include
	CCLAB    = CC/ARM
	DLIBPATH = $(BHOST)/usr/lib

else
	CPP      = g++
	CC       = gcc
	SLIBPATH = -L/usr/lib/x86_64-linux-gnu -L/usr/local/lib/x86_64-linux-gnu
	INCLPATH = -I/usr/include -I/usr/local/include -I./include
	CCLAB    = CC/X86
	DLIBPATH = /usr/local/lib/x86_64-linux-gnu
endif

testSrcsList     := $(shell ls test/test_*.c)
testsList        := $(testSrcsList:test/%.c=%)
fDevSrcsList     := $(shell ls test/fakeSerialDev*.c)
fakeDevsList     := $(fDevSrcsList:test/%.c=%)
fMastersSrcsList := $(shell ls test/fakeMaster*.c)
fakeMastersList  := $(fMastersSrcsList:test/%.c=%)

prjComponents     = RS485_emulator libRS485emulatorAPI.so RS485_updateSignal 

RS485EMULE_GROUP := $(shell sed -n 's/^tty:[^:]*:\([0-9]\+\):.*/\1/p' /etc/group)
VERSION          := $(shell tools/getVer.pl)

#help-menu-symbol
PREFIX           ?= /usr/local
#help-menu-symbol
TESTMODE         ?= 0 
#help-menu-symbol
GDB              ?= 0
#help-menu-symbol
NOMODBUS         ?= 0
#help-menu-symbol
DEBUG            ?= 0

PRJHOME          := $(shell pwd)


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

GCCOPTS = -Wall $(gopt) -std=c11 -D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

.PHONY: emulator clean test cleanall install uninstall help _help

#-------------------------------------------------------------------------------------------------------------------------------
#                                                  M A I N   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------
#help-menu-rule
all:				$(prjComponents)

#help-menu-rule
unit-tests:			$(testsList)

#help-menu-rule
integration-tests:	RS485_emulator RS485_updateSignal $(fakeDevsList) $(fakeMastersList)

#-------------------------------------------------------------------------------------------------------------------------------
#                                              C O M P I L I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

src/%.o:			src/%.c include/%.h
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $< -I./include -o $@         \
					"-DTESTMODE=$(TESTMODE)"                  \
					"-DRS485EMULE_GROUP=$(RS485EMULE_GROUP)"  \
					"-DPREFIX=\"$(PREFIX)\""                  \
					"-DPRJHOME=\"$(PRJHOME)\""                \
					"-DVERSION=\"$(VERSION)\""                \
					"-D__DEBUG__=$(DEBUG)"                    \
					"-DGDB=$(GDB)"                        


test/test_%.o:		test/test_%.c src/%.c
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I./test "-DTESTMODE=$(TESTMODE)" -o $@ 
			

test/test_%.o:		test/test_%.c
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I./test "-DTESTMODE=$(TESTMODE)" -o $@ 
			

test/fakeSerialDev%.o:	test/fakeSerialDev%.c
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I./test "-DTESTMODE=$(TESTMODE)" -o $@


test/libForTests.o:	test/libForTests.c
				@echo "[$(CCLAB) C1] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I./test "-DTESTMODE=$(TESTMODE)" -o $@


test/fakeMaster%.o:	test/fakeMaster%.c
				@echo "[$(CCLAB)] $@"
				@$(CC) $(GCCOPTS) -c $< $(INCLPATH) -I./test "-DTESTMODE=$(TESTMODE)" -o $@


src/%-sh.o:			src/%.c include/%.h
				@echo "[$(CCLAB)-SH] $@"
				@$(CC) $(GCCOPTS) -Wall -Wextra -pedantic -fPIC -shared  -c $< $(INCLPATH) -o $@ \
					"-DTESTMODE=0" "-DPREFIX=\"$(PREFIX)\"" "-D__DEBUG__=$(DEBUG)" 

#-------------------------------------------------------------------------------------------------------------------------------
#                                               B U I L D I N G   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

RS485_emulator:		src/RS485_emulator.o src/RS485_commonLib.o src/RS485_portsDB.o src/RS485_virtualPort.o \
				src/RS485_virtualPortsList.o
				@echo "[LD C1] $@"
				@$(CC) -Wall $^ -lsqlite3 -o $@
	

test_RS485_portsDB:	src/RS485_portsDB.o src/RS485emulatorAPI.o test/test_RS485_portsDB.o src/RS485_commonLib.o
				@echo "[LD C2] $@"
				@$(CC) -Wall -lsqlite3 $^ -o $@


test_RS485_virtualPort:	src/RS485_portsDB.o src/RS485emulatorAPI.o src/RS485_virtualPort.o test/test_RS485_virtualPort.o \
				src/RS485_commonLib.o
				@echo "[LD C3] $@"
				@$(CC) -Wall -lsqlite3 $^ -o $@

RS485_updateSignal:	src/RS485_updateSignal.o src/RS485_commonLib.o
				@echo "[LD C4] $@"
				@$(CC) -Wall $^ -o $@


ifeq ($(NOMODBUS), 1)
fakeSerialDev3:		
				@echo "[SKIPPED] $@"
endif


fakeMaster%:		test/fakeMaster%.o src/RS485_commonLib.o test/libForTests.o src/RS485emulatorAPI.o
				@echo "[LD] $@"
				@$(CC) -Wall $^ -lm -lsqlite3 -o $@


fakeSerialDev%:		test/fakeSerialDev%.o src/RS485_commonLib.o test/libForTests.o src/RS485emulatorAPI.o
				@echo "[LD] $@"
				@$(CC) -Wall $^ -lm -lsqlite3 $(mbLinkOpt) -o $@


test_RS485_%:		src/RS485_%.o test/test_RS485_%.o src/RS485_commonLib.o test/libForTests.o
				@echo "[LD] $@"
				@$(CC) -Wall $^ -lm -o $@


test_%:			test/test_%.o test/libForTests.o
				@echo "[LD] $@"
				@$(CC) -Wall $^ -lm -o $@

libRS485emulatorAPI.so:	src/RS485emulatorAPI-sh.o src/RS485_commonLib-sh.o
				@echo "[LD-SO] $@"
				@$(CC) -Wall $^ -shared -o $@

#-------------------------------------------------------------------------------------------------------------------------------
#                                                 O T H E R   R U L E S
#-------------------------------------------------------------------------------------------------------------------------------

#help-menu-rule
install:			all
				@echo "[INSTALL on $(TGTHOST)]"
ifeq ($(TGTHOST), local)
				@install -g root -o root -m 0750 -v --target-directory=$(PREFIX)/sbin    ./RS485_emulator
				@install -g root -o root -m 4755 -v --target-directory=$(PREFIX)/bin     ./RS485_updateSignal
				@install -g root -o root -m 0755 -v --target-directory=$(PREFIX)/lib     ./libRS485emulatorAPI.so
				@install -g root -o root -m 0644 -v --target-directory=$(PREFIX)/include ./include/RS485_emulator.h
				@install -g root -o root -m 0644 -v --target-directory=$(PREFIX)/include ./include/RS485emulatorAPI.h
				@/usr/sbin/ldconfig --verbose
else
				@scp ./RS485_emulator                          \
					$(TGTHOST):$(PREFIX)/sbin
				@scp ./RS485_updateSignal                      \
					$(TGTHOST):$(PREFIX)/bin
				@scp ./libRS485emulatorAPI.so                  \
					$(TGTHOST):$(PREFIX)/lib
				@ssh $(TGTHOST) chmod 750                      \
					$(PREFIX)/sbin/RS485_emulator            \
					$(PREFIX)/lib/libRS485emulatorAPI.so
				@ssh $(TGTHOST) chmod 4755                     \
					$(PREFIX)/bin/RS485_updateSignal
				@ssh $(TGTHOST) chown root:root                \
					$(PREFIX)/sbin/RS485_emulator            \
					$(PREFIX)/bin/RS485_updateSignal         \
					$(PREFIX)/lib/libRS485emulatorAPI.so
endif

#help-menu-rule
uninstall:
				@echo "[UNINSTALL]"
				@rm -fv $(PREFIX)/sbin/RS485_emulator $(PREFIX)/bin/RS485_updateSignal  \
				$(PREFIX)/lib/libRS485emulatorAPI.so $(PREFIX)/include/RS485_emulator.h \
				$(PREFIX)/include/include/RS485emulatorAPI.h @/usr/sbin/ldconfig 


#help-menu-rule
clean:
				@echo "[CLEAN]"
				@rm -fv src/*.o test/*.o


#help-menu-rule
cleanall:			clean
				@echo "[CLEANALL]"
				@rm -fv $(prjComponents) $(testsList) $(fakeDevsList) $(fakeMastersList)


include tools/helpRule.mkfile

help:				_help
				@echo "	BHOST = <buildroot host filder>"
