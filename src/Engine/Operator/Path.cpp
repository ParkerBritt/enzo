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
    if (!IsValidFormatting(path))
    {
        // NOTE: Need to put something better here for error handling
        path_ = "";
        return;
    }

    path_ = path;
}

enzo::Path::Path(const char* path)
{
    std::string pathString(path);

    if (!IsValidFormatting(pathString))
    {
        // NOTE: Will replace this with proper error handling
        path_ = "";
        return;
    }

    path_ = std::move(pathString);
}

bool enzo::Path::IsEmpty() const
{
    return path_.empty();
}

bool enzo::Path::IsRoot() const
{
    return path_ == "/";
}

bool enzo::Path::IsAbsolute() const
{
    return !IsEmpty() && path_.front() == '/';
}

bool enzo::Path::IsRelative() const
{
    return !IsEmpty() && path_.front() != '/';
}

bool enzo::Path::IsValidName(const std::string& name)
{
    if (name.empty())
        return false;

    for (const char character : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(character)) && character != '_')
            return false;
    }

    return true;
}

bool enzo::Path::IsValidFormatting(const std::string& path)
{
    if (path.empty())
        return false;

    if (path == "/")
        return true;

    std::string_view pathView(path);

    if (pathView.front() == '/')
        pathView.remove_prefix(1);

    std::string_view remaining = pathView;
    while (!remaining.empty())
    {
        size_t slash = remaining.find('/');
        std::string_view pathComponent = remaining.substr(0, slash);

        if (!IsValidName(std::string(pathComponent)))
            return false;

        if (slash == std::string_view::npos)
            break;

        remaining = remaining.substr(slash + 1);

    }

    return true;
}

bool enzo::Path::IsValid() const
{
    // Put all of the validations in here
    return IsValidFormatting(path_);
}

std::string enzo::Path::GetName() const
{
    if (IsEmpty() || IsRoot())
        return "";

    size_t slash = path_.rfind('/');

    if (slash == std::string::npos)
        return path_;

    return path_.substr(slash + 1);
}

enzo::Path enzo::Path::GetParentPath() const
{
    if (IsEmpty() || IsRoot())
        return *this;

    size_t slash = path_.rfind('/');

    if (slash == std::string::npos)
        return Path();

    if (slash == 0)
        return Path("/");

    return Path(path_.substr(0, slash));
}

std::vector<std::string> enzo::Path::SplitPath() const
{
    std::vector<std::string> components;

    if (IsEmpty() || IsRoot())
        return components;

    std::string_view view(path_);

    if (view.front() == '/')
        view.remove_prefix(1);

    while (!view.empty())
    {
        size_t slash = view.find('/');
        components.emplace_back(view.substr(0, slash));

        if (slash == std::string::npos)
            break;

        view.remove_prefix(slash + 1);
    }

    return components;
}

std::vector<enzo::Path> enzo::Path::GetPrefixes() const
{
    std::vector<enzo::Path> prefixes;

    if (IsEmpty() || IsRoot())
        return prefixes;

    std::vector<std::string> components = SplitPath();

    std::string currentPrefix = IsAbsolute() ? "" : "";

    for (size_t i = 0; i < components.size() - 1; i++)
    {
        if (IsAbsolute())
            currentPrefix += "/" + components[i];
        else
            currentPrefix += (i == 0 ? "" : "/") + components[i];

        prefixes.emplace_back(enzo::Path(currentPrefix));
    }

    return prefixes;
}

const std::string& enzo::Path::GetString() const
{
    return path_;
}

enzo::Path enzo::Path::AppendChild(std::string name) const
{
    if (!IsValidName(name))
        return *this;

    if (IsEmpty())
        return enzo::Path(name);

    return Path(path_ + "/" + name);
}

enzo::Path enzo::Path::AppendPath(const enzo::Path& path) const
{
    if (path.IsEmpty())
        return *this;

    if (IsEmpty())
        return path;

    std::string appendedPath = path.IsAbsolute() ? path.GetString().substr(1) : path.GetString();

    return enzo::Path(path_ + "/" + appendedPath);
}

enzo::Path enzo::Path::IncrementName() const
{
    // TODO: Improve variable names
    if (IsEmpty() || IsRoot())
        return *this;

    std::string name = GetName();
    enzo::Path parent = GetParentPath();

    size_t i = name.size();
    while (i > 0 && std::isdigit(static_cast<unsigned char>(name[i - 1])))
        i--;

    std::string base = name.substr(0, i);
    std::string suffix = name.substr(i);

    int number = suffix.empty() ? 0 : std::stoi(suffix);
    std::string incrementedName = base + std::to_string(number + 1);

    return parent.AppendChild(incrementedName);
}

enzo::Path enzo::Path::MakeRelative() const
{
    if (IsEmpty() || IsRoot())
        return *this;

    if (IsRelative())
        return *this;

    return enzo::Path(GetString().substr(1));
}

enzo::Path enzo::Path::MakeAbsolute() const
{
    if (IsEmpty() || IsRoot())
        return *this;

    if (IsAbsolute())
        return *this;

    return enzo::Path("/" + GetString());
}

enzo::Path enzo::Path::MakeRelativeTo(const enzo::Path& anchor) const
{
    if (!IsAbsolute() || !anchor.IsAbsolute())
        return *this;

    if (!HasPrefix(anchor))
        return *this;

    // in the off chance we need to account for the anchor being the same as the path
    if (path_ == anchor.GetString())
        return Path(); 

    std::string anchorStr = anchor.IsRoot() ? "/" : anchor.GetString() + "/";
    std::string relative = path_.substr(anchorStr.size());

    return enzo::Path(relative);
}

bool enzo::Path::HasPrefix(const Path& prefix) const
{
    if (prefix.IsEmpty())
        return false;

    if (prefix.IsRoot())
        return IsAbsolute();

    if (path_ == prefix.GetString())
        return true;

    return path_.compare(0, prefix.GetString().size() + 1, prefix.GetString() + "/") == 0;
}

bool enzo::Path::operator==(const Path& other) const
{
    return path_ == other.GetString();
}

bool enzo::Path::operator!=(const Path& other) const
{
    return path_ != other.GetString();
}

std::ostream& enzo::operator<<(std::ostream& os, const enzo::Path& other)
{
    return os << other.GetString();
}
