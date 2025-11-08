#include "Match.hpp"

namespace Utils {

const SelfMatch& SelfMatch::start(SearchLimits limits_white, SearchLimits limits_black) {

}

const MoveRecord& SelfMatch::getMoveRecord() const {
    return _moves_record;
}

void SelfMatch::newMatch() {
    _moves_record.clear();
    
    for (Search& search : _search_by_side)
        search.registerNewGame();

    std::memset(&_search_limits, 0, sizeof(_search_limits));
}

} // namespace Utils
