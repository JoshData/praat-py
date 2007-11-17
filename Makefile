DISTFILES=README Makefile \
		scripting.c scripting.h python.c \
		patch.pl

all: scripting.o #sendpraat

scripting.o: scripting.c scripting.h python.c
	gcc -g -c scripting.c -o scripting.o
	gcc -g -c python.c -o python.o `python2.5-config --cflags`

sendpraat: ../sys/sendpraat.c
	gcc -g -o sendpraat ../sys/sendpraat.c -DSTANDALONE -lXm

patch-praat:
	perl patch.pl ../makefile "cd artsynth; make" "\tcd scripting; make"
	perl patch.pl ../makefile "sys/libsys.a GSL/libgsl.a kar/libkar.a FLAC/libFLAC.a mp3/libmp3.a \\" "\t\tscripting/scripting.o scripting/python.o \`python2.5-config --ldflags\` `echo -e \"import distutils.sysconfig\nprint distutils.sysconfig.get_config_var('LINKFORSHARED')\" | python` \\"
	perl patch.pl ../sys/Interpreter.c "#include \"Formula.h\"" "#include \"../scripting/scripting.h\""
	perl patch.pl ../sys/Interpreter.c "int Interpreter_run (Interpreter me, wchar_t *text) {" \
		"\tif (scripting_run_praat_script(me, text))\n\t\treturn 1;"
	perl patch.pl --replace ../sys/sendpraat.c "#if 0" "#if STANDALONE"

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
