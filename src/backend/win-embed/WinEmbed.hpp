#pragma once

#if defined(_WIN32)
#include <windows.h>
#endif
#include <stdexcept>
#include <cstddef>

namespace rh {

struct EmbeddedResource {
    const std::byte* data;
    size_t size;
};

_INTERNAL EmbeddedResource loadResource(int id)
{
    HRSRC res = FindResource(nullptr,
                             MAKEINTRESOURCE(id),
                             RT_RCDATA);

    if (!res)
        throw std::runtime_error("FindResource failed");

    DWORD size = SizeofResource(nullptr, res);
    HGLOBAL handle = LoadResource(nullptr, res);

    if (!handle)
        throw std::runtime_error("LoadResource failed");

    void* m = LockResource(handle);

    return { static_cast<const std::byte*>(m),
             static_cast<size_t>(size) };
}

} // namespace rh
