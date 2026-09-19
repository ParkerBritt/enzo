#pragma once
#include "Engine/Attribute/AttributeHandle.h"
#include "Engine/Core/Types.h"
#include "Engine/Expression/DasRuntime.h"
#include <deque>
#include <memory>
#include <optional>
#include <vector>

namespace enzo::geo {
class Primitive;
}

namespace enzo::expr {

class ExpressionContext;

/// @brief A point attribute the script reads or writes as `@name`.
struct AttributeBinding
{
    String name;
    attr::AttributeType type;
    bool written = false;
};

/**
 * @brief A compiled script that runs once per point and edits the point's attributes.
 *
 * The user's code is the body of a function that takes the point offset as `pt`
 * and reads and writes the point's attributes as `@name`.
 *
 * @code
 * @Position.y += sin(@Position.x) * prm("amplitude")
 * i@id = pt
 * @endcode
 *
 * @note Each thread runs its own clone over its own range of points.
 */
class PointScript
{
  public:
    /// @brief Returns the compiled script for the user's code.
    /// @return The compiled script, or null when the code fails to compile.
    static std::shared_ptr<PointScript> compile(const String& code, String& error);

    /// @brief Returns every attribute the code binds, in the order they first appear.
    const std::vector<AttributeBinding>& getBindings() const { return bindings_; }

    /// @brief Returns a copy that runs in its own context.
    /// @return The copy, or null when its globals fail to initialise.
    std::shared_ptr<PointScript> clone() const;

    /// @brief Adds every attribute the script writes to the output, filled with the default value.
    /// @return False when the output cannot hold one of them.
    bool addWrittenAttributes(geo::Primitive& output, String& error) const;

    /**
     * @brief Runs the script for each valid point in a range.
     *
     * Reads bindings from the input and stores written ones to the output.
     *
     * @return False when the script fails on a point.
     * @note The output needs the written attributes added first.
     */
    bool run(
        const geo::Primitive& input,
        geo::Primitive& output,
        Offset begin,
        Offset end,
        const ExpressionContext* context,
        String& error
    );

  private:
    // Values the script works on for one point, one list per attribute type.
    template <typename Value> struct BindingValues
    {
        std::deque<Value> values;
        std::vector<std::optional<attr::AttributeHandleRO<Value>>> inputs;
        std::vector<std::optional<attr::AttributeHandle<Value>>> outputs;

        // Reads a point's values from the input, leaving missing attributes at the default value.
        void loadFrom(Offset point)
        {
            for (size_t valueIndex = 0; valueIndex < values.size(); ++valueIndex)
            {
                const auto& input = inputs[valueIndex];
                values[valueIndex] = input ? input->getValue(point) : Value{};
            }
        }

        // Writes a point's values back to the output attributes.
        void storeTo(Offset point)
        {
            for (size_t valueIndex = 0; valueIndex < values.size(); ++valueIndex)
            {
                auto& output = outputs[valueIndex];
                if (output) output->setValue(point, values[valueIndex]);
            }
        }
    };

    PointScript(std::shared_ptr<CompiledScript> compiled, std::vector<AttributeBinding> bindings);

    template <typename Action> void visitBindingValues(attr::AttributeType type, Action&& action);
    template <typename Action> void visitAllBindingValues(Action&& action);
    bool resolveAttributes(const geo::Primitive& input, geo::Primitive& output, String& error);

    std::shared_ptr<CompiledScript> compiled_;
    std::vector<AttributeBinding> bindings_;

    BindingValues<floatT> floats_;
    BindingValues<intT> ints_;
    BindingValues<Vector3> vectors_;
    BindingValues<boolT> bools_;

    // The point offset followed by a pointer to each binding's value.
    std::vector<ScriptArgument> arguments_;
};

} // namespace enzo::expr
