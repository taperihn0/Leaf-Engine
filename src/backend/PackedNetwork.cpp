#include "PackedNetwork.hpp"

#if !defined(_MSC_VER)
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#if defined(_USE_EMBEDDED_NEURAL_NET)
#include "vendor/incbin.h"

#undef INCBIN_PREFIX
#define INCBIN_PREFIX Glob
#define INCBIN_STYLE INCBIN_STYLE_CAMEL

INCBIN(PackedNetwork, DEFAULT_NEURAL_NET_FILE_NAME);

static const void* EmbeddedNetworkAddr = GlobPackedNetworkData;
static size_t EmbeddedNetworkSize = GlobPackedNetworkSize;
#endif

static bool UseEmbeddedNetwork = false;

namespace nn {

PackedNeuralNetwork GlobPackedNetwork = []() -> PackedNeuralNetwork {
    PackedNeuralNetwork network;
    
    if (!network.loadDefaultNet()) {
        std::cout << "Failed to load default net" << std::endl;
    }

    if (!network.isValid()) {
        std::cout << "Failed to initialize net" << std::endl;
    }

    return network;
}();

PackedNeuralNetwork::PackedNeuralNetwork()
    : _mem_size(0)
    , _mem_buf(nullptr)
    , _bin_path(std::nullopt)
    , _layer_weights{}
    , _layer_biases{}
{}

PackedNeuralNetwork::~PackedNeuralNetwork() {
    if (!UseEmbeddedNetwork) 
        releaseFileMapping();
}

PackedNeuralNetwork::PackedNeuralNetwork(PackedNeuralNetwork&& network) noexcept {
    fromRVal(std::move(network));
}

PackedNeuralNetwork& PackedNeuralNetwork::operator=(PackedNeuralNetwork&& network) {
    if (!UseEmbeddedNetwork) 
        releaseFileMapping();

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
    if (!UseEmbeddedNetwork) 
        releaseFileMapping();

    UseEmbeddedNetwork = false;
    _mem_size = 0;

#if defined(_MSC_VER)
    _fh = CreateFileA(path.data(), GENERIC_READ, FILE_SHARE_READ, 
                      nullptr, OPEN_EXISTING, 
                      FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN,
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

    _mem_size = (static_cast<size_t>(high_size) << 32) | low_size;
    _mem_buf = MapViewOfFile(_maph, FILE_MAP_READ, 0, 0, 0);

    if (!_mem_buf) {
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

    _mem_size = static_cast<size_t>(st.st_size);
    _mem_buf = mmap(nullptr, _mem_size, PROT_READ, MAP_PRIVATE, _fd, 0);

    if (_mem_buf == MAP_FAILED) {
        close(_fd);
        std::cout << ("Couldn't mmap() a file: " + static_cast<std::string>(path)) << std::endl;
        return false;
    }
#endif

    if (loadFromMemory(_mem_buf)) {
        _bin_path = path;
        return true;
    }

    _bin_path = std::nullopt;
    return false;
}

bool PackedNeuralNetwork::loadFromMemory(const void* m) {
    if (!m) {
        std::cout << "Invalid memory address" << std::endl;
        return false;
    }

    _header = *reinterpret_cast<const Header*>(m);

    if (_header.layer_count != 3) {
        std::cout << "Layer number must be 3" << std::endl;
        return false;
    }

    ASSERTNOLOG(_header.layer_count > 0 and _header.layer_count <= MaxLayerCount);

    return initLayerWeightsBiases(m);
}

bool PackedNeuralNetwork::loadDefaultNet() {
    bool status = false;

#if defined(_USE_EMBEDDED_NEURAL_NET)
    UseEmbeddedNetwork = true;
    _mem_buf = nullptr;
    _mem_size = EmbeddedNetworkSize;
    _bin_path = DefaultNetworkFile;
    status = loadFromMemory(EmbeddedNetworkAddr);
#else
    if (std::filesystem::exists(DefaultNetworkFile)) {
        status = loadFromFile(DefaultNetworkFile);
    }
    else {
        std::filesystem::path devpath = DevNetworksDir;
        devpath /= std::string(DefaultNetworkFile);
        status = loadFromFile(std::string_view(devpath.string()));
    }
#endif

    return status;
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

std::string PackedNeuralNetwork::getFilePath() const {
    return _bin_path.value_or("<empty>");
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

bool PackedNeuralNetwork::initLayerWeightsBiases(const void* m) {
    const int16_t* it = reinterpret_cast<const int16_t*>(m) + sizeof(Header) / sizeof(int16_t);
    size_t byte_offset = sizeof(Header);

    for (uint layer_num = 0; layer_num + 1 < _header.layer_count; layer_num++) {
        size_t weight_cnt = getLayerWeightsCount(layer_num);

        _layer_weights[layer_num] = it;
        it += weight_cnt;
        byte_offset += weight_cnt * sizeof(int16_t);

        ASSERTNOLOG(byte_offset <= _mem_size);

        size_t biases_cnt = _header.layer_size[layer_num + 1];

        _layer_biases[layer_num] = it;
        it += biases_cnt;
        byte_offset += biases_cnt * sizeof(int16_t);

        ASSERTNOLOG(byte_offset <= _mem_size);
    }

    return true;
}

void PackedNeuralNetwork::fromRVal(PackedNeuralNetwork&& network) {
#if defined(_MSC_VER)
    _fh = network._fh;
    _maph = network._maph;
    _mem_buf = network._mem_buf;
    _mem_size = network._mem_size;
    _header = network._header;

    network._mem_buf = nullptr;
    network._mem_size = 0;

    for (size_t i = 0; i < MaxLayerCount; i++) {
        _layer_weights[i] = network._layer_weights[i];
        _layer_biases[i] = network._layer_biases[i];
        network._layer_weights[i] = nullptr;
        network._layer_biases[i] = nullptr;
    }
#else
    _mem_buf = network._mem_buf;
    _mem_size = network._mem_size;
    _fd = network._fd;
    _header = network._header;

    network._fd = -1;
    network._mem_buf = nullptr;
    network._mem_size = 0;

    for (size_t i = 0; i < MaxLayerCount; i++) {
        _layer_weights[i] = network._layer_weights[i];
        _layer_biases[i] = network._layer_biases[i];
        network._layer_weights[i] = nullptr;
        network._layer_biases[i] = nullptr;
    }
#endif
}

void PackedNeuralNetwork::releaseFileMapping() {
    if (UseEmbeddedNetwork) 
        return;

#if defined(_MSC_VER)
    if (_fh != INVALID_HANDLE_VALUE and _maph != INVALID_HANDLE_VALUE and _mem_buf) {
        UnmapViewOfFile(_mem_buf);
        CloseHandle(_fh);
        CloseHandle(_maph);
    }
#else
    if (_fd != -1 and _mem_buf and _mem_buf != MAP_FAILED) {
        munmap(_mem_buf, _mem_size);
        close(_fd);
    }
#endif
}

} // namespace nn
