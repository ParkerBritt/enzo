#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Attribute/AttributeNames.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeOperation.h"
#include "Engine/Network/NodeImpl.h"
#include "Engine/Network/NodeRegistry.h"
#include "Engine/Primitives/Mesh.h"
#include <FastNoise/FastNoise.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace {

/// @brief Returns a new generator of this type.
template <typename T> FastNoise::SmartNode<> newGenerator()
{
    auto generator = FastNoise::New<T>();
    // Keeps the noise in world units so frequency alone sets the feature size.
    generator->SetScale(1);
    return generator;
}

/// @brief Returns the generator the noise type names, or nothing when it names none.
FastNoise::SmartNode<> getGenerator(const enzo::String& noiseType)
{
    if (noiseType == "simplex") return newGenerator<FastNoise::Simplex>();
    if (noiseType == "perlin") return newGenerator<FastNoise::Perlin>();
    if (noiseType == "value") return newGenerator<FastNoise::Value>();
    if (noiseType == "cellular") return newGenerator<FastNoise::CellularValue>();
    if (noiseType == "worley") return newGenerator<FastNoise::CellularDistance>();
    return {};
}

/// @brief Returns a fractal of this type over the source noise.
template <typename T>
FastNoise::SmartNode<> newFractal(
    const FastNoise::SmartNode<>& source,
    const int octaves,
    const float lacunarity,
    const float gain,
    const float weightedStrength
)
{
    auto fractal = FastNoise::New<T>();
    fractal->SetSource(source);
    fractal->SetOctaveCount(octaves);
    fractal->SetLacunarity(lacunarity);
    fractal->SetGain(gain);
    fractal->SetWeightedStrength(weightedStrength);
    return fractal;
}

/// @brief Returns the largest value a fractal can reach from source noise between
/// minus one and one.
///
/// @note 3 octaves at a gain of 0.5 give 1 + 0.5 + 0.25 = 1.75.
float getFractalBound(const int octaves, const float gain)
{
    float bound = 0;
    float octaveAmplitude = 1;
    for (int octave = 0; octave < octaves; ++octave)
    {
        bound += octaveAmplitude;
        octaveAmplitude *= std::abs(gain);
    }
    return std::max(bound, 1.0f);
}

/// @brief Returns the position of every element of the owner, in offset order.
///
/// @note A vertex sits at its point, and a face or primitive sits at the origin.
/// @todo Consider moving this to a virtual inside the primitives
std::vector<enzo::Vector3>
getElementPositions(const enzo::geo::Primitive& prim, const enzo::attr::AttributeOwner owner)
{
    using namespace enzo;

    const auto* mesh = dynamic_cast<const geo::Mesh*>(&prim);
    const size_t elementCount = prim.getElementCount(owner);

    if (mesh && owner == attr::AttributeOwner::POINT)
    {
        const std::span<const Vector3> pointPositions = mesh->getPointPositions();
        return std::vector<Vector3>(pointPositions.begin(), pointPositions.end());
    }

    if (mesh && owner == attr::AttributeOwner::VERTEX)
    {
        std::vector<Vector3> vertexPositions(elementCount, Vector3::Zero());
        for (Offset vertexOffset = 0; vertexOffset < elementCount; ++vertexOffset)
        {
            if (!mesh->isValidVertex(vertexOffset)) continue;
            vertexPositions[vertexOffset] = mesh->getPosFromVert(vertexOffset);
        }
        return vertexPositions;
    }

    return std::vector<Vector3>(elementCount, Vector3::Zero());
}

/// @brief Returns the noise value at every position, in the same order.
std::vector<float> sampleNoise(
    const FastNoise::Generator& generator,
    const std::span<const enzo::Vector3> positions,
    const enzo::floatT scale,
    const enzo::floatT frequency,
    const enzo::Vector3& offset,
    const int seed
)
{
    using namespace enzo;

    const size_t positionCount = positions.size();

    std::vector<float> xPositions(positionCount);
    std::vector<float> yPositions(positionCount);
    std::vector<float> zPositions(positionCount);
    for (size_t positionIndex = 0; positionIndex < positionCount; ++positionIndex)
    {
        const Vector3 samplePosition = positions[positionIndex] * frequency + offset;
        xPositions[positionIndex] = samplePosition.x();
        yPositions[positionIndex] = samplePosition.y();
        zPositions[positionIndex] = samplePosition.z();
    }

    std::vector<float> noise(positionCount);
    generator.GenPositionArray3D(
        noise.data(),
        static_cast<int>(positionCount),
        xPositions.data(),
        yPositions.data(),
        zPositions.data(),
        0,
        0,
        0,
        seed
    );
    for (float& value : noise)
        value *= scale;
    return noise;
}

class AttributeNoise : public enzo::nt::NodeImpl
{
  public:
    using NodeImpl::NodeImpl;

    void cook() override;
};

void AttributeNoise::cook()
{
    using namespace enzo;

    if (!outputRequested(0)) return;

    const std::optional<attr::AttributeOwner> owner = attr::getOwner(evalParmString("owner"));
    if (!owner)
    {
        throwError("Unknown owner.");
        return;
    }
    const String ownerName = attr::getOwnerName(*owner);

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

    const FastNoise::SmartNode<> source = getGenerator(evalParmString("noiseType"));
    if (!source)
    {
        throwError("Unknown noise type.");
        return;
    }

    const String fractalType = evalParmString("fractalType");
    const int octaves = static_cast<int>(evalParmInt("octaves"));
    const floatT lacunarity = evalParmFloat("lacunarity");
    const floatT gain = evalParmFloat("gain");
    const floatT weightedStrength = evalParmFloat("weightedStrength");

    FastNoise::SmartNode<> generator = source;
    if (fractalType == "fbm")
        generator =
            newFractal<FastNoise::FractalFBm>(source, octaves, lacunarity, gain, weightedStrength);
    else if (fractalType == "ridged")
        generator = newFractal<FastNoise::FractalRidged>(
            source,
            octaves,
            lacunarity,
            gain,
            weightedStrength
        );
    else if (fractalType != "none")
    {
        throwError("Unknown fractal type.");
        return;
    }

    // Divides out the octaves stacking up so amplitude means the same with or without a fractal.
    const floatT fractalBound = fractalType == "none" ? 1.0f : getFractalBound(octaves, gain);

    const bool alongVector = isVector && evalParmBool("alongVector");
    const String directionName = evalParmString("alongVectorAttribute");

    const floatT amplitude = evalParmFloat("amplitude");
    const floatT noiseScale = amplitude / fractalBound;
    const floatT frequency = evalParmFloat("frequency");
    const Vector3 offset = evalParmVector3("offset");
    const int seed = static_cast<int>(evalParmInt("seed"));

    NodePacket packet = cloneInputPacket(0);

    for (const geo::PrimPtr& prim : packet.getPrimitives())
    {
        if (!prim->hasOwner(*owner)) continue;
        const auto mesh = std::dynamic_pointer_cast<geo::Mesh>(prim);

        if (!prim->tryAddAttribute(*owner, attributeName, *attributeType))
        {
            throwError("The attribute " + attributeName + " can't be written as this type.");
            return;
        }

        const std::vector<Vector3> positions = getElementPositions(*prim, *owner);
        const auto sampleNoiseAtPositions = [&](const int sampleSeed) {
            return sampleNoise(*generator, positions, noiseScale, frequency, offset, sampleSeed);
        };

        if (isFloat)
        {
            const std::vector<float> noise = sampleNoiseAtPositions(seed);

            utils::applyOperation(
                *operation, *prim->getWriteHandle<floatT>(*owner, attributeName, true), noise
            );
            continue;
        }

        // Fills the noise for every element before writing any, since the noise can read
        // the attribute being written.
        std::vector<Vector3> noiseVectors;

        if (alongVector)
        {
            const std::shared_ptr<const attr::Attribute> directionAttribute =
                prim->getAttribByName(*owner, directionName, true);
            const bool hasComputedNormals = mesh && *owner != attr::AttributeOwner::PRIMITIVE;
            const bool alongNormals = directionName == attr::names::normal && hasComputedNormals;
            if (!directionAttribute && !alongNormals)
            {
                throwError(
                    "Noise along vector needs a " + ownerName + " attribute named " +
                    directionName + "."
                );
                return;
            }
            if (directionAttribute && directionAttribute->getType() != attr::AttributeType::vectorT)
            {
                throwError(
                    "Noise along vector needs " + directionName + " to be a vector attribute."
                );
                return;
            }

            const std::vector<float> noise = sampleNoiseAtPositions(seed);

            noiseVectors.resize(noise.size());
            const auto scaleNoiseAlong = [&](const auto& directions) {
                for (size_t elementOffset = 0; elementOffset < noise.size(); ++elementOffset)
                    noiseVectors[elementOffset] = directions[elementOffset] * noise[elementOffset];
            };

            if (directionAttribute)
                scaleNoiseAlong(attr::AttributeHandleRO<Vector3>(directionAttribute));
            else if (*owner == attr::AttributeOwner::POINT)
                scaleNoiseAlong(mesh->getPointNormal());
            else if (*owner == attr::AttributeOwner::VERTEX)
                scaleNoiseAlong(mesh->getVertexNormal());
            else
                scaleNoiseAlong(mesh->getFaceNormal());
        }
        else
        {
            const std::vector<float> xNoise = sampleNoiseAtPositions(seed);
            const std::vector<float> yNoise = sampleNoiseAtPositions(seed + 1);
            const std::vector<float> zNoise = sampleNoiseAtPositions(seed + 2);

            noiseVectors.resize(xNoise.size());
            for (size_t elementOffset = 0; elementOffset < xNoise.size(); ++elementOffset)
                noiseVectors[elementOffset] =
                    Vector3(xNoise[elementOffset], yNoise[elementOffset], zNoise[elementOffset]);
        }

        utils::applyOperation(
            *operation, *prim->getWriteHandle<Vector3>(*owner, attributeName, true), noiseVectors
        );
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(attributeNoise, AttributeNoise)
