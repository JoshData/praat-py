// This file is the glue between Praat's script interpreter
// and our script interpreter.

#include <stdio.h>
#include <wchar.h>

#include "../sys/melder.h"
#include "../sys/praatP.h"
#include "../sys/praat_script.h"

#include "util.h"
#include "scripting.h"

/* Interface from Python (C) into Praat (C++). */

static Interpreter current_interpreter = NULL; // global state; bad programming style, yes

extern "C" void scripting_executePraatCommand2(wchar_t *command) {
	praat_executeCommand (current_interpreter, command);
}

extern "C" wchar_t *scripting_executePraatCommand (wchar_t **commandargs, int divert, int *haderror) {
	// This runs a Praat Script command whose command and arguments are stored
	// in the NULL-terminated commandargs array, which *this function* frees
	// (along with its elements) once it's done using it.
	//
	// If divert is true, the output to the info window is diverted
	// and captured, and returned by this function (as a newly-allocated buffer
	// to be freed by the caller). haderror is set on returning to whether an
	// error ocurred, and if so the error is returned as a newly allocated
	// buffer. If no error occurs and divert is false, NULL is returned.
	
	MelderString value = { 0, 0, NULL };
	
	// Format the complete command. Concatenate the command and arguments
	// by quote-escaping any of the arguments except the last one as needed.

	// Compute the size of the command first so that we can allocate the
	// complete buffer.
	int command_size_max = 0;
	wchar_t **arg = commandargs;
	while (*arg) {
		// For each command/argument, we allocate 3 extra characters
		// for a space and surrounding quotes, and double the length
		// of the string in case we have to escape quotes inside it
		// by doubling them.
		command_size_max += wcslen(*arg)*2 + 3;
		arg++;
	}
	
	// Allocate the command buffer, copy the arguments in escaping as
	// needed, and freeing the commandargs array and elements as we go
	// through.
	wchar_t *command = (wchar_t*)calloc(command_size_max, sizeof(wchar_t));
	arg = commandargs;
	while (*arg) {
		// Before all but the first element, concat a space.
		if (arg != commandargs)
			wcscat(command, L" ");
		
		// If this is the first element (the command name) or the last
		// element (the last argument, which Praat never escapes), then
		// don't escape. Otherwise, see if we need to escape.
		int escape = 0;
		if (arg != commandargs && *(arg + 1)) {
			// Contains a space or quote character?
			if (wcsstr(*arg, L" ") || wcsstr(*arg, L"\""))
				escape = 1;
		}
		
		if (!escape) {
			wcscat(command, *arg);
		} else {
			wcscat(command, L"\"");
			int i;
			for (i = 0; i < wcslen(*arg); i++) {
				wcsncat(command, *arg + i, 1); // copy this character over
				if ((*arg)[i] == '"') // escape the quote by doubling it
					wcsncat(command, *arg + i, 1); // copy this character over
			}
			wcscat(command, L"\"");
		}
	
		free(*arg);
		arg++;
	}
	free(commandargs);
	
	if (divert) Melder_divertInfo (&value);
	scripting_executePraatCommand2 (command);
	if (divert) Melder_divertInfo (NULL);
	
	free(command);
	
	if (Melder_hasError()) {
		MelderString_free(&value);
		*haderror = 1;
		wchar_t *ret = wcsdup (Melder_getError ());
		Melder_clearError ();
		return ret;
	}
	
	*haderror = 0;
	
	if (divert) {
		if (value.string) {
			wchar_t *ret = wcsdup (value.string);
			MelderString_free(&value);
			return ret;
		} else {
			wchar_t *ret = (wchar_t*)malloc(sizeof(wchar_t));
			*ret = 0;
			return ret;
		}
	} else {
		return NULL;
	}
}

extern "C" int is_anything_selected() {
	return praat_selection(NULL) != 0;
}

extern "C" wchar_t *get_name_of_selected() {
	return praat_getNameOfSelected(NULL, 0);
}

extern "C" void write_to_info_window(wchar_t *text) {
	Melder_print (text);
}

/* Interface from Praat (C++) into Python (C). */

int scripting_run_praat_script(Interpreter interpreter, wchar_t *script, wchar_t **argv) {
	if (wcsncmp(script, L"#lang=", 6) != 0)
		return 0;

	current_interpreter = interpreter;

	if (wcsncmp(script, L"#lang=python", 12) == 0) {
		scripting_run_python(script, argv);
	} else {
		Melder_print (L"Unrecognized language in #lang= line in script. Use \"#lang=python\".\n");
	}

	current_interpreter = NULL;
	
	return 1;
}

