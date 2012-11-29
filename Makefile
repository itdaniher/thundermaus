DEVICE  = attiny84a
F_CPU   = 8000000 #
FUSE_L  = 0xE2# internal 8MHz oscillator running ATTINY at 8MHz, 64ms startup delay
FUSE_H  = 0xDD# SPI programming enabled, BOD @ 2.7v
AVRDUDE = avrdude -c avrispmkII -P usb -p attiny84


CFLAGS  =  -I. -DDEBUG_LEVEL=0
OBJECTS = thundermaus.o 

COMPILE = avr-gcc -g -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)
COMPILEPP = avr-g++ -Wall -Os -DF_CPU=$(F_CPU) $(CFLAGS) -mmcu=$(DEVICE)

hex: thundermaus.hex

# symbolic targets:
help:
	@echo "make hex ....... to build thundermaus.hex"
	@echo "make program ... to flash fuses and firmware"
	@echo "make clean ..... to delete objects and hex file"


# flash fuses and program firmware
program: thundermaus.hex 
	@[ "$(FUSE_H)" != "" -a "$(FUSE_L)" != "" ] || \
		{ echo "*** Edit Makefile and choose values for FUSE_L and FUSE_H!"; exit 1; }
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m 
	$(AVRDUDE) -U flash:w:thundermaus.hex:i

# rule for deleting dependent files (those which can be built by Make):
clean:
	rm -f thundermaus.hex thundermaus.lst thundermaus.obj thundermaus.cof thundermaus.list thundermaus.map thundermaus.eep.hex thundermaus.elf *.o  thundermaus.s
.cpp.o:
	$(COMPILEPP) -c $< -o $@


# Generic rule for compiling C files:
.c.o:
	$(COMPILE) -c -std=gnu99 $< -o $@

# Generic rule for assembling Assembler source files:
.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

# Generic rule for compiling C to assembler, used for debugging only.
.c.s:
	$(COMPILE) -S $< -o $@

# file targets:

thundermaus.elf: $(OBJECTS)
	$(COMPILE) -o thundermaus.elf $(OBJECTS)

thundermaus.hex: thundermaus.elf
	rm -f thundermaus.hex thundermaus.eep.hex
	avr-objcopy -j .text -j .data -O ihex thundermaus.elf thundermaus.hex
	avr-size thundermaus.hex

# debugging targets:

disasm:	thundermaus.elf
	avr-objdump -d thundermaus.elf

c:
	$(COMPILE) -E thundermaus.c 

