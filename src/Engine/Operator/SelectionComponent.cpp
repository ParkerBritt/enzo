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
    enzo::ga::Index index;
    while (stream >> index) {
        indices.insert(index);
    }
    return std::make_shared<enzo::ExplicitIndexSet>(std::move(indices));
}
} // namespace

enzo::SelectionComponent enzo::SelectionComponent::fromString(std::string_view string) {
    enzo::SelectionComponent component;

    size_t i = 0;
    while (i < string.size() && std::isspace(static_cast<unsigned char>(string[i]))) ++i;

    size_t pathStart = i;
    while (i < string.size() && !std::isspace(static_cast<unsigned char>(string[i]))) ++i;
    component.primPath_ = std::string(string.substr(pathStart, i - pathStart));

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
    return prim.getPath() == primPath_;
}

bool enzo::SelectionComponent::containsFace(const geo::Primitive& prim, ga::Index index) const {
    if (!containsPrim(prim)) return false;
    if (!faces_) return false;
    return faces_->contains(index);
}
