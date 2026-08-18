#include "Engine/Network/NetworkManager.h"

namespace enzo::nt {

class Serializer
{
  public:
    /// @brief Writes a network out to a file.
    void save(Network& network, std::string filePath);

    /// @brief Reads a file into the network, replacing whatever was there.
    /// @note Loading creates nodes, which runs through undo, cooking, and the signals the
    /// interface listens to, so it goes through the manager rather than the network alone.
    void load(NetworkManager& networkManager, std::string filePath);
};

} // namespace enzo::nt
