#pragma once

#include "Common.hpp"

namespace nn {

static constexpr std::string_view DefaultNetworkFile = DEFAULT_NEURAL_NET_FILE_NAME;
static constexpr std::string_view DevNetworksDir = "src/assets/nets";

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
        uint32_t          layer_size[MaxLayerCount];
        uint16_t          layer_count;
        byte              dual_hl;
        array1d<byte, 13> _padding;
    };
#pragma pack(pop)

    static_assert(sizeof(Header) == 32);
    static_assert(sizeof(Header) % sizeof(int16_t) == 0);

    bool isValid() const;

    bool loadDefaultNet();
    bool loadFromFile(std::string_view path);

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

    bool initLayerWeightsBiases(const void* m);
    void fromRVal(PackedNeuralNetwork&& network);

    void releaseFileMapping();

#if defined(_MSC_VER)
    HANDLE _fh = INVALID_HANDLE_VALUE;
    HANDLE _maph = INVALID_HANDLE_VALUE;
#else
    int _fd = -1;
#endif

    size_t                                 _mem_size;
    void*                                  _mem_buf;
    Header                                 _header;
    std::optional<std::string>             _bin_path;
    array1d<const int16_t*, MaxLayerCount> _layer_weights;
    array1d<const int16_t*, MaxLayerCount> _layer_biases;
};

extern PackedNeuralNetwork GlobPackedNetwork;

} // namespace nn
