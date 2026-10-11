#pragma once

#include <cstdint>

#include "brk-1/mutation_t2.h"

#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif

/** RVA / оффсеты: литерал проходит через obfuscatebyte (см. mutation_t2.h). */
#define OBF_UPTR(v) (static_cast<uintptr_t>(obfuscatebyte((unsigned long long)(v))))

/** Win32 VK, битовые маски, маленькие целые коды. */
#define OBF_I32(v) (static_cast<int>(obfuscatebyte((int)(v))))

#define OBF_U32(v) (static_cast<uint32_t>(obfuscatebyte((unsigned int)(v))))

#define OBF_U8(v) (static_cast<unsigned char>(obfuscatebyte((unsigned char)(v))))
