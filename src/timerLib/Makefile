all: libTimer.a

AR              = msp430-elf-ar

libTimer.a: clocksTimer.o sr.o
	$(AR) crs $@ $^

install: libTimer.a
	mkdir -p ../h ../lib
	mv $^ ../lib
	cp *.h ../h

clean:
	rm -f timerLib.a *.o
