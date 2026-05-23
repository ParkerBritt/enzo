#pragma once

#include "Engine/Types.h"
namespace enzo::prm
{
    class Default
    {
    public:
        Default();
        Default(bt::floatT floatDefault
        // TODO: add string meaning eg.
        // ,  CH_StringMeaning string_meaning = CH_STRING_LITERAL
        );
        Default(const char *stringDefault);
        Default(int intDefault);
        Default(bool boolDefault);

        bt::floatT getFloat() const	{ return floatDefault_; }
        bt::intT getInt() const { return (bt::intT)floatDefault_; }
        const char *getString() const { return stringDefault_; }

        void set(bt::floatT thefloat, const char *thestring);
        void setFloat(bt::floatT value)	{ floatDefault_ = value; }
        void setInt(bt::intT value)	{ floatDefault_ = (bt::intT)value; }
        void setString(const char *value) { stringDefault_ = value; }

    private:
        bt::floatT floatDefault_;
        const char* stringDefault_;
    };
}
