#include "SelectionComponent.h"
#include "Engine/Operator/AttributeHandle.h"
#include "Engine/Utils/StringUtils.h"
#include "IndexSet.h"
#include <cctype>
#include <set>
#include <sstream>

namespace {
std::shared_ptr<enzo::IndexSet> parseBlockContent(std::string_view content) {
    if (enzo::utils::trim(content) == "*") {
        return std::make_shared<enzo::WildcardIndexSet>();
    }
    std::set<enzo::Index> indices;
    std::stringstream stream{std::string(content)};
    std::string token;
    while (stream >> token) {
        // Range token "low-high" expands inclusively into the index set
        size_t dash = token.find('-');
        if (dash != std::string::npos && dash > 0 && dash + 1 < token.size()) {
            enzo::Index low = std::stoull(token.substr(0, dash));
            enzo::Index high = std::stoull(token.substr(dash + 1));
            if (low > high)
                std::swap(low, high);
            for (enzo::Index i = low; i <= high; ++i) {
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

namespace enzo {

std::unique_ptr<SelectionComponent> SelectionComponent::fromString(std::string_view string) {
    std::string_view trimmed = utils::trim(string);
    if (trimmed.empty()) {
        return PathSelectionComponent::parse(trimmed);
    }

    // A leading "<letter>{" is a component selector (e.g. "p{0}")
    bool startsWithSelector = trimmed.size() >= 2 &&
                              std::isalpha(static_cast<unsigned char>(trimmed[0])) &&
                              trimmed[1] == '{';
    bool startsWithPath = trimmed[0] == '/';

    if (!startsWithSelector && !startsWithPath) {
        return fromGroup(trimmed);
    }
    return PathSelectionComponent::parse(trimmed);
}

std::unique_ptr<SelectionComponent> SelectionComponent::fromGroup(std::string_view name) {
    return GroupSelectionComponent::create(name);
}

// ---
// PathSelectionComponent
// ---

std::unique_ptr<PathSelectionComponent> PathSelectionComponent::parse(std::string_view string) {
    auto component = std::unique_ptr<PathSelectionComponent>(new PathSelectionComponent());

    size_t i = 0;
    while (i < string.size() && std::isspace(static_cast<unsigned char>(string[i])))
        ++i;

    // A leading "<letter>{" is a component selector, so there is no path
    bool startsWithSelector = i + 1 < string.size() &&
                              std::isalpha(static_cast<unsigned char>(string[i])) &&
                              string[i + 1] == '{';
    if (!startsWithSelector) {
        size_t pathStart = i;
        while (i < string.size() && !std::isspace(static_cast<unsigned char>(string[i])))
            ++i;
        component->primPath_ = std::string(string.substr(pathStart, i - pathStart));
    }

    while (i < string.size()) {
        while (i < string.size() && std::isspace(static_cast<unsigned char>(string[i])))
            ++i;
        if (i >= string.size())
            break;

        char type = string[i++];
        if (i >= string.size() || string[i] != '{')
            break;
        ++i;

        size_t contentStart = i;
        while (i < string.size() && string[i] != '}')
            ++i;
        std::string_view content = string.substr(contentStart, i - contentStart);
        if (i < string.size())
            ++i;

        auto indexSet = parseBlockContent(content);
        if (type == 'f') {
            component->faces_ = indexSet;
        } else if (type == 'p') {
            component->points_ = indexSet;
        } else if (type == 'v') {
            component->vertices_ = indexSet;
        }
    }

    return component;
}

bool PathSelectionComponent::containsPrim(const geo::Primitive &prim) const {
    // A pathless component applies to every prim
    if (primPath_.empty())
        return true;
    return prim.getPath() == primPath_;
}

bool PathSelectionComponent::containsFace(const geo::Primitive &prim, Index index,
                                          Offset /*offset*/, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    if (!faces_)
        return false;
    bool in = faces_->contains(index);
    return inverted ? !in : in;
}

bool PathSelectionComponent::containsPoint(const geo::Primitive &prim, Index index,
                                           Offset /*offset*/, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    if (!points_)
        return false;
    bool in = points_->contains(index);
    return inverted ? !in : in;
}

bool PathSelectionComponent::containsVertex(const geo::Primitive &prim, Index index,
                                            Offset /*offset*/, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    if (!vertices_)
        return false;
    bool in = vertices_->contains(index);
    return inverted ? !in : in;
}

bool PathSelectionComponent::isWholePrim(const geo::Primitive &) const {
    return !faces_ && !points_ && !vertices_;
}

// ---
// GroupSelectionComponent
// ---

namespace {
// Reads a group's bool at the given offset, false if the group is missing
bool isGroupMember(const geo::Primitive &prim, attr::AttrOwner owner, const std::string &name,
                   Offset offset) {
    auto group = prim.getGroupByName(owner, name);
    if (!group)
        return false;
    attr::AttributeHandleRO<boolT> handle(group);
    return handle.getValue(offset);
}
} // namespace

std::unique_ptr<GroupSelectionComponent> GroupSelectionComponent::create(std::string_view name) {
    auto component = std::unique_ptr<GroupSelectionComponent>(new GroupSelectionComponent());
    component->groupName_ = std::string(utils::trim(name));
    return component;
}

bool GroupSelectionComponent::containsPrim(const geo::Primitive &prim) const {
    // A primitive group with the flag set selects the whole prim
    if (auto primGroup = prim.getGroupByName(attr::AttrOwner::PRIMITIVE, groupName_)) {
        attr::AttributeHandleRO<boolT> handle(primGroup);
        return handle.getValue(0);
    }
    // Otherwise the prim is in scope if any other store has the group
    for (auto owner : {attr::AttrOwner::FACE, attr::AttrOwner::POINT, attr::AttrOwner::VERTEX}) {
        if (prim.getGroupByName(owner, groupName_))
            return true;
    }
    return false;
}

bool GroupSelectionComponent::containsFace(const geo::Primitive &prim, Index /*index*/,
                                           Offset offset, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    // The group must exist on this element type. Otherwise it says nothing about
    // faces and selects none, even when inverted.
    if (!prim.getGroupByName(attr::AttrOwner::FACE, groupName_))
        return false;
    bool in = isGroupMember(prim, attr::AttrOwner::FACE, groupName_, offset);
    return inverted ? !in : in;
}

bool GroupSelectionComponent::containsPoint(const geo::Primitive &prim, Index /*index*/,
                                            Offset offset, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    if (!prim.getGroupByName(attr::AttrOwner::POINT, groupName_))
        return false;
    bool in = isGroupMember(prim, attr::AttrOwner::POINT, groupName_, offset);
    return inverted ? !in : in;
}

bool GroupSelectionComponent::containsVertex(const geo::Primitive &prim, Index /*index*/,
                                             Offset offset, bool inverted) const {
    if (!containsPrim(prim))
        return false;
    if (isWholePrim(prim))
        return !inverted;
    if (!prim.getGroupByName(attr::AttrOwner::VERTEX, groupName_))
        return false;
    bool in = isGroupMember(prim, attr::AttrOwner::VERTEX, groupName_, offset);
    return inverted ? !in : in;
}

bool GroupSelectionComponent::isWholePrim(const geo::Primitive &prim) const {
    auto primGroup = prim.getGroupByName(attr::AttrOwner::PRIMITIVE, groupName_);
    if (!primGroup)
        return false;
    attr::AttributeHandleRO<boolT> handle(primGroup);
    return handle.getValue(0);
}

} // namespace enzo
