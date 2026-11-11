#pragma once
#include <atomic>
#include <memory>
#include <tbb/spin_mutex.h>

namespace enzo {

/**
 * @brief A value built on first read and shared with copies until one of them clears it.
 *
 * @note Reads are safe from many threads at once, clearing is not.
 */
template <typename T> class SharedCache
{
  public:
    SharedCache() = default;
    SharedCache(const SharedCache& other) { *this = other; }
    SharedCache& operator=(const SharedCache& other)
    {
        if (this == &other) return *this;
        tbb::spin_mutex::scoped_lock lock(other.mutex_);
        value_ = other.value_;
        ready_.store(other.ready_.load());
        return *this;
    }

    /// @brief Returns the value, building it first when it is missing.
    template <typename Build> const T& get(Build&& build) const
    {
        if (!ready_.load())
        {
            tbb::spin_mutex::scoped_lock lock(mutex_);
            if (!ready_.load())
            {
                value_ = std::make_shared<const T>(build());
                ready_.store(true);
            }
        }
        return *value_;
    }

    /// @brief Marks the value as stale so the next read builds it again.
    void clear() { ready_.store(false); }

  private:
    mutable std::shared_ptr<const T> value_;
    mutable std::atomic<bool> ready_{false};
    mutable tbb::spin_mutex mutex_;
};

} // namespace enzo
