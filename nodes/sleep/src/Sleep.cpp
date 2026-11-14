#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include <chrono>
#include <thread>

namespace {

/// @brief A debug node that stalls its cook to stand in for a heavy node.
class Sleep : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Sleep::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const std::chrono::duration<double> duration(evalParmFloat("duration"));
    std::this_thread::sleep_for(duration);

    NodePacket packet = hasInput(0) ? cloneInputPacket(0) : NodePacket();
    setOutputPacket(0, std::move(packet));
}

} // namespace

ENZO_REGISTER_NODE(sleep, Sleep)
