SOURCES=mono_embed.c mono_embed_host.c mono_embed_libs.o
DISTFILES=README Makefile \
		mono_embed.c mono_embed.h PraatMonoEmbed.cs \
		patch.pl \
		libs/PraatMonoEmbed.dll libs/Iron*.dll

all: libs/PraatMonoEmbed.dll mono_embed

dist: dist/praat-py.zip dist/praat-py_linux_i386
	scp -r dist publius:www/code/praat-py

PraatTypeWrappers.cs: create_class_wrappers.pl
	perl create_class_wrappers.pl > PraatTypeWrappers.cs

libs/PraatMonoEmbed.dll: PraatMonoEmbed.cs
	gmcs PraatMonoEmbed.cs -target:library -out:libs/PraatMonoEmbed.dll \
		-r:libs/IronPython.dll -d:PYTHON

#		-r:libs/js.dll -d:JAVASCRIPT \

mono_embed_host.c: libs/PraatMonoEmbed.dll
	MONO_PATH=libs mkbundle2 -c --nomain -z \
		-o mono_embed_host.c -oo mono_embed_libs.o \
		PraatMonoEmbed.dll \
		IronPython.dll IronMath.dll

#		js.dll IKVM.GNU.Classpath.dll IKVM.Runtime.dll \

mono_embed_libs.o: mono_embed_host.c

mono_embed:  $(SOURCES)
	gcc -c mono_embed.c -o mono_embed.o `pkg-config --cflags mono`
	gcc -c mono_embed_host.c -o mono_embed_host.o `pkg-config --cflags mono`

libs/js.dll: 
	cd rhino1_6R5; ant smalljar
	cp rhino1_6R5/build/rhino1_6R5/smalljs.jar libs/js.jar
	ikvmc libs/js.jar
	rm libs/js.jar

patch-praat:
	perl patch.pl ../makefile "cd artsynth; make" "\tcd scripting; make"
	perl patch.pl ../makefile "artsynth/libartsynth.a" "\t\tscripting/mono_embed.o scripting/mono_embed_host.o scripting/mono_embed_libs.o \`pkg-config --libs mono\` -lz \\"
	perl patch.pl ../sys/Interpreter.c "#include \"Formula.h\"" "#include \"../scripting/mono_embed.h\""
	perl patch.pl ../sys/Interpreter.c "int Interpreter_run \(" \
		"\tif (strncmp(text, \"#python\", 7) == 0) {\n\t\tmono_embed_run_praat_script(me, text);\n\t\treturn 1;\n\t}"

dist/praat-py.zip: $(DISTFILES)
	rm -f dist/praat-py.zip
	zip dist/praat-py.zip $(DISTFILES)

dist/praat-py_linux_i386: dist/praat-py.zip
	cd ..; make
	cp ../praat dist/praat-py_linux_i386

