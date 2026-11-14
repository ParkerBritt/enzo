#pragma once

namespace enzo::nt {

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

    UpdateLock(const UpdateLock&) = delete;
    UpdateLock& operator=(const UpdateLock&) = delete;

    /// @brief Takes over the other lock, which no longer holds updates.
    UpdateLock(UpdateLock&& other);

    /**
     * @brief Returns the number of locks across all UpdateLock objects
     * @returns When this value is greater than 0 a lock is in place and the network cannot update,
     * when it's equal to zero the network is able to update again.
     */
    static unsigned int getNumLocks();

    /**
     * @brief Returns whether the lock is active or not. The lock is active as long as #getNumLocks
     * > 0
     */
    static bool isLocked();

    /**
     * @brief Returns whether the lock is active or not. The lock is active as long as #getNumLocks
     * > 0
     */
    static bool isUnlocked();

    /// @brief Returns whether an update is waiting for the last lock to be released.
    static bool isUpdateHeld();

  private:
    UpdateLock();

    // Whether this lock counts towards the locks in place, false once moved from.
    bool isActive_ = true;

    static unsigned int lockCounter;
    static bool updateWhenUnlocked;

    friend nt::NetworkManager;
};
} // namespace enzo::nt
