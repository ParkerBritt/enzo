#pragma once
#include "Engine/Parameter/Template.h"
#include "Engine/Types.h"
#include <boost/signals2.hpp>
#include <variant>

namespace enzo::prm
{

using PrmValues = std::variant<std::vector<bt::floatT>, std::vector<bt::intT>, std::vector<bt::String>>;

class Parameter
{
public:
    Parameter(Template prmTemplate);
    std::string getName() const;
    std::string getLabel() const;
    enzo::prm::Type getType() const;
    unsigned int getVectorSize() const;

    bt::floatT evalFloat(unsigned int index=0) const;
    bt::String evalString(unsigned int index=0) const;
    bt::intT evalInt(unsigned int index=0) const;
    
    void setInt(bt::intT value, unsigned int index=0);
    void setFloat(bt::floatT value, unsigned int index=0);
    void setString(bt::String value, unsigned int index=0);

    PrmValues getValues() const;
    void setValues(const PrmValues& values);

    const Template& getTemplate();

    boost::signals2::signal<void ()> valueChanged;
private:
    Template template_;
    PrmValues values_;

};
}
