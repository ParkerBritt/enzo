#include "Engine/Network/NetworkManager.h"

namespace enzo::nt
{

class Serializer
{
    public:
        void save(NetworkManager& networkManager, std::string filePath);
        void load(NetworkManager& networkManager, std::string filePath);

};

}
