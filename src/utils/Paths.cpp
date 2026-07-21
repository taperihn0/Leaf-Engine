#include "UtilsCommon.hpp"
#include "Paths.hpp"

namespace utils::paths {

ProjectPathsManager::ProjectPathsManager() 
    : _current_path(std::filesystem::current_path()) 
{
    initMapping();
}

ProjectPathsManager& ProjectPathsManager::getManager() {
    static ProjectPathsManager manager;
    return manager;
}

const std::filesystem::path& ProjectPathsManager::getDir(enumDir dir) const {
    const auto elem = _dir.find(dir);

    if (elem == _dir.end())
        throw std::out_of_range("ProjectPathsManager::getDir - no mapped dir found");

    return elem->second;
}

std::filesystem::path ProjectPathsManager::getWhiteWinOutputFileName(int thread_num) {
    ASSERT_NOLOG(thread_num <= PlatformThreadLimit);
    return std::filesystem::path("selfplay_white_win_thread_" + std::to_string(thread_num))
                            .replace_extension(".tdf");
}

std::filesystem::path ProjectPathsManager::getBlackWinOutputFileName(int thread_num) {
    ASSERT_NOLOG(thread_num <= PlatformThreadLimit);
    return std::filesystem::path("selfplay_black_win_thread_" + std::to_string(thread_num))
                            .replace_extension(".tdf");
}

std::filesystem::path ProjectPathsManager::getDrawOutputFileName(int thread_num) {
    ASSERT_NOLOG(thread_num <= PlatformThreadLimit);
    return std::filesystem::path("selfplay_draw_thread_" + std::to_string(thread_num))
                            .replace_extension(".tdf");
}

void ProjectPathsManager::initMapping() {
    if (std::filesystem::exists("assets") and 
        !std::filesystem::is_directory("assets"))
        throw std::runtime_error("'assets' directory could not be found at the current directory");
    
    if (std::filesystem::exists("data") and 
        !std::filesystem::is_directory("data"))
        throw std::runtime_error("'data' directory could not be found at the current directory");

    std::filesystem::create_directories("assets");
    std::filesystem::create_directories("data");

    {
        const auto openings_dir = _current_path / "assets" / "books";
        emplaceMapping(enumDir::OPENING_BOOKS_DIRECTORY, openings_dir);
    }

    {
        const auto sets_dir = _current_path / "assets" / "nets";
        emplaceMapping(enumDir::NETS_DIRECTORY, sets_dir);
    }

    {
        const auto sets_dir = _current_path / "assets" / "sets";
        emplaceMapping(enumDir::SETS_DIRECTORY, sets_dir);
    }

    {
        const auto spsa_dir = _current_path / "data" / "spsa";
        emplaceMapping(enumDir::DATA_SPSA_DIRECTORY, spsa_dir);
    }

    {
        const auto selfplay_dir = _current_path / "data" / "selfplay";
        emplaceMapping(enumDir::DATA_SELFPLAY_DIRECTORY, selfplay_dir);
    }

    {
        const auto training_dir = _current_path / "data" / "training";
        emplaceMapping(enumDir::DATA_TRAINING_DIRECTORY, training_dir);
    }

    _dir[enumDir::TEMP_DIRECTORY] = std::filesystem::temp_directory_path();
}

void ProjectPathsManager::emplaceMapping(enumDir key, const std::filesystem::path& dir, bool required) {
    if (std::filesystem::exists(dir) and 
        !std::filesystem::is_directory(dir))
        throw std::runtime_error(dir.string() + " directory could not be found at the current directory");

    if (!required)
#if !defined(_MSC_VER)
        std::filesystem::create_directories(dir);
#else
        // Working directory on Windows is just problematic, do not bother creating those directories
        static_cast<void>(0);
#endif
    else if (!std::filesystem::exists(dir))
        throw std::runtime_error(dir.string() + " directory could not be found at the current directory");

    _dir[key] = dir;
}

} // namespace utils::paths
