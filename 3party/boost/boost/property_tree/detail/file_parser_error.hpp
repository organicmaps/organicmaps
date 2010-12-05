// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_FILE_PARSER_ERROR_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_FILE_PARSER_ERROR_HPP_INCLUDED

#include <boost/property_tree/ptree.hpp>
#include <string>

namespace boost { namespace property_tree
{

    //! File parse error
    class file_parser_error: public ptree_error
    {

    public:

        ///////////////////////////////////////////////////////////////////////
        // Construction & destruction

        // Construct error
        file_parser_error(const std::string &message,
                          const std::string &filename,
                          unsigned long line) :
            ptree_error(format_what(message, filename, line)),
            m_message(message), m_filename(filename), m_line(line)
        {
        }

        ~file_parser_error() throw()
            // gcc 3.4.2 complains about lack of throw specifier on compiler
            // generated dtor
        {
        }

        ///////////////////////////////////////////////////////////////////////
        // Data access

        // Get error message (without line and file - use what() to get
        // full message)
        std::string message()
        {
            return m_message;
        }

        // Get error filename
        std::string filename()
        {
            return m_filename;
        }

        // Get error line number
        unsigned long line()
        {
            return m_line;
        }

    private:

        std::string m_message;
        std::string m_filename;
        unsigned long m_line;

        // Format error message to be returned by std::runtime_error::what()
        std::string format_what(const std::string &message,
                                const std::string &filename,
                                unsigned long line)
        {
            std::stringstream stream;
            if (line > 0)
                stream << (filename.empty() ? "<unspecified file>"
                                            : filename.c_str())
                       << '(' << line << "): "
                       << message;
            else
                stream << (filename.empty() ? "<unspecified file>"
                                            : filename.c_str())
                       << ": " << message;
            return stream.str();
        }

    };

} }

#endif
