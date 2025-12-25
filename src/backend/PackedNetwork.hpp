#pragma once

#include "Common.hpp"

namespace nn {

static constexpr size_t MaxLayerCount = 4;

static constexpr size_t           TestNetworkInputSize = 768;
static constexpr size_t           TestNetworkHiddenLayerSize = 64;
static constexpr std::string_view TestNetworkPath = "beans.bin";

class PackedNeuralNetwork {
public:
    PackedNeuralNetwork() = default;
    ~PackedNeuralNetwork();

    struct Header {
        uint32_t version;
        uint32_t layer_size[MaxLayerCount];
        uint32_t layer_count;
    };

    bool loadFromFile(std::string_view path);
    bool loadDefaultNet();
private:
    bool initLayerWeightsBiases();

#if defined(_MSC_VER)
#error "Windows not supported" 
#else
    void*   _fileBuff;
    size_t  _fileSize;
    int     _fd = -1;
#endif

    Header   _header;
    int16_t* _layer_weights[MaxLayerCount];
    int16_t* _layer_biases[MaxLayerCount];
};

} // namespace nn
