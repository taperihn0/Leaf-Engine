#pragma once

#include "backend/Game.hpp"
#include "backend/Search.hpp"

namespace Utils {

class SelfMatch {
public:
    SelfMatch() = default;

    const SelfMatch& start(SearchLimits limits_white, SearchLimits limits_black);

    const MoveRecord& getMoveRecord() const;

    void newMatch();
private:
    MoveRecord                  _moves_record;
    std::array<Search, 2>       _search_by_side;
    std::array<SearchLimits, 2> _search_limits;
};

} // namespace Utils
