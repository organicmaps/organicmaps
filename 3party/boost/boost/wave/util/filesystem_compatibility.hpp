/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_WAVE_FILESYSTEM_COMPATIBILITY_MAR_09_2009_0142PM)
#define BOOST_WAVE_FILESYSTEM_COMPATIBILITY_MAR_09_2009_0142PM

#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

namespace boost { namespace wave { namespace util
{
///////////////////////////////////////////////////////////////////////////////
// filesystem wrappers allowing to handle different Boost versions
#if !defined(BOOST_FILESYSTEM_NO_DEPRECATED)
// interface wrappers for older Boost versions
    inline boost::filesystem::path initial_path()
    {
        return boost::filesystem::initial_path(); 
    }

    inline boost::filesystem::path current_path()
    {
        return boost::filesystem::current_path(); 
    }

    template <typename String>
    inline boost::filesystem::path create_path(String const& p)
    {
        return boost::filesystem::path(p, boost::filesystem::native);
    }

    inline std::string leaf(boost::filesystem::path const& p) 
    { 
        return p.leaf(); 
    }

    inline boost::filesystem::path branch_path(boost::filesystem::path const& p) 
    { 
        return p.branch_path(); 
    }

    inline boost::filesystem::path normalize(boost::filesystem::path& p)
    {
        return p.normalize();
    }

    inline std::string native_file_string(boost::filesystem::path const& p) 
    { 
        return p.native_file_string(); 
    }

#else
// interface wrappers if deprecated functions do not exist
    inline boost::filesystem::path initial_path()
    { 
        return boost::filesystem::initial_path<boost::filesystem::path>();
    }

    inline boost::filesystem::path current_path()
    { 
        return boost::filesystem::current_path<boost::filesystem::path>();
    }

    template <typename String>
    inline boost::filesystem::path create_path(String const& p)
    {
        return boost::filesystem::path(p);
    }

    inline std::string leaf(boost::filesystem::path const& p) 
    { 
        return p.filename(); 
    }

    inline boost::filesystem::path branch_path(boost::filesystem::path const& p) 
    { 
        return p.parent_path(); 
    }

    inline boost::filesystem::path normalize(boost::filesystem::path& p)
    {
        return p; // function doesn't exist anymore
    }

    inline std::string native_file_string(boost::filesystem::path const& p) 
    { 
        return p.file_string(); 
    }

#endif

}}}

#endif
