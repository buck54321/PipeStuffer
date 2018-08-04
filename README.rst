===========
PipeStuffer
===========

PipeStuffer is a real-time i/o redirection wrapper for console programs. It uses named pipes for interprocess communication. PipeStuffer is based on `RTConsole <https://www.codeproject.com/Articles/16163/Real-Time-Console-Output-Redirection>`_, but is designed to work around some quirky behavior that exist with consoles in Windows 10. To use, simply put the compiled executable in a known place. If you wanted the real-time output from a program called `target.exe`, instead of calling :code:`CreateProcess(NULL, "path/to/target.exe --opt1 arg 1", ...)`, you would create a named pipe, and pass the name to PipeStuffer like 

::

	CreateProcess("NULL, /path/to/pipe-stuffer.exe [PIPE NAME] path/to/target.exe --opt1 arg1", NULL, NULL, FALSE, ...)

where :code:`[PIPE NAME]` is only the trailing part of the named pipe name, i.e. :code:`//./pipe/[PIPE NAME]`. You should set :code:`bInheritHandles` argument of :code:`CreateProcess` to :code:`FALSE`. 



PipeStuffer uses a console screen buffer for communication with the target program so that output is not buffered. Any Windows console inherited by PipeStuffer is immediately dropped with :code:`FreeConsole()`, and output from the target program will come through a named pipe. This prevents spurious behavior associated with Windows 10 implementation of consoles. 

Note: When running your parent program in a Windows Command Prompt, sometimes you can correct the issues PipeStuffer solves by setting the Windows Command Prompt settings to use legacy mode. Sometimes you can't. 



