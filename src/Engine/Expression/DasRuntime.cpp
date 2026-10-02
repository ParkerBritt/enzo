#include "Engine/Expression/DasRuntime.h"
#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ScriptContext.h"
#include "Engine/Expression/VectorOperators.h"
#include "daScript/daScript.h"
#include <mutex>
#include <optional>
#include <utility>

// Makes daslang's builtin modules (math, strings, and the rest) available.
DECLARE_ALL_DEFAULT_MODULES;

// Makes our expression functions module (prm, frame and friends) available.
DECLARE_MODULE(ExpressionModule);

// Makes our geometry functions module (pointAttr and friends) available.
DECLARE_MODULE(GeometryModule);

namespace enzo::expr {

static_assert(maxScriptArguments == DAS_MAX_FUNCTION_ARGUMENTS);

// Keeps the daslang objects out of the header so the heavy daslang header is
// compiled here once, not in every file that uses a script.
struct CompiledScript::Impl
{
    // The source file that error locations point into.
    das::FileAccessPtr fileAccess;
    das::ProgramPtr program;
    std::shared_ptr<DasContext> context;
};

CompiledScript::CompiledScript(Impl impl) : impl_(std::make_unique<Impl>(std::move(impl))) {}

CompiledScript::~CompiledScript() = default;

namespace {
String collectErrors(const das::ProgramPtr& program)
{
    String message;
    for (const auto& error : program->errors)
    {
        message += das::reportError(error.at, error.what, error.extra, error.fixme, error.cerr);
    }
    return message;
}

vec4f toRawArgument(const ScriptArgument& argument)
{
    if (const intT* integer = std::get_if<intT>(&argument))
    {
        return das::cast<int64_t>::from(*integer);
    }
    return das::cast<void*>::from(std::get<void*>(argument));
}

bool evalRaw(
    DasContext& context,
    const String& functionName,
    std::span<const ScriptArgument> arguments,
    const ExpressionContext* expressionContext,
    std::optional<size_t> primitiveIndex,
    vec4f& result,
    String& error
)
{
    das::SimFunction* function = context.findFunction(functionName.c_str());
    if (!function)
    {
        error = "function not found: " + functionName;
        return false;
    }

    if (arguments.size() > maxScriptArguments)
    {
        error = "too many arguments for " + functionName;
        return false;
    }

    vec4f rawArguments[maxScriptArguments];
    for (size_t argumentIndex = 0; argumentIndex < arguments.size(); ++argumentIndex)
    {
        rawArguments[argumentIndex] = toRawArgument(arguments[argumentIndex]);
    }

    // Swaps in this run's world and restores the outer one after,
    // since a nested prm() can run on this same context.
    const ExpressionContext* outerExpressionContext =
        std::exchange(context.expressionContext, expressionContext);
    const std::optional<size_t> outerPrimitiveIndex =
        std::exchange(context.primitiveIndex, primitiveIndex);
    result = context.evalWithCatch(function, rawArguments);
    context.expressionContext = outerExpressionContext;
    context.primitiveIndex = outerPrimitiveIndex;

    if (const char* exception = context.getException())
    {
        const das::LineInfo& location = context.exceptionAt;
        error = location.fileInfo ? location.describe() + ": " + exception : exception;
        return false;
    }
    return true;
}
} // namespace

bool CompiledScript::evalFloat(
    const String& functionName,
    const ExpressionContext* context,
    floatT& result,
    String& error
)
{
    vec4f raw;
    if (!evalRaw(*impl_->context, functionName, {}, context, std::nullopt, raw, error)) return false;
    result = das::cast<float>::to(raw);
    return true;
}

bool CompiledScript::evalInt(
    const String& functionName,
    const ExpressionContext* context,
    intT& result,
    String& error
)
{
    vec4f raw;
    if (!evalRaw(*impl_->context, functionName, {}, context, std::nullopt, raw, error)) return false;
    result = das::cast<int64_t>::to(raw);
    return true;
}

bool CompiledScript::evalString(
    const String& functionName,
    const ExpressionContext* context,
    String& result,
    String& error
)
{
    vec4f raw;
    if (!evalRaw(*impl_->context, functionName, {}, context, std::nullopt, raw, error)) return false;

    // daslang hands back a heap string as a char pointer, null when empty.
    const char* text = das::cast<char*>::to(raw);
    result = text ? text : "";
    return true;
}

bool CompiledScript::run(
    const String& functionName,
    std::span<const ScriptArgument> arguments,
    const ScriptContext* context,
    size_t primitiveIndex,
    String& error
)
{
    // Leaves the primitive unset without a context, so attribute reads fail cleanly.
    const std::optional<size_t> scriptPrimitiveIndex =
        context ? std::optional(primitiveIndex) : std::nullopt;

    vec4f raw;
    return evalRaw(*impl_->context, functionName, arguments, context, scriptPrimitiveIndex, raw, error);
}

std::vector<bool> CompiledScript::getWrittenArguments(const String& functionName) const
{
    das::FunctionPtr function =
        impl_->program->getThisModule()->findUniqueFunction(functionName);
    if (!function) return {};

    std::vector<bool> written;
    for (const das::VariablePtr& argument : function->arguments)
    {
        written.push_back(argument->access_ref);
    }
    return written;
}

std::vector<String> CompiledScript::takeWarnings() { return std::exchange(impl_->context->warnings, {}); }

std::shared_ptr<CompiledScript> CompiledScript::clone() const
{
    // Clones one at a time, since daslang counts references to the program without atomics.
    static std::mutex cloneMutex;
    std::lock_guard lock(cloneMutex);

    auto context = std::make_shared<DasContext>(
        *impl_->context,
        uint32_t(das::ContextCategory::thread_clone)
    );
    if (context->failed) return nullptr;

    return std::shared_ptr<CompiledScript>(new CompiledScript({impl_->fileAccess, impl_->program, context}));
}

DasRuntime& DasRuntime::instance()
{
    static DasRuntime runtime;
    return runtime;
}

DasRuntime::DasRuntime()
{
    PULL_ALL_DEFAULT_MODULES;
    PULL_MODULE(ExpressionModule);
    PULL_MODULE(GeometryModule);
    das::Module::Initialize();
}

DasRuntime::~DasRuntime() { das::Module::ShutdownStandalone(); }

std::shared_ptr<CompiledScript>
DasRuntime::compile(const String& name, const String& source, String& error)
{
    das::TextPrinter logs;
    das::ModuleGroup libraryGroup;

    // daslang compiles from files, so the source is registered as a virtual file.
    const String fileName = name + ".das";
    auto fileAccess = das::make_smart<das::FsFileAccess>();
    const auto addFile = [&fileAccess](const String& path, const char* text) {
        fileAccess->setFileInfo(
            path.c_str(),
            das::make_unique<das::TextFileInfo>(
                text,
                static_cast<uint32_t>(std::char_traits<char>::length(text)),
                false
            )
        );
    };
    addFile(fileName, source.c_str());
    addFile(String(vectorOperatorsModule) + ".das", vectorOperatorsSource);

    // Parse and type check.
    das::ProgramPtr program =
        das::compileDaScript(fileName.c_str(), fileAccess, logs, libraryGroup);
    if (program->failed())
    {
        error = collectErrors(program);
        return nullptr;
    }

    // Build the runtime context that runs the program.
    auto context = std::make_shared<DasContext>(program->getContextStackSize());
    if (!program->simulate(*context, logs))
    {
        error = collectErrors(program);
        return nullptr;
    }

    return std::shared_ptr<CompiledScript>(new CompiledScript({fileAccess, program, context}));
}

} // namespace enzo::expr
