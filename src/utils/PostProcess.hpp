#pragma once

#include "UtilsCommon.hpp"

namespace Utils {

class PostProcess {
public:
    static bool mergeBinaryFiles(std::vector<std::string> filepaths, std::ofstream& into) {
        ASSERT(into);

        std::vector<std::ifstream> inputs(filepaths.size());
        size_t max_bytes_length = 0;

        for (size_t i = 0; i < filepaths.size(); i++) {
            if (!inputs[i].open(filepaths[i], std::ios::ios_base::binary)) {
                ASSERT(false, "Failed to open file: " + filepaths[i])
                return false;
            }

            size_t bytes_left = streamBytesLeft();
            max_bytes_length = std::max(bytes_left, max_bytes_length);
        }

        std::unique_ptr<char[]> alloc_buff = std::make_unique<char[]>(new char[max_bytes_length]);

        for (size_t i = 0; i < inputs.size(); i++) {
            size_t bytes_left = streamBytesLeft();

            if (!inputs[i].read(alloc_buff.get(), bytes_left)) {
                ASSERT(false, "Failed to read buffer from file: " + filepaths[i]);
                return false;
            }

            if (!into.write(alloc_buff.get(), bytes_left)) {
                ASSERT(false, "Failed to write buffer from file: " + filepaths[i]);
                return false;
            }
        }

        return into.good();
    }

    static bool convertToSfBinpackFile(std::ifstream& pck_input, std::ofstream& binpack_output) {
        ASSERT(pck_input and binpack_output);

        std::vector<PackedPosition> packs = PackedPosition::fullRead(pck_input);
        std::ostringstream buff(std::ios::binary);

        for (PackedPosition& pack : packs) {
            SfBinFormatPosition sfpack = SfBinFormatPosition::SffromPacked(pack);
            SfBinFormatPosition::write(buff, sfpack);
        }

        const std::string& str = buff.str();

        if (!binpack_output.write(str.data(), str.size())) {
            ASSERT(false, "Failed to write binary string buffer to given file");
            return false;
        }

        return binpack_output.good();
    }
};

} // namespace Utils
