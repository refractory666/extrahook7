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
	std::vector<DWORD_PTR> Aobscan(int processId, const std::vector<BYTE>& findSignature);
	void ReplaceSignature(DWORD processId, DWORD_PTR address, const std::vector<BYTE>& newSignature);
}
