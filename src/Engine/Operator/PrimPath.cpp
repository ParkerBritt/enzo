#include "PrimPath.h"
#include <sstream>

namespace enzo {

PrimPath::PrimPath(const std::string &path)
    : path_(path)
{
    std::istringstream stream(path);
    std::string token;
    while (std::getline(stream, token, Delimiter)) {
        if (!token.empty())
            components_.push_back(token);
    }
}

std::string PrimPath::name() const
{
    if (components_.empty())
        return {};
    return components_.back();
}

PrimPath PrimPath::parent() const
{
    if (components_.size() <= 1)
        return PrimPath("/");

    std::string result;
    for (size_t i = 0; i < components_.size() - 1; ++i)
        result += Delimiter + components_[i];
    return PrimPath(result);
}

} // namespace enzo
