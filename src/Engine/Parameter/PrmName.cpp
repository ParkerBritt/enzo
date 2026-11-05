#include "Engine/Parameter/PrmName.h"

namespace enzo {

prm::Name::Name(String token, String label, String icon)
    : token_{token}, label_{label}, icon_{icon}
{
}

prm::Name::Name(String token) : token_{token}, label_{token} {}

prm::Name::Name() : token_{""}, label_{""} {}

String prm::Name::getToken() const { return token_; }

String prm::Name::getLabel() const { return label_; }

String prm::Name::getIcon() const { return icon_; }

} // namespace enzo
