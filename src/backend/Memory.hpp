#pragma once

#include "Simd.hpp"

#include <memory>
#include <cstddef>

// modify it as you wish
#define _ENABLE_PREFETCH

static _FORCEINLINE void prefetch(const void* addr) {
#ifdef _ENABLE_PREFETCH
#if defined(_MSC_VER) or defined(_INTEL_COMPILER)
	_mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T2);
#else
	__builtin_prefetch(addr, 1, 2);
#endif
#endif // _ENABLE_PREFETCH
}

_INLINE void* memCopy(void* dst, const void* src, size_t cnt) {
	std::byte* d = reinterpret_cast<std::byte*>(dst);
	const std::byte* s = reinterpret_cast<const std::byte*>(src);
	std::copy_n(s, cnt, d);
	return dst;
}

_INLINE void memSet(void* dst, uint8_t ch, size_t cnt) {
    std::byte* d = reinterpret_cast<std::byte*>(dst);
	std::fill(d, d + cnt, std::byte(ch));
}

_INLINE void* alignedMemset(void* dst, uint8_t ch, size_t cnt) {
	std::byte* d = reinterpret_cast<std::byte*>(dst);

#if defined (LEAF_SIMD_AVX512)
	ASSERT(cnt % AlignmentBound == 0, "Size must be a multiple of 64");
	__m512i pack8i_ch = _mm512_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 64) {
		_mm512_store_si512(reinterpret_cast<__m512*>(d + i), pack8i_ch);
	}

#elif defined (LEAF_SIMD_AVX2)
	ASSERT(cnt % AlignmentBound == 0, "Size must be a multiple of 32");
	__m256i pack4i_ch = _mm256_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 32) {
		_mm256_store_si256(reinterpret_cast<__m256i*>(d + i), pack4i_ch);
	}

#elif defined (LEAF_SIMD_SSE4_2)
	ASSERT(cnt % AlignmentBound == 0, "Size must be a multiple of 16");
	__m128i pack2i_ch = _mm_set1_epi8(ch);

	for (size_t i = 0; i < cnt; i += 16) {
		_mm_store_si128(reinterpret_cast<__m128i*>(d + i), pack2i_ch);
	}

#else
	_declUnused(d);
	memSet(dst, ch, cnt);
#endif

	return dst;
}

_NODISCARD _INLINE void* alignedMalloc(size_t size, size_t alignment) {
#if defined (_MSC_VER)
	void* m = _aligned_malloc(size, alignment);
#else
    ASSERT(size % alignment == 0, "Size must be multiple of alignment for some platforms");
	void* m = std::aligned_alloc(alignment, size);
#endif
	ASSERT(m, "Failed to allocate memory");
	return m;
}

_INLINE void alignedFree(void* block) {
#if defined (_MSC_VER)
	_aligned_free(block);
#else
	std::free(block);
#endif
}

template <typename T>
struct AlignedDeleter {
    void operator()(T* p) const { 
        if (p != nullptr)
            alignedFree(reinterpret_cast<void*>(p)); 
    }
};

template <typename T>
using AlignedUniquePtr = std::unique_ptr<T, AlignedDeleter<T>>;

template <typename T>
_NODISCARD AlignedUniquePtr<T> makeAlignedUnique(size_t size) {
    T* p = reinterpret_cast<T*>(alignedMalloc(size, alignof(T)));
    return AlignedUniquePtr<T>(p);
}


