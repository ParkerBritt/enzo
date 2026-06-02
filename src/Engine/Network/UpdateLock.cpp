#include "Engine/Network/UpdateLock.h"
#include "Engine/Network/NetworkManager.h"
#include <icecream.hpp>

namespace enzo {

unsigned int nt::UpdateLock::lockCounter=0;

nt::UpdateLock::UpdateLock()
{
    lockCounter++;
    IC("up", lockCounter);
}

nt::UpdateLock::~UpdateLock()
{
    lockCounter--;
    IC("down", lockCounter);
    if(lockCounter == 0)
    {
        nm().update();
    }
}

unsigned int nt::UpdateLock::getNumLocks()
{
    return lockCounter;
}

bool nt::UpdateLock::isLocked()
{
    return lockCounter>0;
}

bool nt::UpdateLock::isUnlocked()
{
    return !isLocked();
}

} // namespace enzo
