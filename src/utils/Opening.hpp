#pragma once

#include "backend/Common.hpp"
#include "backend/Position.hpp"

#include <vector>
#include <fstream>

namespace Utils {

class OpeningManBase
{
public:
    virtual const Position& getRandomPosition(int& moves_done) const = 0;
    virtual bool isEmpty() const = 0;
    virtual ~OpeningManBase() = default;
};

/* OpeningGenerator takes care of openings set.
*  Usually, these are too small to be just used while setting up 
*  very long tournamets, so we need to add some noise to the existing positions.
*/
class OpeningGenerator : public OpeningManBase {
public:
    static OpeningGenerator create();
    void load();
    const Position& getRandomPosition(int& moves_done) const override;
    bool isEmpty() const override;
private:
    OpeningGenerator() = default;
    static inline const std::string _OpeningFile = "src/assets/sets/crafty_2500_new.epd";

    static constexpr int            _OpeningEvalThreshold = 200;
#if !defined(DEBUG)
    static constexpr int            _RandomPerPos   = 25;
#else
    static constexpr int            _RandomPerPos   = 2;
#endif
    static constexpr int            _MinRandomMoves = 2;
    static constexpr int            _MaxRandomMoves = 14;

    struct GeneratedPosition {
        int moves_done;
        Position pos;

        bool operator==(const GeneratedPosition& p);
        bool operator!=(const GeneratedPosition& p);
    };

    static std::vector<GeneratedPosition> _positions;
};

inline OpeningGenerator GlobOpeningGenerator = []() -> OpeningGenerator {
    return OpeningGenerator::create();
}();

/* Regular EPD Openings loader.
*  Setups very little opening positions.
*/
class OpeningSuite : public OpeningManBase {
public:
    OpeningSuite() = default;
    OpeningSuite(std::string path);

    void load(std::string path);
    const Position& getRandomPosition(int& moves_done) const override;
    bool isEmpty() const override;
private:
    std::vector<Position> _positions;
};

} // namespace Utils
