#pragma once

#include <string>
#include <vector>
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
namespace Console
{
	void Log(const char* fmt, ...);
	void Log(const std::string& line);
	void Clear();
	std::vector<std::string> GetLines();
}
