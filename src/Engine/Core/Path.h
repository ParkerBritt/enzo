
#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace enzo
{
    /**
    * @brief A default path class for creation and manipulation of tree-based paths.
    */
    class Path
    {
        public:
            /// @brief Constructs an empty enzo::Path object.
            Path();
            /// @brief Constructs an enzo::Path object from a given std::string.
            Path(const std::string& path);
            /// @brief Constructs an enzo::Path object from a C string.
            Path(const char* path);

            /// @brief Returns the string with surrounding whitespace removed.
            static std::string strip(const std::string& str);

            /// @brief Checks if the Path has no characters.
            bool isEmpty() const;

            /// @brief Returns true if the Path is "/".
            bool isRoot() const;

            /// @brief Returns true if the Path is an absolute path.
            bool isAbsolute() const;

            /// @brief Returns true if the Path is a relative path.
            bool isRelative() const;

            /**
            * @brief Runs every validation check and returns true if the path is fully valid.
            *
            * A path is valid when it is non empty and every component is a valid name.
            * The root path "/" is also valid.
            *
            * Examples
            *   "/geo/mesh_01" valid
            *   "geo/mesh"     valid relative path
            *   "/geo//mesh"   invalid empty component
            *   "/geo/me!sh"   invalid illegal character
            *
            * @see isValidName for the per component character rules.
            */
            virtual bool isValid() const;

            /// @brief Returns the name of the Path, ie, the last component of the path.
            std::string getName() const;

            /// @brief Returns the parent path as a Path if it exists.
            Path getParent() const;

            /**
            * @brief Returns every ancestor path leading up to this path, ordered from
            * shortest to longest. The path itself is not included.
            *
            * E.g. the path "/World/Set/Props/Chair" returns
            *   /World
            *   /World/Set
            *   /World/Set/Props
            */
            std::vector<Path> getPrefixes() const;

            /// @brief Splits all of the components of a Path and returns them as a vector of strings.
            std::vector<std::string> split() const;

            /// @brief Return the path of the Path as an std::string.
            const std::string& getString() const;

            /**
            * @brief Checks the path to make sure it fits the alphanumeric guidelines.
            *
            * @param pathString A std::string representation of the path.
            *
            * @returns true or false based on whether the path formatting is valid or not.
            *
            * @see isValidName for the per component character rules.
            */
            static bool isValidFormatting(const std::string& pathString);

            /**
            * @brief Checks the given name to make sure it fits the alphanumeric guidelines.
            *
            * A valid name is non empty and contains only alphanumeric characters and underscores.
            *
            * @param name The name that is being checked.
            *
            * @returns true or false based on whether the name is valid or not.
            */
            static bool isValidName(const std::string& name);

            /**
            * @brief Returns a new path formed by appending the given path to this one.
            *
            * The appended path may be a single component or a multi component path. A
            * leading root delimiter on the appended path is ignored so it joins as a
            * relative segment.
            *
            * @param path The path to append to the end of this path.
            *
            * @returns A new Path with @p path appended.
            */
            Path append(const enzo::Path& path) const;

            /**
            * @brief Increments the numerical suffix of the path's name by a given amount.
            *
            * A name with no numerical suffix is treated as 0, so the first increment
            * produces a "1" suffix.
            *
            * @param increment The amount to increment by. Defaults to 1.
            *
            * @returns A new Path with the incremented name.
            */
            virtual Path increment(int increment = 1) const;

            /**
            * @brief Converts an absolute path to relative, stripping the root character.
            *
            * @returns A new Path object with the new relative path.
            */
            Path makeRelative() const;

            /**
            * @brief Converts a relative path to absolute, adding a root character.
            *
            * @returns A new Path object with the absolute path.
            */
            Path makeAbsolute() const;

            /**
            * @brief Makes a path relative to a given anchor path.
            *
            * @param anchor Another Path object to act as an anchor, the method will return the original path if the anchor is the same as the path.
            *
            * @returns A Path object of the new relative path.
            */
            Path makeRelativeTo(const Path& anchor) const;

            /**
            * @brief Checks if the Path has any additional prefixes.
            *
            * @returns true or false depending on whether the Path contains any prefixes.
            */
            bool hasPrefix(const Path& prefix) const;

            /// @brief Allows a Path to be viewed as its underlying string, e.g. for comparisons against string_views.
            operator std::string_view() const;

            bool operator==(std::string_view other) const;
            bool operator!=(std::string_view other) const;

        protected:
            std::string path_;
    };

    std::ostream& operator<<(std::ostream& os, const enzo::Path& other);
}
