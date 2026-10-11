#include "console.h"
#include <cstdarg>
#include <cstdio>
#include <mutex>

#include "../brk-1/cryptmemory_inc.h"
#include "../brk-1/generateaddress.h"
#include "../brk-1/mutation.h"
#include "../brk-2/entrypoint.h"
#include "../brk-2/debugger.h"
#include "../brk-2/safe_section.h"
#include "../brk-2/antiattach.h"
#include "../brk-2/break_entrypoint.h"
#include "../brk-2/unaccessable_point.h"
#include "../brk-1/mutation_t2.h"
#include "../brk-2/anti_debug.h"
#include "../brk-2/hidden_fastfail.h"
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

namespace Console
{
	static std::vector<std::string> s_lines;
	static std::mutex s_mutex;
	static const size_t s_maxLines = 512;

	void Log(const char* fmt, ...)
	{
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
		std::lock_guard<std::mutex> lock(s_mutex);
		s_lines.push_back(buf);
		if (s_lines.size() > s_maxLines)
			s_lines.erase(s_lines.begin());
	}

	void Log(const std::string& line)
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		s_lines.push_back(line);
		if (s_lines.size() > s_maxLines)
			s_lines.erase(s_lines.begin());
	}

	void Clear()
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		s_lines.clear();
	}

	std::vector<std::string> GetLines()
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		return s_lines;
	}
}
