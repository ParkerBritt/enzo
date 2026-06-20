#include "Engine/Network/NetworkPath.h"
#include <utility>

namespace enzo {

NetworkPath::NetworkPath(const std::string& path)
{
    // Trim surrounding whitespace before validating so padding does not throw off the checks
    std::string stripped = strip(path);

    if (!isValidFormatting(stripped))
    {
        path_ = "";
        return;
    }

    path_ = std::move(stripped);
}

NetworkPath::NetworkPath(const char* path) : NetworkPath(std::string(path)) {}

size_t NetworkPath::parameterDelimiter(std::string_view pathString)
{
    size_t lastSlash = pathString.rfind('/');
    size_t searchStart = (lastSlash == std::string_view::npos) ? 0 : lastSlash + 1;

    return pathString.find('.', searchStart);
}

bool NetworkPath::isValidFormatting(const std::string& pathString)
{
    size_t dotPos = parameterDelimiter(pathString);

    // With no parameter the whole string is an ordinary node path
    if (dotPos == std::string::npos) return Path::isValidFormatting(pathString);

    // Split the parameter off the end and validate the node portion and the name separately
    std::string nodeString = pathString.substr(0, dotPos);
    std::string parameter = pathString.substr(dotPos + 1);

    return Path::isValidFormatting(nodeString) && isValidName(parameter);
}

bool NetworkPath::isValid() const { return isValidFormatting(path_); }

bool NetworkPath::hasParameter() const { return parameterDelimiter(path_) != std::string::npos; }

std::string NetworkPath::getParameter() const
{
    size_t dotPos = parameterDelimiter(path_);

    if (dotPos == std::string::npos) return "";

    return path_.substr(dotPos + 1);
}

NetworkPath NetworkPath::getNode() const
{
    size_t dotPos = parameterDelimiter(path_);

    if (dotPos == std::string::npos) return *this;

    return NetworkPath(path_.substr(0, dotPos));
}

} // namespace enzo
