DISTFILES=README Makefile \
		scripting.c scripting.h python.c util.c util.h \
		sendpraat_main.c \
		praat-py.patch

CCOPTS=-g -std=c99

all: scripting.o python.o util.o sendpraat

clean:
	rm *.o

scripting.o: scripting.c scripting.h
	gcc $(CCOPTS) -c scripting.c -o scripting.o
	
python.o: python.c
	gcc $(CCOPTS) -c python.c -o python.o `python-config --cflags`

util.o: util.c util.h
	gcc $(CCOPTS) -c util.c -o util.o

sendpraat: ../sys/sendpraat.c sendpraat_main.c
	gcc $(CCOPTS) -o sendpraat ../sys/sendpraat.c sendpraat_main.c -lXm

praat-py.patch:
	diff -ur -x "*.[oa]" ../../sources_5104/ ..|grep -v "Only in .." > praat-py.patch

patch-praat:
	patch -p0 < praat-py.patch

dist/praat-py.zip: $(DISTFILES)
	rm -f dist/praat-py.zip
	zip dist/praat-py.zip $(DISTFILES)

dist/praat-py_linux_i386: dist/praat-py.zip
	cd ..; make
	cp ../praat dist/praat-py_linux_i386

praat: all
	cd ..; make
run-praat: all
	cd ..; make; ./praat

deploy: dist/praat-py.zip dist/praat-py_linux_i386
	scp -r dist occams.info:www/code/praat-py
