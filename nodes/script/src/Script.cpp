#include "Engine/Core/Types.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Expression/PointScript.h"
#include "Engine/Expression/ScriptEngine.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <memory>
#include <mutex>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>

namespace {

// The number of points one thread takes at a time. A whole multiple of 64 keeps
// two threads off the same word of a bool attribute.
constexpr enzo::Offset pointsPerChunk = 1024;

static_assert(pointsPerChunk % 64 == 0);

/// @brief Runs a compiled script over every point of every mesh in the packet, in parallel.
///
/// @return The first error a mesh or a point produced, or an empty string.
enzo::String runOverPoints(
    const enzo::expr::PointScript& script,
    enzo::NodePacket& packet,
    const enzo::expr::ExpressionContext& context
)
{
    using namespace enzo;

    // Clones the script for each thread, since one script runs a single point at a time.
    tbb::enumerable_thread_specific<std::shared_ptr<expr::PointScript>> threadScripts(
        [&script] { return script.clone(); }
    );

    std::mutex errorMutex;
    String error;

    for (const geo::PrimPtr& prim : packet.getPrimitives())
    {
        if (prim->getType() != geo::PrimType::MESH) continue;
        const auto mesh = std::static_pointer_cast<geo::Mesh>(prim);

        if (!script.addWrittenAttributes(*mesh, error)) return error;

        const Offset pointCount = mesh->getNumPoints();
        const Offset chunkCount = (pointCount + pointsPerChunk - 1) / pointsPerChunk;

        tbb::parallel_for(Offset(0), chunkCount, [&](const Offset chunk) {
            const std::shared_ptr<expr::PointScript>& threadScript = threadScripts.local();

            String chunkError;
            if (!threadScript)
                chunkError = "The script's globals failed to initialise.";
            else
            {
                const Offset begin = chunk * pointsPerChunk;
                const Offset end = std::min(begin + pointsPerChunk, pointCount);
                threadScript->run(*mesh, *mesh, begin, end, &context, chunkError);
            }

            if (chunkError.empty()) return;

            std::lock_guard lock(errorMutex);
            if (error.empty()) error = chunkError;
        });

        if (!error.empty()) return error;
    }

    return error;
}

class Script : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void Script::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const String code = evalParmString("code");

    NodePacket packet = cloneInputPacket(0);

    if (code.empty())
    {
        setOutputPacket(0, packet);
        return;
    }

    String error;
    const std::shared_ptr<const expr::PointScript> script =
        expr::ScriptEngine::instance().compile(code, error);
    if (!script)
    {
        throwError(error);
        return;
    }

    // Shares one context across the cook, so a parameter the script reads is
    // evaluated and recorded as a dependency once.
    expr::ExpressionContext context(getNodeId());
    error = runOverPoints(*script, packet, context);

    // Records what the script read even when it failed, so changing one of those
    // parameters cooks the node again.
    expr::submitExpressionDependencies(context);

    if (!error.empty())
    {
        throwError(error);
        return;
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(script, Script)
