#pragma once

#include "UtilsCommon.hpp"
#include "Entry.hpp"

#include <memory>
#include <sstream>

namespace Utils {

class PostProcess {
public:
    static bool mergeBinaryFiles(std::vector<std::string>& filepaths, std::ofstream& into) {
        ASSERT(into, "Input file is not valid");

        std::vector<std::ifstream> inputs(filepaths.size());
        size_t max_bytes_length = 0;

        for (size_t i = 0; i < filepaths.size(); i++) {
            inputs[i].open(filepaths[i], std::ios::ios_base::binary);

            if (!inputs[i]) {
                ASSERT(false, "Failed to open file: " + filepaths[i]);
                return false;
            }

            size_t bytes_left = streamBytesLeft(inputs[i]);
            max_bytes_length = std::max(bytes_left, max_bytes_length);
        }

        std::unique_ptr<char[]> alloc_buff(new char[max_bytes_length]);

        for (size_t i = 0; i < inputs.size(); i++) {
            std::cout << "Processing file: " << filepaths[i] << std::endl;

            size_t bytes_left = streamBytesLeft(inputs[i]);

            if (!inputs[i].read(alloc_buff.get(), bytes_left)) {
                ASSERT(false, "Failed to read buffer from file: " + filepaths[i]);
                return false;
            }

            if (!into.write(alloc_buff.get(), bytes_left)) {
                ASSERT(false, "Failed to write buffer from file: " + filepaths[i]);
                return false;
            }
        }

        into.flush();

        return into.good();
    }

    static bool packedPos2TrainingEntryFile(std::ifstream& pck_input, 
                                            std::ofstream& train_data_output, 
                                            TrainingDataEntry::Result8b game_result) 
    {
        ASSERT(pck_input and train_data_output, "Given files are not valid");

        std::vector<PackedPosition> packs = PackedPosition::fullRead(pck_input);
        std::ostringstream buff(std::ios::binary);

        for (PackedPosition& pack : packs) {
            Position full_position = PackedPosition::unpacked(pack);
            Score white_score = Eval::staticEval(full_position);
            TrainingDataEntry entry(pack, white_score, game_result);
            TrainingDataEntry::write(buff, entry);
        }

        buff.flush();

        const std::string& str = buff.str();

        if (!train_data_output.write(str.data(), str.size())) {
            ASSERT(false, "Failed to write binary string buffer to given file");
            return false;
        }

        return train_data_output.good();
    }
};

} // namespace Utils
