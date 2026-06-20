
#pragma once

#include <ostream>
#include <string>
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
            /// @brief Constructs a enzo::Path object from a given std::string.
            Path(const std::string& path);
            /// @brief Implicity construct an enzo::Path object.
            Path(const char*);

            static std::string Strip(const std::string& str);

            /// @brief Checks if the Path has no characters.
            bool isEmpty() const;

            /// @brief Returns true if the Path is "/".
            bool isRoot() const;

            /// @brief Returns true if the Path is an absolute path.
            bool isAbsolute() const;

            /// @brief Returns true if the Path is a relative path.
            bool isRelative() const;

            /** 
            * @brief Runs multiple validation checks and returns true if the path is fully valid.
            *
            * Path class only accepts alphanumeric values and underscores.
            */
            virtual bool isValid() const;

            /// @brief Returns the name of the Path, ie, the last component of the path.
            std::string getName() const;

            /// @brief Returns the parent path as a Path if it exists.
            Path getParent() const;

            /// @brief Returns a vector of Path objects of all the prefixes to the specified path.
            std::vector<Path> getPrefixes() const;

            /// @brief Splits all of the components of a Path and returns them as a vector of strings.
            std::vector<std::string> split() const;

            /// @brief Return the path of the Path as an std::string.
            const std::string& getString() const;

            /**
            * @brief Checks the path to make sure it fits the alphanumeric guidelines.
            *
            * @param path A std::string representation of the path.
            *
            * @returns true or false based on whether the path formatting is valid or not.
            */
            static bool isValidFormatting(const std::string& pathString);

            /**
            * @brief Checks the given name to make sure it fits the alphanumeric guidelines.
            *
            * @param name The name that is being checked.
            *
            * @returns true or false based on whether the name is valid or not.
            */
            static bool isValidName(const std::string& name);

            /**
            * @brief Appends the name to the end of the path and constructs a new Path based off this.
            *
            * @param name The correctly formatted name of the child you want to append to the end of the path.
            *
            * @returns A Path object of the path the child has been appended to.
            */
            Path join(std::string name) const;

            /**
            * @brief Appends the path to the end of the path and constructs a new Path based off this.
            *
            * @param name The Path object you want to append to the end of the path.
            *
            * @returns A Path object of the path the path has been appended to.
            */
            Path joinPath(const enzo::Path& path) const;

            /**
            * @brief Increments the given path by an increment value which is defaulted at 1.
            *
            * @param increment The value you want to increment the path by. Default is 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            Path increment(int increment = 1) const;

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

            bool operator==(const Path& other) const;
            bool operator!=(const Path& other) const;
        
        protected:
            std::string path_;
    };

    std::ostream& operator<<(std::ostream& os, const enzo::Path& other);
}
