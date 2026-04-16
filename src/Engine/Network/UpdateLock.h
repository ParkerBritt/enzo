#pragma once

namespace enzo::nt
{

// forward declaration
class NetworkManager;

/**
* @class UpdateLock
* @brief Prevents updates on the network manager while in scope
*
* See #nt::NetworkManager for constructing
*/
class UpdateLock
{
public:
    ~UpdateLock();

    /**
    * @brief Returns the number of locks across all UpdateLock objects
    * @returns When this value is greater than 0 a lock is in place and the network cannot update, when it's equal to zero the network is able to update again.
    */
    static unsigned int getNumLocks();

    /**
    * @brief Returns whether the lock is active or not. The lock is active as long as #getNumLocks > 0
    */
    static bool isLocked();

    /**
    * @brief Returns whether the lock is active or not. The lock is active as long as #getNumLocks > 0
    */
    static bool isUnlocked();


private:
    UpdateLock();

    static unsigned int lockCounter;

    friend nt::NetworkManager;
};
}
