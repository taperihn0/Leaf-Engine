#include "Common.hpp"

#include <unordered_map>

#if defined(_ENABLE_TUNING)

class TunableParametersMap {
public:
    TunableParametersMap() = default;
    void createMapping();
    _NODISCARD void* getAddressOf(const std::string& str);
private:
    std::unordered_map<std::string, void*> _addr;
};

extern TunableParametersMap GlobParamMapping;

#endif // _ENABLE_TUNING
