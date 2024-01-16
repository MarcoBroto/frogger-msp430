all: libp2sw.a

AR              = msp430-elf-ar
OBJECTS         = p2switches.o

libp2sw.a: $(OBJECTS)
	$(AR) crs $@ $^

$(OBJECTS): p2switches.h

install: libp2sw.a
	mkdir -p ../h ../lib
	mv $^ ../lib
	cp *.h ../h

clean:
	rm -f *.a *.o
