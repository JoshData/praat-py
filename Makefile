SOURCES=mono_embed.c mono_embed_host.c mono_embed_libs.o
ORIGINALS=originals-4509
DISTFILES=README Makefile \
		mono_embed.c mono_embed.h PraatMonoEmbed.cs PraatTypeWrappers.cs \
		makefile.patch Interpreter.patch \
		libs/*

all: libs/js.dll libs/PraatMonoEmbed.dll mono_embed

dist: dist/praat-py.zip dist/praat-py_linux_i386

PraatTypeWrappers.cs: create_class_wrappers.pl
	perl create_class_wrappers.pl > PraatTypeWrappers.cs

libs/PraatMonoEmbed.dll: PraatMonoEmbed.cs PraatTypeWrappers.cs
	gmcs PraatMonoEmbed.cs PraatTypeWrappers.cs -target:library -out:libs/PraatMonoEmbed.dll \
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

patches: Interpreter.patch makefile.patch

Interpreter.patch: $(ORIGINALS)/Interpreter.c ../sys/Interpreter.c
	true || diff -u $(ORIGINALS)/Interpreter.c ../sys/Interpreter.c > Interpreter.patch

makefile.patch: $(ORIGINALS)/makefile ../makefile
	true || diff -u $(ORIGINALS)/makefile ../makefile > makefile.patch

patch-praat:
	patch ../makefile makefile.patch
	patch ../sys/Interpreter.c Interpreter.patch

dist/praat-py.zip: $(DISTFILES)
	rm -f dist/praat-py.zip
	zip dist/praat-py.zip $(DISTFILES)

dist/praat-py_linux_i386: dist/praat-py.zip
	cd ..; make
	cp ../praat dist/praat-py_linux_i386

