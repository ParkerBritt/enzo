#include "Engine/Parameter/Default.h"

enzo::prm::Default::Default()
: floatDefault_{0}, stringDefault_{""}
{

}

enzo::prm::Default::Default(floatT floatDefault)
: floatDefault_{floatDefault}, stringDefault_{""}
{

}

enzo::prm::Default::Default(const char *stringDefault)
: floatDefault_{0}, stringDefault_{stringDefault}
{

}

enzo::prm::Default::Default(int intDefault)
: floatDefault_(intDefault), stringDefault_{""}
{

}

enzo::prm::Default::Default(bool boolDefault)
: floatDefault_(boolDefault), stringDefault_{""}
{

}
