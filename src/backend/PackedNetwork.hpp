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

#include "Common.hpp"
#include "utils/Paths.hpp"
#include "Memory.hpp"

namespace nn {

static std::filesystem::path DefaultNetworkFile = DEFAULT_NEURAL_NET_FILE_NAME;
static std::filesystem::path DevNetworksDir = ::utils::paths::PathsManager.getDir(
                                                ::utils::paths::enumDir::NETS_DIRECTORY);

static constexpr size_t  MaxLayerCount = 4;
static constexpr size_t  NetworkInputSize = 768;
static constexpr size_t  NetworkHiddenLayerSize = 128;
static constexpr size_t  NetworkOutputSize = 1;
static constexpr size_t  NetworkLayerCount = 3;
static constexpr int16_t NetworkWeightQuant = 255;
static constexpr int16_t NetworkBiasQuant = 64;
static constexpr int16_t NetworkOutputScale = 400;
static constexpr bool    NetworkDualHiddenLayer = true;

class PackedNeuralNetwork {
public:
    PackedNeuralNetwork();
    ~PackedNeuralNetwork();

    PackedNeuralNetwork(const PackedNeuralNetwork&) = delete;
    PackedNeuralNetwork(PackedNeuralNetwork&& network) noexcept;

    PackedNeuralNetwork& operator=(const PackedNeuralNetwork&) = delete;
    PackedNeuralNetwork& operator=(PackedNeuralNetwork&& network);

#pragma pack(push, 1)
    // Add activaction function info
    struct Header {
        uint32_t               layer_size[MaxLayerCount];
        uint16_t               layer_count;
        bool                   dual_hl;
        array1d<std::byte, 13> _padding;
    };
#pragma pack(pop)

    static_assert(sizeof(Header) == 32);
    static_assert(sizeof(Header) % sizeof(int16_t) == 0);

    bool isValid() const;

    bool loadDefaultNet();
    bool loadFromFile(std::filesystem::path path);

    uint getAccumulatorSize() const;
    uint getLayerSize(size_t layer_num) const;

    const int16_t* getLayerWeights(size_t layer_num) const;
    const int16_t* getLayerBiases(size_t layer_num) const;

    size_t getLayerWeightsCount(size_t layer_num) const;
    size_t getLayerBiasesCount(size_t layer_num) const;

    std::string getFilePath() const;

    static bool rewriteWithHeader(std::string_view in_path,
                                  std::string_view out_path,
                                  const Header& header);
private:
    bool loadFromMemory(const void* m);

    /* Create own aligned buffer for embedded network and return handler. */
    mem::AlignedUniquePtr<std::byte> createAlignedBuffer(size_t size, const void* data);

    bool initLayerWeightsBiases(const void* m);
    void fromRVal(PackedNeuralNetwork&& network) noexcept;

    void releaseFileMapping();

#if defined(_WIN32)
    HANDLE _fh = INVALID_HANDLE_VALUE;
    HANDLE _maph = INVALID_HANDLE_VALUE;
#else
    int _fd = -1;
#endif

	/* We map embedded network onto our own aligned buffer 
    *  to guarantee proper alignment for SIMD operations.
    */
	mem::AlignedUniquePtr<std::byte>	   _align_buf_handler;
    size_t                                 _mem_size;
    std::optional<void*>                   _file_mem_buf;
    Header                                 _header;
    std::optional<std::string>             _bin_path;
    array1d<const int16_t*, MaxLayerCount> _layer_weights;
    array1d<const int16_t*, MaxLayerCount> _layer_biases;
};

extern PackedNeuralNetwork GlobPackedNetwork;

} // namespace nn
