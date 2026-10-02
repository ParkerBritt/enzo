#include "Engine/Daslang/ThreadState.h"
#include "Engine/Script/ScriptContext.h"
#include "daScript/ast/ast_interop.h"
#include <algorithm>
#include <cctype>

// The daslang module that lets scripts read the attributes of their input mesh.

namespace enzo::script {

namespace {

// The daslang type a script receives for each attribute value type.
template <typename Value> struct ScriptValue
{
    using Type = Value;
};

template <> struct ScriptValue<intT>
{
    using Type = int64_t;
};

template <> struct ScriptValue<Vector3>
{
    using Type = das::float3;
};

template <typename Value> typename ScriptValue<Value>::Type toScriptValue(const Value& value)
{
    if constexpr (std::is_same_v<Value, Vector3>) return das::float3(value.x(), value.y(), value.z());
    else return value;
}

/**
 * @brief Returns the name of the script function that reads an attribute type on an owner.
 *
 * getReadName(POINT, vectorT) gives "pointAttrVector".
 */
String getReadName(attr::AttributeOwner owner, attr::AttributeType type)
{
    const String readName = attr::getOwnerName(owner) + "Attr";
    if (type == attr::AttributeType::floatT) return readName;

    String typeSuffix = attr::getTypeName(type);
    typeSuffix[0] = static_cast<char>(std::toupper(typeSuffix[0]));
    return readName + typeSuffix;
}

const MeshAttributeReader& getAttributeReader(das::Context* dasContext)
{
    const auto* threadState = static_cast<daslang::ThreadState*>(dasContext);
    if (!threadState->primitiveIndex)
        dasContext->throw_error("attribute functions only run inside a script node");

    const auto* scriptContext = static_cast<const ScriptContext*>(threadState->expressionContext);
    return scriptContext->getAttributeReader(*threadState->primitiveIndex);
}

/// @brief Adds a warning to the context running the script, skipping one it already holds.
/// @return Zero, the value a failed read gives the script.
template <typename Value>
typename ScriptValue<Value>::Type failRead(das::Context* dasContext, const String& warning)
{
    std::vector<String>& warnings = static_cast<daslang::ThreadState*>(dasContext)->warnings;
    if (std::ranges::find(warnings, warning) == warnings.end()) warnings.push_back(warning);

    if constexpr (std::is_same_v<Value, Vector3>) return das::float3(0.0f, 0.0f, 0.0f);
    else return Value{};
}

/**
 * @brief Returns an attribute's value on the element at an index.
 *
 * @return The value, or zero with a warning on the node when the attribute is
 * missing, holds another type, or has no element at the index.
 */
template <attr::AttributeOwner owner, typename Value, typename Index>
typename ScriptValue<Value>::Type readAttribute(const char* name, Index index, das::Context* dasContext)
{
    const MeshAttributeReader& reader = getAttributeReader(dasContext);
    const attr::AttributeType type = attr::getAttributeType<Value>();
    const char* safeName = name ? name : "";

    const ScriptAttribute* attribute = reader.getAttribute(owner, safeName);
    if (!attribute)
    {
        return failRead<Value>(
            dasContext,
            getReadName(owner, type) + " found no " + attr::getOwnerName(owner) + " attribute \"" + safeName + "\""
        );
    }

    const auto* handle = std::get_if<attr::AttributeHandleRO<Value>>(&attribute->handle);
    if (!handle)
    {
        return failRead<Value>(
            dasContext,
            getReadName(owner, type) + " read \"" + safeName + "\" as " +
                attr::getTypeName(type) + " but it is " +
                attr::getTypeName(attribute->type)
        );
    }

    if (!reader.hasElement(owner, index))
    {
        return failRead<Value>(
            dasContext,
            getReadName(owner, type) + " read a " + attr::getOwnerName(owner) + " index that doesn't exist"
        );
    }

    return toScriptValue(handle->getValue(static_cast<Offset>(index)));
}

/// @brief Returns the value of an attribute on the primitive, which has one element.
template <typename Value>
typename ScriptValue<Value>::Type readPrimitiveAttribute(const char* name, das::Context* dasContext)
{
    return readAttribute<attr::AttributeOwner::PRIMITIVE, Value, intT>(name, 0, dasContext);
}

/// @brief Returns whether the input has an attribute with a name on an owner, of any type.
template <attr::AttributeOwner owner> bool hasAttribute(const char* name, das::Context* dasContext)
{
    return getAttributeReader(dasContext).getAttribute(owner, name ? name : "") != nullptr;
}

// Adds a function to the module, marked as reading external state so daslang never
// folds a call to a constant.
template <auto function>
void addFunction(das::Module& module, const das::ModuleLibrary& lib, const String& name, std::initializer_list<const char*> argumentNames)
{
    das::addExtern<decltype(function), function>(
        module,
        lib,
        name.c_str(),
        das::SideEffects::modifyExternal
    )
        ->args(argumentNames);
}

// Adds the reads of one attribute value type on an owner, taking an index as int or int64.
template <attr::AttributeOwner owner, typename Value>
void addElementRead(das::Module& module, const das::ModuleLibrary& lib)
{
    const String name = getReadName(owner, attr::getAttributeType<Value>());
    addFunction<&readAttribute<owner, Value, int32_t>>(module, lib, name, {"name", "index", "context"});
    addFunction<&readAttribute<owner, Value, int64_t>>(module, lib, name, {"name", "index", "context"});
}

template <attr::AttributeOwner owner>
void addElementFunctions(das::Module& module, const das::ModuleLibrary& lib)
{
    addElementRead<owner, floatT>(module, lib);
    addElementRead<owner, intT>(module, lib);
    addElementRead<owner, Vector3>(module, lib);
    addElementRead<owner, boolT>(module, lib);
}

template <typename Value>
void addPrimitiveRead(das::Module& module, const das::ModuleLibrary& lib)
{
    const String name = getReadName(attr::AttributeOwner::PRIMITIVE, attr::getAttributeType<Value>());
    addFunction<&readPrimitiveAttribute<Value>>(module, lib, name, {"name", "context"});
}

template <attr::AttributeOwner owner>
void addHasAttribute(das::Module& module, const das::ModuleLibrary& lib, const String& name)
{
    addFunction<&hasAttribute<owner>>(module, lib, name, {"name", "context"});
}

} // namespace

class GeometryModule : public das::Module
{
  public:
    GeometryModule() : das::Module("enzo_geometry")
    {
        das::ModuleLibrary lib(this);
        lib.addBuiltInModule();

        addElementFunctions<attr::AttributeOwner::POINT>(*this, lib);
        addElementFunctions<attr::AttributeOwner::VERTEX>(*this, lib);
        addElementFunctions<attr::AttributeOwner::FACE>(*this, lib);

        addPrimitiveRead<floatT>(*this, lib);
        addPrimitiveRead<intT>(*this, lib);
        addPrimitiveRead<Vector3>(*this, lib);
        addPrimitiveRead<boolT>(*this, lib);

        addHasAttribute<attr::AttributeOwner::POINT>(*this, lib, "hasPointAttr");
        addHasAttribute<attr::AttributeOwner::VERTEX>(*this, lib, "hasVertexAttr");
        addHasAttribute<attr::AttributeOwner::FACE>(*this, lib, "hasFaceAttr");
        addHasAttribute<attr::AttributeOwner::PRIMITIVE>(*this, lib, "hasPrimitiveAttr");
    }
};

} // namespace enzo::script

REGISTER_MODULE_IN_NAMESPACE(GeometryModule, enzo::script);
