#include "SelectionComponent.h"
#include "IndexSet.h"
#include <cctype>
#include <set>
#include <sstream>

namespace {
std::shared_ptr<enzo::IndexSet> parseBlockContent(std::string_view content) {
    // TODO: trim whitespace before checking for wildcard
    if (content == "*") {
        return std::make_shared<enzo::WildcardIndexSet>();
    }
    std::set<enzo::ga::Index> indices;
    std::stringstream stream{std::string(content)};
    std::string token;
    while (stream >> token) {
        // Range token "low-high" expands inclusively into the index set
        size_t dash = token.find('-');
        if (dash != std::string::npos && dash > 0 && dash + 1 < token.size()) {
            enzo::ga::Index low = std::stoull(token.substr(0, dash));
            enzo::ga::Index high = std::stoull(token.substr(dash + 1));
            if (low > high) std::swap(low, high);
            for (enzo::ga::Index i = low; i <= high; ++i) {
                indices.insert(i);
            }
            continue;
        }
        // Plain single index
        indices.insert(std::stoull(token));
    }
    return std::make_shared<enzo::ExplicitIndexSet>(std::move(indices));
}
} // namespace

enzo::SelectionComponent enzo::SelectionComponent::fromString(std::string_view string) {
    enzo::SelectionComponent component;

    size_t i = 0;
    while (i < string.size() && std::isspace(static_cast<unsigned char>(string[i]))) ++i;

    // A leading "<letter>{" is a component selector (e.g. "p{0}"), not a path.
    bool startsWithSelector =
        i + 1 < string.size() &&
        std::isalpha(static_cast<unsigned char>(string[i])) &&
        string[i + 1] == '{';
    if (!startsWithSelector) {
        size_t pathStart = i;
        while (i < string.size() && !std::isspace(static_cast<unsigned char>(string[i]))) ++i;
        component.primPath_ = std::string(string.substr(pathStart, i - pathStart));
    }

    while (i < string.size()) {
        while (i < string.size() && std::isspace(static_cast<unsigned char>(string[i]))) ++i;
        if (i >= string.size()) break;

        char type = string[i++];
        if (i >= string.size() || string[i] != '{') break;
        ++i;

        size_t contentStart = i;
        while (i < string.size() && string[i] != '}') ++i;
        std::string_view content = string.substr(contentStart, i - contentStart);
        if (i < string.size()) ++i;

        auto indexSet = parseBlockContent(content);
        if (type == 'f') {
            component.faces_ = indexSet;
        } else if (type == 'p') {
            component.points_ = indexSet;
        } else if (type == 'v') {
            component.vertices_ = indexSet;
        }
    }

    return component;
}

bool enzo::SelectionComponent::containsPrim(const geo::Primitive& prim) const {
    // A pathless component applies to every prim. With no selectors this is the
    // implicit "everything" component produced by an empty selection string.
    if (primPath_.empty()) return true;
    return prim.getPath() == primPath_;
}

bool enzo::SelectionComponent::containsFace(const geo::Primitive& prim, ga::Index index, bool inverted) const {
    if (!containsPrim(prim)) return false;
    // Whole-prim selection (no blocks): every face is implicitly included.
    if (isWholePrim()) return !inverted;
    // Face-less partial selection: out of face scope, inversion doesn't expand it.
    if (!faces_) return false;
    bool in = faces_->contains(index);
    return inverted ? !in : in;
}

bool enzo::SelectionComponent::containsPoint(const geo::Primitive& prim, ga::Index index, bool inverted) const {
    if (!containsPrim(prim)) return false;
    if (isWholePrim()) return !inverted;
    if (!points_) return false;
    bool in = points_->contains(index);
    return inverted ? !in : in;
}

bool enzo::SelectionComponent::containsVertex(const geo::Primitive& prim, ga::Index index, bool inverted) const {
    if (!containsPrim(prim)) return false;
    if (isWholePrim()) return !inverted;
    if (!vertices_) return false;
    bool in = vertices_->contains(index);
    return inverted ? !in : in;
}

bool enzo::SelectionComponent::isWholePrim() const {
    return !faces_ && !points_ && !vertices_;
}
