// This is included in Interpreter.h and the .c files here.

int scripting_run_praat_script(void *interpreter, wchar_t *script, wchar_t **argv);
void scripting_run_python(wchar_t *script, wchar_t **argv);
void scripting_executePraatCommand2 (wchar_t *command);
wchar_t *scripting_executePraatCommand (wchar_t **commandargs, int divert, int *haderror);
char *wc2c(wchar_t *wc, int doFree);
