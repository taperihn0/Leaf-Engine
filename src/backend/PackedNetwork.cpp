#include "PackedNetwork.hpp"

#if !defined(_MSC_VER)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace nn {

PackedNeuralNetwork GlobPackedNetwork = []() -> PackedNeuralNetwork {
    PackedNeuralNetwork network;
    network.loadDefaultNet();
    return network;
}();

PackedNeuralNetwork::PackedNeuralNetwork()
    : _layer_weights{}
    , _layer_biases{}
{}

PackedNeuralNetwork::~PackedNeuralNetwork() {
    release();
}

PackedNeuralNetwork::PackedNeuralNetwork(PackedNeuralNetwork&& network) noexcept {
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

#if defined(_MSC_VER)

    _fh = CreateFileA(path.data(), GENERIC_READ, FILE_SHARE_READ, 
                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 
                      nullptr);

    if (_fh == INVALID_HANDLE_VALUE) {
        std::cout << ("Couldn't open() a file: " + static_cast<std::string>(path)) << std::endl;
        std::cout << "Windows error code: " << GetLastError() << std::endl;
        return false;
    }

    DWORD high_size = 0;
    DWORD low_size = GetFileSize(_fh, &high_size);

    if (low_size == INVALID_FILE_SIZE) {
        std::cout << ("Failed to retrieve file size via GetFileSizeEx(): " 
                      + static_cast<std::string>(path)) << std::endl;
        std::cout << "Windows error code: " << GetLastError() << std::endl;
        return false;
    }


    _maph = CreateFileMapping(_fh, nullptr, PAGE_READONLY, high_size, low_size, nullptr);

    if (_maph == INVALID_HANDLE_VALUE or _maph == nullptr) {
        std::cout << ("Failed to create a memory maping for file: " 
                      + static_cast<std::string>(path)) << std::endl;
        std::cout << "Windows error code: " << GetLastError() << std::endl;
        return false;
    }

    _file_size = (static_cast<size_t>(high_size) << 32) | low_size;
    _file_buff = MapViewOfFile(_maph, FILE_MAP_READ, 0, 0, 0);

    if (!_file_buff) {
        std::cout << ("Couldn't obtain file buffer for file: " 
                      + static_cast<std::string>(path)) << std::endl;
        std::cout << "Windows error code: " << GetLastError() << std::endl;
        return false;
    }

#else

    _fd = open(path.data(), O_RDONLY);
    
    if (_fd == -1) {
        std::cout << ("Couldn't open() a file: " + static_cast<std::string>(path)) << std::endl;
        return false;
    }

    struct stat st;
    if (fstat(_fd, &st) == -1) {
        close(_fd);
        std::cout << ("Couldn't fstat() a file: " + static_cast<std::string>(path)) << std::endl;
        return false;
    }

    _file_size = static_cast<size_t>(st.st_size);
    _file_buff = mmap(nullptr, _file_size, PROT_READ, MAP_PRIVATE, _fd, 0);

    if (_file_buff == MAP_FAILED) {
        close(_fd);
        std::cout << ("Couldn't mmap() a file: " + static_cast<std::string>(path)) << std::endl;
        return false;
    }

#endif

    _header = *reinterpret_cast<Header*>(_file_buff);

    if (_header.layer_count != 3) {
        std::cout << "Layer number must be 3" << std::endl;
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
#if defined(_MSC_VER)

    // TODO
    return false;

#else

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

#endif
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

#if defined(_MSC_VER)

    _fh = network._fh;
    _maph = network._maph;
    _file_buff = network._file_buff;
    _file_size = network._file_size;
    _header = network._header;

    network._file_buff = nullptr;
    network._file_size = 0;

    for (size_t i = 0; i < MaxLayerCount; i++) {
        _layer_weights[i] = network._layer_weights[i];
        _layer_biases[i] = network._layer_biases[i];
        network._layer_weights[i] = nullptr;
        network._layer_biases[i] = nullptr;
    }

#else

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

#endif
}

void PackedNeuralNetwork::release() {

#if defined(_MSC_VER)

    if (_fh != INVALID_HANDLE_VALUE and _maph != INVALID_HANDLE_VALUE and _file_buff) {
        UnmapViewOfFile(_file_buff);
        CloseHandle(_fh);
        CloseHandle(_maph);
    }

#else

    if (_fd != -1 and _file_buff and _file_buff != MAP_FAILED) {
        munmap(_file_buff, _file_size);
        close(_fd);
    }

#endif
}

} // namespace nn
