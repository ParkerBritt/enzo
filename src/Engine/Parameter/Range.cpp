#include "Engine/Parameter/Range.h"


enzo::prm::Range::Range(bt::floatT minValue, RangeFlag minFlag, bt::floatT maxValue, RangeFlag maxFlag)
: minValue_{minValue}, minFlag_{minFlag}, maxValue_{maxValue}, maxFlag_{maxFlag}
{

}
