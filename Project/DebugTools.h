#ifndef DEBUGTOOLS_H_
#define DEBUGTOOLS_H_

#include <string>
#include <sstream>

namespace Debug {
	//Max frame logs (All the logs from the same frame counts as 1 entry)
	//So this is like "Max no. of prev frames who can display their logs"
	const unsigned MaxFrameLogs{ 5 };

	void EnableLogs(bool enable = true);
	bool IsLogsEnabled();

	//C-style debug log. Prints to std::cout and to Debug::stream
	void CDebugLog(const char* fmt, ...);
	extern std::stringstream stream;

	std::string GetDebugLogs(unsigned num = MaxFrameLogs);

	//Handles debug logs.
	//1. Prints to std::cout
	//2. Saves the Debug::stream logs to the queue and cleans it for next frame.
	//3. (If enabled) Renders the Debug::stream logs on-screen
	void PrintLogs();
}

#endif // DEBUGTOOLS_H_

