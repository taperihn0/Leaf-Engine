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
    if (_fd != -1 and _fileBuff and _fileBuff != MAP_FAILED) {
        munmap(_fileBuff, _fileSize);
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

    _fileSize = static_cast<size_t>(st.st_size);
    _fileBuff = mmap(nullptr, _fileSize, PROT_READ, MAP_PRIVATE, _fd, 0);

    if (_fileBuff == MAP_FAILED) {
        close(_fd);
        ASSERT(false, "Couldn't mmap() a file: " + static_cast<std::string>(path));
        return false;
    }

    _header = Header{ 0_ui32, { TestNetworkInputSize, TestNetworkHiddenLayerSize, 1 }, 3 };

    initLayerWeightsBiases();

    return true;
}   

bool PackedNeuralNetwork::loadDefaultNet() {
    return loadFromFile(TestNetworkPath);
}

bool PackedNeuralNetwork::initLayerWeightsBiases() {
    int16_t* i16_filebuff = reinterpret_cast<int16_t*>(_fileBuff);
    int16_t* it = i16_filebuff;

    for (uint i = 0; i < _header.layer_count - 1; i++) {
        size_t weight_cnt = _header.layer_size[i] * _header.layer_size[i + 1];
        
        _layer_weights[i] = it;
        it += weight_cnt;

        ASSERTNOLOG(static_cast<size_t>(it - i16_filebuff) < _fileSize);

        _layer_biases[i] = it;
        it += _header.layer_size[i];

        ASSERTNOLOG(static_cast<size_t>(it - i16_filebuff) < _fileSize);
    }

    return true;
}

} // namespace nn
