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

            /// @brief Checks if the Path has no characters.
            bool IsEmpty() const;

            /// @brief Returns true if the Path is "/".
            bool IsRoot() const;

            /// @brief Returns true if the Path is an absolute path.
            bool IsAbsolute() const;

            /// @brief Returns true if the Path is a relative path.
            bool IsRelative() const;

            /// @brief Runs multiple validation checks and returns true if the path is fully valid.
            bool IsValid() const;

            /// @brief Returns the name of the Path, ie, the last component of the path.
            std::string GetName() const;

            /// @brief Returns the parent path as a Path if it exists.
            Path GetParentPath() const;

            /// @brief Returns a vector of Path objects of all the prefixes to the specified path.
            std::vector<Path> GetPrefixes() const;

            /// @brief Splits all of the components of a Path and returns them as a vector of strings.
            std::vector<std::string> SplitPath() const;

            /// @brief Return the path of the Path as an std::string.
            const std::string& GetString() const;

            /**
            * @brief Checks the path to make sure it fits the alphanumeric guidelines.
            *
            * @param path A std::string representation of the path.
            *
            * @returns true or false based on whether the path formatting is valid or not.
            */
            static bool IsValidFormatting(const std::string& path);

            /**
            * @brief Checks the given name to make sure it fits the alphanumeric guidelines.
            *
            * @param name The name that is being checked.
            *
            * @returns true or false based on whether the name is valid or not.
            */
            static bool IsValidName(const std::string& name);

            /**
            * @brief Appends the name to the end of the path and constructs a new Path based off this.
            *
            * @param name The correctly formatted name of the child you want to append to the end of the path.
            *
            * @returns A Path object of the path the child has been appended to.
            */
            Path AppendChild(std::string name) const;

            /**
            * @brief Appends the path to the end of the path and constructs a new Path based off this.
            *
            * @param The Path object you want to append to the end of the path.
            *
            * @returns A Path object of the path the path has been appended to.
            */
            Path AppendPath(const enzo::Path& path) const;

            /**
            * @brief Increments the given path by 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            Path IncrementName() const;

            /**
            * @brief Increments the given path by 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            Path MakeRelative() const;

            /**
            * @brief Increments the given path by 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            Path MakeAbsolute() const;

            /**
            * @brief Increments the given path by 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            Path MakeRelativeTo(const Path& anchor) const;

            /**
            * @brief Increments the given path by 1.
            *
            * @returns A Path object with the incremented value or a value of 1 if no incrementation exists.
            */
            bool HasPrefix(const Path& prefix) const;

            bool operator==(const Path& other) const;
            bool operator!=(const Path& other) const;
        
        protected:
            std::string path_;
    };

    std::ostream& operator<<(std::ostream& os, const enzo::Path& other);
}
