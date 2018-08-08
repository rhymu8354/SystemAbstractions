#ifndef SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_HPP
#define SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_HPP

/**
 * @file DynamicLibrary.hpp
 *
 * This module declares the SystemAbstractions::DynamicLibrary class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     * This class represents a dynamically loaded library.
     */
    class DynamicLibrary {
        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        DynamicLibrary();

        /**
         * This is an instance constructor.
         *
         * @param impl
         *     This is an existing set of properties to encapsulate
         *     into a new object.
         */
        DynamicLibrary(std::unique_ptr< struct DynamicLibraryImpl >&& impl) noexcept;

        /**
         * This is the instance move constructor.
         */
        DynamicLibrary(DynamicLibrary&& other) noexcept;

        /**
         * This is the instance destructor.
         */
        ~DynamicLibrary();

        /**
         * This is the move assignment operator.
         */
        DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;

        /**
         * This method loads the dynamic library from storage,
         * linking it into the running program.
         *
         * @param[in] path
         *     This is the path to the directory containing
         *     the dynamic library.
         *
         * @param[in] name
         *     This is the name of the dynamic library, without
         *     and prefix, suffix, or file extension.
         *
         * @return
         *     An indication of whether or not the library was successfully
         *     loaded and linked to the program is returned.
         */
        bool Load(const std::string& path, const std::string& name);

        /**
         * This method unlinks the dynamic library from the running program.
         *
         * @note
         *     Do not call this method until all objects and resources
         *     (i.e. threads) from the library have been destroyed,
         *     otherwise the program may crash.
         */
        void Unload();

        /**
         * This method locates the procedure (function) that has the given
         * name in the library, and returns its address.
         *
         * @note
         *     This method should only be called while the library is loaded.
         *
         * @param[in] name
         *     This is the name of the function in the library to locate.
         *
         * @return
         *     The address of the given function in the library is returned.
         *
         * @retval nullptr
         *     This is returned if the loader could not find any function
         *     with the given name in the library.
         */
        void* GetProcedure(const std::string& name);

        /**
         * This method returns a human-readable string describing
         * the last error that occurred calling another
         * method on the object.
         *
         * @return
         *     A human-readable string describing the last error
         *     that occurred calling another method of the
         *     object is returned.
         */
        std::string GetLastError();

        // Disable copy constructor and assignment operator.
        DynamicLibrary(const DynamicLibrary&) = delete;
        DynamicLibrary& operator=(const DynamicLibrary&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct DynamicLibraryImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_HPP */
