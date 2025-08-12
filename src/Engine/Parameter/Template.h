#pragma once
#include "Engine/Parameter/Default.h"
#include "Engine/Parameter/PrmName.h"
#include "Engine/Parameter/Range.h"
#include "Engine/Types.h"

namespace enzo::prm
{

class Template
{
public:
    Template(
        enzo::prm::Type type,
        prm::Name name
    );
    Template(
        enzo::prm::Type type,
        prm::Name name,
        unsigned int vectorSize
    );
    Template(
        enzo::prm::Type type,
        prm::Name name,
        std::vector<prm::Default> defaults,
        unsigned int vectorSize = 1,
        std::vector<prm::Range> ranges=std::vector<prm::Range>()
    );
    Template(
        enzo::prm::Type type,
        prm::Name name,
        prm::Default theDefault,
        unsigned int vectorSize = 1,
        Range range=Range()
    );
    Template();
    // get name and get token are identical
    enzo::bt::String getName() const;
    enzo::bt::String getToken() const;
    enzo::bt::String getLabel() const;
    const prm::Default getDefault(unsigned int index=0) const;
    const prm::Range& getRange(unsigned int index=0) const;
    const prm::Type getType() const;
    const unsigned int getSize() const;
    const unsigned int getNumDefaults() const;
    // bool isValid() const;
private:
    enzo::prm::Type type_;
    std::vector<prm::Default> defaults_;
    std::vector<prm::Range> ranges_;
    // TODO: make a class that holds token and name
    prm::Name name_;
    unsigned int vectorSize_;

};

inline enzo::prm::Template Terminator = enzo::prm::Template();

}

