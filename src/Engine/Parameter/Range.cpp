#include "Engine/Parameter/Range.h"

namespace enzo {

prm::Range::Range(floatT minValue, floatT maxValue, RangeFlag minFlag, RangeFlag maxFlag)
    : minValue_{minValue}, minFlag_{minFlag}, maxValue_{maxValue}, maxFlag_{maxFlag}
{
}

} // namespace enzo
