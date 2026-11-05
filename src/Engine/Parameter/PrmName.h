#pragma once

#include "Engine/Core/Types.h"
namespace enzo::prm {
class Name
{
  public:
    Name(String token, String label, String icon = {});
    Name(String token);
    Name();

    String getToken() const;
    String getLabel() const;

    /// @brief Returns the Lucide icon name shown beside the label.
    /// @return The name, empty when there is no icon.
    String getIcon() const;

  private:
    String token_;
    String label_;
    String icon_;
};

} // namespace enzo::prm
