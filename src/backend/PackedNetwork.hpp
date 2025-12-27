#pragma once

#include "Common.hpp"

namespace nn {

static constexpr size_t MaxLayerCount = 4;

static constexpr size_t           TestNetworkInputSize = 768;
static constexpr size_t           TestNetworkHiddenLayerSize = 64;
static constexpr std::string_view TestNetworkPath = "src/assets/nets/net.bin";

class PackedNeuralNetwork {
public:
    PackedNeuralNetwork() = default;
    ~PackedNeuralNetwork();

#pragma pack(push, 1)
    // Add activaction function info
    struct Header {
        uint32_t layer_size[MaxLayerCount];
        uint16_t layer_count;
        byte     dual_hl;
        byte     _padding;
    };
#pragma pack(pop)

    static_assert(sizeof(Header) == 20);

    bool loadFromFile(std::string_view path);
    bool loadDefaultNet();

    uint getAccumulatorSize();
    uint getLayerSize(size_t layer_num);

    int16_t* getLayerWeights(size_t layer_num);
    int16_t* getLayerBiases(size_t layer_num);
private:
    bool initLayerWeightsBiases();

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

} // namespace nn
