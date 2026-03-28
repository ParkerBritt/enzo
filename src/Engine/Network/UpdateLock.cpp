#include "Engine/Network/UpdateLock.h"
#include "Engine/Network/NetworkManager.h"
#include <icecream.hpp>

unsigned int enzo::nt::UpdateLock::lockCounter=0;

enzo::nt::UpdateLock::UpdateLock()
{
    lockCounter++;
    IC("up", lockCounter);
}

enzo::nt::UpdateLock::~UpdateLock()
{
    lockCounter--;
    IC("down", lockCounter);
    if(lockCounter == 0)
    {
        nm().update();
    }
}

unsigned int enzo::nt::UpdateLock::getNumLocks()
{
    return lockCounter;
}

bool enzo::nt::UpdateLock::isLocked()
{
    return lockCounter>0;
}

bool enzo::nt::UpdateLock::isUnlocked()
{
    return !isLocked();
}
