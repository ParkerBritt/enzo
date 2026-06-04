#pragma once

#include "Engine/Core/Types.h"
namespace enzo::prm {
class Default
{
  public:
    Default();
    Default(
        floatT floatDefault
        // TODO: add string meaning eg.
        // ,  CH_StringMeaning string_meaning = CH_STRING_LITERAL
    );
    Default(const char* stringDefault);
    Default(int intDefault);
    Default(bool boolDefault);

    floatT getFloat() const { return floatDefault_; }
    intT getInt() const { return (intT)floatDefault_; }
    const char* getString() const { return stringDefault_; }

    void set(floatT thefloat, const char* thestring);
    void setFloat(floatT value) { floatDefault_ = value; }
    void setInt(intT value) { floatDefault_ = (intT)value; }
    void setString(const char* value) { stringDefault_ = value; }

  private:
    floatT floatDefault_;
    const char* stringDefault_;
};
} // namespace enzo::prm
