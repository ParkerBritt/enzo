#pragma once
#include "Engine/Parameter/Parameter.h"
#include "Engine/Core/Types.h"

namespace enzo::prm {

class NodeParameter : public Parameter {
  public:
    NodeParameter(Template prmTemplate, enzo::nt::OpId opId);
    enzo::nt::OpId getOpId() const { return opId_; }

  protected:
    void onFloatSet_(const PrmValues &before) override;

  private:
    void addUndo_(PrmValues before);

    enzo::nt::OpId opId_;
};
} // namespace enzo::prm
