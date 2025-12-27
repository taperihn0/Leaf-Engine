#include "PackedNetwork.hpp"

#if defined(_MSC_VER)
#error "Windows not supported"
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace nn
{

PackedNeuralNetwork::~PackedNeuralNetwork() {
    if (_fd != -1 and _file_buff and _file_buff != MAP_FAILED) {
        munmap(_file_buff, _file_size);
        close(_fd);
    }
}

bool PackedNeuralNetwork::loadFromFile(std::string_view path) {
    _fd = open(path.data(), O_RDONLY);

    if (_fd == -1) {
        ASSERT(false, "Couldn't open() a file: " + static_cast<std::string>(path));
        return false;
    }

    struct stat st;
    if (fstat(_fd, &st) == -1) {
        close(_fd);
        ASSERT(false, "Couldn't fstat() a file: " + static_cast<std::string>(path));
        return false;
    }

    _file_size = static_cast<size_t>(st.st_size);
    _file_buff = mmap(nullptr, _file_size, PROT_READ, MAP_PRIVATE, _fd, 0);

    if (_file_buff == MAP_FAILED) {
        close(_fd);
        ASSERT(false, "Couldn't mmap() a file: " + static_cast<std::string>(path));
        return false;
    }

    _header = *reinterpret_cast<Header*>(_file_buff);

    ASSERTNOLOG(_header.layer_count > 0 and _header.layer_count <= MaxLayerCount);

    initLayerWeightsBiases();

    return true;
}   

bool PackedNeuralNetwork::loadDefaultNet() {
    return loadFromFile(TestNetworkPath);
}

uint PackedNeuralNetwork::getAccumulatorSize() {
    return getLayerSize(0);
}

uint PackedNeuralNetwork::getLayerSize(size_t layer_num) {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _header.layer_size[layer_num];
}

int16_t* PackedNeuralNetwork::getLayerWeights(size_t layer_num) {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _layer_weights[layer_num];
}

int16_t* PackedNeuralNetwork::getLayerBiases(size_t layer_num) {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _layer_biases[layer_num];
}

bool PackedNeuralNetwork::initLayerWeightsBiases() {
    int16_t* it = reinterpret_cast<int16_t*>(_file_buff) + sizeof(Header) / sizeof(int16_t);
    size_t byte_offset = sizeof(Header);

    for (uint layer_num = 0; layer_num + 1 < _header.layer_count; layer_num++) {
        size_t weight_cnt = _header.layer_size[layer_num] * _header.layer_size[layer_num + 1];

        if (layer_num == 1 and _header.dual_hl)
            weight_cnt *= 2;

        _layer_weights[layer_num] = it;
        it += weight_cnt;
        byte_offset += weight_cnt * sizeof(int16_t);

        ASSERTNOLOG(byte_offset <= _file_size);

        size_t biases_cnt = _header.layer_size[layer_num + 1];

        _layer_biases[layer_num] = it;
        it += biases_cnt;
        byte_offset += biases_cnt * sizeof(int16_t);

        ASSERTNOLOG(byte_offset <= _file_size);
    }

    return true;
}

} // namespace nn
