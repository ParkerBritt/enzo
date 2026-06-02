#include "Engine/Parameter/Default.h"

namespace enzo {

prm::Default::Default()
: floatDefault_{0}, stringDefault_{""}
{

}

prm::Default::Default(floatT floatDefault)
: floatDefault_{floatDefault}, stringDefault_{""}
{

}

prm::Default::Default(const char *stringDefault)
: floatDefault_{0}, stringDefault_{stringDefault}
{

}

prm::Default::Default(int intDefault)
: floatDefault_(intDefault), stringDefault_{""}
{

}

prm::Default::Default(bool boolDefault)
: floatDefault_(boolDefault), stringDefault_{""}
{

}

} // namespace enzo
