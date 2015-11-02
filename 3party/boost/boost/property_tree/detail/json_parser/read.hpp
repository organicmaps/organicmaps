// ----------------------------------------------------------------------------
// Copyright (C) 2015 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------
#ifndef BOOST_PROPERTY_TREE_DETAIL_JSON_PARSER_READ_HPP
#define BOOST_PROPERTY_TREE_DETAIL_JSON_PARSER_READ_HPP

#include <boost/property_tree/detail/json_parser/parser.hpp>
#include <boost/property_tree/detail/json_parser/narrow_encoding.hpp>
#include <boost/property_tree/detail/json_parser/wide_encoding.hpp>
#include <boost/property_tree/detail/json_parser/standard_callbacks.hpp>

#include <boost/range/iterator_range_core.hpp>

#include <istream>
#include <iterator>
#include <string>

namespace boost { namespace property_tree {
    namespace json_parser { namespace detail
{

    template <typename Ch> struct encoding;
    template <> struct encoding<char> : utf8_utf8_encoding {};
    template <> struct encoding<wchar_t> : wide_wide_encoding {};

    template <typename Ptree>
    void read_json_internal(
        std::basic_istream<typename Ptree::key_type::value_type> &stream,
        Ptree &pt, const std::string &filename)
    {
        typedef typename Ptree::key_type::value_type char_type;
        typedef standard_callbacks<Ptree> callbacks_type;
        typedef detail::encoding<char_type> encoding_type;
        typedef std::istreambuf_iterator<char_type> iterator;
        callbacks_type callbacks;
        encoding_type encoding;
        detail::parser<callbacks_type, encoding_type, iterator, iterator>
            parser(callbacks, encoding);
        parser.set_input(filename,
            boost::make_iterator_range(iterator(stream), iterator()));
        parser.parse_value();
        parser.finish();

        pt.swap(callbacks.output());
    }

}}}}

#endif
