#include "DebugTools.h"
#include <cstdarg>
#include <iostream>
#include <cstdio>
#include <deque>
#include "Helpers/RenderUtils.h"
#include "Rendering/RenderingManager.h"

namespace {
	const size_t MaxBufferSize{ 256 };
	std::deque<std::string> debugLogQueue{};
	bool enable{ false };
}

namespace Debug {
	std::stringstream stream{};

	void EnableLogs(bool _enable)
	{
		enable = _enable;
	}

	bool IsLogsEnabled()
	{
		return enable;
	}

	void CDebugLog(const char* fmt, ...) {
		char* buffer = new char[MaxBufferSize];
		va_list args;
		va_start(args, fmt);
		vsnprintf_s(buffer, MaxBufferSize, MaxBufferSize, fmt, args);
#ifdef _DEBUG
		vprintf(fmt, args);
#endif

		if (debugLogQueue.size() >= MaxFrameLogs) {
			debugLogQueue.pop_front(); //Pop oldest
		}
		debugLogQueue.push_back(std::string{ buffer });
		delete[] buffer;
	}

	std::string GetDebugLogs(unsigned num)
	{
		unsigned i{};
		std::string str;
		for (std::string const& s : debugLogQueue) {
			str += s + "\n";
			++i;
			if (i >= num) break;
		}
		return str;
	}
	void PrintLogs()
	{
		std::cout << stream.str();
		//This frame, there are new logs
		if (!stream.str().empty()) {
			if (debugLogQueue.size() >= MaxFrameLogs) {
				debugLogQueue.pop_front(); //Pop oldest
			}
			debugLogQueue.push_back(stream.str());
			stream.str(std::string{});
		}

		if (!enable) {
			return;
		}

		DrawAETextbox(RenderingManager::GetInstance()->GetFont(),
			GetDebugLogs(), {AEGfxGetWinMaxX() - 5, AEGfxGetWinMaxY() - 5}, AEGfxGetWinMaxX() * 0.75f, 0.25f, 0.01f, {0,250,250,255},
			TEXT_LOWER_RIGHT, TEXTBOX_ORIGIN_POS::TOP);
	}
}
