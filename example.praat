#python
# Make a temporary selection from the original sound:
cursor = praat.getNum("Get cursor")
start = cursor - 0.02
end = cursor + 0.02
praat.go("Select...", start, end)

# name the new Sound object according to the time point
# where the cursor was
milliseconds = int(cursor * 1000)
objName = "FFT_" + str(milliseconds) + "ms"

praat.go("Extract windowed selection...", objName, "Kaiser2", "2", "no")

# leave the Sound editor for a while to calculate and draw
# the spectrum
praat.go("endeditor")

# Make the Spectrum object from the new Sound
praat.go("To Spectrum")
praat.go("Edit")
praat.go("editor", "Spectrum " + objName)
# zoom the spectrum to a comfortable frequency view...
praat.go("Zoom...", 0, 5000)
praat.go("endeditor")

# select and remove the temporary Sound object 
praat.select("Sound " + objName)
praat.go("Remove")

# return to the Sound editor window and recall the original
# cursor position
praat.go("editor")
praat.go("Move cursor to...", cursor)