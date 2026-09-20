#include "Engine/Expression/PointScript.h"
#include "Engine/Expression/VectorOperators.h"
#include "Engine/Attribute/AttributeNames.h"
#include "Engine/Primitives/Primitive.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <regex>

namespace enzo::expr {

namespace {
constexpr const char* runFunctionName = "enzoRunScript";
constexpr const char* bindingPrefix = "enzoAttrib_";

// The lines the wrapper adds above the user's code.
constexpr int headerLineCount = 6;

bool isIdentifierStart(char character)
{
    return std::isalpha(static_cast<unsigned char>(character)) || character == '_';
}

bool isIdentifierCharacter(char character)
{
    return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
}

std::optional<attr::AttributeType> getPrefixType(char prefix)
{
    switch (prefix)
    {
    case 'f':
        return attr::AttributeType::floatT;
    case 'i':
        return attr::AttributeType::intT;
    case 'v':
        return attr::AttributeType::vectorT;
    case 'b':
        return attr::AttributeType::boolT;
    default:
        return std::nullopt;
    }
}

std::optional<attr::AttributeType> getFixedType(const String& name)
{
    const bool isVectorName = name == attr::names::position || name == attr::names::normal ||
                              name == attr::names::up;
    if (isVectorName) return attr::AttributeType::vectorT;
    return std::nullopt;
}

String getDaslangTypeName(attr::AttributeType type)
{
    switch (type)
    {
    case attr::AttributeType::intT:
        return "int64";
    case attr::AttributeType::vectorT:
        return "float3";
    case attr::AttributeType::boolT:
        return "bool";
    default:
        return "float";
    }
}

// The rewriter that turns each `@name` in the user's code into a function argument and
// collects the bindings. Strings and comments are copied unchanged, apart from the code
// inside a string's `{}` interpolation.
class BindingRewriter
{
  public:
    explicit BindingRewriter(const String& code) : code_(code) {}

    // Returns the rewritten code, or nothing when a binding has two types.
    std::optional<String> rewrite(std::vector<AttributeBinding>& bindings, String& error)
    {
        rewriteCode(false);
        if (!error_.empty())
        {
            error = error_;
            return std::nullopt;
        }

        for (const DeclaredBinding& declared : declared_)
        {
            const attr::AttributeType type =
                declared.type.value_or(attr::AttributeType::floatT);
            bindings.push_back(AttributeBinding{declared.name, type});
        }
        return output_;
    }

  private:
    // A binding's name and the type its uses give it.
    struct DeclaredBinding
    {
        String name;
        std::optional<attr::AttributeType> type;
    };

    // Copies code until the end, or until the `}` closing an interpolation.
    void rewriteCode(bool insideInterpolation)
    {
        int braceDepth = 0;
        while (index_ < code_.size())
        {
            const char character = code_[index_];
            if (startsWith("//"))
            {
                copyLineComment();
                continue;
            }
            if (startsWith("/*"))
            {
                copyBlockComment();
                continue;
            }
            if (character == '"')
            {
                copyString();
                continue;
            }
            if (character == '\'')
            {
                copyCharacterLiteral();
                continue;
            }
            if (character == '@')
            {
                rewriteAt();
                continue;
            }
            if (character == '{') ++braceDepth;
            if (character == '}')
            {
                if (insideInterpolation && braceDepth == 0) return;
                --braceDepth;
            }
            copyCharacters(1);
        }
    }

    void rewriteAt()
    {
        // Leaves daslang's own uses of @ alone, `@@` function pointers, `@(` and `@{`
        // lambdas, and `@capture(` lambdas with a capture list.
        if (startsWith("@@"))
        {
            copyCharacters(2);
            return;
        }
        const size_t nameStart = index_ + 1;
        if (nameStart >= code_.size() || !isIdentifierStart(code_[nameStart]))
        {
            copyCharacters(1);
            return;
        }

        size_t nameEnd = nameStart;
        while (nameEnd < code_.size() && isIdentifierCharacter(code_[nameEnd])) ++nameEnd;
        const String name = code_.substr(nameStart, nameEnd - nameStart);

        if (name == "capture" && isFollowedByParenthesis(nameEnd))
        {
            copyCharacters(1);
            return;
        }

        declareBinding(name, takeTypePrefix());
        output_ += bindingPrefix + name;
        index_ = nameEnd;
    }

    // Removes a type prefix like the `v` in `v@dir` from the output and returns its type.
    std::optional<attr::AttributeType> takeTypePrefix()
    {
        if (index_ == 0) return std::nullopt;

        const char prefix = code_[index_ - 1];
        const bool prefixStandsAlone = index_ == 1 || !isIdentifierCharacter(code_[index_ - 2]);
        std::optional<attr::AttributeType> type = getPrefixType(prefix);
        if (!type || !prefixStandsAlone) return std::nullopt;

        output_.pop_back();
        return type;
    }

    // Records a use of `@name`, and reports the first use that contradicts an earlier type.
    void declareBinding(const String& name, std::optional<attr::AttributeType> prefixType)
    {
        auto declared = std::ranges::find_if(declared_, [&](const DeclaredBinding& candidate) {
            return candidate.name == name;
        });
        if (declared == declared_.end())
        {
            declared_.push_back(DeclaredBinding{name, getFixedType(name)});
            declared = std::prev(declared_.end());
        }

        if (!prefixType) return;
        if (!declared->type)
        {
            declared->type = prefixType;
            return;
        }
        if (*declared->type != *prefixType && error_.empty())
        {
            error_ = "@" + name + " is used as both " + attr::getTypeName(*declared->type) +
                     " and " + attr::getTypeName(*prefixType);
        }
    }

    bool isFollowedByParenthesis(size_t position) const
    {
        while (position < code_.size() && std::isspace(static_cast<unsigned char>(code_[position])))
        {
            ++position;
        }
        return position < code_.size() && code_[position] == '(';
    }

    void copyLineComment()
    {
        while (index_ < code_.size() && code_[index_] != '\n') copyCharacters(1);
    }

    // Copies a block comment, which can hold other block comments.
    void copyBlockComment()
    {
        int depth = 0;
        while (index_ < code_.size())
        {
            if (startsWith("/*"))
            {
                ++depth;
                copyCharacters(2);
            }
            else if (startsWith("*/"))
            {
                copyCharacters(2);
                if (--depth == 0) return;
            }
            else
            {
                copyCharacters(1);
            }
        }
    }

    void copyString()
    {
        copyCharacters(1);
        while (index_ < code_.size())
        {
            const char character = code_[index_];
            if (character == '\\')
            {
                copyCharacters(2);
                continue;
            }
            if (character == '"')
            {
                copyCharacters(1);
                return;
            }
            if (character == '{')
            {
                copyCharacters(1);
                rewriteCode(true);
                copyCharacters(1);
                continue;
            }
            copyCharacters(1);
        }
    }

    void copyCharacterLiteral()
    {
        copyCharacters(1);
        while (index_ < code_.size())
        {
            const char character = code_[index_];
            copyCharacters(character == '\\' ? 2 : 1);
            if (character == '\'') return;
        }
    }

    bool startsWith(const char* text) const { return code_.compare(index_, std::strlen(text), text) == 0; }

    void copyCharacters(size_t count)
    {
        const size_t available = std::min(count, code_.size() - index_);
        output_.append(code_, index_, available);
        index_ += available;
    }

    const String& code_;
    size_t index_ = 0;
    String output_;

    std::vector<DeclaredBinding> declared_;
    String error_;
};

// Returns the user's code wrapped in an exported function that takes the point
// offset and a reference to each binding.
String wrapScript(const String& code, const std::vector<AttributeBinding>& bindings)
{
    String arguments = "pt : int64";
    for (const AttributeBinding& binding : bindings)
    {
        arguments += "; var " + String(bindingPrefix) + binding.name + " : " +
                     getDaslangTypeName(binding.type) + "&";
    }

    return "options gen2\n"
           "require math\n"
           "require enzo_expression\n"
           "require " +
           String(vectorOperatorsModule) + "\n"
           "[export]\n"
           "def " +
           String(runFunctionName) + "(" + arguments + ") {\n" + code + "\n}\n";
}

// Returns an error with line numbers counted from the user's first line and
// bindings written as `@name`.
String toUserError(const String& error)
{
    static const std::regex lineNumber(R"(script\.das:(\d+):)");

    String userError;
    auto searchStart = error.cbegin();
    std::smatch match;
    while (std::regex_search(searchStart, error.cend(), match, lineNumber))
    {
        const int line = std::stoi(match[1].str()) - headerLineCount;
        userError += match.prefix().str() + "line " + std::to_string(line) + ":";
        searchStart = match.suffix().first;
    }
    userError.append(searchStart, error.cend());

    static const std::regex binding(bindingPrefix);
    return std::regex_replace(userError, binding, "@");
}
} // namespace

PointScript::PointScript(
    std::shared_ptr<CompiledScript> compiled,
    std::vector<AttributeBinding> bindings
)
    : compiled_(std::move(compiled)), bindings_(std::move(bindings))
{
    arguments_.push_back(intT(0));
    for (const AttributeBinding& binding : bindings_)
    {
        visitBindingValues(binding.type, [&]<typename Value>(BindingValues<Value>& bindingValues) {
            Value& value = bindingValues.values.emplace_back();
            arguments_.push_back(static_cast<void*>(&value));
        });
    }
}

template <typename Action> void PointScript::visitAllBindingValues(Action&& action)
{
    action(floats_);
    action(ints_);
    action(vectors_);
    action(bools_);
}

template <typename Action>
void PointScript::visitBindingValues(attr::AttributeType type, Action&& action)
{
    switch (type)
    {
    case attr::AttributeType::intT:
        action(ints_);
        break;
    case attr::AttributeType::vectorT:
        action(vectors_);
        break;
    case attr::AttributeType::boolT:
        action(bools_);
        break;
    default:
        action(floats_);
        break;
    }
}

std::shared_ptr<PointScript> PointScript::compile(const String& code, String& error)
{
    std::vector<AttributeBinding> bindings;
    std::optional<String> rewrittenCode = BindingRewriter(code).rewrite(bindings, error);
    if (!rewrittenCode) return nullptr;

    if (bindings.size() + 1 > maxScriptArguments)
    {
        error = "a script can use at most " + std::to_string(maxScriptArguments - 1) +
                " attributes";
        return nullptr;
    }

    auto compiled =
        DasRuntime::instance().compile("script", wrapScript(*rewrittenCode, bindings), error);
    if (!compiled)
    {
        error = toUserError(error);
        return nullptr;
    }

    // Marks the bindings the code assigns to, skipping the point offset argument.
    const std::vector<bool> writtenArguments = compiled->getWrittenArguments(runFunctionName);
    for (size_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex)
    {
        bindings[bindingIndex].written = writtenArguments.at(bindingIndex + 1);
    }

    return std::shared_ptr<PointScript>(new PointScript(compiled, std::move(bindings)));
}

std::shared_ptr<PointScript> PointScript::clone() const
{
    std::shared_ptr<CompiledScript> compiledClone = compiled_->clone();
    if (!compiledClone) return nullptr;
    return std::shared_ptr<PointScript>(new PointScript(compiledClone, bindings_));
}

bool PointScript::addWrittenAttributes(geo::Primitive& output, String& error) const
{
    for (const AttributeBinding& binding : bindings_)
    {
        if (!binding.written) continue;
        if (!output.tryAddAttribute(attr::AttributeOwner::POINT, binding.name, binding.type))
        {
            error = "can't write to attribute @" + binding.name;
            return false;
        }
    }
    return true;
}

bool PointScript::resolveAttributes(
    const geo::Primitive& input,
    geo::Primitive& output,
    String& error
)
{
    visitAllBindingValues([](auto& bindingValues) {
        bindingValues.inputs.clear();
        bindingValues.outputs.clear();
    });

    for (const AttributeBinding& binding : bindings_)
    {
        std::shared_ptr<const attr::Attribute> inputAttribute =
            input.getAttribByName(attr::AttributeOwner::POINT, binding.name, true);
        const bool inputMatches = inputAttribute && inputAttribute->getType() == binding.type;

        std::shared_ptr<attr::Attribute> outputAttribute;
        if (binding.written)
        {
            outputAttribute = output.getAttribByName(attr::AttributeOwner::POINT, binding.name, true);
            if (!outputAttribute || outputAttribute->getType() != binding.type)
            {
                error = "the output has no " + attr::getTypeName(binding.type) + " attribute @" +
                        binding.name;
                return false;
            }
        }

        visitBindingValues(binding.type, [&]<typename Value>(BindingValues<Value>& bindingValues) {
            std::optional<attr::AttributeHandleRO<Value>> inputHandle;
            if (inputMatches) inputHandle.emplace(inputAttribute);
            bindingValues.inputs.push_back(inputHandle);

            std::optional<attr::AttributeHandle<Value>> outputHandle;
            if (outputAttribute) outputHandle.emplace(outputAttribute);
            bindingValues.outputs.push_back(outputHandle);
        });
    }
    return true;
}

bool PointScript::run(
    const geo::Primitive& input,
    geo::Primitive& output,
    Offset begin,
    Offset end,
    const ExpressionContext* context,
    String& error
)
{
    if (!resolveAttributes(input, output, error)) return false;

    for (Offset point = begin; point < end; ++point)
    {
        if (!input.isValidPoint(point)) continue;

        visitAllBindingValues([point](auto& bindingValues) { bindingValues.loadFrom(point); });

        arguments_[0] = intT(point);
        if (!compiled_->run(runFunctionName, arguments_, context, error))
        {
            error = toUserError(error);
            return false;
        }

        visitAllBindingValues([point](auto& bindingValues) { bindingValues.storeTo(point); });
    }
    return true;
}

} // namespace enzo::expr
