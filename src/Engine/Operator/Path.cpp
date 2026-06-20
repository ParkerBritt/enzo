#include "Path.h"
#include <cctype>
#include <string_view>
#include <utility>

enzo::Path::Path()
    : path_("")
{
}

enzo::Path::Path(const std::string& path)
{
    // Trim surrounding whitespace before validating so checks like isRoot are not
    // thrown off by padding such as " / "
    std::string stripped = strip(path);

    if (!isValidFormatting(stripped))
    {
        path_ = "";
        return;
    }

    path_ = std::move(stripped);
}

enzo::Path::Path(const char* path)
    : Path(std::string(path))
{
}

std::string enzo::Path::strip(const std::string& str)
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

    // Walk the components between the slashes and validate each one
    // e.g. "foo/bar/baz" checks "foo" then "bar" then "baz"
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

    std::string currentPrefix;
    for (size_t i = 0; i + 1 < components.size(); i++)
    {
        // Delimiter is "/" except before the first component of a relative path
        std::string delimiter = (isAbsolute() || i != 0) ? "/" : "";
        currentPrefix += delimiter + components[i];
        prefixes.emplace_back(currentPrefix);
    }

    return prefixes;
}

const std::string& enzo::Path::getString() const
{
    return path_;
}

enzo::Path enzo::Path::append(const enzo::Path& path) const
{
    if (path.isEmpty())
        return *this;

    if (isEmpty())
        return path;

    // Drop the appended path's root delimiter so it joins as a relative segment
    std::string appendedPath = path.isAbsolute() ? path.getString().substr(1) : path.getString();

    return enzo::Path(path_ + "/" + appendedPath);
}

enzo::Path enzo::Path::increment(int increment) const
{
    if (isEmpty() || isRoot())
        return *this;

    std::string name = getName();
    enzo::Path parent = getParent();

    // Find where the numerical suffix starts
    size_t digitStartPos = name.size();
    while (digitStartPos > 0 && std::isdigit(static_cast<unsigned char>(name[digitStartPos - 1])))
        digitStartPos--;

    std::string base = name.substr(0, digitStartPos);
    std::string suffix = name.substr(digitStartPos);

    int number = suffix.empty() ? 0 : std::stoi(suffix);
    std::string incrementedName = base + std::to_string(number + increment);

    return parent.append(enzo::Path(incrementedName));
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
    // Only paths that share the anchor as a prefix can be made relative to it
    if (!(isAbsolute() && anchor.isAbsolute() && hasPrefix(anchor)))
        return *this;

    // Account for the anchor being the same as the path
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

enzo::Path::operator std::string_view() const
{
    return path_;
}

bool enzo::Path::operator==(std::string_view other) const
{
    return path_ == other;
}

bool enzo::Path::operator!=(std::string_view other) const
{
    return path_ != other;
}

std::ostream& enzo::operator<<(std::ostream& os, const enzo::Path& other)
{
    return os << other.getString();
}
