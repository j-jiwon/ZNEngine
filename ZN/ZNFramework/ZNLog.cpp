#include "ZNLog.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

using namespace ZNFramework;

ZNLog& ZNLog::Get()
{
	static ZNLog instance;
	return instance;
}

ZNLog::ZNLog()
{
	for (bool& on : channelOn)
		on = true;
}

void ZNLog::SetChannelEnabled(LogChannel c, bool e)
{
	const int i = static_cast<int>(c);
	if (i >= 0 && i < ChannelCount)
		channelOn[i] = e;
}

bool ZNLog::IsChannelEnabled(LogChannel c) const
{
	const int i = static_cast<int>(c);
	return i >= 0 && i < ChannelCount && channelOn[i];
}

const char* ZNLog::ChannelName(LogChannel c)
{
	switch (c)
	{
	case LogChannel::General: return "General";
	case LogChannel::Input:   return "Input";
	case LogChannel::Render:  return "Render";
	case LogChannel::Scene:   return "Scene";
	case LogChannel::Asset:   return "Asset";
	default:                  return "?";
	}
}

const char* ZNLog::LevelName(LogLevel l)
{
	switch (l)
	{
	case LogLevel::Trace: return "TRACE";
	case LogLevel::Info:  return "INFO";
	case LogLevel::Warn:  return "WARN";
	case LogLevel::Error: return "ERROR";
	default:              return "?";
	}
}

void ZNLog::Write(LogChannel c, LogLevel l, const char* fmt, ...)
{
	// Filter first so a disabled channel / below-threshold level costs almost nothing.
	if (static_cast<int>(l) < static_cast<int>(minLevel) || !IsChannelEnabled(c))
		return;

	char msg[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	// Unified single-line format for the debugger + console sinks.
	char line[1200];
	snprintf(line, sizeof(line), "[%llu][%s][%s] %s\n",
		static_cast<unsigned long long>(frame), ChannelName(c), LevelName(l), msg);

#ifdef _WIN32
	// Colour the console output by level (Warn = yellow, Error = red) via the Win32 console API.
	// Falls back to plain text when stdout isn't a real console (e.g. redirected to a file).
	HANDLE hOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	const bool console = (hOut && hOut != INVALID_HANDLE_VALUE &&
	                      ::GetConsoleScreenBufferInfo(hOut, &csbi));
	if (console)
	{
		WORD attr = csbi.wAttributes;
		switch (l)
		{
		case LogLevel::Warn:  attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break; // yellow
		case LogLevel::Error: attr = FOREGROUND_RED | FOREGROUND_INTENSITY;                    break; // red
		default: break; // Info/Trace keep the console's default colour
		}
		::SetConsoleTextAttribute(hOut, attr);
	}
#endif

	// Flush all the way to the OS (cout -> stdio -> handle) so (a) the coloured text is on screen
	// before we restore the attribute below, and (b) logs survive a crash.
	std::cout << line;
	std::cout.flush();
	fflush(stdout);

#ifdef _WIN32
	if (console)
		::SetConsoleTextAttribute(hOut, csbi.wAttributes); // restore previous colour
	::OutputDebugStringA(line);
#endif

	entries.push_back({ frame, c, l, msg });
	if (entries.size() > maxEntries)
		entries.erase(entries.begin(), entries.begin() + (entries.size() - maxEntries));
}
