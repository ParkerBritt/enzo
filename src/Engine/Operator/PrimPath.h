#pragma once
#include <string>
#include <vector>

namespace enzo {

class PrimPath
{
public:
    static constexpr char Delimiter = '/';

    PrimPath() = default;
    explicit PrimPath(const std::string &path);

    const std::string &string() const { return path_; }
    const std::vector<std::string> &components() const { return components_; }
    std::string name() const;
    PrimPath parent() const;
    bool isRoot() const { return components_.empty(); }
    bool empty() const { return path_.empty(); }

private:
    std::string path_;
    std::vector<std::string> components_;
};

} // namespace enzo
