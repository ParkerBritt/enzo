#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeOperation.h"
#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Parameter/Ramp.h"
#include "Engine/Primitives/Mesh.h"
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <vector>

namespace {

/// @brief The settings that shape the wave.
struct WaveSettings
{
    bool radial = false;
    enzo::Vector3 center = enzo::Vector3::Zero();
    enzo::floatT frequency = 1;
    enzo::Vector3 offset = enzo::Vector3::Zero();
    enzo::floatT amplitude = 1;
    bool zeroCentered = true;
};

/// @brief Returns the wave value at every point of the mesh, in point order.
std::vector<enzo::floatT>
sampleWave(const enzo::geo::Mesh& mesh, const WaveSettings& settings, const enzo::prm::Ramp& remap)
{
    using namespace enzo;

    const std::span<const Vector3> positions = mesh.pointPosSpan();
    std::vector<floatT> wave(positions.size());

    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, positions.size()),
        [&positions, &wave, &settings, &remap](const tbb::blocked_range<size_t>& range) {
            for (size_t pointOffset = range.begin(); pointOffset != range.end(); ++pointOffset)
            {
                const Vector3 position = positions[pointOffset] + settings.offset;
                const floatT distance =
                    settings.radial ? (position - settings.center).norm() : position.x();
                const floatT sine = std::sin(distance * settings.frequency);

                // Maps the sine into the ramp's zero to one domain.
                const floatT remapped = remap.sample((sine + 1) * 0.5f);
                const floatT ranged = settings.zeroCentered ? remapped * 2 - 1 : remapped;
                wave[pointOffset] = ranged * settings.amplitude;
            }
        }
    );
    return wave;
}

class SineWave : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void SineWave::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const String attributeName = evalParmString("name");
    if (attributeName.empty())
    {
        throwError("The attribute needs a name.");
        return;
    }

    const std::optional<attr::AttributeType> attributeType = attr::getType(evalParmString("type"));
    const bool isFloat = attributeType == attr::AttributeType::floatT;
    const bool isVector = attributeType == attr::AttributeType::vectorT;
    if (!isFloat && !isVector)
    {
        throwError("Unknown attribute type.");
        return;
    }

    const std::optional<utils::AttributeOperation> operation =
        utils::getAttributeOperation(evalParmString("operation"));
    if (!operation)
    {
        throwError("Unknown operation.");
        return;
    }

    const String range = evalParmString("range");
    if (range != "positive" && range != "zeroCentered")
    {
        throwError("Unknown range.");
        return;
    }

    WaveSettings settings;
    settings.radial = evalParmBool("radial");
    settings.center = evalParmVector3("center");
    settings.frequency = evalParmFloat("frequency");
    settings.offset = evalParmVector3("offset");
    settings.amplitude = evalParmFloat("amplitude");
    settings.zeroCentered = range == "zeroCentered";
    const prm::Ramp remap = evalParmRamp("remap");

    const bool alongVector = isVector && evalParmBool("alongVector");
    const String directionName = evalParmString("alongVectorAttribute");

    NodePacket packet = cloneInputPacket(0);

    for (const geo::PrimPtr& prim : packet.getPrimitives())
    {
        if (prim->getType() != geo::PrimType::MESH) continue;
        const auto mesh = std::static_pointer_cast<geo::Mesh>(prim);

        // Samples the wave before writing, since the attribute being written may be P.
        const std::vector<floatT> wave = sampleWave(*mesh, settings, remap);

        // Finds the attribute including intrinsics so P can be written.
        std::shared_ptr<attr::Attribute> attribute =
            mesh->getAttribByName(attr::AttributeOwner::POINT, attributeName, true);
        const bool typeMatches = attribute && attribute->getType() == *attributeType;
        if (!typeMatches)
        {
            if (attribute && attribute->isIntrinsic())
            {
                throwError("The attribute " + attributeName + " can't change type.");
                return;
            }
            attribute =
                mesh->addAttribute(attr::AttributeOwner::POINT, attributeName, *attributeType);
        }

        if (isFloat)
        {
            utils::applyOperation(*operation, attr::AttributeHandle<floatT>(attribute), wave);
            continue;
        }

        // Builds every wave vector before writing, since the direction may be the attribute
        // being written.
        std::vector<Vector3> waveVectors(wave.size());

        if (alongVector)
        {
            const std::shared_ptr<attr::Attribute> directionAttribute =
                mesh->getAttribByName(attr::AttributeOwner::POINT, directionName, true);
            if (directionAttribute && directionAttribute->getType() != attr::AttributeType::vectorT)
            {
                throwError(
                    "Wave along vector needs " + directionName + " to be a vector attribute."
                );
                return;
            }

            std::vector<Vector3> directions;
            if (directionAttribute)
            {
                const std::span<const Vector3> directionSpan =
                    attr::AttributeHandle<Vector3>(directionAttribute).getSpan();
                directions.assign(directionSpan.begin(), directionSpan.end());
            }
            else
            {
                directions = utils::computePointNormals(*mesh);
            }

            for (size_t pointOffset = 0; pointOffset < wave.size(); ++pointOffset)
                waveVectors[pointOffset] = directions[pointOffset] * wave[pointOffset];
        }
        else
        {
            for (size_t pointOffset = 0; pointOffset < wave.size(); ++pointOffset)
                waveVectors[pointOffset] = Vector3::Constant(wave[pointOffset]);
        }

        utils::applyOperation(*operation, attr::AttributeHandle<Vector3>(attribute), waveVectors);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(sineWave, SineWave)
