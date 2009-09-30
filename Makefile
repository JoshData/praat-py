UPVER=5117

include ../makefile.defs

DISTFILES=README Makefile \
		scripting.c scripting.h python.c util.c util.h \
		praat-py.patch

ifeq ($(EXE), praat.exe)
	CC += -I winbuild/Python-2.6.2/Include -I winbuild/Python-2.6.2 -DMS_WIN32
endif

all: scripting.o python.o util.o

clean:
	rm *.o

scripting.o: scripting.c scripting.h
	$(CC) -c scripting.c -o scripting.o
	
python.o: python.c
	$(CC) -c python.c -o python.o `python-config --cflags`

util.o: util.c util.h
	$(CC) -c util.c -o util.o

sendpraat: ../sys/sendpraat.c sendpraat_main.c
	$(CC) -o sendpraat ../sys/sendpraat.c sendpraat_main.c -lXm

praat-py.patch:
	diff -ur -x "*.[oa]" ../../sources_${UPVER}/ ..|grep -v "Only in .." > praat-py.patch

patch-praat:
	patch -p0 < praat-py.patch

dist/praat-py.zip: $(DISTFILES)
	rm -f dist/praat-py.zip
	zip dist/praat-py.zip $(DISTFILES)

dist/ChangeLog: ChangeLog
	cp ChangeLog dist/ChangeLog

dist/ubuntu_jaunty/praat-py: dist/praat-py.zip
	mkdir -p dist/ubuntu_jaunty
	cd ..; bash build_linux.sh
	cp ../praat dist/ubuntu_jaunty/praat-py

dist/win32/praat-py.exe: dist/praat-py.zip
	mkdir -p dist/win32
	cd ..; bash build_win32.sh
	cp ../praat.exe dist/win32/praat-py.exe

praat: all
	cd ..; make
run-praat: all
	cd ..; make; ./praat

deploy: dist/ChangeLog dist/praat-py.zip dist/ubuntu_jaunty/praat-py dist/win32/praat-py.exe
	scp -r dist occams.info:www/code/praat-py
