#pragma once

#include <Windows.h>
#include <cstddef>
#include <vector>


namespace mem
{
	class Memory;
}

namespace Scanner
{
	void SetMemoryInstance(mem::Memory* memory);
	size_t FindLibUnity();
}
