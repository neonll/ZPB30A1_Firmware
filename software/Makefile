CC=sdcc
STM8FLASH=stm8flash
CFLAGS=-mstm8 --std-c11 --opt-code-size
DEFINES=STM8S005
PROCESSOR=STM8S005K6
STLINK_VERSION=2

MAIN=electronic_load.c
SRC=
BUILDDIR=build

RELS=$(patsubst %.c,$(BUILDDIR)/%.rel, $(MAIN) $(SRC))
IHX=$(MAIN:%.c=$(BUILDDIR)/%.ihx)
HEX=$(IHX:.ihx=.hex)

.PHONY: all bin mkdir clean flash unlock

all: bin

# SDCC includes working wrong on Windows with "/" slahes
ifeq ($(OS),Windows_NT)
INCLUDES=-I.\\inc
ifeq ($(STLINK_VERSION),2)
PROGRAMMER=ST-LINK-V2
else
PROGRAMMER=ST-LINK
endif
flash: flash_windows
unlock: unlock_windows
else
INCLUDES=-I./inc
PROGRAMMER=stlinkv$(STLINK_VERSION)
flash: flash_unix
unlock: unlock_unix
endif

mkdir:
	@mkdir -p $(BUILDDIR)

bin: mkdir $(IHX)

$(IHX): $(RELS)
	$(CC) $(CFLAGS) $(RELS) -o $@

$(BUILDDIR)/%.rel: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -D $(DEFINES) $< -o $@

$(HEX): $(IHX)
	packihx $< > $@

clean:
	rm -rf $(BUILDDIR)

flash_windows: $(HEX)
	stvp_cmdline -BoardName=$(PROGRAMMER) -Device=$(PROCESSOR) -Port=USB -ProgMode=SWIM \
	-verbose -no_loop -no_log \
	-FileProg=$(OUTPUT_FW)$(OUTPUT_FILE).hex

flash_unix: $(IHX)
	$(STM8FLASH) -c $(PROGRAMMER) -p $(PROCESSOR) -w $(OUTPUT_FW)$(OUTPUT_FILE).hex

unlock_windows:
	@echo "Unlocking is currently unsupported on windows"
	false

unlock_linux:
	$(STM8FLASH) -c $(PROGRAMMER) -p $(PROCESSOR) -u # Works in stm8flash versions after git commit f3f547b4
