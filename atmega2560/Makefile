CPUFREQ=16000000L
FLASHDEV=/dev/ttyU0
FLASHBAUD=115200
FLASHMETHOD=wiring

SRCFILES=src/co2dumper.c \
	src/serial.c \
	src/sysclock.c
HEADFILES=src/co2dumper.h \
	src/serial.h \
	src/sysclock.h

all: bin/co2dumper.hex

bin/co2dumper.bin: $(SRCFILES) $(HEADFILES)

	avr-gcc -Wall -Os -mmcu=atmega2560 -DF_CPU=$(CPUFREQ) -o bin/co2dumper.bin $(SRCFILES)

bin/co2dumper.hex: bin/co2dumper.bin

	avr-size -t bin/co2dumper.bin
	avr-objcopy -j .text -j .data -O ihex bin/co2dumper.bin bin/co2dumper.hex

flash: bin/co2dumper.hex

	sudo chmod 666 $(FLASHDEV)
	avrdude -v -p atmega2560 -c $(FLASHMETHOD) -P $(FLASHDEV) -b $(FLASHBAUD) -D -U flash:w:bin/co2dumper.hex:i

framac: $(SRCFILES)

	-rm framacreport.csv
	frama-c -wp-verbose 0 -wp -rte -wp-rte -wp-dynamic -wp-timeout 300 -cpp-extra-args="-I/usr/home/tsp/framaclib/ -DF_CPU=16000000L -D__AVR_ATmega2560__ -DFRAMAC_SKIP" $(SRCFILES) -then -no-unicode -report -report-csv framacreport.csv

clean:

	-rm *.bin
	-rm bin/*.bin

cleanall: clean

	-rm *.hex
	-rm bin/*.hex

.PHONY: all clean cleanall
