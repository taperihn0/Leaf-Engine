/*
 * Leaf, a UCI Chess Engine
 * Copyright (C) 2026 taperihn0
 *
 * Leaf is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Leaf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "Simd.hpp"

#if defined(__GNUC__) and !defined(_WIN32)
#include <sys/mman.h>
#endif

#include <memory>
#include <cstddef>

// modify it as you wish
#define _ENABLE_PREFETCH

namespace mem {

constexpr static _FORCEINLINE size_t getAlignedUpSize(size_t size, size_t align) {
    if (size % align == 0)
        return size;
    return size + (align - size % align);
}

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
    fill(d, d + cnt, std::byte(ch));
}

template <typename T, typename Tv>
_FORCEINLINE void fill(T* first, T* last, const Tv& value) {
    std::fill(first, last, value);
}

_NODISCARD _INLINE void* alignedMalloc(size_t size, size_t alignment) {
#if defined(_WIN32)
    void* m = _aligned_malloc(size, alignment);
#else
    if (size % alignment != 0) {
        throw std::invalid_argument("Size must be a multiple of alignment for POSIX systems");
    }

    void* m = std::aligned_alloc(alignment, size);
#endif

    if (!m) {
        throw std::bad_alloc();
    }
    
    return m;
}

_INLINE void alignedFree(void* m) {
#if defined(_WIN32)
    _aligned_free(m);
#else
    std::free(m);
#endif
}

#if defined(_WIN32)
_INTERNAL bool enableLargePagesPrivilegeWin32() {
    HANDLE htoken;
    
    if (!OpenProcessToken(GetCurrentProcess(), 
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &htoken)) {
        throw std::runtime_error("Failed to `OpenProcessToken`");
    }

    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &luid)) {
        CloseHandle(htoken);
        throw std::runtime_error("Failed to `LookupPrivilegeValue`");
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL result = AdjustTokenPrivileges(htoken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
    DWORD error = GetLastError();
    CloseHandle(htoken);

    if (!result or error != ERROR_SUCCESS) {
        throw std::runtime_error("`AdjustTokenPrivileges` somehow failed, error code: " + std::to_string(error));
    }

    return true;
}
#endif

_NODISCARD _INLINE void* largePageAlignedMalloc(size_t size) {
    if (size == 0) {
        throw std::invalid_argument("Invalid allocation size: " + std::to_string(size));
    }

#if defined(_WIN32)
    void* m = VirtualAlloc(nullptr, size, 
                           MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, 
                           PAGE_READWRITE);
    
    if (!m) {
        m = VirtualAlloc(nullptr, size, 
                         MEM_COMMIT | MEM_RESERVE, 
                         PAGE_READWRITE);
    }
    
    if (!m)
        throw std::bad_alloc();
    
    return m;

#else
    void* m = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
                  
    if (m == MAP_FAILED) {
        m = mmap(nullptr, size, PROT_READ | PROT_WRITE, 
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }
    
    if (m == MAP_FAILED)
        throw std::bad_alloc();

    return m;
#endif
}

_INLINE void pageAlignedFree(void* m, size_t size) { 
    if (!m) return;

#if defined(_WIN32)
    _declUnused(size);

    if (!VirtualFree(m, 0, MEM_RELEASE)) {
        throw std::runtime_error("Failed to execute `VirtualFree`");
    }
#else
    if (munmap(m, size) != 0) {
        throw std::runtime_error("Failed to execute `munmap`");
    }
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
using AlignedSharedPtr = std::shared_ptr<T>;

template <typename T>
_NODISCARD _INTERNAL AlignedUniquePtr<T> makeAlignedUnique(size_t count, size_t alignment = alignof(T)) {
    T* p = reinterpret_cast<T*>(alignedMalloc(sizeof(T) * count, alignment));
    return AlignedUniquePtr<T>(p);
}

template <typename T>
_NODISCARD _INTERNAL AlignedSharedPtr<T> makeAlignedShared(size_t count, size_t alignment = alignof(T)) {
    T* p = reinterpret_cast<T*>(alignedMalloc(sizeof(T) * count, alignment));
    return AlignedSharedPtr<T>(p, AlignedDeleter<T>());
}

template <typename T>
class PageDeleter {
public:
    PageDeleter() = delete;

    PageDeleter(const PageDeleter&) noexcept = default;
    PageDeleter(PageDeleter&&) noexcept = default;

    PageDeleter& operator=(const PageDeleter&) noexcept = default;
    PageDeleter& operator=(PageDeleter&&) noexcept = default;

    explicit PageDeleter(size_t size) noexcept
        : _size(size) {}

    void operator()(T* p) const {
        if (p != nullptr)
            pageAlignedFree(reinterpret_cast<T*>(p), _size);
    }
private:
    size_t _size;
};

template <typename T>
using PageAlignedUniquePtr = std::unique_ptr<T, PageDeleter<T>>;

template <typename T>
_NODISCARD _INTERNAL PageAlignedUniquePtr<T> makePageAlignedUnique(size_t count) {
    const size_t size = sizeof(T) * count;
    T* p = reinterpret_cast<T*>(largePageAlignedMalloc(size));
    return PageAlignedUniquePtr<T>(p, mem::PageDeleter<T>(size));
}

} // namespace mem
