#include "Path.h"
#include <cctype>
#include <string_view>
#include <utility>
#include <cctype>

enzo::Path::Path() 
    : path_("")
{
}

enzo::Path::Path(const std::string& path)
{   
    if (!isValidFormatting(path))
    {
        // NOTE: Need to put something better here for error handling
        path_ = "";
        return;
    }

    path_ = Strip(path);
}

enzo::Path::Path(const char* path)
{
    std::string pathString(path);

    if (!isValidFormatting(pathString))
    {
        // NOTE: Will replace this with proper error handling
        path_ = "";
        return;
    }

    path_ = std::move(pathString);
}

std::string enzo::Path::Strip(const std::string& str)
{
    size_t startWhitespace = str.find_first_not_of(" \t\n\r\f\v");

    if (startWhitespace == std::string::npos)
        return "";

    size_t endWhitespace = str.find_last_not_of(" \t\n\r\f\v");

    return str.substr(startWhitespace, endWhitespace - startWhitespace + 1);
}

bool enzo::Path::isEmpty() const
{
    return path_.empty();
}

bool enzo::Path::isRoot() const
{
    return path_ == "/";
}

bool enzo::Path::isAbsolute() const
{
    return !isEmpty() && path_.front() == '/';
}

bool enzo::Path::isRelative() const
{
    return !isEmpty() && path_.front() != '/';
}

bool enzo::Path::isValidName(const std::string& name)
{
    if (name.empty())
        return false;

    for (const char character : name)
    {
        // Fails non-alphanumeric characters except underscores
        if (!std::isalnum(static_cast<unsigned char>(character)) && character != '_')
            return false;
    }

    return true;
}

bool enzo::Path::isValidFormatting(const std::string& pathString)
{
    if (pathString.empty())
        return false;

    if (pathString == "/")
        return true;

    // Create string_view to avoid modifying original string
    std::string_view pathView(pathString);

    // Strip root delimeter
    if (pathView.front() == '/')
        pathView.remove_prefix(1);

    std::string_view remaining = pathView;
    while (!remaining.empty())
    {
        size_t slashPos = remaining.find('/');
        std::string_view pathComponent = remaining.substr(0, slashPos);

        if (!isValidName(std::string(pathComponent)))
            return false;

        if (slashPos == std::string_view::npos)
            break;

        remaining = remaining.substr(slashPos + 1);
    }

    return true;
}

bool enzo::Path::isValid() const
{
    // Put all of the validations in here
    return isValidFormatting(path_);
}

std::string enzo::Path::getName() const
{
    if (isEmpty() || isRoot())
        return "";

    size_t slashPos = path_.rfind('/');

    if (slashPos == std::string::npos)
        return path_;

    return path_.substr(slashPos + 1);
}

enzo::Path enzo::Path::getParent() const
{
    if (isEmpty() || isRoot())
        return *this;

    size_t slashPos = path_.rfind('/');

    if (slashPos == std::string::npos)
        return Path();

    if (slashPos == 0)
        return Path("/");

    return Path(path_.substr(0, slashPos));
}

std::vector<std::string> enzo::Path::split() const
{
    std::vector<std::string> components;

    if (isEmpty() || isRoot())
        return components;

    std::string_view view(path_);

    if (view.front() == '/')
        view.remove_prefix(1);

    while (!view.empty())
    {
        size_t slashPos = view.find('/');
        components.emplace_back(view.substr(0, slashPos));

        if (slashPos == std::string::npos)
            break;

        view.remove_prefix(slashPos + 1);
    }

    return components;
}

std::vector<enzo::Path> enzo::Path::getPrefixes() const
{
    std::vector<enzo::Path> prefixes;

    if (isEmpty() || isRoot())
        return prefixes;

    std::vector<std::string> components = split();

    std::string currentPrefix = isAbsolute() ? "" : "";

    for (size_t i = 0; i < components.size() - 1; i++)
    {
        if (isAbsolute())
            currentPrefix += "/" + components[i];
        else
            currentPrefix += (i == 0 ? "" : "/") + components[i];

        prefixes.emplace_back(enzo::Path(currentPrefix));
    }

    return prefixes;
}

const std::string& enzo::Path::getString() const
{
    return path_;
}

enzo::Path enzo::Path::join(std::string name) const
{
    if (!isValidName(name))
        return *this;

    if (isEmpty())
        return enzo::Path(name);

    return Path(path_ + "/" + name);
}

enzo::Path enzo::Path::joinPath(const enzo::Path& path) const
{
    if (path.isEmpty())
        return *this;

    if (isEmpty())
        return path;

    std::string appendedPath = path.isAbsolute() ? path.getString().substr(1) : path.getString();

    return enzo::Path(path_ + "/" + appendedPath);
}

enzo::Path enzo::Path::increment(int increment) const
{
    if (isEmpty() || isRoot())
        return *this;

    std::string name = getName();
    enzo::Path parent = getParent();

    // Find where numerical suffix starts
    size_t digitStartPos = name.size();
    while (digitStartPos > 0 && std::isdigit(static_cast<unsigned char>(name[digitStartPos - 1])))
        digitStartPos--;

    std::string base = name.substr(0, digitStartPos);
    std::string suffix = name.substr(digitStartPos);

    int number = suffix.empty() ? 0 : std::stoi(suffix);
    std::string incrementedName = base + std::to_string(number + increment);

    return parent.join(incrementedName);
}

enzo::Path enzo::Path::makeRelative() const
{
    if (isEmpty() || isRoot() || isRelative())
        return *this;

    return enzo::Path(getString().substr(1));
}

enzo::Path enzo::Path::makeAbsolute() const
{
    if (isEmpty() || isRoot() || isAbsolute())
        return *this;

    return enzo::Path("/" + getString());
}

enzo::Path enzo::Path::makeRelativeTo(const enzo::Path& anchor) const
{
    if (!isAbsolute() || !anchor.isAbsolute() || !hasPrefix(anchor))
        return *this;

    // in the off chance we need to account for the anchor being the same as the path
    if (path_ == anchor.getString())
        return Path(); 

    std::string anchorStr = anchor.isRoot() ? "/" : anchor.getString() + "/";
    std::string relative = path_.substr(anchorStr.size());

    return enzo::Path(relative);
}

bool enzo::Path::hasPrefix(const Path& prefix) const
{
    if (prefix.isEmpty())
        return false;

    if (prefix.isRoot())
        return isAbsolute();

    if (path_ == prefix.getString())
        return true;

    return path_.compare(0, prefix.getString().size() + 1, prefix.getString() + "/") == 0;
}

bool enzo::Path::operator==(const Path& other) const
{
    return path_ == other.getString();
}

bool enzo::Path::operator!=(const Path& other) const
{
    return path_ != other.getString();
}

std::ostream& enzo::operator<<(std::ostream& os, const enzo::Path& other)
{
    return os << other.getString();
}
