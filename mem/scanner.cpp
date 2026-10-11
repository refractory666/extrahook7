#include "scanner.h"
#include "memory.h"
#include "obf_strings.h"
#include <future>
#include <mutex>
#include <cstring>
#include <string>
#include <cstdint>

namespace Scanner
{
	const char* libunity_so_name()
	{
		static const std::string s(obfuscatebyte("libunity.so"));
		return s.c_str();
	}
	const char* libunity_short_name()
	{
		static const std::string s(obfuscatebyte("libunity"));
		return s.c_str();
	}

	static mem::Memory* s_memory = nullptr;
	static std::mutex s_scanMutex;

	static std::vector<DWORD_PTR> FindSignatureAddressesInRegion(HANDLE processHandle, const std::vector<BYTE>& findSignature, MEMORY_BASIC_INFORMATION memoryInfo)
	{
		std::vector<DWORD_PTR> foundAddresses;
		if (memoryInfo.State != MEM_COMMIT) return foundAddresses;
		if (memoryInfo.Protect != PAGE_READWRITE && memoryInfo.Protect != PAGE_EXECUTE_READWRITE) return foundAddresses;

		std::vector<BYTE> buffer(memoryInfo.RegionSize);
		SIZE_T bytesRead = 0;
		if (!ReadProcessMemory(processHandle, memoryInfo.BaseAddress, buffer.data(), memoryInfo.RegionSize, &bytesRead))
			return foundAddresses;
		if (bytesRead < findSignature.size())
			return foundAddresses;

		for (SIZE_T i = 0; i <= bytesRead - findSignature.size(); ++i)
		{
			bool found = true;
			for (size_t j = 0; j < findSignature.size(); ++j)
			{
				if (findSignature[j] != 0xAA && buffer[i + j] != findSignature[j]) {
					found = false;
					break;
				}
			}
			if (found) {
				std::lock_guard<std::mutex> guard(s_scanMutex);
				foundAddresses.push_back(reinterpret_cast<DWORD_PTR>(memoryInfo.BaseAddress) + i);
			}
		}

		return foundAddresses;
	}

	void SetMemoryInstance(mem::Memory* memory)
	{
		s_memory = memory;
	}

	static bool ReadSpan(uintptr_t addr, void* out, size_t size)
	{
		if (!s_memory || !s_memory->IsReady())
			return false;
		return s_memory->Read((size_t)addr, out, size);
	}

	template<typename T>
	static bool ReadValue(uintptr_t addr, T& out)
	{
		return ReadSpan(addr, &out, sizeof(T));
	}

	static bool EqAsciiIgnoreCase(const char* a, const char* b)
	{
		if (!a || !b) return false;
		while (*a && *b) {
			unsigned ca = (unsigned char)*a++;
			unsigned cb = (unsigned char)*b++;
			if (ca >= 'A' && ca <= 'Z') ca += ('a' - 'A');
			if (cb >= 'A' && cb <= 'Z') cb += ('a' - 'A');
			if (ca != cb) return false;
		}
		return *a == 0 && *b == 0;
	}

	static bool ContainsAsciiIgnoreCaseRaw(const char* hay, size_t hayLen, const char* needle)
	{
		if (!hay || !needle || !needle[0] || hayLen == 0) return false;
		const size_t nlen = std::strlen(needle);
		if (nlen > hayLen) return false;
		for (size_t i = 0; i + nlen <= hayLen; ++i) {
			bool ok = true;
			for (size_t j = 0; j < nlen; ++j) {
				unsigned char a = (unsigned char)hay[i + j];
				unsigned char b = (unsigned char)needle[j];
				if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
				if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
				if (a != b) { ok = false; break; }
			}
			if (ok) return true;
		}
		return false;
	}

#pragma pack(push, 1)
	struct Elf32Ehdr {
		unsigned char e_ident[16];
		uint16_t e_type;
		uint16_t e_machine;
		uint32_t e_version;
		uint32_t e_entry;
		uint32_t e_phoff;
		uint32_t e_shoff;
		uint32_t e_flags;
		uint16_t e_ehsize;
		uint16_t e_phentsize;
		uint16_t e_phnum;
		uint16_t e_shentsize;
		uint16_t e_shnum;
		uint16_t e_shstrndx;
	};
	struct Elf32Phdr {
		uint32_t p_type;
		uint32_t p_offset;
		uint32_t p_vaddr;
		uint32_t p_paddr;
		uint32_t p_filesz;
		uint32_t p_memsz;
		uint32_t p_flags;
		uint32_t p_align;
	};
	struct Elf32Dyn {
		int32_t d_tag;
		uint32_t d_val;
	};
	struct Elf64Ehdr {
		unsigned char e_ident[16];
		uint16_t e_type;
		uint16_t e_machine;
		uint32_t e_version;
		uint64_t e_entry;
		uint64_t e_phoff;
		uint64_t e_shoff;
		uint32_t e_flags;
		uint16_t e_ehsize;
		uint16_t e_phentsize;
		uint16_t e_phnum;
		uint16_t e_shentsize;
		uint16_t e_shnum;
		uint16_t e_shstrndx;
	};
	struct Elf64Phdr {
		uint32_t p_type;
		uint32_t p_flags;
		uint64_t p_offset;
		uint64_t p_vaddr;
		uint64_t p_paddr;
		uint64_t p_filesz;
		uint64_t p_memsz;
		uint64_t p_align;
	};
	struct Elf64Dyn {
		int64_t d_tag;
		uint64_t d_val;
	};
#pragma pack(pop)

	static bool IsLibUnityElf(uintptr_t base)
	{
		constexpr uint8_t kElfClass32 = 1;
		constexpr uint8_t kElfClass64 = 2;
		constexpr uint32_t kPtDynamic = 2;
		constexpr int64_t kDtNull = 0;
		constexpr int64_t kDtStrTab = 5;
		constexpr int64_t kDtStrSz = 10;
		constexpr int64_t kDtSoName = 14;
		constexpr int kMaxPh = 128;
		constexpr int kMaxDyn = 1024;
		constexpr size_t kMaxStr = 0x8000;

		unsigned char ident[16]{};
		if (!ReadSpan(base, ident, sizeof(ident))) return false;
		if (!(ident[0] == 0x7F && ident[1] == 'E' && ident[2] == 'L' && ident[3] == 'F')) return false;
		uint8_t cls = ident[4];

		if (cls == kElfClass32) {
			Elf32Ehdr eh{};
			if (!ReadValue(base, eh)) return false;
			if (eh.e_phoff == 0 || eh.e_phentsize < sizeof(Elf32Phdr) || eh.e_phnum == 0 || eh.e_phnum > kMaxPh) return false;

			uint32_t dynVaddr = 0, dynSize = 0;
			for (uint16_t i = 0; i < eh.e_phnum; ++i) {
				Elf32Phdr ph{};
				uintptr_t phAddr = base + (uintptr_t)eh.e_phoff + (uintptr_t)i * eh.e_phentsize;
				if (!ReadValue(phAddr, ph)) return false;
				if (ph.p_type == kPtDynamic) { dynVaddr = ph.p_vaddr; dynSize = ph.p_memsz; break; }
			}
			if (dynVaddr == 0 || dynSize == 0) return false;

			uint32_t strTab = 0, strSz = 0, soName = 0;
			int calc = (int)(dynSize / (uint32_t)sizeof(Elf32Dyn));
			int dynLimit = (calc < kMaxDyn) ? calc : kMaxDyn;
			for (int i = 0; i < dynLimit; ++i) {
				Elf32Dyn d{};
				if (!ReadValue(base + (uintptr_t)dynVaddr + (uintptr_t)i * sizeof(Elf32Dyn), d)) return false;
				if (d.d_tag == kDtNull) break;
				if (d.d_tag == kDtStrTab) strTab = d.d_val;
				else if (d.d_tag == kDtStrSz) strSz = d.d_val;
				else if (d.d_tag == kDtSoName) soName = d.d_val;
			}
			if (!strTab || !strSz || strSz > kMaxStr) return false;
			std::vector<char> strtab((size_t)strSz + 1, 0);
			if (!ReadSpan(base + (uintptr_t)strTab, strtab.data(), strSz)) return false;

			if (soName < strSz && EqAsciiIgnoreCase(strtab.data() + soName, libunity_so_name())) return true;
			return ContainsAsciiIgnoreCaseRaw(strtab.data(), (size_t)strSz, libunity_short_name());
		}

		if (cls == kElfClass64) {
			Elf64Ehdr eh{};
			if (!ReadValue(base, eh)) return false;
			if (eh.e_phoff == 0 || eh.e_phentsize < sizeof(Elf64Phdr) || eh.e_phnum == 0 || eh.e_phnum > kMaxPh) return false;

			uint64_t dynVaddr = 0, dynSize = 0;
			for (uint16_t i = 0; i < eh.e_phnum; ++i) {
				Elf64Phdr ph{};
				uintptr_t phAddr = base + (uintptr_t)eh.e_phoff + (uintptr_t)i * eh.e_phentsize;
				if (!ReadValue(phAddr, ph)) return false;
				if (ph.p_type == kPtDynamic) { dynVaddr = ph.p_vaddr; dynSize = ph.p_memsz; break; }
			}
			if (dynVaddr == 0 || dynSize == 0) return false;

			uint64_t strTab = 0, strSz = 0, soName = 0;
			int calc = (int)(dynSize / (uint64_t)sizeof(Elf64Dyn));
			int dynLimit = (calc < kMaxDyn) ? calc : kMaxDyn;
			for (int i = 0; i < dynLimit; ++i) {
				Elf64Dyn d{};
				if (!ReadValue(base + (uintptr_t)dynVaddr + (uintptr_t)i * sizeof(Elf64Dyn), d)) return false;
				if (d.d_tag == kDtNull) break;
				if (d.d_tag == kDtStrTab) strTab = d.d_val;
				else if (d.d_tag == kDtStrSz) strSz = d.d_val;
				else if (d.d_tag == kDtSoName) soName = d.d_val;
			}
			if (!strTab || !strSz || strSz > kMaxStr) return false;
			std::vector<char> strtab((size_t)strSz + 1, 0);
			if (!ReadSpan(base + (uintptr_t)strTab, strtab.data(), (size_t)strSz)) return false;

			if (soName < strSz && EqAsciiIgnoreCase(strtab.data() + soName, libunity_so_name())) return true;
			return ContainsAsciiIgnoreCaseRaw(strtab.data(), (size_t)strSz, libunity_short_name());
		}

		return false;
	}

	size_t FindLibUnity()
	{
		if (!s_memory || !s_memory->IsReady())
			return 0;

		const uintptr_t kStep = 0x1000;
		struct ScanRange { uintptr_t start; uintptr_t end; };
		const ScanRange ranges[] = {
			{ (uintptr_t)0xF0000000, (uintptr_t)0x30000000 },
			{ (uintptr_t)0x0000007F00000000ull, (uintptr_t)0x0000007000000000ull }
		};

		DWORD started = GetTickCount();
		const DWORD timeoutMs = 5000;
		for (const auto& r : ranges) {
			for (uintptr_t addr = r.start;; addr -= kStep) {
				if (GetTickCount() - started > timeoutMs)
					return 0;
				uint32_t magic = 0;
				if (!ReadValue(addr, magic) || magic != 0x464C457F) {
					if (addr <= r.end || addr < kStep) break;
					continue;
				}
				if (IsLibUnityElf(addr))
					return (size_t)addr;
				if (addr <= r.end || addr < kStep) break;
			}
		}
		return 0;
	}

	std::vector<DWORD_PTR> Aobscan(int processId, const std::vector<BYTE>& findSignature)
	{
		std::vector<DWORD_PTR> foundAddresses;
		HANDLE processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
		if (!processHandle) return foundAddresses;

		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		DWORD_PTR currentAddress = reinterpret_cast<DWORD_PTR>(systemInfo.lpMinimumApplicationAddress);
		DWORD_PTR maxAddress = reinterpret_cast<DWORD_PTR>(systemInfo.lpMaximumApplicationAddress);
		MEMORY_BASIC_INFORMATION memoryInfo;
		std::vector<std::future<std::vector<DWORD_PTR>>> futures;

		while (currentAddress < maxAddress)
		{
			if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(currentAddress), &memoryInfo, sizeof(memoryInfo)))
				futures.push_back(std::async(std::launch::async, FindSignatureAddressesInRegion, processHandle, std::cref(findSignature), memoryInfo));
			currentAddress += memoryInfo.RegionSize;
		}

		for (auto& f : futures)
		{
			auto part = f.get();
			foundAddresses.insert(foundAddresses.end(), part.begin(), part.end());
		}

		CloseHandle(processHandle);
		return foundAddresses;
	}

	void ReplaceSignature(DWORD processId, DWORD_PTR address, const std::vector<BYTE>& newSignature)
	{
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
		if (!processHandle)
			return;

		WriteProcessMemory(processHandle, reinterpret_cast<LPVOID>(address), newSignature.data(), static_cast<SIZE_T>(newSignature.size()), nullptr);
		CloseHandle(processHandle);
	}
}
