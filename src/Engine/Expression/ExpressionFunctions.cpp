#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "Engine/Network/NetworkManager.h"
#include "daScript/ast/ast_interop.h"
#include <bit>

// The daslang module that exposes enzo's functions to expressions. Each one
// returns a single value.

namespace enzo::expr {

namespace {

// Returns the context the running expression belongs to, or null for a preview
// evaluation that has none.
const ExpressionContext* expressionContextOf(das::Context* dasContext)
{
    return static_cast<DasContext*>(dasContext)->expressionContext;
}

// Returns a parameter's value, with the path resolved relative to the node the
// running expression belongs to.
//
// Raises a daslang error when there is no context or the path matches nothing,
// so the failure surfaces as the expression's error.
//
// TODO: when a node error API exists, a failing parameter eval during a node's
// cook should also raise that node's error, not just the expression's.
template <typename Value>
Value readParameter(const char* path, int32_t index, das::Context* dasContext)
{
    const ExpressionContext* context = expressionContextOf(dasContext);
    if (!context) dasContext->throw_error("parameter functions need a node to resolve against");

    const char* safePath = path ? path : "";
    auto value = context->readParameter<Value>(safePath, static_cast<unsigned int>(index));
    if (!value) dasContext->throw_error_ex("no parameter matches path '%s'", safePath);

    return *value;
}

// Parameter functions exposed to daslang expressions

/// @brief Returns a parameter's value as a float, one component at a time.
///
/// e.g. prm("grid_1.t", 1) reads the second component of a vector, while the
/// index defaults to 0 so prm("grid_1.tx") reads the first.
floatT prm(const char* path, int32_t index, das::Context* dasContext)
{
    return readParameter<floatT>(path, index, dasContext);
}

/// @brief Returns a parameter's value as an integer, e.g. prmI("copies.count").
intT prmI(const char* path, int32_t index, das::Context* dasContext)
{
    return readParameter<intT>(path, index, dasContext);
}

/// @brief Returns a parameter's value as a string, e.g. prmS("file.name").
char* prmS(const char* path, int32_t index, das::Context* dasContext)
{
    const String value = readParameter<String>(path, index, dasContext);
    return dasContext->allocateString(value, nullptr);
}

// Time functions exposed to daslang expressions

// Notes on the context that the running expression read the time. A preview
// evaluation has no context and records nothing.
void recordTimeDependency(das::Context* dasContext)
{
    const ExpressionContext* context = expressionContextOf(dasContext);
    if (context) context->recordTimeDependency();
}

/// @brief Returns the frame the scene sits on.
/// @note The frame can be fractional.
floatT frame(das::Context* dasContext)
{
    recordTimeDependency(dasContext);
    return nt::nm().getFrame();
}

/// @brief Returns the scene time in seconds, measured from the start of frame 1.
floatT time(das::Context* dasContext)
{
    recordTimeDependency(dasContext);
    return nt::nm().getTime();
}

// Random functions exposed to daslang expressions

// Mixes a value into a running hash so nearby inputs give unrelated outputs.
uint32_t hashCombine(uint32_t hash, uint32_t value)
{
    hash = (hash ^ value) + 0x9e3779b9u;
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    hash ^= hash >> 16;
    return hash;
}

// Returns the bits of a float as a hash input, with -0 and 0 giving the same bits.
uint32_t getFloatBits(floatT value)
{
    return std::bit_cast<uint32_t>(value + 0.0f);
}

// Returns the top 24 bits of a hash as a value in [0, 1), since a float holds 24 bits exactly.
floatT hashToUnit(uint32_t hash)
{
    return static_cast<floatT>(hash >> 8) / 16777216.0f;
}

// Returns the hash of a seed.
uint32_t hashSeed(intT seed)
{
    const uint64_t seedBits = static_cast<uint64_t>(seed);
    const uint32_t lowBits = static_cast<uint32_t>(seedBits);
    const uint32_t highBits = static_cast<uint32_t>(seedBits >> 32);
    return hashCombine(hashCombine(0, lowBits), highBits);
}

uint32_t hashSeed(floatT seed)
{
    return hashCombine(0, getFloatBits(seed));
}

uint32_t hashSeed(das::float3 seed)
{
    uint32_t hash = hashCombine(0, getFloatBits(seed.x));
    hash = hashCombine(hash, getFloatBits(seed.y));
    hash = hashCombine(hash, getFloatBits(seed.z));
    return hash;
}

// Returns a vector in [0, 1) on each axis, with the axis index mixed into the
// hash so the axes are unrelated.
das::float3 hashToUnitVector(uint32_t hash)
{
    return das::float3(
        hashToUnit(hashCombine(hash, 0)),
        hashToUnit(hashCombine(hash, 1)),
        hashToUnit(hashCombine(hash, 2))
    );
}

/// @brief Returns a random value in [0, 1) that is always the same for a seed.
/// @note Gives the same value as the int64 seed of the same number.
floatT randFromInt32(int32_t seed)
{
    return hashToUnit(hashSeed(intT(seed)));
}

/// @brief Returns a random value in [0, 1) that is always the same for a seed.
floatT randFromInt64(intT seed)
{
    return hashToUnit(hashSeed(seed));
}

/// @brief Returns a random value in [0, 1) that is always the same for a seed.
floatT randFromFloat(floatT seed)
{
    return hashToUnit(hashSeed(seed));
}

/// @brief Returns a random value in [0, 1) that is always the same for a seed.
///
/// e.g. rand(@Position) gives each point a value tied to where it sits.
floatT randFromVector(das::float3 seed)
{
    return hashToUnit(hashSeed(seed));
}

/// @brief Returns a random vector in [0, 1) on each axis that is always the same for a seed.
/// @note Gives the same value as the int64 seed of the same number.
das::float3 randVectorFromInt32(int32_t seed)
{
    return hashToUnitVector(hashSeed(intT(seed)));
}

/// @brief Returns a random vector in [0, 1) on each axis that is always the same for a seed.
///
/// e.g. randVector(curPt()) gives each point its own colour.
das::float3 randVectorFromInt64(intT seed)
{
    return hashToUnitVector(hashSeed(seed));
}

/// @brief Returns a random vector in [0, 1) on each axis that is always the same for a seed.
das::float3 randVectorFromFloat(floatT seed)
{
    return hashToUnitVector(hashSeed(seed));
}

/// @brief Returns a random vector in [0, 1) on each axis that is always the same for a seed.
das::float3 randVectorFromVector(das::float3 seed)
{
    return hashToUnitVector(hashSeed(seed));
}

} // namespace

class ExpressionModule : public das::Module
{
  public:
    ExpressionModule() : das::Module("enzo_expression")
    {
        das::ModuleLibrary lib(this);
        lib.addBuiltInModule();

        // Marks the functions as reading external mutable state, so daslang does
        // not fold repeated calls to a constant.
        das::addExtern<DAS_BIND_FUN(prm)>(
            *this,
            lib,
            "prm",
            das::SideEffects::modifyExternal,
            "enzo::expr::prm"
        )
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));

        das::addExtern<DAS_BIND_FUN(prmI)>(
            *this,
            lib,
            "prmI",
            das::SideEffects::modifyExternal,
            "enzo::expr::prmI"
        )
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));

        das::addExtern<DAS_BIND_FUN(prmS)>(
            *this,
            lib,
            "prmS",
            das::SideEffects::modifyExternal,
            "enzo::expr::prmS"
        )
            ->args({"path", "index", "context"})
            ->arg_init(1, new das::ExprConstInt(0));

        das::addExtern<DAS_BIND_FUN(frame)>(
            *this,
            lib,
            "frame",
            das::SideEffects::modifyExternal,
            "enzo::expr::frame"
        )
            ->args({"context"});

        das::addExtern<DAS_BIND_FUN(time)>(
            *this,
            lib,
            "time",
            das::SideEffects::modifyExternal,
            "enzo::expr::time"
        )
            ->args({"context"});

        // Marks the random functions pure so daslang can fold a constant seed.
        das::addExtern<DAS_BIND_FUN(randFromInt32)>(
            *this,
            lib,
            "rand",
            das::SideEffects::none,
            "enzo::expr::randFromInt32"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randFromInt64)>(
            *this,
            lib,
            "rand",
            das::SideEffects::none,
            "enzo::expr::randFromInt64"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randFromFloat)>(
            *this,
            lib,
            "rand",
            das::SideEffects::none,
            "enzo::expr::randFromFloat"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randFromVector)>(
            *this,
            lib,
            "rand",
            das::SideEffects::none,
            "enzo::expr::randFromVector"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randVectorFromInt32)>(
            *this,
            lib,
            "randVector",
            das::SideEffects::none,
            "enzo::expr::randVectorFromInt32"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randVectorFromInt64)>(
            *this,
            lib,
            "randVector",
            das::SideEffects::none,
            "enzo::expr::randVectorFromInt64"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randVectorFromFloat)>(
            *this,
            lib,
            "randVector",
            das::SideEffects::none,
            "enzo::expr::randVectorFromFloat"
        )
            ->args({"seed"});

        das::addExtern<DAS_BIND_FUN(randVectorFromVector)>(
            *this,
            lib,
            "randVector",
            das::SideEffects::none,
            "enzo::expr::randVectorFromVector"
        )
            ->args({"seed"});
    }
};

} // namespace enzo::expr

REGISTER_MODULE_IN_NAMESPACE(ExpressionModule, enzo::expr);
