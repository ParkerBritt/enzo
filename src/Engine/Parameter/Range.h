#pragma once

#include "Engine/Core/Types.h"
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
        Range(floatT minValue=0, floatT maxValue=10, RangeFlag minFlag=RangeFlag::UNLOCKED, RangeFlag maxFlag=RangeFlag::UNLOCKED);

        floatT getMin() const	    { return minValue_; }
        floatT getMax() const	    { return maxValue_; }
        RangeFlag getMinFlag() const    { return minFlag_; }
        RangeFlag getMaxFlag() const    { return maxFlag_; }

    private:
        floatT minValue_;
        floatT maxValue_;
        RangeFlag minFlag_;
        RangeFlag maxFlag_;
    };
}
