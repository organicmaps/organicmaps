/*
 * Boost.Foreach support for pugixml classes.
 * This file is provided to the public domain.
 * Written by Arseny Kapoulkine (arseny.kapoulkine@gmail.com)
 */

#ifndef HEADER_PUGIXML_FOREACH_HPP
#define HEADER_PUGIXML_FOREACH_HPP

#include <boost/range/iterator.hpp>

#include "pugixml.hpp"

/*
 * These types add support for BOOST_FOREACH macro to xml_node and xml_document classes (child iteration only).
 * Example usage:
 * BOOST_FOREACH(xml_node n, doc) {}
 */

namespace boost
{
	template<> struct range_mutable_iterator<pugi::xml_node>
	{
		typedef pugi::xml_node::iterator type;
	};

	template<> struct range_const_iterator<pugi::xml_node>
	{
		typedef pugi::xml_node::iterator type;
	};

	template<> struct range_mutable_iterator<pugi::xml_document>
	{
		typedef pugi::xml_document::iterator type;
	};

	template<> struct range_const_iterator<pugi::xml_document>
	{
		typedef pugi::xml_document::iterator type;
	};
}

/*
 * These types add support for BOOST_FOREACH macro to xml_node and xml_document classes (child/attribute iteration).
 * Example usage:
 * BOOST_FOREACH(xml_node n, children(doc)) {}
 * BOOST_FOREACH(xml_node n, attributes(doc)) {}
 */

namespace pugi
{
	inline xml_object_range<xml_node_iterator> children(const pugi::xml_node& node)
	{
		return node.children();
	}

	inline xml_object_range<xml_attribute_iterator> attributes(const pugi::xml_node& node)
	{
		return node.attributes();
	}
}

#endif
