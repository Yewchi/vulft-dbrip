C = gcc
F =
LANGUAGE_OUT = 
DISCLAIM =

DEBUG = -g -DDEBUG
VERBOSE = -g -DVERBOSE

LUA_TARGET = build/dotabuff-prepend-o-lua.o

ifeq ($(D),d)
	F=$(DEBUG)
endif
ifeq ($(D),v)
	F=$(VERBOSE)
endif

ifeq ($(L),Lua)
	LANGUAGE_OUT=$(LUA_TARGET)
else # Add any coditionals here
	LANGUAGE_OUT=$(LUA_TARGET) # Defaults to Lua
endif

ifneq ($(DBD),$(NULL))
	F+=-DDBD
endif

default: prepend $(DISCLAIM) 

prepend: build/dotabuff-prepend.o build/dotabuff-prepend-parser.o build/dotabuff-prepend-process.o build/dotabuff-prepend-o.o $(LANGUAGE_OUT)
	$(C) $(F) -o $@ $^

all: prepend disclaimer download-strip-name built-in download-recent-guide

rip: disclaimer download-strip-name built-in download-recent-guide

disclaimer:
	@echo "C files default to Windows 'Sleep' and #include <windows.h>, wget is used to get data, the build env is MinGW MSYS2. If you can't fix it, don't use it! Default data rip domain is localhost, not DotaBuff."
	@echo ""
	@echo "Data Rip Workflow:"
	@echo "   Download https://github.com/SteamDatabase/GameTracking-Dota2/blob/master/game/dota/resource/dota_english.txt or another data rip of the same format. \"DOTA_Tooltip_Stuff_ingame_name\" \"Ingame Stuff\". Rename it \"d2vpkr.dat\" -- Or use VPK unpacker -> resources/dota_english"
	@echo "   run: ./download-strip-names ./built-in-item ./download-recent-guide ./prepend (from the /lib_hero/hero folder with .dats present.)"
	@echo ""
	@echo "Building..."

download-strip-name: strip-names.c
	$(C) $(F) $< -o $@

built-in: readable-to-built-in-item.c
	$(C) $(F) $< -o $@

download-recent-guide: download-recent-match-data.c
	$(C) $(F) $< -o $@

build/dotabuff-prepend.o: dotabuff-prepend.c dotabuff-prepend.h
	$(C) $(F) -o $@ -c $<

build/dotabuff-prepend-parser.o: dotabuff-prepend-parser.c dotabuff-prepend-parser.h
	$(C) $(F) -o $@ -c $<

build/dotabuff-prepend-process.o: dotabuff-prepend-process.c dotabuff-prepend-process.h
	$(C) $(F) -o $@ -c $<

build/dotabuff-prepend-o.o: dotabuff-prepend-o.c dotabuff-prepend-o.h
	$(C) $(F) -o $@ -c $<

build/dotabuff-prepend-o-lua.o: dotabuff-prepend-o-lua.c dotabuff-prepend-o.h 
	$(C) $(F) -o $@ -c $<

clean:
	@rm --force download-strip-name
	@rm --force built-in
	@rm --force download-recent-guide
	@rm --force prepend
	@rm --force build/*
	@echo " ..Clean."
