#pragma once

#include "Engine/Types.h"
#include <boost/signals2.hpp>

namespace enzo::nt
{
class GeometryOperator;

/**
* @class GeometryConnection
* @brief Directional edges to connect nodes
*
*
* @todo Currently geometry connections are stored on nodes. It might
* make more sense to move them to a container within the network manager
* so they're all in the same place
*
* Input and output are in relation to data flow.
*
* @todo Currently geometry connections are stored on nodes. It might
* make more sense to move them to a container within the network manager
* so they're all in the same place
*/

class GeometryConnection
{
public:
    /**
    * @brief Constructs a connection between two nodes
    *
    * Input and output are in relation to data flow.
    *
    * @param inputOpId The [Operator ID](@ref OpId) in which data flows from.
    * @param outputOpId The [Operator ID](@ref OpId) in which data flows to.
    * @param inputOpIndex The output socket number of @p inputOpId in which data flows from.
    * @param outputOpIndex The input socket number of @p outputOpId in which data flows to.
    */
    GeometryConnection(enzo::nt::OpId inputOpId, unsigned int inputOpIndex, enzo::nt::OpId outputOpId, unsigned int outputOpIndex);

    bool operator==(const enzo::nt::GeometryConnection& other) const
    {
        return (
            getInputOpId()==other.getInputOpId() &&
            getOutputOpId()==other.getOutputOpId() &&
            getInputIndex()==other.getInputIndex() &&
            getOutputIndex()==other.getOutputIndex()
        );
    }

    
    /// @brief Returns the [Operator ID](@ref OpId) of the connection input (where data flows from).
    enzo::nt::OpId getInputOpId() const;
    /// @brief Returns the [Operator ID](@ref OpId) of the connection output (where data flows to).
    enzo::nt::OpId getOutputOpId() const;
    /// @brief Returns the socket number of #getInputOpId in which data flows from.
    unsigned int getInputIndex() const;
    /// @brief Returns the socket number of #getOutputOpId in which data flows to.
    unsigned int getOutputIndex() const;

    /// @brief Provides an ostream representation of the connection, useful for debugging.
    friend std::ostream& operator<<(std::ostream& os, const GeometryConnection& p)
    {
        return os << p.inputOperatorId_ << ":" << p.inputIndex_ << " -> " << p.outputOperatorId_ << ":" << p.outputIndex_ << "\n";
    }

    /// @brief A signal emitted when the connection is removed
    boost::signals2::signal<void ()> removed;
    /// @brief Removes the connection from it's associated nodes. Does not delete the object.
    void remove();
    // bool isValid();
private:
    enzo::nt::OpId inputOperatorId_;
    enzo::nt::OpId outputOperatorId_;
    unsigned int inputIndex_;
    unsigned int outputIndex_;
};
}
