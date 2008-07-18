// Exerpted and modified from:

/* sendpraat.c */
/* by Paul Boersma */
/* December 24, 2007 */

/*
 * The sendpraat subroutine (Unix with X Window; Windows; Macintosh) sends a message
 * to a running program that uses the Praat shell.
 * The sendpraat program does the same from a Unix command shell,
 * from a Windows or DOS console, or from a MacOS X terminal window.
 *
 * New versions: http://www.praat.org or http://www.fon.hum.uva.nl/praat/sendpraat.html
 *
 * On Windows NT, 2000, and XP, this version works only with Praat version 4.3.28 and higher.
 * On Macintosh, this version works only with Praat version 3.8.75 and higher.
 */

/*******************************************************************

   THIS CODE CAN BE COPIED, USED, AND DISTRIBUTED FREELY.
   IF YOU MODIFY IT, PLEASE KEEP MY NAME AND MARK THE CHANGES.
   IF YOU IMPROVE IT, PLEASE NOTIFY ME AT paul.boersma@uva.nl.

*******************************************************************/

#if defined (_WIN32)
	#include <windows.h>
	#include <stdio.h>
	#include <wchar.h>
	#define xwin 0
	#define win 1
	#define mac 0
#elif defined (macintosh) || defined (__MACH__)
	#include <signal.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <unistd.h>
	#include <ctype.h>
	#include <wchar.h>
	#include <AppleEvents.h>
	#ifdef __MACH__
		#include <AEMach.h>
	#endif
	#include <MacErrors.h>
	#define xwin 0
	#define win 0
	#define mac 1
#else   /* Assume we are on Unix. */
	#include <sys/types.h>
	#include <signal.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <ctype.h>
	#include <wchar.h>
	#include <X11/Intrinsic.h>
	#include <sys/stat.h>
	#define xwin 1
	#define win 0
	#define mac 0
#endif

/*
 * The way to call the sendpraat subroutine from another program.
 */
char *sendpraat (void *display, const char *programName, long timeOut, const char *text);
wchar_t *sendpraatW (void *display, const wchar_t *programName, long timeOut, const wchar_t *text);
/*
 * Parameters:
 * 'display' is the Display pointer, which will be available if you call sendpraat from an X Window program.
 *    If 'display' is NULL, sendpraat will open the display by itself, and close it after use.
 *    On Windows and Macintosh, sendpraat ignores the 'display' parameter.
 * 'programName' is the name of the program that receives the message.
 *    This program must have been built with the Praat shell (the most common such programs are Praat and ALS).
 *    On Unix, the program name is usually all lower case, e.g. "praat" or "als", or the name of any other program.
 *    On Windows, you can use either "Praat", "praat", or the name of any other program.
 *    On Macintosh, 'programName' must be "Praat", "praat", "ALS", or the Macintosh signature of any other program.
 * 'timeOut' is the time (in seconds) after which sendpraat will return with a time-out error message
 *    if the receiving program sends no notification of completion.
 *    On Unix and Macintosh, the message is sent asynchronously if 'timeOut' is 0;
 *    this means that sendpraat will return OK (NULL) without waiting for the receiving program
 *    to handle the message.
 *    On Windows, the time out is ignored.
 * 'text' contains the contents of the Praat script to be sent to the receiving program.
 */

/*
 * To compile sendpraat as a stand-alone program:
 * temporarily change the following line to "#if 1" instead of "#if 0":
 */
#if 1
/*
 * To compile on MacOS X:
cc -o sendpraat -framework CoreServices -framework ApplicationServices -I/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/Headers -I/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/Headers sendpraat.c
*/
int main (int argc, char **argv) {
	int iarg, line, length = 0;
	long timeOut = 10;   /* Default. */
	char programName [64], *message, *result;
	int done;
	struct stat statbuf;
	
	if (argc == 1) {
		printf ("Syntax:\n");
		printf ("   sendpraat [-p <praat>] [-t <timeOut>] <message>\n");
		printf ("\n");
		printf ("Arguments:\n");
		printf ("   <praat>: the name of a running program that uses the Praat shell\n");
		printf ("            (default \"praat\").\n");
		printf ("   <message>: a sequence of Praat shell lines (commands and directives),\n");
		printf ("              or if <message> is an existing file, instructs praat to execute\n");
		printf ("              it as a script.\n");
		#if ! win
			printf ("   <timeOut>: the number of seconds that sendpraat will wait for an answer\n");
			printf ("              before writing an error message. A <timeOut> of 0 means that\n");
			printf ("              the message will be sent asynchronously, i.e., that sendpraat\n");
			printf ("              will return immediately without issuing any error message.\n");
		#else
			printf ("   <timeOut>: not available on Windows\n");
		#endif
		printf ("\n");
		printf ("Usage:\n");
		printf ("   Each line is a separate argument.\n");
		printf ("   Lines that contain spaces should be put inside double quotes.\n");
		printf ("\n");
		printf ("Examples:\n");
		printf ("\n");
		#if win
			printf ("   sendpraat Quit\n");
		#else
			printf ("   sendpraat Quit\n");
		#endif
		printf ("      Causes the program \"praat\" to quit (gracefully).\n");
		printf ("      This works because \"Quit\" is a fixed command in Praat's Control menu.\n");
		#if ! win
			printf ("      Sendpraat will return immediately.\n");
		#endif
		printf ("\n");
		#if win
			printf ("   sendpraat \"Play reverse\"\n");
		#else
			printf ("   sendpraat -p praat -t 1000 \"Play reverse\"\n");
		#endif
		printf ("      Causes the program \"praat\", which can play sounds,\n");
		printf ("      to play the selected Sound objects backwards.\n");
		printf ("      This works because \"Play reverse\" is an action command\n");
		printf ("      that becomes available in Praat's dynamic menu when Sounds are selected.\n");
		#if ! win
			printf ("      Sendpraat will allow \"praat\" at most 1000 seconds to perform this.\n");
		#endif
		printf ("\n");
		#if win
			printf ("   sendpraat \"execute C:\\MyDocuments\\MyScript.praat\"\n");
		#else
			printf ("   sendpraat \"execute ~/MyResearch/MyProject/MyScript.praat\"\n");
		#endif
		printf ("   or\n");
		#if win
			printf ("   sendpraat \"C:\\MyDocuments\\MyScript.praat\"\n");
		#else
			printf ("   sendpraat \"~/MyResearch/MyProject/MyScript.praat\"\n");
		#endif
		printf ("      Causes the program \"praat\" to execute a script.\n");
		#if ! win
			printf ("      Sendpraat will allow \"praat\" at most 10 seconds (the default time out).\n");
		#endif
		printf ("\n");
		printf ("   sendpraat -p als \"for i from 1 to 5\" \"Draw circle... 0.5 0.5 0.1*i\" \"endfor\"\n");
		printf ("      Causes the program \"als\" to draw five concentric circles\n");
		printf ("      into its Picture window.\n");
		exit (0);
	}
	iarg = 1;
	
	strcpy(programName, "praat");
	
	while (iarg < argc-1 && argv[iarg][0] == '-') {
		done = 0;
		switch (argv[iarg][1]) {
		case 'p':
			strcpy (programName, argv [iarg+1]);
			iarg += 2;
			break;
		case 't':
			if (isdigit (argv [iarg+1] [0]))
				timeOut = atol (argv [iarg+1]);
			iarg += 2;
			break;
		case '-': // double hyphens, don't process any more arguments
			iarg++;
			done = 1;
			break;
		default:
			iarg++;
			break;
		}
		
		if (done) break;
	}

	/*
	 * Create the message string.
	 */
	for (line = iarg; line < argc; line ++) length += strlen (argv [line]) + 1;
	length --;
	if (stat(argv[iarg], &statbuf) == 0) length += 8;
	message = malloc (length + 1);
	message [0] = '\0';
	if (stat(argv[iarg], &statbuf) == 0) strcat(message, "execute ");
	for (line = iarg; line < argc; line ++) {
		strcat (message, argv [line]);
		if (line < argc - 1) strcat (message, "\n");
	}

	/*
	 * Send message.
	 */
	result = sendpraat (NULL, programName, timeOut, message);
	if (result != NULL)
		{ fprintf (stderr, "sendpraat: %s\n", result); exit (1); }

	exit (0);
	return 0;
}
#endif

/* End of file sendpraat.c */
