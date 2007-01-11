// This file is the C# entry point for executing scripts.
// It is called from mono_embed.c.

using System;
using System.Runtime.InteropServices;

#if JAVASCRIPT
using org.mozilla.javascript;
#endif

#if PYTHON
using IronPython.Hosting;
#endif

public class PraatMonoEmbed {
	// Call-backs into the C program.  These functions are defined in mono_embed.c
	// and provide scripts with access to Praat functionality.

	[DllImport ("__Internal", EntryPoint="mono_embed_echo")]
	public static extern void Echo (string message);

	[DllImport ("__Internal", EntryPoint="mono_embed_praat_executeCommand")]
	public static extern string Exec (string command, int divert, out int hadError);
	
	[DllImport ("__Internal")]
	public static extern IntPtr praat_getNameOfSelected (IntPtr klass, int inplace);

	// The main entry point.
	
	public static void RunScript(string script) {
		if (script.StartsWith("#python")) {
			#if PYTHON
				RunScript_Python(script);
			#else
				Echo("This version of PraatJS was not compiled for Python scripting.\n");
			#endif
			return;
		}
		if (script.StartsWith("//javascript")) {
			#if JAVASCRIPT
				RunScript_JavaScript(script);
			#else
				Echo("This version of PraatJS was not compiled for JavaScript scripting.\n");
			#endif
			return;
		}
		
		// We should never get here.  C checks that the beginning of the script looks like one of these.
		Echo("Bad script.\n");
	}
	
	#if PYTHON
	public static void RunScript_Python(string script) {
		PythonEngine engine = new PythonEngine();
		
		engine.SetStandardOutput(new PraatInfoWindowStream());
		
		engine.Globals["console"] = new ConsoleWrapper();
		engine.Globals["praat"] = new Praat();
		
		try {
			engine.Execute(script);
		} catch (Exception e) {
			Echo(e.Message + "\n");
		}
	}
	#endif
	
	#if JAVASCRIPT
	public static void RunScript_JavaScript(string script) {
		Context cx = Context.enter();
		try {
			Scriptable scope = cx.initStandardObjects();
			
			Object wrappedOut = Context.javaToJS(new ConsoleWrapper(), scope);
			Object wrappedPraat = Context.javaToJS(new Praat(), scope);
			
			ScriptableObject.putProperty(scope, "console", wrappedOut);
			ScriptableObject.putProperty(scope, "praat", wrappedPraat);
			
			cx.evaluateString(scope, script, "<script>", 1, null); 
		} catch (RhinoException exception) {
			//Echo(exception.Message == "" ? exception.details() : exception.Message + " at line " + exception.lineNumber() + ":" + exception.columnNumber());
			Echo(exception.details() + " at line " + exception.lineNumber() + ":" + exception.columnNumber() + "\n");
		} finally {
			Context.exit();
			System.GC.Collect();
		}
	}
	#endif
	
	class Praat : PraatTypes.PraatStaticMethods {
		public void print(string message) {
			PraatMonoEmbed.Echo(message + "\n");
		}
		
		private string make_command(string command, object[] args) {
			string ret = command;
			for (int i = 0; i < args.Length; i++) {
				ret += " ";
				string a = args[i].ToString();
				if (i != args.Length-1 && (a.IndexOf(' ') == -1 || (args[i] is double && a.IndexOf('.') == -1))) {
					// need to quote the argument, but we never quote the last one (as per Praat docs)
					ret += "\"" + a.Replace("\"", "\"\"") + "\"";
				} else {
					ret += a;
				}
			}
			return ret;
		}
		
		private string go_internal(string command, object[] args, bool captureOutput) {
			int hadError;
			string ret = PraatMonoEmbed.Exec(make_command(command, args), captureOutput ? 1 : 0, out hadError);
			if (hadError != 0)
				throw new Exception("Error: " + ret);
			return ret;
		}

		public void go(string command, params object[] args) {
			go_internal(command, args, false);
		}

		public string getString(string command, params object[] args) {
			return go_internal(command, args, true);
		}
		
		public double getNum(string command, params object[] args) {
			string result = go_internal(command, args, true);
			return double.Parse(result);
		}
		
		public void select(string objectName) {
			go("select", objectName);
		}

		public string selected() {
			IntPtr ret = PraatMonoEmbed.praat_getNameOfSelected(IntPtr.Zero, 0);
			return Marshal.PtrToStringAnsi(ret);
		}
	}
	
	class ConsoleWrapper {
		public void print(object obj) {
			Console.Write(obj);
		}
		public void println(object obj) {
			Console.WriteLine(obj);
		}
	}
	
	class PraatInfoWindowStream : System.IO.Stream {
		System.Text.StringBuilder buffer = new System.Text.StringBuilder();
		
		public override bool CanRead { get { return false; } }
		public override bool CanSeek { get { return false; } }
		public override bool CanWrite { get { return true; } }

		public override long Length { get { throw new InvalidOperationException(); } }
		public override long Position { get { throw new InvalidOperationException(); } set { throw new InvalidOperationException(); }  }
		
		public override void Flush() {
		}
		
		public override int Read(byte[] data, int offset, int length) {
			throw new InvalidOperationException();
		}

		public override void Write(byte[] data, int offset, int length) {
			buffer.Append(System.Text.Encoding.UTF8.GetString(data, offset, length));
			for (int i = 0; i < buffer.Length; i++) {
				if (buffer[i] == '\n') {
					PraatMonoEmbed.Echo(buffer.ToString(0, i+1));
					buffer.Remove(0, i+1);
					i = -1;
				}
			}
		}

		public override long Seek(long position, System.IO.SeekOrigin o) {
			throw new InvalidOperationException();
		}
		
		public override void SetLength(long length) {
			throw new InvalidOperationException();
		}
	}
}
