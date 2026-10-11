#include "memory.h"
#include "MinHook.h"
#include <thread>
#include <chrono>
#include <string>
#include <intrin.h>
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

namespace {
const char* vmm_dll_name()
{
	static const std::string s(obfuscatebyte("BstkVMM.dll"));
	return s.c_str();
}
const char* exp_phys_read()
{
	static const std::string s(obfuscatebyte("PGMPhysRead"));
	return s.c_str();
}
const char* exp_phys_write()
{
	static const std::string s(obfuscatebyte("PGMPhysSimpleWriteGCPhys"));
	return s.c_str();
}
const char* exp_gcptr2gcp()
{
	static const std::string s(obfuscatebyte("PGMPhysGCPtr2GCPhys"));
	return s.c_str();
}
const char* exp_get_cpu()
{
	static const std::string s(obfuscatebyte("VMMGetCpuById"));
	return s.c_str();
}
} // namespace

namespace mem
{
	void* Memory::s_vm = nullptr;
	volatile long Memory::s_captured = 0;
	volatile long Memory::s_lock = 0;

	PGMPhysRead_t Memory::s_target_read = nullptr;
	PGMPhysSimpleWriteGCPhys_t Memory::s_target_write = nullptr;
	PGMPhysGCPtr2GCPhys_t Memory::s_target_convert = nullptr;
	VMMGetCpuById_t Memory::s_target_get_cpu = nullptr;

	PGMPhysRead_t Memory::s_orig_read = nullptr;
	PGMPhysSimpleWriteGCPhys_t Memory::s_orig_write = nullptr;
	std::unordered_map<size_t, Memory::PageEntry> Memory::s_pageCache;
	std::shared_mutex Memory::s_cacheMutex;
	uint64_t Memory::s_cacheGcMs = 0;
	std::array<void*, Memory::kMaxCpuContexts> Memory::s_cpuArr{};
	std::atomic<int> Memory::s_cpuCount{ 0 };
	std::mutex Memory::s_cpuMutex;
	bool Memory::s_amd_slow_pause = false;

	namespace {
		// On AMD Zen the PAUSE instruction takes ~140 cycles (vs ~10 on Intel and
		// pre-Zen AMD), which makes naive spin-wait loops disastrous. Detect AMD
		// once and skip PAUSE there.
		bool detect_amd_slow_pause()
		{
			int regs[4] = { 0, 0, 0, 0 };
			__cpuid(regs, 0);
			char vendor[13] = {};
			memcpy(vendor + 0, &regs[1], 4);
			memcpy(vendor + 4, &regs[3], 4);
			memcpy(vendor + 8, &regs[2], 4);
			vendor[12] = '\0';
			return _stricmp(vendor, "AuthenticAMD") == 0;
		}

		inline void cpu_spin_relax(bool amd_slow_pause)
		{
			if (amd_slow_pause) {
				// Cheap NOP avoids the multi-hundred-cycle PAUSE penalty on Zen
				// while still preventing the compiler from collapsing the loop.
				_mm_lfence();
			} else {
				YieldProcessor();
			}
		}
	}

	Memory::Memory() : m_init(false), m_module(nullptr)
	{
	}

	Memory::~Memory()
	{
		Dispose();
	}

	bool Memory::IsReady() const
	{
		return InterlockedCompareExchange((volatile long*)&s_captured, 0, 0) == 1;
	}

	bool Memory::Init()
	{
		if (m_init)
			return true;

		// One-time CPU vendor check — controls spin-wait strategy globally.
		static bool s_vendor_detected = false;
		if (!s_vendor_detected) {
			s_amd_slow_pause = detect_amd_slow_pause();
			s_vendor_detected = true;
		}

		MH_STATUS status = MH_Initialize();
		if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
			return false;

		if (!LoadTargets())
			return false;

		if (!CreateHooks())
			return false;

		m_init = true;
		return true;
	}

	bool Memory::LoadTargets()
	{
		m_module = LoadLibraryA(vmm_dll_name());
		if (!m_module)
			return false;

		void* ptr_read;
		if (!GetFunction(m_module, exp_phys_read(), &ptr_read))
			return false;
		s_target_read = (PGMPhysRead_t)ptr_read;

		void* ptr_write;
		if (!GetFunction(m_module, exp_phys_write(), &ptr_write))
			return false;
		s_target_write = (PGMPhysSimpleWriteGCPhys_t)ptr_write;

		void* ptr_conv;
		if (!GetFunction(m_module, exp_gcptr2gcp(), &ptr_conv))
			return false;
		s_target_convert = (PGMPhysGCPtr2GCPhys_t)ptr_conv;

		void* ptr_cpu;
		if (!GetFunction(m_module, exp_get_cpu(), &ptr_cpu))
			return false;
		s_target_get_cpu = (VMMGetCpuById_t)ptr_cpu;

		return true;
	}

	bool Memory::GetFunction(HMODULE mod, const char* name, void** ptr)
	{
		if (!mod || !name || !ptr)
			return false;
		FARPROC addr = GetProcAddress(mod, name);
		if (addr)
		{
			*ptr = (void*)addr;
			return true;
		}
		return false;
	}

	bool Memory::CreateHooks()
	{
		void* org_read;
		if (MH_CreateHook(s_target_read, DetourRead, &org_read) != MH_OK)
			return false;
		s_orig_read = (PGMPhysRead_t)org_read;

		void* org_write;
		if (MH_CreateHook(s_target_write, DetourWrite, &org_write) != MH_OK)
			return false;
		s_orig_write = (PGMPhysSimpleWriteGCPhys_t)org_write;

		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
			return false;
		return true;
	}

	int __cdecl Memory::DetourRead(void* vm, size_t addr, void* buf, size_t size)
	{
		Capture(vm);
		return s_orig_read(vm, addr, buf, size);
	}

	int __cdecl Memory::DetourWrite(void* vm, size_t addr, void* buf, size_t size)
	{
		Capture(vm);
		return s_orig_write(vm, addr, buf, size);
	}

	void Memory::Capture(void* vm)
	{
		if (InterlockedCompareExchange((volatile long*)&s_captured, 0, 0) == 1)
			return;
		const bool amd = s_amd_slow_pause;
		int spin_count = 0;
		while (InterlockedCompareExchange(&s_lock, 1, 0) != 0)
		{
			// Short spin (cheap NOP on AMD, PAUSE on Intel), then yield to the OS.
			if (spin_count < (amd ? 8 : 64))
				cpu_spin_relax(amd);
			else
				Sleep(0);
			++spin_count;
		}
		if (s_captured == 0)
		{
			s_vm = vm;
			{
				std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
				s_pageCache.clear();
			}
			{
				std::lock_guard<std::mutex> lock(s_cpuMutex);
				s_cpuArr.fill(nullptr);
				s_cpuCount.store(0, std::memory_order_release);
			}
			InterlockedExchange(&s_captured, 1);
		}
		InterlockedExchange(&s_lock, 0);
	}

	void Memory::RefreshCpuContexts()
	{
		// Fast lock-free check: already populated.
		if (s_cpuCount.load(std::memory_order_acquire) > 0)
			return;
		if (!s_vm || !s_target_get_cpu)
			return;

		std::lock_guard<std::mutex> lock(s_cpuMutex);
		if (s_cpuCount.load(std::memory_order_acquire) > 0)
			return;

		// Parse every reachable CPU id via VMMGetCpuById. BstkVMM exposes one
		// context per virtual CPU; pull them all out so address translation can
		// use the CPU that actually owns the active CR3.
		std::array<void*, kMaxCpuContexts> tmp{};
		int found = 0;
		int trailing_misses = 0;
		for (int i = 0; i < kMaxCpuContexts; ++i)
		{
			void* cpu = s_target_get_cpu(s_vm, i);
			if (cpu)
			{
				tmp[found++] = cpu;
				trailing_misses = 0;
			}
			else if (found > 0)
			{
				++trailing_misses;
				if (trailing_misses >= 4)
					break;
			}
		}

		if (found == 0)
		{
			for (int i = 0; i < 4; ++i)
			{
				void* cpu = s_target_get_cpu(s_vm, i);
				if (cpu) tmp[found++] = cpu;
			}
		}

		// Publish atomically (count last) so other threads see a consistent view.
		for (int i = 0; i < found; ++i)
			s_cpuArr[i] = tmp[i];
		s_cpuCount.store(found, std::memory_order_release);
	}

	bool Memory::WaitCapture(int timeout)
	{
		auto start = std::chrono::steady_clock::now();
		while (!IsReady())
		{
			auto elapsed = std::chrono::steady_clock::now() - start;
			if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout)
				return false;
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return true;
	}

	bool Memory::ToPhys(size_t virt, size_t* phys)
	{
		if (!phys || !s_vm)
			return false;
		*phys = 0;

		RefreshCpuContexts();
		const int cpu_count = s_cpuCount.load(std::memory_order_acquire);
		for (int i = 0; i < cpu_count; ++i)
		{
			void* cpu = s_cpuArr[i];
			if (!cpu) continue;
			size_t tmp = 0;
			if (s_target_convert(cpu, virt, &tmp) == 0)
			{
				*phys = tmp;
				return true;
			}
		}
		return false;
	}

	bool Memory::Translate(size_t virtAddr, size_t* physAddr)
	{
		if (!physAddr || !s_vm || virtAddr == 0)
			return false;
		*physAddr = 0;
		size_t vpn = virtAddr >> 12;
		size_t offset = virtAddr & 0xFFF;

		const uint64_t now_ms = (uint64_t)GetTickCount64();

		// Fast path: shared (reader) lock — many threads can hit the cache in
		// parallel without contention. With dense ESP scans the vast majority of
		// reads land on already-translated pages, so this removes the previous
		// mutex bottleneck. Expired entries fall through and get re-translated so
		// a page the guest has since remapped can never keep serving stale bytes.
		{
			std::shared_lock<std::shared_mutex> lock(s_cacheMutex);
			auto it = s_pageCache.find(vpn);
			if (it != s_pageCache.end() && (now_ms - it->second.stamp_ms) < kPageCacheTtlMs)
			{
				*physAddr = it->second.phys_base + offset;
				return true;
			}
		}

		RefreshCpuContexts();
		const int cpu_count = s_cpuCount.load(std::memory_order_acquire);
		if (cpu_count == 0)
			return false;

		for (int retry = 0; retry < 2; retry++)
		{
			for (int i = 0; i < cpu_count; ++i)
			{
				void* cpu = s_cpuArr[i];
				if (!cpu) continue;
				size_t tempPhys = 0;
				if (s_target_convert(cpu, virtAddr, &tempPhys) == 0 && tempPhys != 0)
				{
					size_t physPageBase = tempPhys - offset;
					{
						std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
						s_pageCache[vpn] = PageEntry{ physPageBase, now_ms };
						// Entries now expire instead of being overwritten forever, so
						// drop the dead ones occasionally to keep the map bounded.
						if (now_ms - s_cacheGcMs > 5000ULL)
						{
							s_cacheGcMs = now_ms;
							for (auto it = s_pageCache.begin(); it != s_pageCache.end();)
							{
								if (now_ms - it->second.stamp_ms > (kPageCacheTtlMs * 4))
									it = s_pageCache.erase(it);
								else
									++it;
							}
						}
					}
					*physAddr = tempPhys;
					return true;
				}
			}
			if (retry == 0)
				std::this_thread::yield();
		}
		return false;
	}

	bool Memory::Read(size_t addr, void* buf, size_t size)
	{
		if (!buf || size == 0 || !IsReady() || addr < 0x10000)
			return false;
		unsigned char* currentBuf = (unsigned char*)buf;
		size_t currentAddr = addr;
		size_t bytesRemaining = size;
		while (bytesRemaining > 0)
		{
			size_t bytesUntilPageBoundary = 0x1000 - (currentAddr & 0xFFF);
			size_t bytesToRead = (bytesRemaining < bytesUntilPageBoundary) ? bytesRemaining : bytesUntilPageBoundary;
			size_t physAddr = 0;
			if (!Translate(currentAddr, &physAddr))
			{
				{
					std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
					s_pageCache.erase(currentAddr >> 12);
				}
				if (!Translate(currentAddr, &physAddr))
					return false;
			}
			if (s_orig_read(s_vm, physAddr, currentBuf, bytesToRead) != 0)
			{
				{
					std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
					s_pageCache.erase(currentAddr >> 12);
				}
				return false;
			}
			bytesRemaining -= bytesToRead;
			currentAddr += bytesToRead;
			currentBuf += bytesToRead;
		}
		return true;
	}

	bool Memory::Write(size_t addr, const void* data, size_t size)
	{
		if (!data || size == 0 || !IsReady() || addr < 0x10000)
			return false;
		const unsigned char* currentData = (const unsigned char*)data;
		size_t currentAddr = addr;
		size_t bytesRemaining = size;
		while (bytesRemaining > 0)
		{
			size_t bytesUntilPageBoundary = 0x1000 - (currentAddr & 0xFFF);
			size_t bytesToWrite = (bytesRemaining < bytesUntilPageBoundary) ? bytesRemaining : bytesUntilPageBoundary;
			size_t physAddr = 0;
			if (!Translate(currentAddr, &physAddr))
			{
				{
					std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
					s_pageCache.erase(currentAddr >> 12);
				}
				if (!Translate(currentAddr, &physAddr))
					return false;
			}
			if (s_orig_write(s_vm, physAddr, (void*)currentData, bytesToWrite) != 0)
			{
				{
					std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
					s_pageCache.erase(currentAddr >> 12);
				}
				return false;
			}
			bytesRemaining -= bytesToWrite;
			currentAddr += bytesToWrite;
			currentData += bytesToWrite;
		}
		return true;
	}

	void Memory::ClearCache()
	{
		std::unique_lock<std::shared_mutex> lock(s_cacheMutex);
		s_pageCache.clear();
	}

	void Memory::Dispose()
	{
		if (!m_init)
			return;
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
		{
			std::lock_guard<std::mutex> lock(s_cpuMutex);
			s_cpuArr.fill(nullptr);
			s_cpuCount.store(0, std::memory_order_release);
		}
		ClearCache();
		s_vm = nullptr;
		s_target_read = nullptr;
		s_target_write = nullptr;
		s_target_convert = nullptr;
		s_target_get_cpu = nullptr;
		s_orig_read = nullptr;
		s_orig_write = nullptr;
		InterlockedExchange(&s_captured, 0);
		InterlockedExchange(&s_lock, 0);
		if (m_module)
		{
			FreeLibrary(m_module);
			m_module = nullptr;
		}
		m_init = false;
	}
}
