#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h> 
#include <tchar.h>


#include <chrono>
#include <thread>

#include <string>
#include <shellapi.h> 

#pragma comment (lib, "Shell32.lib")


// Remove after testing
#include <iostream>
#include <fstream>


using namespace std;

#define SLEEP_TIME 50 // reactivity: sleep time to wait for subprocess to output some more data

// #define UNICODE

CONST COORD origin = { 0, 0 };

HANDLE winMasterJob;

bool inline initializeWinFork() {
	winMasterJob = CreateJobObject( NULL, NULL );
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobELInfo = { 0 };
	jobELInfo.BasicLimitInformation.LimitFlags =
		//JOB_OBJECT_LIMIT_BREAKAWAY_OK |
		//JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK |
		JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION |
		JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if ( !SetInformationJobObject( winMasterJob, JobObjectExtendedLimitInformation, &jobELInfo, sizeof( jobELInfo ) ) ) {
		return false;
	}
	if ( !AssignProcessToJobObject( winMasterJob, GetCurrentProcess() ) ) {
		DWORD errCode = GetLastError();
		if ( errCode == ERROR_ACCESS_DENIED ) {
			return true;
		}
		SetLastError( errCode );
		return false;
	}
	return true;
}


int main( int argc, char* argv[] )
{
	// It's possible the first will error, but we don't really care.



	/*ofstream logFile;
	logFile.open("C:\\Users\\Public\\Documents\\StrataMiner\\logs\\stuffer-log.txt");
	logFile << "logger started" << endl;
	logFile.flush();*/
	initializeWinFork();


	FreeConsole();
	AllocConsole();



	//PROCESS_INFORMATION cmdPi; ZeroMemory( &cmdPi, sizeof( PROCESS_INFORMATION ) );
	//STARTUPINFO cmdSi;			ZeroMemory( &cmdSi, sizeof( STARTUPINFO ) );

	//cmdSi.cb = sizeof( STARTUPINFO );
	//cmdSi.dwFlags = STARTF_FORCEOFFFEEDBACK;

	//string cmdCmd = "cmd";

	//if ( ! CreateProcess( NULL, (LPSTR)cmdCmd.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &cmdSi, &cmdPi ) ) {
	//	logFile << "--Failed to create cmd process. Error code " << to_string( GetLastError() ) << endl;
	//	return 0;
	//}

	//// Process id at cmdPi.dwProcessId

	//AttachConsole(cmdPi.dwProcessId);

	// get pipe/console to output to
	HANDLE hOutput = GetStdHandle( STD_OUTPUT_HANDLE );

	if ( argc < 3 ) {
		return 0;
	}
	string pipeName = "\\\\.\\pipe\\" + string( argv[1] );

	string commandLine = "";
	for ( int i=2; i < argc; i++ ) {
		commandLine += string( argv[i] ) + " ";
	}
	

	HANDLE namedPipe = CreateFile( &pipeName[0],
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL 
	);

	if ( namedPipe == INVALID_HANDLE_VALUE ) {
		return 0;
	}

	// prepare the console window & inherited screen buffer
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof( SECURITY_ATTRIBUTES );
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	/*HANDLE hConsole = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa,
		CONSOLE_TEXTMODE_BUFFER, NULL );*/

	HANDLE hConsole;
	if ( (hConsole = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, CONSOLE_TEXTMODE_BUFFER, NULL )) == INVALID_HANDLE_VALUE ) {
		return 0;
	}

	DWORD dwDummy;
	FillConsoleOutputCharacterA( hConsole, '\0', MAXLONG, origin, &dwDummy ); // fill screen buffer with zeroes
	//  SetStdHandle( STD_OUTPUT_HANDLE, hConsole ); // to be inherited by child process

												 // start the subprocess
	PROCESS_INFORMATION pi; ZeroMemory( &pi, sizeof( PROCESS_INFORMATION ) );
	STARTUPINFO si;			ZeroMemory( &si, sizeof( STARTUPINFO ) );

	si.cb = sizeof( STARTUPINFO );
	si.dwFlags = STARTF_FORCEOFFFEEDBACK | STARTF_USESHOWWINDOW;
	si.dwFlags = STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = hConsole;
	si.hStdError = hConsole;

	if ( SetConsoleActiveScreenBuffer( hConsole ) == 0 ) {
		CloseHandle( hConsole );
		return 1;
	}

	// all other default options are already good : we want subprocess to share the same console and to inherit our STD handles
	if ( !CreateProcess( NULL, (LPSTR)commandLine.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi ) )
	{
		CloseHandle( hConsole );
		return -3;
	}
	COORD lastpos = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	bool exitNow = false;

	do
	{
		if( WaitForSingleObject( pi.hProcess, 0 ) != WAIT_TIMEOUT )
			exitNow = true; // exit after this last iteration

		if( GetNamedPipeInfo( namedPipe, 0, 0, 0, 0 ) == 0 )
		{
			//logFile << "Failure bro: " << GetLastError() << " - " << test << endl;
			exitNow = true; // exit after this last iteration
		}

							// get screen buffer state
		GetConsoleScreenBufferInfo( hConsole, &csbi );
		int lineWidth = csbi.dwSize.X;

		if ( (csbi.dwCursorPosition.X == lastpos.X) && (csbi.dwCursorPosition.Y == lastpos.Y) )
		{
			Sleep( SLEEP_TIME ); // text cursor did not move, sleep a while
		}
		else
		{
			DWORD count = (csbi.dwCursorPosition.Y - lastpos.Y) * lineWidth + csbi.dwCursorPosition.X - lastpos.X;
			// read newly output characters starting from last cursor position

			LPBYTE buffer = new BYTE[count];

			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

			ReadConsoleOutputCharacterA( hConsole, (LPSTR)buffer, count, lastpos, &count );

			// fill screen buffer with zeroes
			FillConsoleOutputCharacterA( hConsole, '\0', count, lastpos, &dwDummy );
			lastpos = csbi.dwCursorPosition;
			GetConsoleScreenBufferInfo( hConsole, &csbi );
			if ( (csbi.dwCursorPosition.X == lastpos.X) && (csbi.dwCursorPosition.Y == lastpos.Y) )
			{ // text cursor did not move since this treatment, hurry to reset it to home
				SetConsoleCursorPosition( hConsole, origin );
				lastpos = origin;
			}

			SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

			LPSTR scan = (LPSTR)buffer;

			// scan screen buffer and transmit character to real output handle
			do
			{
				if ( *scan )
				{
					DWORD len = 1;
					while ( scan[len] && (len < count) )
						len++;

					/*logFile << scan << endl;*/
					WriteFile( namedPipe, scan, len, &dwDummy, NULL );
					scan += len;
					count -= len;
				}
				else
				{
					DWORD len = 1;
					while ( !scan[len] && (len < count) )
						len++;
					scan += len;
					count -= len;
					len = (len + lineWidth - 1) / lineWidth;
					for ( ; len; len-- )
						/*logFile << endl;*/
						WriteFile( namedPipe, "\r\n" , 2 * sizeof( TCHAR ), &dwDummy, NULL );
				}
			} while ( count );

			delete[] buffer;
		}
		// loop until end of subprocess
	} while ( !exitNow && namedPipe != INVALID_HANDLE_VALUE );

	// release subprocess handle

	CloseHandle( hConsole );
	CloseHandle( namedPipe );
	TerminateProcess( pi.hProcess, 0 );
	DWORD exitCode;
	if ( !GetExitCodeProcess( pi.hProcess, &exitCode ) ) {
		exitCode = -3;
	}
	return exitCode;
}
