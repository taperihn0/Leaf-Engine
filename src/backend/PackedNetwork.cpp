#include "PackedNetwork.hpp"

#if defined(_MSC_VER)
#error "Windows not supported"
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace nn {

PackedNeuralNetwork::PackedNeuralNetwork()
    : _layer_weights{}
    , _layer_biases{}
{}

PackedNeuralNetwork::~PackedNeuralNetwork() {
    release();
}

PackedNeuralNetwork::PackedNeuralNetwork(PackedNeuralNetwork&& network) {
    fromRVal(std::move(network));
}

PackedNeuralNetwork& PackedNeuralNetwork::operator=(PackedNeuralNetwork&& network) {
    release();
    fromRVal(std::move(network));
    return *this;
}

bool PackedNeuralNetwork::isValid() const {
    if (!_header.layer_count or _header.layer_size[2] != 1)
        return false;

    for (size_t i = 0; i < _header.layer_count; i++) {
        if (!_header.layer_size[i])
            return false;

        if (!i) continue;

        if (!_layer_weights[i - 1] or
            !_layer_biases[i - 1])
            return false;
    }

    return true;
}

bool PackedNeuralNetwork::loadFromFile(std::string_view path) {
    release();

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

    if (_header.layer_count != 3) {
        ASSERT(false, "Layer number must be 3");
        return false;
    }

    ASSERTNOLOG(_header.layer_count > 0 and _header.layer_count <= MaxLayerCount);

    initLayerWeightsBiases();

    return true;
}   

bool PackedNeuralNetwork::loadDefaultNet() {
    return loadFromFile(DefaultNetworkPath);
}

uint PackedNeuralNetwork::getAccumulatorSize() const {
    return getLayerSize(0);
}

uint PackedNeuralNetwork::getLayerSize(size_t layer_num) const {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _header.layer_size[layer_num];
}

const int16_t* PackedNeuralNetwork::getLayerWeights(size_t layer_num) const {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _layer_weights[layer_num];
}

const int16_t* PackedNeuralNetwork::getLayerBiases(size_t layer_num) const {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _layer_biases[layer_num];
}

size_t PackedNeuralNetwork::getLayerWeightsCount(size_t layer_num) const {
    ASSERTNOLOG(layer_num + 1 < _header.layer_count);

    size_t weight_cnt = _header.layer_size[layer_num] * _header.layer_size[layer_num + 1];

    if (layer_num == 1 and _header.dual_hl)
        weight_cnt *= 2;

    return weight_cnt;
}

size_t PackedNeuralNetwork::getLayerBiasesCount(size_t layer_num) const {
    ASSERTNOLOG(layer_num < _header.layer_count);
    return _header.layer_size[layer_num];
}

bool PackedNeuralNetwork::rewriteWithHeader(std::string_view in_path,
                                            std::string_view out_path,
                                            const Header& header) 
{
    int in_fd = open(in_path.data(), O_RDONLY);
    if (in_fd == -1)
        return false;

    struct stat st;
    if (fstat(in_fd, &st) == -1) {
        close(in_fd);
        return false;
    }

    const size_t in_size = static_cast<size_t>(st.st_size);

    std::vector<std::byte> buffer(in_size);
    ssize_t read_bytes = read(in_fd, buffer.data(), in_size);
    close(in_fd);

    if (read_bytes != static_cast<ssize_t>(in_size))
        return false;

    int out_fd = open(out_path.data(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);
    if (out_fd == -1)
        return false;

    if (write(out_fd, &header, sizeof(Header)) != sizeof(Header)) {
        close(out_fd);
        return false;
    }

    if (write(out_fd, buffer.data(), buffer.size()) !=
        static_cast<ssize_t>(buffer.size())) {
        close(out_fd);
        return false;
    }

    close(out_fd);
    return true;
}

bool PackedNeuralNetwork::initLayerWeightsBiases() {
    int16_t* it = reinterpret_cast<int16_t*>(_file_buff) + sizeof(Header) / sizeof(int16_t);
    size_t byte_offset = sizeof(Header);

    for (uint layer_num = 0; layer_num + 1 < _header.layer_count; layer_num++) {
        size_t weight_cnt = getLayerWeightsCount(layer_num);

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

void PackedNeuralNetwork::fromRVal(PackedNeuralNetwork&& network) {
    _file_buff = network._file_buff;
    _file_size = network._file_size;
    _fd = network._fd;
    _header = network._header;

    network._fd = -1;
    network._file_buff = nullptr;
    network._file_size = 0;

    for (size_t i = 0; i < MaxLayerCount; i++) {
        _layer_weights[i] = network._layer_weights[i];
        _layer_biases[i] = network._layer_biases[i];
        network._layer_weights[i] = nullptr;
        network._layer_biases[i] = nullptr;
    }
}

void PackedNeuralNetwork::release() {
    if (_fd != -1 and _file_buff and _file_buff != MAP_FAILED) {
        munmap(_file_buff, _file_size);
        close(_fd);
    }
}

} // namespace nn
