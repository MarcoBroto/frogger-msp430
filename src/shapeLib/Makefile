all: libShape.a

AR              = msp430-elf-ar
OBJECTS         = shape.o region.o rect.o vec2.o layer.o rarrow.o

libShape.a: $(OBJECTS)
	$(AR) crs $@ $^

$(OBJECTS): shape.h

install: libShape.a
	mkdir -p ../h ../lib
	mv $^ ../lib
	cp *.h ../h

clean:
	rm -f libShape.a *.o
