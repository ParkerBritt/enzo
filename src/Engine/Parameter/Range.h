#pragma once

#include "Engine/Types.h"
namespace enzo::prm
{
    enum class RangeFlag
    {
        UNLOCKED,
        LOCKED

    };

    class Range
    {
    public:
        Range(bt::floatT minValue=0, RangeFlag minFlag=RangeFlag::UNLOCKED, bt::floatT maxValue=10, RangeFlag maxFlag=RangeFlag::UNLOCKED);

        bt::floatT getMin() const	    { return minValue_; }
        bt::floatT getMax() const	    { return maxValue_; }
        RangeFlag getMinFlag() const    { return minFlag_; }
        RangeFlag getMaxFlag() const    { return maxFlag_; }

    private:
        bt::floatT minValue_;
        bt::floatT maxValue_;
        RangeFlag minFlag_;
        RangeFlag maxFlag_;
    };
}
