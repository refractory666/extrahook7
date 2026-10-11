#pragma once

#include <Windows.h>
#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <array>
#include <vector>


namespace mem
{
	// Function pointer types for BstkVMM.dll
	typedef int(__cdecl* PGMPhysRead_t)(void* vm, size_t addr, void* buf, size_t size);
	typedef int(__cdecl* PGMPhysSimpleWriteGCPhys_t)(void* vm, size_t addr, void* buf, size_t size);
	typedef int(__cdecl* PGMPhysGCPtr2GCPhys_t)(void* cpu, size_t virt, size_t* phys);
	typedef void*(__cdecl* VMMGetCpuById_t)(void* vm, int id);

	class Memory
	{
	private:
		static void* s_vm;
		static volatile long s_captured;
		static volatile long s_lock;
		bool m_init;

		static PGMPhysRead_t s_target_read;
		static PGMPhysSimpleWriteGCPhys_t s_target_write;
		static PGMPhysGCPtr2GCPhys_t s_target_convert;
		static VMMGetCpuById_t s_target_get_cpu;

		static PGMPhysRead_t s_orig_read;
		static PGMPhysSimpleWriteGCPhys_t s_orig_write;

		HMODULE m_module;

		// Page translation cache (VPN -> {physical page base, insert tick}).
		// Read-heavy: most requests are cache hits, so we use a shared_mutex to let
		// many threads look up in parallel and only block on inserts.
		//
		// Entries EXPIRE. The guest can remap a guest-virtual page onto a different
		// guest-physical page (GC compaction, page reuse, paging). A stale entry
		// still translates and still reads successfully — it just returns the old
		// page's contents, which is exactly what made ESP freeze in place until a
		// manual Reload. The TTL bounds how long that can last.
		struct PageEntry
		{
			size_t phys_base;
			uint64_t stamp_ms;
		};
		static constexpr uint64_t kPageCacheTtlMs = 250;
		static std::unordered_map<size_t, PageEntry> s_pageCache;
		static std::shared_mutex s_cacheMutex;
		static uint64_t s_cacheGcMs;

		// CPU contexts captured from VMMGetCpuById. After initial enumeration
		// they are stable, so we publish a snapshot in a lock-free array and
		// readers can iterate without taking a mutex.
		static constexpr int kMaxCpuContexts = 32;
		static std::array<void*, kMaxCpuContexts> s_cpuArr;
		static std::atomic<int> s_cpuCount;
		static std::mutex s_cpuMutex;

		// True on AMD CPUs where the PAUSE instruction has very high latency
		// (≥ Zen). Spin loops avoid YieldProcessor() in that case.
		static bool s_amd_slow_pause;

		static int __cdecl DetourRead(void* vm, size_t addr, void* buf, size_t size);
		static int __cdecl DetourWrite(void* vm, size_t addr, void* buf, size_t size);
		static void Capture(void* vm);
		static void RefreshCpuContexts();

		bool LoadTargets();
		bool GetFunction(HMODULE mod, const char* name, void** ptr);
		bool CreateHooks();
		bool ToPhys(size_t virt, size_t* phys);
		bool Translate(size_t virtAddr, size_t* physAddr);

	public:
		Memory();
		~Memory();

		bool IsReady() const;
		bool Init();
		bool WaitCapture(int timeout = 3000);
		bool Read(size_t addr, void* buf, size_t size);
		bool Write(size_t addr, const void* data, size_t size);
		void Dispose();
		void ClearCache();
	};
}
