#include "Engine/Expression/DasRuntime.h"
#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "daScript/daScript.h"

// Makes daslang's builtin modules (math, strings, and the rest) available.
DECLARE_ALL_DEFAULT_MODULES;

// Makes our expression functions module (prm, frame and friends) available.
DECLARE_MODULE(ExpressionModule);

namespace enzo::expr {

static_assert(maxScriptArguments == DAS_MAX_FUNCTION_ARGUMENTS);

// Keeps the daslang objects out of the header so the heavy daslang header is
// compiled here once, not in every file that uses a script.
struct CompiledScript::Impl
{
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

// Installs the evaluation world on the context for one run and restores it
// after, so a nested prm() call leaves the outer evaluation's world intact.
class ScopedExpressionContext
{
  public:
    ScopedExpressionContext(DasContext& context, const ExpressionContext* expressionContext)
        : context_(context), previous_(context.expressionContext)
    {
        context_.expressionContext = expressionContext;
    }
    ~ScopedExpressionContext() { context_.expressionContext = previous_; }

  private:
    DasContext& context_;
    const ExpressionContext* previous_;
};

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
    vec4f& result,
    String& error
)
{
    ScopedExpressionContext scope(context, expressionContext);

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

    result = context.evalWithCatch(function, rawArguments);
    if (const char* exception = context.getException())
    {
        error = exception;
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
    if (!evalRaw(*impl_->context, functionName, {}, context, raw, error)) return false;
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
    if (!evalRaw(*impl_->context, functionName, {}, context, raw, error)) return false;
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
    if (!evalRaw(*impl_->context, functionName, {}, context, raw, error)) return false;

    // daslang hands back a heap string as a char pointer, null when empty.
    const char* text = das::cast<char*>::to(raw);
    result = text ? text : "";
    return true;
}

bool CompiledScript::run(
    const String& functionName,
    std::span<const ScriptArgument> arguments,
    const ExpressionContext* context,
    String& error
)
{
    vec4f raw;
    return evalRaw(*impl_->context, functionName, arguments, context, raw, error);
}

std::shared_ptr<CompiledScript> CompiledScript::clone() const
{
    auto context = std::make_shared<DasContext>(
        *impl_->context,
        uint32_t(das::ContextCategory::thread_clone)
    );
    if (context->failed) return nullptr;

    return std::shared_ptr<CompiledScript>(new CompiledScript({impl_->program, context}));
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
    fileAccess->setFileInfo(
        fileName.c_str(),
        das::make_unique<das::TextFileInfo>(
            source.c_str(),
            static_cast<uint32_t>(source.size()),
            false
        )
    );

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

    return std::shared_ptr<CompiledScript>(new CompiledScript({program, context}));
}

} // namespace enzo::expr
