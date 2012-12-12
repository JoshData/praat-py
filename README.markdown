Praat-Py: Extended Praat for Python Scripting

By Joshua Tauberer <http://razor.occams.info>

Originally developed in 2009. Only vaguely being currently maintained.

## Introduction

Praat-Py is a custom build of [Praat](http://www.fon.hum.uva.nl/praat/), the
computer program used by linguists for doing phonetic analysis on sound files,
to allow for scripts to be written in the [Python](http://www.python.org)
programming language, rather than in Praat's built-in language.

The motivation for this is to remove the need for Praat script writers to
learn an entirely new programming language if they already know Python.
Additionally, Python is a more complete programming language and so scripts of
any complexity should be more easily written in Python. The goal isn't to make
all scripts easier or shorter to write, but to make it possible to do some
complex tasks more easily.

All of the functionality of Praat's scripting language is retained when using
Python, except forms. Likewise, because the Python interpreter is used, all
Python functionality is possible, such as using all Python modules that you
happen to have installed.

Praat-Py is a replacement for the Praat executable file you download from
the Praat website. Because this project is no longer well maintained, there
is no ready-to-go replacement file available. Praat-Py must be compiled from
source files. Instructions for compiling Praat-Py are given later.

This project is released under the same terms as Praat itself, the GPL v2 or
later.

## Example

The following is an example script for Praat written in Python, and after
is the corresponding script in Praat's script language. The script converts all
.aif files in a directory into .wav format, resamping them in the process. The
Praat script on the right is based on the script
[here](http://www.helsinki.fi/~lennes/praat-
scripts/public/convert_aiff_files_to_16kHz_wav.praat) by Mietta Lennes.

Praat script using Praat-Py: 

    
    #lang=python
    from glob import glob
    
    aiffdir = "/home/tauberer/"
    delete_AIFF_files_after_conversion = False
    
    for file in glob(aiffdir + "*.aiff") :
       print "Converting file", file, "..."
    	
       aiff = go("Read from file...", file)
       resampled = go("Resample...", 16000, 50)
       soundtype, soundname = aiff
    	
       go("Write to WAV file...", aiffdir + soundname + ".wav")
       if delete_AIFF_files_after_conversion :
          go("filedelete", file)
    		
       remove(aiff)
       remove(resampled)
    
The original Praat script, which can be run either in Praat or
in Praat-Py.

      
    Create Strings as file list... aifffiles 'aiffdir$'*.aif
    numberOfFiles = Get number of strings
    
    for file to numberOfFiles
       select Strings aifffiles
       file$ = Get string... file
       Read from file... 'aiffdir$''file$'
       soundname$ = selected$ ("Sound", 1)
       Rename... temp
       printline Converting file 'file$'...
       Resample... 16000 50
       Rename... 'soundname$'
       select Sound temp
       Remove
       select Sound 'soundname$'
       Write to WAV file... 'aiffdir$''soundname$'.wav
       if delete_AIFF_files_after_conversion = 1
          filedelete 'aiffdir$''file$'
       endif
       Remove
    endfor

## Using Python in Praat-Py

### Starting a Praat-Py Script

Once you have Praat-Py, you can write scripts in either Praat's built-in
scripting language or in Python. You should be roughly familiar with Praat
scripting before reading on (see [this
page](http://www.fon.hum.uva.nl/praat/manual/Scripting.html)), as well as
[Python](http://www.python.org/). To script with Python, all you have to do is
begin your script code with this:


    #lang=python

That is, the very first line of your script must be a hash (#) followed
immediately by "lang=python" in lowercase letters. This instructs Praat to
interpret the remainder of your script as Python code. Without this special
line, the script is run as a normal Praat script.

### "Print" and "Go" Functions

There are several ways your Python script can make Praat do things. The first
way is via the `print` Python command, which will output a string to the Praat
Info window:


    #lang=python
    print 11500 * 20

The next way is to have Praat do one of its commands on a menu or the main
window buttons. You do this with the `go(_command_, _arguments..._)` function.

    
    #lang=python
    go("To Spectrogram...", .005, maxfreq, .002, 20, "Gaussian")
    go("View")

The first argument to the `go` function is the name of a command on a Praat
menu to run. The remaning arguments are any options that that command takes,
in the order that they show up in Praat. You'll notice that as compared to
Praat Script, they way you put the arguments together is significantly more
straight-forward. To use variables, for instance, you just use them (no
strange quotes). In Praat Script, you never quote the final argument. Here,
you quote all arguments uniformly: strings are quoted, and nothing else, as in
Python normally.

`go(...)` returns the selected object after the command completes, or None if
nothing is selected. This is particularly useful for commands that create new
objects in the Praat list. See `selected()` below. Here's an example:

    
    #lang=python
    wav = go("Read from file...", "myfile.wav")
    spec = go("To Spectrogram...", .005, maxfreq, .002, 20, "Gaussian")
    select(wav)
    part = go("Extract part...", 0.0, 1.0, True)
    remove(wav)
    remove(spec)
    remove(part)

The third way is to have Praat do something but to capture the result of what
it does. You can capture numeric results with `getNum(_command_,
_arguments..._)`:

    
    #lang=python
    print "The end time is: ", getNum("Get end time")*1000, " (ms)"

As in regular Praat scripts, this works by capturing the first number the
command would normally print to Praat's Info window. As with `go`, you can
provide any number of arguments to `getNum`. `getNum` returns a Python float
value.

There is also a function `getString` which works like `getNum` but it returns
the entire output line sent to the Info window by the command as a Python
string value.

    
    #lang=python
    print "The end time is: ", getString("Get end time")

### Selection Functions

Some additional commands are provided to make it easier to work with Praat's
selected objects. `select(_object name1_, [_object name2_, ...])` selects one
or more objects by their names. It's just a convenience; you can also use
`go("select", _objectname_)`. The argument(s) to select can be either Python
strings that have the name of the object, or a tuple of the object type and
name separately: `select( (_type1_, _name1_) [, ...])` . Here are some
examples:

    
    #lang=python
    select("Sound mysound") # selects just a sound
    select("Sound mysound", "TextGrid mysound") # selects these two together (and unselects everything else)
    select( ("Sound", "mysound") ) # same as above, might be useful in some cases, note the double parens!
    select( ("Sound", "mysound"), ("TextGrid", "mysound") ) # same as above

To add or remove objects from the selection, use `plus` and `minus` as in
Praat scripting. They work just the same as `select`:

    
    #lang=python
    select("Sound mysound") # selects just a sound
    plus("TextGrid mysound") # selects additionally the TextGrid
    minus("Sound mysound", "TextGrid mysound") # deselects them
    minus( ("Sound", "mysound"), ("TextGrid", "mysound") ) # deselects them using the tuple form

`selected()` is a function that returns the selected object. It returns a
_tuple_ containing the type and name of the object separately. If nothing is
currently selected, None is returned.

    
    #lang=python
    select("Sound mysound")
    (type, name) = selected() # returns ('Sound', 'MySound')
    
    s = selected()
    print s # prints "('Sound', 'MySound')"
    print s[0] # prints the type of the object, "Sound"
    print s[1] # prints the name of the object, "MySound"
    minus(s) # deselects the currently selected object


### Other Shortcuts

Because removing objects from the Praat objects list is such a common task in
scripts that batch-process files, there is a shortcut for selecting and then
removing objects using `remove(object)`:

    
    #lang=python
    wav = go("Read from file...", "myfile.wav")
    ...
    remove(wav)
       or
    remove('Sound myfile')

### Running the Script from the Command Line

As with Praat Scripts normally, you can run a script from the command-line
directly (and never see Praat itself), besides running it from a script
window. Say "myscript.praatpy" is a Praat-Py script, i.e. a Python program
beginning with "#lang=python". You can then run it as:

    
    praat-py myscript.praatpy

You can also pass command-line parameters to the script:

    
    praat-py myscript.praatpy arg1 arg2

...and access them with `argv` similar to the usual Python sys.argv list. The
first element in this list is the name of the script itself.

    
    #lang=python
    print argv     # this prints [u'myscript.praatpy', u'arg1', u'arg2']  (they're Unicode strings, assumed UTF-8 on command line)

## Building Praat-Py from Sources

You can build Praat-Py on any Unix platform... at least in principle. I build
it on Ubuntu. YMMV elsewhere. I've also been able to compile it with mingw32
(running on Linux) so that it can be run on Windows.

1) You will need to have Python 2.5 or 2.6 installed and the "python-dev"
package (or equivalent) for your distribution, or an install from sources.

2) Get the [Praat
sources](http://www.fon.hum.uva.nl/praat/download_sources.html) and follow the
instructions to build Praat on your platform. (You don't really need to build
Praat now, but it would be a really good idea to make sure it builds and runs
before going forward.)

For instance, on Linux, with the latest version of Praat at the time of
writing:

    
    wget http://www.fon.hum.uva.nl/praat/praat5332_sources.tar.gz
    tar -zxf praat5332_sources.tar.gz
    cd sources_5332
    ln -s makefiles/makefile.defs.linux ./makefile.defs
    make
    ./praat

3) Clone this git repository into a directory called `scripting` inside the
Praat sources directory:

    # assuming you are still in the sources directory
    git clone git://github.com/tauberer/praat-py.git scripting

4) The Praat sources must be patched to build Praat-Py and to invoke the
Python interpreter instead of the Praat script interpreter when the script
begins with the magic string "#lang=python". Do this to patch your Praat
source files:
    
    # assuming you are still in the sources directory
    patch -p1 < scripting/praat-py.patch
    

5) Build Praat (now Praat-Py).

    
    make
    ./praat-py # it runs!
    

The `praat-py` executable in the current directory will now support Python
scripts.


