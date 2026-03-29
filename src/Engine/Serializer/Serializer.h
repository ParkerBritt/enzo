#include "Engine/Network/NetworkManager.h"

namespace enzo::nt
{

class Serializer
{
    public:
        void save(NetworkManager& networkManager);
        void load(NetworkManager& networkManager);

};

}
