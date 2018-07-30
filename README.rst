===========
PipeStuffer
===========

PipeStuffer is a real-time console redirection program that uses named pipes for interprocess communication. PipeStuffer is based on `RTConsole <https://www.codeproject.com/Articles/16163/Real-Time-Console-Output-Redirection>`_, but is designed to work around some quirky behavior that exist with consoles in Windows 10. To use, simply put the compiled executable in a known place. If you wanted the real-time output from a program called `target.exe`, instead of calling :code:`CreateProcess(NULL, "path/to/target.exe --opt1 arg 1", ...)`, you would call create a named pipe, and pass the name to PipeStuffer like 

::

	CreateProcess("NULL, /path/to/pipe-stuffer.exe [PIPE NAME] path/to/target.exe --opt1 arg1", NULL, NULL, FALSE, ...)

And then handle communication the via the named pipe. 

Because PipeStuffer uses a console screen buffer for communication with the target program, output is not buffered. Because any console inherited from the parent is immediately dropped with :code:`FreeConsole()`, buggy behavior associated with Windows 10 consoles is avoided. 



