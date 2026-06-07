#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Template.h"
#include <boost/signals2.hpp>
#include <memory>
#include <string_view>
#include <variant>

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

    // Read every value the parameter holds as a list. A scalar parameter
    // returns one element, a vector parameter one per component, and a
    // multi-value parameter such as a multi select dropdown one per selection.
    std::vector<floatT> evalFloats() const;
    std::vector<String> evalStrings() const;
    std::vector<intT> evalInts() const;

    void setInt(intT value, unsigned int index = 0);
    void setFloat(floatT value, unsigned int index = 0);
    void setString(String value, unsigned int index = 0);

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
    std::vector<std::shared_ptr<Parameter>> buildInstance_(unsigned int instanceIndex);

    Template template_;
    PrmValues values_;
    std::vector<std::vector<std::shared_ptr<Parameter>>> instances_;
};
} // namespace enzo::prm
