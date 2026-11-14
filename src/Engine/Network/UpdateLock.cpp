#include "Engine/Network/UpdateLock.h"
#include "Engine/Network/NetworkManager.h"

namespace enzo {

unsigned int nt::UpdateLock::lockCounter = 0;
bool nt::UpdateLock::updateWhenUnlocked = false;

nt::UpdateLock::UpdateLock()
{
    lockCounter++;
}

nt::UpdateLock::UpdateLock(UpdateLock&& other) { other.isActive_ = false; }

nt::UpdateLock::~UpdateLock()
{
    if (!isActive_) return;

    lockCounter--;
    if (lockCounter == 0 && updateWhenUnlocked)
    {
        updateWhenUnlocked = false;
        nm().update();
    }
}

unsigned int nt::UpdateLock::getNumLocks() { return lockCounter; }

bool nt::UpdateLock::isLocked() { return lockCounter > 0; }

bool nt::UpdateLock::isUnlocked() { return !isLocked(); }

bool nt::UpdateLock::isUpdateHeld() { return updateWhenUnlocked; }

} // namespace enzo
