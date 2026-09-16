#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/GeometryAlgorithms/Normals.h"
#include "Engine/Network/NetworkManager.h"
#include "Engine/Network/Node.h"
#include "Engine/Network/NodeLoader.h"
#include "Engine/Network/NodeTypeTable.h"
#include "Engine/Primitives/Mesh.h"
#include <catch2/catch_approx.hpp>
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

using namespace enzo;

namespace {

struct NMReset
{
    NMReset() { nt::nm()._reset(); }
    ~NMReset() { nt::nm()._reset(); }
};

// Returns the mesh a cooked node put on its first output.
std::shared_ptr<const geo::Mesh> getMesh(nt::Node& node)
{
    const auto mesh =
        std::dynamic_pointer_cast<const geo::Mesh>(node.getOutputPacket(0)->getPrimitive(0));
    REQUIRE(mesh != nullptr);
    return mesh;
}

// Returns an uncooked attribute noise wired to the input node.
nt::NodeId addAttributeNoiseAfter(nt::NodeId input)
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::attributeNoise"));
    nm.connectNodes(input, 0, attributeNoise, 0);
    nm.getNode(attributeNoise).getParameter("frequency").lock()->setFloat(0.37f);
    return attributeNoise;
}

nt::NodeId addGrid()
{
    nt::NodeLoader::loadNodes();
    return nt::nm().createNode(nt::NodeTypeTable::requireNodeType("enzo::grid"));
}

// Sets the node to add vector noise to P.
void setNoiseToAddToP(nt::Node& node)
{
    node.getParameter("name").lock()->setString("P");
    node.getParameter("type").lock()->setString("vector");
    node.getParameter("operation").lock()->setString("add");
}

// Returns how far every point moved between the two meshes.
std::vector<Vector3> getPointMovements(const geo::Mesh& inputMesh, const geo::Mesh& outputMesh)
{
    REQUIRE(outputMesh.getNumPoints() == inputMesh.getNumPoints());

    std::vector<Vector3> movements;
    for (Offset pointOffset = 0; pointOffset < inputMesh.getNumPoints(); ++pointOffset)
        movements.push_back(outputMesh.getPointPos(pointOffset) - inputMesh.getPointPos(pointOffset));
    return movements;
}

// Checks every pattern stays between minus one and one and that no two of them match.
void requireDistinctPatterns(const std::vector<std::vector<floatT>>& patterns)
{
    for (size_t patternIndex = 0; patternIndex < patterns.size(); ++patternIndex)
    {
        const std::vector<floatT>& values = patterns[patternIndex];
        for (const floatT value : values)
        {
            REQUIRE(value >= -1.0f);
            REQUIRE(value <= 1.0f);
        }

        for (size_t otherIndex = 0; otherIndex < patternIndex; ++otherIndex)
            REQUIRE(values != patterns[otherIndex]);
    }
}

// Returns the float noise values a cooked node wrote.
std::vector<floatT> getNoiseValues(nt::Node& node)
{
    const auto mesh = getMesh(node);
    const auto attribute = mesh->getAttribByName(attr::AttributeOwner::POINT, "noise");
    REQUIRE(attribute != nullptr);

    const attr::AttributeHandleRO<floatT> handle(attribute);
    REQUIRE(handle.getSize() == mesh->getNumPoints());
    return handle.getAllValues();
}

} // namespace

TEST_CASE_METHOD(NMReset, "Every point gets a noise value between minus one and one")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(addGrid());
    nm.cook(attributeNoise);

    const std::vector<floatT> values = getNoiseValues(nm.getNode(attributeNoise));
    for (const floatT value : values)
    {
        REQUIRE(value >= -1.0f);
        REQUIRE(value <= 1.0f);
    }

    // Points at different positions get different values.
    REQUIRE(values.front() != values.back());
}

TEST_CASE_METHOD(NMReset, "A different seed gives a different pattern")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(addGrid());

    nm.cook(attributeNoise);
    const std::vector<floatT> firstSeedValues = getNoiseValues(nm.getNode(attributeNoise));

    nm.getNode(attributeNoise).getParameter("seed").lock()->setInt(7);
    nm.cook(attributeNoise);
    const std::vector<floatT> secondSeedValues = getNoiseValues(nm.getNode(attributeNoise));

    REQUIRE(firstSeedValues != secondSeedValues);
}

TEST_CASE_METHOD(NMReset, "Each noise type gives its own pattern within minus one and one")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(addGrid());
    auto& node = nm.getNode(attributeNoise);

    // Cooks one noise type and returns the values it wrote.
    const auto cookNoiseType = [&](const String& noiseType, const String& cellularMode) {
        node.getParameter("noiseType").lock()->setString(noiseType);
        node.getParameter("cellularMode").lock()->setString(cellularMode);
        nm.cook(attributeNoise);
        return getNoiseValues(nm.getNode(attributeNoise));
    };

    const std::vector<std::vector<floatT>> patterns = {
        cookNoiseType("simplex", "value"),
        cookNoiseType("superSimplex", "value"),
        cookNoiseType("perlin", "value"),
        cookNoiseType("value", "value"),
        cookNoiseType("cellular", "value"),
        cookNoiseType("cellular", "distance"),
    };

    requireDistinctPatterns(patterns);
}

TEST_CASE_METHOD(NMReset, "Each fractal type layers octaves into its own pattern")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(addGrid());
    auto& node = nm.getNode(attributeNoise);

    // Cooks one fractal type and octave count and returns the values it wrote.
    const auto cookFractal = [&](const String& fractalType, int octaves) {
        node.getParameter("fractalType").lock()->setString(fractalType);
        node.getParameter("octaves").lock()->setInt(octaves);
        nm.cook(attributeNoise);
        return getNoiseValues(nm.getNode(attributeNoise));
    };

    const std::vector<std::vector<floatT>> patterns = {
        cookFractal("none", 3),
        cookFractal("fbm", 3),
        cookFractal("fbm", 6),
        cookFractal("ridged", 3),
    };

    requireDistinctPatterns(patterns);
}

TEST_CASE_METHOD(NMReset, "Amplitude scales every noise value")
{
    auto& nm = nt::nm();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(addGrid());

    nm.cook(attributeNoise);
    const std::vector<floatT> unscaledValues = getNoiseValues(nm.getNode(attributeNoise));

    nm.getNode(attributeNoise).getParameter("amplitude").lock()->setFloat(3.0f);
    nm.cook(attributeNoise);
    const std::vector<floatT> scaledValues = getNoiseValues(nm.getNode(attributeNoise));

    for (size_t pointOffset = 0; pointOffset < unscaledValues.size(); ++pointOffset)
        REQUIRE(scaledValues[pointOffset] == Catch::Approx(unscaledValues[pointOffset] * 3.0f));
}

TEST_CASE_METHOD(NMReset, "Adding noise to P moves each axis of every point by up to one")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = addGrid();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(grid);
    setNoiseToAddToP(nm.getNode(attributeNoise));
    nm.cook(attributeNoise);

    const std::vector<Vector3> movements =
        getPointMovements(*getMesh(nm.getNode(grid)), *getMesh(nm.getNode(attributeNoise)));

    bool anyPointMoved = false;
    for (const Vector3& movement : movements)
    {
        REQUIRE(movement.cwiseAbs().maxCoeff() <= 1.0f);
        if (!movement.isZero()) anyPointMoved = true;
    }
    REQUIRE(anyPointMoved);

    // The axes get separate noise rather than one value on all three.
    REQUIRE(movements.front().x() != movements.front().y());
}

TEST_CASE_METHOD(NMReset, "Noise along a vector moves each point along that vector")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = addGrid();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(grid);
    auto& node = nm.getNode(attributeNoise);
    setNoiseToAddToP(node);
    node.getParameter("alongVector").lock()->setInt(1);
    node.getParameter("alongVectorAttribute").lock()->setString("P");
    nm.cook(attributeNoise);

    const auto inputMesh = getMesh(nm.getNode(grid));
    const std::vector<Vector3> movements =
        getPointMovements(*inputMesh, *getMesh(nm.getNode(attributeNoise)));

    bool anyPointMoved = false;
    for (Offset pointOffset = 0; pointOffset < movements.size(); ++pointOffset)
    {
        const Vector3 direction = inputMesh->getPointPos(pointOffset);
        const Vector3& movement = movements[pointOffset];

        // Checks the movement is parallel to the position it moved along.
        REQUIRE(movement.cross(direction).norm() == Catch::Approx(0.0f).margin(1e-4));
        if (!movement.isZero()) anyPointMoved = true;
    }
    REQUIRE(anyPointMoved);
}

TEST_CASE_METHOD(NMReset, "Amplitude scales noise along a vector")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = addGrid();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(grid);
    auto& node = nm.getNode(attributeNoise);
    setNoiseToAddToP(node);
    node.getParameter("alongVector").lock()->setInt(1);
    node.getParameter("alongVectorAttribute").lock()->setString("P");

    // Returns how far every point moved.
    const auto cookMovements = [&]() {
        nm.cook(attributeNoise);
        return getPointMovements(*getMesh(nm.getNode(grid)), *getMesh(nm.getNode(attributeNoise)));
    };

    const std::vector<Vector3> unscaledMovements = cookMovements();
    node.getParameter("amplitude").lock()->setFloat(3.0f);
    const std::vector<Vector3> scaledMovements = cookMovements();

    for (size_t pointOffset = 0; pointOffset < unscaledMovements.size(); ++pointOffset)
        REQUIRE(scaledMovements[pointOffset].isApprox(unscaledMovements[pointOffset] * 3.0f, 1e-4f));
}

TEST_CASE_METHOD(NMReset, "Noise along a missing attribute moves points along their computed normals")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = addGrid();
    const nt::NodeId attributeNoise = addAttributeNoiseAfter(grid);
    auto& node = nm.getNode(attributeNoise);
    setNoiseToAddToP(node);
    node.getParameter("alongVector").lock()->setInt(1);
    node.getParameter("alongVectorAttribute").lock()->setString("missing");
    nm.cook(attributeNoise);

    const auto inputMesh = getMesh(nm.getNode(grid));
    const std::vector<Vector3> movements =
        getPointMovements(*inputMesh, *getMesh(nm.getNode(attributeNoise)));
    const std::vector<Vector3> normals = utils::computePointNormals(*inputMesh);

    bool anyPointMoved = false;
    for (Offset pointOffset = 0; pointOffset < movements.size(); ++pointOffset)
    {
        const Vector3& movement = movements[pointOffset];
        REQUIRE(movement.cross(normals[pointOffset]).norm() == Catch::Approx(0.0f).margin(1e-4));
        if (!movement.isZero()) anyPointMoved = true;
    }
    REQUIRE(anyPointMoved);
}

TEST_CASE_METHOD(NMReset, "Each operation combines the noise with the value already there")
{
    auto& nm = nt::nm();
    const nt::NodeId grid = addGrid();

    // Writes 0.25 on every point for the operations to combine with.
    const nt::NodeId attributeCreate =
        nm.createNode(nt::NodeTypeTable::requireNodeType("enzo::attributeCreate"));
    nm.connectNodes(grid, 0, attributeCreate, 0);
    nm.getNode(attributeCreate).getParameter("name").lock()->setString("noise");
    nm.getNode(attributeCreate).getParameter("floatValue").lock()->setFloat(0.25f);

    const nt::NodeId attributeNoise = addAttributeNoiseAfter(attributeCreate);
    nm.getNode(attributeNoise).getParameter("operation").lock()->setString("set");
    nm.cook(attributeNoise);
    const std::vector<floatT> noise = getNoiseValues(nm.getNode(attributeNoise));

    const auto requireOperation = [&](const String& operation, auto expected) {
        nm.getNode(attributeNoise).getParameter("operation").lock()->setString(operation);
        nm.cook(attributeNoise);
        const std::vector<floatT> values = getNoiseValues(nm.getNode(attributeNoise));
        for (size_t pointOffset = 0; pointOffset < noise.size(); ++pointOffset)
            REQUIRE(values[pointOffset] == Catch::Approx(expected(noise[pointOffset])));
    };

    requireOperation("add", [](floatT noiseValue) { return 0.25f + noiseValue; });
    requireOperation("subtract", [](floatT noiseValue) { return 0.25f - noiseValue; });
    requireOperation("multiply", [](floatT noiseValue) { return 0.25f * noiseValue; });
    requireOperation("minimum", [](floatT noiseValue) { return std::min(0.25f, noiseValue); });
    requireOperation("maximum", [](floatT noiseValue) { return std::max(0.25f, noiseValue); });
}
