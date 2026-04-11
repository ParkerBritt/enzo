#include "Engine/Parameter/Range.h"


enzo::prm::Range::Range(bt::floatT minValue, bt::floatT maxValue, RangeFlag minFlag, RangeFlag maxFlag)
: minValue_{minValue}, minFlag_{minFlag}, maxValue_{maxValue}, maxFlag_{maxFlag}
{

}
