#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace ZNFramework
{
	enum class LogLevel   : int { Trace = 0, Info, Warn, Error };
	enum class LogChannel : int { General = 0, Input, Render, Scene, Asset, Count };

	struct LogEntry
	{
		uint64_t    frame;
		LogChannel  channel;
		LogLevel    level;
		std::string text;   // message only (the [frame][chan][lvl] prefix is added on output)
	};

	// Minimal synchronous logging facade: per-channel on/off + a level threshold, a unified
	// "[frame][CHANNEL][LEVEL] message" format, routed to the debugger (OutputDebugString), the
	// console (stdout) and an in-memory ring buffer that the ImGui Log panel reads.
	// Intentionally no file/async logging — out of scope (see zn_refactoring_plan R6).
	class ZNLog
	{
	public:
		static ZNLog& Get();

		void     SetFrame(uint64_t f)          { frame = f; }
		void     SetMinLevel(LogLevel l)       { minLevel = l; }
		LogLevel GetMinLevel() const           { return minLevel; }
		void     SetChannelEnabled(LogChannel c, bool e);
		bool     IsChannelEnabled(LogChannel c) const;

		// printf-style. Early-outs (no format, no store) when the channel is off or level < min.
		void Write(LogChannel c, LogLevel l, const char* fmt, ...);

		const std::vector<LogEntry>& Entries() const { return entries; }
		void Clear()                           { entries.clear(); }

		static const char* ChannelName(LogChannel c);
		static const char* LevelName(LogLevel l);
		static constexpr int ChannelCount = static_cast<int>(LogChannel::Count);

	private:
		ZNLog();

		uint64_t frame    = 0;
		LogLevel minLevel = LogLevel::Info;   // Trace hidden by default (keeps key-input etc. quiet)
		bool     channelOn[ChannelCount];
		std::vector<LogEntry> entries;
		size_t   maxEntries = 2000;           // ring buffer cap
	};
}

// Convenience macros. `chan` is a ZNFramework::LogChannel, then printf-style args:
//   ZNLOG_INFO(LogChannel::Scene, "loaded %d meshes", n);
#define ZNLOG(chan, lvl, ...)  ::ZNFramework::ZNLog::Get().Write((chan), (lvl), __VA_ARGS__)
#define ZNLOG_TRACE(chan, ...) ZNLOG((chan), ::ZNFramework::LogLevel::Trace, __VA_ARGS__)
#define ZNLOG_INFO(chan, ...)  ZNLOG((chan), ::ZNFramework::LogLevel::Info,  __VA_ARGS__)
#define ZNLOG_WARN(chan, ...)  ZNLOG((chan), ::ZNFramework::LogLevel::Warn,  __VA_ARGS__)
#define ZNLOG_ERROR(chan, ...) ZNLOG((chan), ::ZNFramework::LogLevel::Error, __VA_ARGS__)
