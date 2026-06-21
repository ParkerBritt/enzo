#include "Engine/Expression/DasRuntime.h"
#include "Engine/Expression/DasContext.h"
#include "Engine/Expression/ExpressionContext.h"
#include "daScript/daScript.h"

// Makes daslang's builtin modules (math, strings, and the rest) available.
DECLARE_ALL_DEFAULT_MODULES;

// Makes our parameter functions module (prm and friends) available.
DECLARE_MODULE(ParameterModule);

namespace enzo::expr {

// Keeps the daslang objects out of the header so the heavy daslang header is
// compiled here once, not in every file that uses a script.
struct CompiledScript::Impl
{
    das::ProgramPtr program;
    std::shared_ptr<DasContext> context;
};

CompiledScript::CompiledScript() : impl_(std::make_unique<Impl>()) {}

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

bool evalRaw(DasContext& context, const String& functionName, vec4f& result, String& error)
{
    das::SimFunction* function = context.findFunction(functionName.c_str());
    if (!function)
    {
        error = "function not found: " + functionName;
        return false;
    }

    result = context.evalWithCatch(function, nullptr);
    if (const char* exception = context.getException())
    {
        error = exception;
        return false;
    }
    return true;
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
} // namespace

bool CompiledScript::evalFloat(
    const String& functionName,
    const ExpressionContext* context,
    floatT& result,
    String& error
)
{
    ScopedExpressionContext scope(*impl_->context, context);
    vec4f raw;
    if (!evalRaw(*impl_->context, functionName, raw, error)) return false;
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
    ScopedExpressionContext scope(*impl_->context, context);
    vec4f raw;
    if (!evalRaw(*impl_->context, functionName, raw, error)) return false;
    result = das::cast<int64_t>::to(raw);
    return true;
}

DasRuntime& DasRuntime::instance()
{
    static DasRuntime runtime;
    return runtime;
}

DasRuntime::DasRuntime()
{
    PULL_ALL_DEFAULT_MODULES;
    PULL_MODULE(ParameterModule);
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

    std::shared_ptr<CompiledScript> script(new CompiledScript());
    script->impl_->program = program;
    script->impl_->context = context;
    return script;
}

} // namespace enzo::expr
