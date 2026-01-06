#pragma once

#include "Common.hpp"

namespace nn {

static constexpr std::string_view DefaultNetworkPath = "src/assets/nets/publius_net128_0_h.bin";

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
    PackedNeuralNetwork(PackedNeuralNetwork&& network);

    PackedNeuralNetwork& operator=(const PackedNeuralNetwork&) = delete;
    PackedNeuralNetwork& operator=(PackedNeuralNetwork&& network);

#pragma pack(push, 1)
    // Add activaction function info
    struct Header {
        uint32_t layer_size[MaxLayerCount];
        uint16_t layer_count;
        byte     dual_hl;
        byte     _padding[13];
    };
#pragma pack(pop)

    static_assert(sizeof(Header) == 32);
    static_assert(sizeof(Header) % sizeof(int16_t) == 0);

    bool isValid() const;

    bool loadFromFile(std::string_view path);
    bool loadDefaultNet();

    uint getAccumulatorSize() const;
    uint getLayerSize(size_t layer_num) const;

    const int16_t* getLayerWeights(size_t layer_num) const;
    const int16_t* getLayerBiases(size_t layer_num) const;

    size_t getLayerWeightsCount(size_t layer_num) const;
    size_t getLayerBiasesCount(size_t layer_num) const;

    static bool rewriteWithHeader(std::string_view in_path,
                                  std::string_view out_path,
                                  const Header& header);
private:
    bool initLayerWeightsBiases();
    void fromRVal(PackedNeuralNetwork&& network);

    void release();

#if defined(_MSC_VER)
#error "Windows not supported" 
#else
    void*   _file_buff;
    size_t  _file_size;
    int     _fd = -1;
#endif

    Header   _header;
    int16_t* _layer_weights[MaxLayerCount];
    int16_t* _layer_biases[MaxLayerCount];
};

extern PackedNeuralNetwork GlobPackedNetwork;

} // namespace nn
