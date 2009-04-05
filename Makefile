DISTFILES=README Makefile \
		scripting.c scripting.h python.c util.c util.h \
		sendpraat_main.c \
		patch.pl

CCOPTS=-g -std=c99

all: scripting.o python.o util.o sendpraat

clean:
	rm *.o

scripting.o: scripting.c scripting.h
	gcc $(CCOPTS) -c scripting.c -o scripting.o
	
python.o: python.c
	gcc $(CCOPTS) -c python.c -o python.o `python2.5-config --cflags`

util.o: util.c util.h
	gcc $(CCOPTS) -c util.c -o util.o

sendpraat: ../sys/sendpraat.c sendpraat_main.c
	gcc $(CCOPTS) -o sendpraat ../sys/sendpraat.c sendpraat_main.c -lXm

patch-praat:
	perl patch.pl ../makefile "cd artsynth; make" "\tcd scripting; make"
	perl patch.pl ../makefile "audio/libaudio.a FLAC/libFLAC.a mp3/libmp3.a contrib/ola/libOla.a \\" "\t\tscripting/scripting.o scripting/python.o scripting/util.o `python2.5-config --ldflags` `python -c \"import distutils.sysconfig; print distutils.sysconfig.get_config_var('LINKFORSHARED')\"` \\"
	perl patch.pl ../sys/Interpreter.c "#include \"Formula.h\"" "#include \"../scripting/scripting.h\""
	perl patch.pl ../sys/Interpreter.c "int Interpreter_run (Interpreter me, wchar_t *text) {" \
		"\tif (scripting_run_praat_script(me, text))\n\t\treturn 1;"
	#perl patch.pl --replace ../sys/sendpraat.c "#if 0" "#if STANDALONE"

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
