# Makefile
#-----------------------------------------------------------------------------------------------------------------------------------
#    __  __       _             _     _ _          _____ _           _        _           _   ____            _                 
#   |  \/  | ___ | |_ ___  _ __| |__ (_) | _____  | ____| | ___  ___| |_ _ __(_) ___ __ _| | / ___| _   _ ___| |_ ___ _ __ ___  
#   | |\/| |/ _ \| __/ _ \| '__| '_ \| | |/ / _ \ |  _| | |/ _ \/ __| __| '__| |/ __/ _` | | \___ \| | | / __| __/ _ \ '_ ` _ \ 
#   | |  | | (_) | || (_) | |  | |_) | |   <  __/ | |___| |  __/ (__| |_| |  | | (_| (_| | |  ___) | |_| \__ \ ||  __/ | | | | |
#   |_|  |_|\___/ \__\___/|_|  |_.__/|_|_|\_\___| |_____|_|\___|\___|\__|_|  |_|\___\__,_|_| |____/ \__, |___/\__\___|_| |_| |_|
#                                                                                                   |___/
#
# File name: Makefile
#
# Author: Silvano Catinella <catinella@yahoo.com>
#
# Description:
#	This makefile allows you to produce all image files starting by their ascii definitions
#
#	SVGBOB (https://lib.rs/crates/svgbob_cli):
#		Svgbob can create a nice graphical representation of your text diagrams. It provides a CLI which takes text as input
#		and creates an svg image as an output.
#
#
#-----------------------------------------------------------------------------------------------------------------------------------

#
# If you installed Rust locally, don't forget to load the environment
# source "$HOME/.cargo/env"
#

SRCs := $(shell ls *.asciiDiag)
TGTs := $(SRCs:.asciiDiag=.svg)
TOOL ?= aasvg

all:		$(TGTs)

%.svg:	%.asciiDiag
		@echo "[GEN] $@"
		@$(TOOL) < $< > $@

clean:
		@rm -fv $(TGTs)
