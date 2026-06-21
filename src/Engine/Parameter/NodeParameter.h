#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Parameter.h"

namespace enzo::prm {

class NodeParameter : public Parameter
{
  public:
    NodeParameter(Template prmTemplate, enzo::nt::OpId opId);
    enzo::nt::OpId getOpId() const { return opId_; }

  protected:
    void onFloatSet_(const PrmValues& before) override;

    /// @brief Builds the context an expression runs from, e.g. what node the
    /// parameter belongs to.
    std::unique_ptr<expr::ExpressionContext> makeExpressionContext_() const override;

  private:
    void addUndo_(PrmValues before);

    enzo::nt::OpId opId_;
};
} // namespace enzo::prm
