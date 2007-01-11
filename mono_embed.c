// This file is the glue between Praat's script interpreter
// and the script interpreters in the embedded Mono runtime.

#include <stdio.h>
#include <mono/jit/jit.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/assembly.h>

#include "../sys/melder.h"
int praat_executeCommand (void *me, const char *command);

MonoDomain *domain = NULL;

static void *current_interpreter = NULL; // global state; bad programming style, yes

static char *assembly_name = "PraatMonoEmbed.dll";
static char *driver_name = "PraatMonoEmbed:RunScript(string)";

static void mono_embed_main_thread_handler (gpointer user_data) {
	if (!user_data) {
		fprintf(stderr, "Script is NULL.\n");
		return;
	}

	MonoAssembly *assembly;
	
	assembly = mono_domain_assembly_open (domain, assembly_name);
	if (!assembly) {
		fprintf(stderr, "Could not find PraatMonoEmbed.dll.\n");
		return;
	}
	
	MonoMethodDesc* driver_desc = mono_method_desc_new (driver_name, 0);
	MonoMethod* driver = mono_method_desc_search_in_image (driver_desc, mono_assembly_get_image(assembly));
	mono_method_desc_free(driver_desc);
	if (!driver) {
		fprintf(stderr, "Could not find %s method.\n", driver_name);
		return;
	}
	
	void* params[] = { mono_string_new(domain, user_data) };
	MonoObject* ret = mono_runtime_invoke(driver, NULL, params, NULL);
}

static void mono_embed_init() {
	if (domain) return;
	
	mono_config_parse (NULL);
	mono_set_defaults(0, mono_parse_default_optimizations(NULL));

	mono_mkbundle_init();

	domain = mono_jit_init_version ("main-domain", "v2.0.50727");
}

static void mono_embed_shutdown() {
	mono_jit_cleanup (domain);
	domain = NULL;
}

static void mono_embed_run_script(char *script) {
	void *user_data = script;
	mono_runtime_exec_managed_code (domain, mono_embed_main_thread_handler, user_data);
}

void mono_embed_run_praat_script(void *interpreter, char *script) {
	current_interpreter = interpreter;
	mono_embed_init();
	mono_embed_run_script(script);
}

// This is called from the C# code to execute a command.
char *mono_embed_praat_executeCommand (const char *command, int divert, int *haderror) {
	
	MelderStringA value = { 0, 0, NULL };

	if (divert) Melder_divertInfo (&value);
	praat_executeCommand (current_interpreter, command);
	if (divert) Melder_divertInfo (NULL);
	
	if (Melder_hasError()) {
		MelderStringA_free(&value);
		*haderror = 1;
		char *ret = g_strdup (Melder_getError ());
		Melder_clearError ();
		return ret;
	}
	
	*haderror = 0;
	
	if (divert) {
		char *ret = g_strdup (value.string);
		MelderStringA_free(&value);
		return ret;
	} else {
		return NULL;
	}
}

// This is a simple wrapper around Melder_print which prints a line to the Info window.
void mono_embed_echo(const char *message) {
	Melder_print (message);
}


