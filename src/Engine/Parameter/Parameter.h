#pragma once
#include "Engine/Core/Types.h"
#include "Engine/Parameter/Template.h"
#include <boost/signals2.hpp>
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
    unsigned int getVectorSize() const;

    floatT evalFloat(unsigned int index = 0) const;
    String evalString(unsigned int index = 0) const;
    intT evalInt(unsigned int index = 0) const;

    void setInt(intT value, unsigned int index = 0);
    void setFloat(floatT value, unsigned int index = 0);
    void setString(String value, unsigned int index = 0);

    PrmValues getValues() const;
    void setValues(const PrmValues& values);

    const Template& getTemplate();

    boost::signals2::signal<void()> valueChanged;

  protected:
    virtual void onFloatSet_(const PrmValues& before) {}
    void handleValueChange_();

    Template template_;
    PrmValues values_;
};
} // namespace enzo::prm
