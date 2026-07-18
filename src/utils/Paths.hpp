#pragma once

#include "Common.hpp"

#include <filesystem>
#include <unordered_map>

namespace utils::paths {

enum class enumDir : uint8_t {
    OPENING_BOOKS_DIRECTORY,
    NETS_DIRECTORY,
    SETS_DIRECTORY,
    DATA_SPSA_DIRECTORY,
    DATA_SELFPLAY_DIRECTORY,
    DATA_TRAINING_DIRECTORY,
    TEMP_DIRECTORY,
};

class ProjectPathsManager {
public:
    ProjectPathsManager& operator=(const ProjectPathsManager&) = delete;
    ProjectPathsManager& operator=(ProjectPathsManager&&) = delete;

    static ProjectPathsManager& getManager();
    const std::filesystem::path& getDir(enumDir dir) const;
private:
    ProjectPathsManager();
    void initMapping();
    void emplaceMapping(enumDir key, const std::filesystem::path& dir, bool required = false);

    std::unordered_map<enumDir, std::filesystem::path> _dir;
    std::filesystem::path _current_path;
};

extern const ProjectPathsManager& PathsManager;

} // namespace utils::paths
