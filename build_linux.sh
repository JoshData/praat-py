cd ..
rm -f praat praat-py
make clean
rm -f makefile.defs
ln -s makefiles/makefile.defs.linux.dynamic makefile.defs
sed -i s/_snwprintf/swprintf/g */*.c
make
