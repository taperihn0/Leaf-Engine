#pragma once

#if defined(_WIN32)
#include <windows.h>
#endif
#include <stdexcept>
#include <cstddef>
#include "Resource.h"

namespace rh {

struct EmbeddedResource {
    const std::byte* data;
    size_t size;
};

_INTERNAL EmbeddedResource loadResource(int id)
{
    HMODULE hmodule = GetModuleHandle(NULL);

    HRSRC res = FindResource(hmodule,
                             MAKEINTRESOURCE(id),
                             RT_RCDATA);

    if (!res) {
        DWORD err = GetLastError();
        throw std::runtime_error("FindResource failed with error code: " + std::to_string(err));
    }

    DWORD size = SizeofResource(nullptr, res);
    HGLOBAL handle = LoadResource(nullptr, res);

    if (!handle)
        throw std::runtime_error("LoadResource failed");

    void* m = LockResource(handle);

    return { static_cast<const std::byte*>(m),
             static_cast<size_t>(size) };
}

} // namespace rh
