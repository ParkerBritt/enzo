#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/AttributeOperation.h"
#include "Engine/GeometryAlgorithms/Normals.h"
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

/// @brief Returns the generator the noise type and cellular mode name, or nothing
/// when they name none.
FastNoise::SmartNode<> getGenerator(const enzo::String& noiseType, const enzo::String& cellularMode)
{
    if (noiseType == "simplex") return newGenerator<FastNoise::Simplex>();
    if (noiseType == "superSimplex") return newGenerator<FastNoise::SuperSimplex>();
    if (noiseType == "perlin") return newGenerator<FastNoise::Perlin>();
    if (noiseType == "value") return newGenerator<FastNoise::Value>();
    if (noiseType == "cellular" && cellularMode == "value")
        return newGenerator<FastNoise::CellularValue>();
    if (noiseType == "cellular" && cellularMode == "distance")
        return newGenerator<FastNoise::CellularDistance>();
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

/// @brief Returns the noise value at every point of the mesh, in point order.
std::vector<float> sampleNoise(
    const FastNoise::Generator& generator,
    const enzo::geo::Mesh& mesh,
    const enzo::floatT scale,
    const enzo::floatT frequency,
    const enzo::Vector3& offset,
    const int seed
)
{
    using namespace enzo;

    const std::span<const Vector3> positions = mesh.pointPosSpan();
    const size_t pointCount = positions.size();

    std::vector<float> xPositions(pointCount);
    std::vector<float> yPositions(pointCount);
    std::vector<float> zPositions(pointCount);
    for (size_t pointOffset = 0; pointOffset < pointCount; ++pointOffset)
    {
        const Vector3 samplePosition = positions[pointOffset] * frequency + offset;
        xPositions[pointOffset] = samplePosition.x();
        yPositions[pointOffset] = samplePosition.y();
        zPositions[pointOffset] = samplePosition.z();
    }

    std::vector<float> noise(pointCount);
    generator.GenPositionArray3D(
        noise.data(),
        static_cast<int>(pointCount),
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

    const FastNoise::SmartNode<> source =
        getGenerator(evalParmString("noiseType"), evalParmString("cellularMode"));
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
        if (prim->getType() != geo::PrimType::MESH) continue;
        const auto mesh = std::static_pointer_cast<geo::Mesh>(prim);

        const std::shared_ptr<attr::Attribute> attribute =
            mesh->tryAddAttribute(attr::AttributeOwner::POINT, attributeName, *attributeType);
        if (!attribute)
        {
            throwError("The attribute " + attributeName + " can't be written as this type.");
            return;
        }

        if (isFloat)
        {
            const std::vector<float> noise =
                sampleNoise(*generator, *mesh, noiseScale, frequency, offset, seed);

            utils::applyOperation(*operation, attr::AttributeHandle<floatT>(attribute), noise);
            continue;
        }

        // The noise for every point. It is filled before any of it is written, since the
        // noise can be built from the attribute being written.
        std::vector<Vector3> noiseVectors;

        if (alongVector)
        {
            const std::shared_ptr<attr::Attribute> directionAttribute =
                mesh->getAttribByName(attr::AttributeOwner::POINT, directionName, true);
            if (directionAttribute && directionAttribute->getType() != attr::AttributeType::vectorT)
            {
                throwError(
                    "Noise along vector needs " + directionName + " to be a vector attribute."
                );
                return;
            }

            // TODO: Replace the computed normal fallback with an abstraction that can read implicit
            // or defined normals on vertex or points, whatever is available
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

            const std::vector<float> noise =
                sampleNoise(*generator, *mesh, noiseScale, frequency, offset, seed);

            noiseVectors.resize(noise.size());
            for (size_t pointOffset = 0; pointOffset < noise.size(); ++pointOffset)
                noiseVectors[pointOffset] = directions[pointOffset] * noise[pointOffset];
        }
        else
        {
            const std::vector<float> xNoise =
                sampleNoise(*generator, *mesh, noiseScale, frequency, offset, seed);
            const std::vector<float> yNoise =
                sampleNoise(*generator, *mesh, noiseScale, frequency, offset, seed + 1);
            const std::vector<float> zNoise =
                sampleNoise(*generator, *mesh, noiseScale, frequency, offset, seed + 2);

            noiseVectors.resize(xNoise.size());
            for (size_t pointOffset = 0; pointOffset < xNoise.size(); ++pointOffset)
                noiseVectors[pointOffset] =
                    Vector3(xNoise[pointOffset], yNoise[pointOffset], zNoise[pointOffset]);
        }

        utils::applyOperation(*operation, attr::AttributeHandle<Vector3>(attribute), noiseVectors);
    }

    setOutputPacket(0, packet);
}

} // namespace

ENZO_REGISTER_NODE(attributeNoise, AttributeNoise)
