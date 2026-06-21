#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Template.h"
#include <boost/signals2.hpp>
#include <memory>
#include <optional>
#include <string_view>
#include <variant>

namespace enzo::expr {
class ExpressionContext;
}

namespace enzo::prm {

using PrmValues = std::variant<std::vector<floatT>, std::vector<intT>, std::vector<String>>;

class Parameter
{
  public:
    Parameter(Template prmTemplate);
    virtual ~Parameter() = default;

    std::string getName() const;
    std::string getLabel() const;
    enzo::prm::Type getType() const;
    enzo::prm::ValueType getValueType() const;
    unsigned int getVectorSize() const;

    floatT evalFloat(unsigned int index = 0) const;
    String evalString(unsigned int index = 0) const;
    intT evalInt(unsigned int index = 0) const;

    // Variants that report a driving expression's failure. The value still falls
    // back to the stored literal on failure, and error is left empty on success
    // or when the component holds no expression.
    floatT evalFloat(unsigned int index, String& error) const;
    intT evalInt(unsigned int index, String& error) const;

    // Read every value the parameter holds as a list. A scalar parameter
    // returns one element, a vector parameter one per component, and a
    // multi-value parameter such as a multi select dropdown one per selection.
    std::vector<floatT> evalFloats() const;
    std::vector<String> evalStrings() const;
    std::vector<intT> evalInts() const;

    void setInt(intT value, unsigned int index = 0);
    void setFloat(floatT value, unsigned int index = 0);
    void setString(String value, unsigned int index = 0);

    // A component may hold a daslang expression instead of a literal. When set,
    // the eval functions run the expression to produce the value, and setting a
    // literal clears it.
    void setExpression(String expression, unsigned int index = 0);
    void clearExpression(unsigned int index = 0);
    bool hasExpression(unsigned int index = 0) const;

    /// @brief The expression on a component.
    /// @return The expression when one is set, otherwise nullopt.
    std::optional<String> getExpression(unsigned int index = 0) const;

    /// @brief Evaluates a component's expression and reports any failure.
    ///
    /// e.g. an expression of "1 +" reports a compile error while "5 + 5"
    /// reports nothing.
    ///
    /// @return The error text when the expression fails to compile or run,
    /// otherwise nullopt, including when the component holds no expression.
    std::optional<String> getExpressionError(unsigned int index = 0) const;

    PrmValues getValues() const;
    void setValues(const PrmValues& values);

    // Multiparm instances. Each instance is a group of field parameters cloned
    // from the instance template. The live count is the number of instances.
    unsigned int getInstanceCount() const;
    const std::vector<std::shared_ptr<Parameter>>& getInstance(unsigned int instanceIndex) const;
    std::shared_ptr<Parameter>
    getInstanceField(unsigned int instanceIndex, std::string_view fieldName) const;
    void addInstance();
    void removeInstance(unsigned int instanceIndex);
    void moveInstance(unsigned int fromIndex, unsigned int toIndex);

    const Template& getTemplate();

    boost::signals2::signal<void()> valueChanged;

  protected:
    virtual void onFloatSet_(const PrmValues& before) {}
    void handleValueChange_();

    /// @brief The world an expression on this parameter reads, e.g. for prm().
    /// @return A context, or null when the parameter has nothing to offer such
    /// as a bare parameter with no owning node.
    virtual std::unique_ptr<expr::ExpressionContext> makeExpressionContext_() const;

    /// @brief Hands the dependencies an expression captured to the network graph.
    /// @note Only a parameter with an owning node has a place to record them, so
    /// the base does nothing.
    virtual void
    submitExpressionDependencies_(const expr::ExpressionContext& context, unsigned int index) const
    {
    }

    /// @brief Returns the value pulled within a component's locked range bounds.
    floatT clampToRange_(floatT value, unsigned int index) const;
    intT clampToRange_(intT value, unsigned int index) const;
    std::vector<std::shared_ptr<Parameter>> buildInstance_(unsigned int instanceIndex);

    Template template_;
    PrmValues values_;
    // One optional expression per component, parallel to the value vector. An
    // empty slot means the component uses its literal value.
    std::vector<std::optional<String>> expressions_;
    std::vector<std::vector<std::shared_ptr<Parameter>>> instances_;
};
} // namespace enzo::prm
