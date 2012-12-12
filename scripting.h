// This is included in Interpreter.cpp and scripting.cpp here.

int scripting_run_praat_script(Interpreter interpreter, wchar_t *script, wchar_t **argv);
extern "C" void scripting_run_python(wchar_t *script, wchar_t **argv);
extern "C" void scripting_executePraatCommand2 (wchar_t *command);
extern "C" wchar_t *scripting_executePraatCommand (wchar_t **commandargs, int divert, int *haderror);
extern "C" int is_anything_selected();
extern "C" wchar_t *get_name_of_selected();
extern "C" void write_to_info_window(wchar_t *text);
