#pragma once

#include "Engine/Core/Path.h"
#include <cstddef>
#include <string>
#include <string_view>

namespace enzo
{
    /**
    * @brief A path that references a network entity, either a node or a parameter on a node.
    *
    * A node is written as a plain path. A parameter adds a "." and the parameter name onto the
    * final component of an otherwise ordinary node path.
    *
    * Examples
    *   "/geo/mesh"     references a node
    *   "/geo/mesh.tx"  references the "tx" parameter on that node
    */
    class NetworkPath : public Path
    {
        public:
            /// @brief Constructs an empty NetworkPath.
            NetworkPath() = default;
            /// @brief Constructs a NetworkPath from a given std::string.
            NetworkPath(const std::string& path);
            /// @brief Constructs a NetworkPath from a C string.
            NetworkPath(const char* path);

            /**
            * @brief Runs every validation check and returns true if the path is a valid node or parameter reference.
            *
            * The node portion follows the same component rules as Path and the optional parameter
            * must be a valid name.
            *
            * @see Path::isValidName for the per component character rules.
            */
            bool isValid() const override;

            /// @brief Checks whether the path carries a parameter component.
            bool hasParameter() const;

            /// @brief Returns the parameter name, or an empty string when the path references a node.
            std::string getParameter() const;

            /**
            * @brief Returns the node portion of the path with any parameter removed.
            *
            * A path that already references a node returns an equivalent copy.
            *
            * E.g. "/geo/mesh.tx" returns "/geo/mesh"
            */
            NetworkPath getNode() const;

            /**
            * @brief Checks whether a string is a valid node or parameter reference.
            *
            * @param pathString A std::string representation of the path.
            *
            * @returns true when the node portion and any parameter are both valid.
            */
            static bool isValidFormatting(const std::string& pathString);

        private:
            /**
            * @brief Returns the index of the "." that begins the parameter, or npos when there is none.
            *
            * Only the final component may carry a parameter, so the search starts after the last slash.
            */
            static size_t parameterDelimiter(std::string_view pathString);
    };
}
