cd ..
rm -f praat.exe praat-py.exe
make clean
rm -f makefile.defs
ln -s makefiles/makefile.defs.mingw32 makefile.defs
sed -i s/swprintf/_snwprintf/g */*.c
make
