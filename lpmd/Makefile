#
#
#

include ../Makefile.params

PROGRAMS = lpmd

all: $(PROGRAMS)

%.o: %.cc %.h
	$(CXX) -c $(CXXFLAGS) -I../lib $< -o $@

lpmd: ../lib/liblpmd.so cmdline.o input.o main.o
	$(CXX) -o lpmd -Wl,-rpath,$(CURDIR)/../lib cmdline.o input.o main.o $(LDFLAGS) -L../lib -llpmd $(LIBS) 

../lib/liblpmd.so:
	cd ../lib && $(MAKE)

clean:
	-rm -fr $(PROGRAMS) *.o *~ 

