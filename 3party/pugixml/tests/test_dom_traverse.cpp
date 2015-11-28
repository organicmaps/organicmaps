#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_DEPRECATE

#include "common.hpp"

#include <string.h>
#include <stdio.h>
#include <wchar.h>

#include <utility>
#include <vector>
#include <iterator>
#include <string>

#include "helpers.hpp"

#ifdef PUGIXML_NO_STL
template <typename I> static I move_iter(I base, int n)
{
	if (n > 0) while (n--) ++base;
	else while (n++) --base;
	return base;
}
#else
template <typename I> static I move_iter(I base, int n)
{
	std::advance(base, n);
	return base;
}
#endif

TEST_XML(dom_attr_bool_ops, "<node attr='1'/>")
{
	generic_bool_ops_test(doc.child(STR("node")).attribute(STR("attr")));
}

TEST_XML(dom_attr_eq_ops, "<node attr1='1' attr2='2'/>")
{
	generic_eq_ops_test(doc.child(STR("node")).attribute(STR("attr1")), doc.child(STR("node")).attribute(STR("attr2")));
}

TEST_XML(dom_attr_rel_ops, "<node attr1='1' attr2='2'/>")
{
	generic_rel_ops_test(doc.child(STR("node")).attribute(STR("attr1")), doc.child(STR("node")).attribute(STR("attr2")));
}

TEST_XML(dom_attr_empty, "<node attr='1'/>")
{
	generic_empty_test(doc.child(STR("node")).attribute(STR("attr")));
}

TEST_XML(dom_attr_next_previous_attribute, "<node attr1='1' attr2='2' />")
{
	xml_attribute attr1 = doc.child(STR("node")).attribute(STR("attr1"));
	xml_attribute attr2 = doc.child(STR("node")).attribute(STR("attr2"));

	CHECK(attr1.next_attribute() == attr2);
	CHECK(attr2.next_attribute() == xml_attribute());
	
	CHECK(attr1.previous_attribute() == xml_attribute());
	CHECK(attr2.previous_attribute() == attr1);
	
	CHECK(xml_attribute().next_attribute() == xml_attribute());
	CHECK(xml_attribute().previous_attribute() == xml_attribute());
}

TEST_XML(dom_attr_name_value, "<node attr='1'/>")
{
	xml_attribute attr = doc.child(STR("node")).attribute(STR("attr"));

	CHECK_NAME_VALUE(attr, STR("attr"), STR("1"));
	CHECK_NAME_VALUE(xml_attribute(), STR(""), STR(""));
}

TEST_XML(dom_attr_as_string, "<node attr='1'/>")
{
	xml_attribute attr = doc.child(STR("node")).attribute(STR("attr"));

	CHECK_STRING(attr.as_string(), STR("1"));
	CHECK_STRING(xml_attribute().as_string(), STR(""));
}

TEST_XML(dom_attr_as_int, "<node attr1='1' attr2='-1' attr3='-2147483648' attr4='2147483647' attr5='0'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_int() == 0);
	CHECK(node.attribute(STR("attr1")).as_int() == 1);
	CHECK(node.attribute(STR("attr2")).as_int() == -1);
	CHECK(node.attribute(STR("attr3")).as_int() == -2147483647 - 1);
	CHECK(node.attribute(STR("attr4")).as_int() == 2147483647);
	CHECK(node.attribute(STR("attr5")).as_int() == 0);
}

TEST_XML(dom_attr_as_int_hex, "<node attr1='0777' attr2='0x5ab' attr3='0XFf' attr4='-0x20' attr5='-0x80000000' attr6='0x'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == 777); // no octal support! intentional
	CHECK(node.attribute(STR("attr2")).as_int() == 1451);
	CHECK(node.attribute(STR("attr3")).as_int() == 255);
	CHECK(node.attribute(STR("attr4")).as_int() == -32);
	CHECK(node.attribute(STR("attr5")).as_int() == -2147483647 - 1);
	CHECK(node.attribute(STR("attr6")).as_int() == 0);
}

TEST_XML(dom_attr_as_uint, "<node attr1='0' attr2='1' attr3='2147483647' attr4='4294967295' attr5='0'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_uint() == 0);
	CHECK(node.attribute(STR("attr1")).as_uint() == 0);
	CHECK(node.attribute(STR("attr2")).as_uint() == 1);
	CHECK(node.attribute(STR("attr3")).as_uint() == 2147483647);
	CHECK(node.attribute(STR("attr4")).as_uint() == 4294967295u);
	CHECK(node.attribute(STR("attr5")).as_uint() == 0);
}

TEST_XML(dom_attr_as_uint_hex, "<node attr1='0777' attr2='0x5ab' attr3='0XFf' attr4='0x20' attr5='0xFFFFFFFF' attr6='0x'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_uint() == 777); // no octal support! intentional
	CHECK(node.attribute(STR("attr2")).as_uint() == 1451);
	CHECK(node.attribute(STR("attr3")).as_uint() == 255);
	CHECK(node.attribute(STR("attr4")).as_uint() == 32);
	CHECK(node.attribute(STR("attr5")).as_uint() == 4294967295u);
	CHECK(node.attribute(STR("attr6")).as_uint() == 0);
}

TEST_XML(dom_attr_as_integer_space, "<node attr1=' \t1234' attr2='\t 0x123' attr3='- 16' attr4='- 0x10'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == 1234);
	CHECK(node.attribute(STR("attr2")).as_int() == 291);
	CHECK(node.attribute(STR("attr3")).as_int() == 0);
	CHECK(node.attribute(STR("attr4")).as_int() == 0);
}

TEST_XML(dom_attr_as_float, "<node attr1='0' attr2='1' attr3='0.12' attr4='-5.1' attr5='3e-4' attr6='3.14159265358979323846'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_float() == 0);
	CHECK_DOUBLE(node.attribute(STR("attr1")).as_float(), 0);
	CHECK_DOUBLE(node.attribute(STR("attr2")).as_float(), 1);
	CHECK_DOUBLE(node.attribute(STR("attr3")).as_float(), 0.12);
	CHECK_DOUBLE(node.attribute(STR("attr4")).as_float(), -5.1);
	CHECK_DOUBLE(node.attribute(STR("attr5")).as_float(), 3e-4);
	CHECK_DOUBLE(node.attribute(STR("attr6")).as_float(), 3.14159265358979323846);
}

TEST_XML(dom_attr_as_double, "<node attr1='0' attr2='1' attr3='0.12' attr4='-5.1' attr5='3e-4' attr6='3.14159265358979323846'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_double() == 0);
	CHECK_DOUBLE(node.attribute(STR("attr1")).as_double(), 0);
	CHECK_DOUBLE(node.attribute(STR("attr2")).as_double(), 1);
	CHECK_DOUBLE(node.attribute(STR("attr3")).as_double(), 0.12);
	CHECK_DOUBLE(node.attribute(STR("attr4")).as_double(), -5.1);
	CHECK_DOUBLE(node.attribute(STR("attr5")).as_double(), 3e-4);
	CHECK_DOUBLE(node.attribute(STR("attr6")).as_double(), 3.14159265358979323846);
}

TEST_XML(dom_attr_as_bool, "<node attr1='0' attr2='1' attr3='true' attr4='True' attr5='Yes' attr6='yes' attr7='false'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(!xml_attribute().as_bool());
	CHECK(!node.attribute(STR("attr1")).as_bool());
	CHECK(node.attribute(STR("attr2")).as_bool());
	CHECK(node.attribute(STR("attr3")).as_bool());
	CHECK(node.attribute(STR("attr4")).as_bool());
	CHECK(node.attribute(STR("attr5")).as_bool());
	CHECK(node.attribute(STR("attr6")).as_bool());
	CHECK(!node.attribute(STR("attr7")).as_bool());
}

#ifdef PUGIXML_HAS_LONG_LONG
TEST_XML(dom_attr_as_llong, "<node attr1='1' attr2='-1' attr3='-9223372036854775808' attr4='9223372036854775807' attr5='0'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_llong() == 0);
	CHECK(node.attribute(STR("attr1")).as_llong() == 1);
	CHECK(node.attribute(STR("attr2")).as_llong() == -1);
	CHECK(node.attribute(STR("attr3")).as_llong() == -9223372036854775807ll - 1);
	CHECK(node.attribute(STR("attr4")).as_llong() == 9223372036854775807ll);
	CHECK(node.attribute(STR("attr5")).as_llong() == 0);
}

TEST_XML(dom_attr_as_llong_hex, "<node attr1='0777' attr2='0x5ab' attr3='0XFf' attr4='-0x20' attr5='-0x8000000000000000' attr6='0x'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_llong() == 777); // no octal support! intentional
	CHECK(node.attribute(STR("attr2")).as_llong() == 1451);
	CHECK(node.attribute(STR("attr3")).as_llong() == 255);
	CHECK(node.attribute(STR("attr4")).as_llong() == -32);
	CHECK(node.attribute(STR("attr5")).as_llong() == -9223372036854775807ll - 1);
	CHECK(node.attribute(STR("attr6")).as_llong() == 0);
}

TEST_XML(dom_attr_as_ullong, "<node attr1='0' attr2='1' attr3='9223372036854775807' attr4='18446744073709551615' attr5='0'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(xml_attribute().as_ullong() == 0);
	CHECK(node.attribute(STR("attr1")).as_ullong() == 0);
	CHECK(node.attribute(STR("attr2")).as_ullong() == 1);
	CHECK(node.attribute(STR("attr3")).as_ullong() == 9223372036854775807ull);
	CHECK(node.attribute(STR("attr4")).as_ullong() == 18446744073709551615ull);
	CHECK(node.attribute(STR("attr5")).as_ullong() == 0);
}

TEST_XML(dom_attr_as_ullong_hex, "<node attr1='0777' attr2='0x5ab' attr3='0XFf' attr4='0x20' attr5='0xFFFFFFFFFFFFFFFF' attr6='0x'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_ullong() == 777); // no octal support! intentional
	CHECK(node.attribute(STR("attr2")).as_ullong() == 1451);
	CHECK(node.attribute(STR("attr3")).as_ullong() == 255);
	CHECK(node.attribute(STR("attr4")).as_ullong() == 32);
	CHECK(node.attribute(STR("attr5")).as_ullong() == 18446744073709551615ull);
	CHECK(node.attribute(STR("attr6")).as_ullong() == 0);
}
#endif

TEST(dom_attr_defaults)
{
    xml_attribute attr;

    CHECK_STRING(attr.as_string(STR("foo")), STR("foo"));
    CHECK(attr.as_int(42) == 42);
    CHECK(attr.as_uint(42) == 42);
    CHECK(attr.as_double(42) == 42);
    CHECK(attr.as_float(42) == 42);
    CHECK(attr.as_bool(true) == true);

#ifdef PUGIXML_HAS_LONG_LONG
    CHECK(attr.as_llong(42) == 42);
    CHECK(attr.as_ullong(42) == 42);
#endif
}

TEST_XML(dom_attr_iterator, "<node><node1 attr1='0'/><node2 attr1='0' attr2='1'/><node3/></node>")
{
	xml_node node1 = doc.child(STR("node")).child(STR("node1"));
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));
	xml_node node3 = doc.child(STR("node")).child(STR("node3"));

	CHECK(xml_node().attributes_begin() == xml_attribute_iterator());
	CHECK(xml_node().attributes_end() == xml_attribute_iterator());

	CHECK(node1.attributes_begin() == xml_attribute_iterator(node1.attribute(STR("attr1")), node1));
	CHECK(move_iter(node1.attributes_begin(), 1) == node1.attributes_end());
	CHECK(move_iter(node1.attributes_end(), -1) == node1.attributes_begin());
	CHECK(*node1.attributes_begin() == node1.attribute(STR("attr1")));
	CHECK_STRING(node1.attributes_begin()->name(), STR("attr1"));

	CHECK(move_iter(node2.attributes_begin(), 2) == node2.attributes_end());
	CHECK(move_iter(node2.attributes_end(), -2) == node2.attributes_begin());

	CHECK(node3.attributes_begin() != xml_attribute_iterator());
	CHECK(node3.attributes_begin() == node3.attributes_end());

	xml_attribute_iterator it = xml_attribute_iterator(node2.attribute(STR("attr2")), node2);
	xml_attribute_iterator itt = it;

	CHECK(itt++ == it);
	CHECK(itt == node2.attributes_end());

	CHECK(itt-- == node2.attributes_end());
	CHECK(itt == it);

	CHECK(++itt == node2.attributes_end());
	CHECK(itt == node2.attributes_end());

	CHECK(--itt == it);
	CHECK(itt == it);

	CHECK(++itt != it);
}

TEST_XML(dom_attr_iterator_end, "<node><node1 attr1='0'/><node2 attr1='0' attr2='1'/><node3/></node>")
{
	xml_node node1 = doc.child(STR("node")).child(STR("node1"));
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));
	xml_node node3 = doc.child(STR("node")).child(STR("node3"));

	CHECK(node1.attributes_end() != node2.attributes_end() && node1.attributes_end() != node3.attributes_end() && node2.attributes_end() != node3.attributes_end());
	CHECK(node1.attributes_end() != xml_attribute_iterator() && node2.attributes_end() != xml_attribute_iterator() && node3.attributes_end() != xml_attribute_iterator());
}

TEST_XML(dom_attr_iterator_invalidate, "<node><node1 attr1='0'/><node2 attr1='0' attr2='1'/><node3/></node>")
{
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));

	xml_attribute_iterator it1 = node2.attributes_begin();
	xml_attribute_iterator it2 = move_iter(it1, 1);
	xml_attribute_iterator it3 = move_iter(it2, 1);

	CHECK(it3 == node2.attributes_end());

	// removing attr2, it2 is invalid now, it3 is still past-the-end
	node2.remove_attribute(*it2);

	CHECK(node2.attributes_end() == it3);
	CHECK(move_iter(it1, 1) == it3);
	CHECK(move_iter(it3, -1) == it1);
	CHECK_STRING(it1->name(), STR("attr1"));

	// adding attr2 back, it3 is still past-the-end!
	xml_attribute_iterator it2new = xml_attribute_iterator(node2.append_attribute(STR("attr2-new")), node2);

	CHECK(node2.attributes_end() == it3);
	CHECK(move_iter(it1, 1) == it2new);
	CHECK(move_iter(it2new, 1) == it3);
	CHECK(move_iter(it3, -1) == it2new);
	CHECK_STRING(it2new->name(), STR("attr2-new"));

	// removing both attributes, it3 is now equal to the begin
	node2.remove_attribute(*it1);
	node2.remove_attribute(*it2new);
	CHECK(!node2.first_attribute());

	CHECK(node2.attributes_begin() == it3);
	CHECK(node2.attributes_end() == it3);
}

TEST_XML(dom_attr_iterator_const, "<node attr1='0' attr2='1'/>")
{
    pugi::xml_node node = doc.child(STR("node"));

    const pugi::xml_attribute_iterator i1 = node.attributes_begin();
    const pugi::xml_attribute_iterator i2 = ++xml_attribute_iterator(i1);
    const pugi::xml_attribute_iterator i3 = ++xml_attribute_iterator(i2);

    CHECK(*i1 == node.attribute(STR("attr1")));
    CHECK(*i2 == node.attribute(STR("attr2")));
    CHECK(i3 == node.attributes_end());

    CHECK_STRING(i1->name(), STR("attr1"));
    CHECK_STRING(i2->name(), STR("attr2"));
}

TEST_XML(dom_node_bool_ops, "<node/>")
{
	generic_bool_ops_test(doc.child(STR("node")));
}

TEST_XML(dom_node_eq_ops, "<node><node1/><node2/></node>")
{
	generic_eq_ops_test(doc.child(STR("node")).child(STR("node1")), doc.child(STR("node")).child(STR("node2")));
}

TEST_XML(dom_node_rel_ops, "<node><node1/><node2/></node>")
{
	generic_rel_ops_test(doc.child(STR("node")).child(STR("node1")), doc.child(STR("node")).child(STR("node2")));
}

TEST_XML(dom_node_empty, "<node/>")
{
	generic_empty_test(doc.child(STR("node")));
}

TEST_XML(dom_node_iterator, "<node><node1><child1/></node1><node2><child1/><child2/></node2><node3/></node>")
{
	xml_node node1 = doc.child(STR("node")).child(STR("node1"));
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));
	xml_node node3 = doc.child(STR("node")).child(STR("node3"));

	CHECK(xml_node().begin() == xml_node_iterator());
	CHECK(xml_node().end() == xml_node_iterator());

	CHECK(node1.begin() == xml_node_iterator(node1.child(STR("child1"))));
	CHECK(move_iter(node1.begin(), 1) == node1.end());
	CHECK(move_iter(node1.end(), -1) == node1.begin());
	CHECK(*node1.begin() == node1.child(STR("child1")));
	CHECK_STRING(node1.begin()->name(), STR("child1"));

	CHECK(move_iter(node2.begin(), 2) == node2.end());
	CHECK(move_iter(node2.end(), -2) == node2.begin());

	CHECK(node3.begin() != xml_node_iterator());
	CHECK(node3.begin() == node3.end());

	xml_node_iterator it = node2.child(STR("child2"));
	xml_node_iterator itt = it;

	CHECK(itt++ == it);
	CHECK(itt == node2.end());

	CHECK(itt-- == node2.end());
	CHECK(itt == it);

	CHECK(++itt == node2.end());
	CHECK(itt == node2.end());

	CHECK(--itt == it);
	CHECK(itt == it);

	CHECK(++itt != it);
}

TEST_XML(dom_node_iterator_end, "<node><node1><child1/></node1><node2><child1/><child2/></node2><node3/></node>")
{
	xml_node node1 = doc.child(STR("node")).child(STR("node1"));
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));
	xml_node node3 = doc.child(STR("node")).child(STR("node3"));

	CHECK(node1.end() != node2.end() && node1.end() != node3.end() && node2.end() != node3.end());
	CHECK(node1.end() != xml_node_iterator() && node2.end() != xml_node_iterator() && node3.end() != xml_node_iterator());
}

TEST_XML(dom_node_iterator_invalidate, "<node><node1><child1/></node1><node2><child1/><child2/></node2><node3/></node>")
{
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));

	xml_node_iterator it1 = node2.begin();
	xml_node_iterator it2 = move_iter(it1, 1);
	xml_node_iterator it3 = move_iter(it2, 1);

	CHECK(it3 == node2.end());

	// removing child2, it2 is invalid now, it3 is still past-the-end
	node2.remove_child(*it2);

	CHECK(node2.end() == it3);
	CHECK(move_iter(it1, 1) == it3);
	CHECK(move_iter(it3, -1) == it1);
	CHECK_STRING(it1->name(), STR("child1"));

	// adding attr2 back, it3 is still past-the-end!
	xml_node_iterator it2new = node2.append_child();
	it2new->set_name(STR("child2-new"));

	CHECK(node2.end() == it3);
	CHECK(move_iter(it1, 1) == it2new);
	CHECK(move_iter(it2new, 1) == it3);
	CHECK(move_iter(it3, -1) == it2new);
	CHECK_STRING(it2new->name(), STR("child2-new"));

	// removing both nodes, it3 is now equal to the begin
	node2.remove_child(*it1);
	node2.remove_child(*it2new);
	CHECK(!node2.first_child());

	CHECK(node2.begin() == it3);
	CHECK(node2.end() == it3);
}

TEST_XML(dom_node_iterator_const, "<node><child1/><child2/></node>")
{
    pugi::xml_node node = doc.child(STR("node"));

    const pugi::xml_node_iterator i1 = node.begin();
    const pugi::xml_node_iterator i2 = ++xml_node_iterator(i1);
    const pugi::xml_node_iterator i3 = ++xml_node_iterator(i2);

    CHECK(*i1 == node.child(STR("child1")));
    CHECK(*i2 == node.child(STR("child2")));
    CHECK(i3 == node.end());

    CHECK_STRING(i1->name(), STR("child1"));
    CHECK_STRING(i2->name(), STR("child2"));
}

TEST_XML(dom_node_parent, "<node><child/></node>")
{
	CHECK(xml_node().parent() == xml_node());
	CHECK(doc.child(STR("node")).child(STR("child")).parent() == doc.child(STR("node")));
	CHECK(doc.child(STR("node")).parent() == doc);
}

TEST_XML(dom_node_root, "<node><child/></node>")
{
	CHECK(xml_node().root() == xml_node());
	CHECK(doc.child(STR("node")).child(STR("child")).root() == doc);
	CHECK(doc.child(STR("node")).root() == doc);
}

TEST_XML_FLAGS(dom_node_type, "<?xml?><!DOCTYPE><?pi?><!--comment--><node>pcdata<![CDATA[cdata]]></node>", parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype)
{
	CHECK(xml_node().type() == node_null);
	CHECK(doc.type() == node_document);

	xml_node_iterator it = doc.begin();

	CHECK((it++)->type() == node_declaration);
	CHECK((it++)->type() == node_doctype);
	CHECK((it++)->type() == node_pi);
	CHECK((it++)->type() == node_comment);
	CHECK((it++)->type() == node_element);

	xml_node_iterator cit = doc.child(STR("node")).begin();
	
	CHECK((cit++)->type() == node_pcdata);
	CHECK((cit++)->type() == node_cdata);
}

TEST_XML_FLAGS(dom_node_name_value, "<?xml?><!DOCTYPE id><?pi?><!--comment--><node>pcdata<![CDATA[cdata]]></node>", parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype)
{
	CHECK_NAME_VALUE(xml_node(), STR(""), STR(""));
	CHECK_NAME_VALUE(doc, STR(""), STR(""));

	xml_node_iterator it = doc.begin();

	CHECK_NAME_VALUE(*it++, STR("xml"), STR(""));
	CHECK_NAME_VALUE(*it++, STR(""), STR("id"));
	CHECK_NAME_VALUE(*it++, STR("pi"), STR(""));
	CHECK_NAME_VALUE(*it++, STR(""), STR("comment"));
	CHECK_NAME_VALUE(*it++, STR("node"), STR(""));

	xml_node_iterator cit = doc.child(STR("node")).begin();
	
	CHECK_NAME_VALUE(*cit++, STR(""), STR("pcdata"));
	CHECK_NAME_VALUE(*cit++, STR(""), STR("cdata"));
}

TEST_XML(dom_node_child, "<node><child1/><child2/></node>")
{
	CHECK(xml_node().child(STR("n")) == xml_node());

	CHECK(doc.child(STR("n")) == xml_node());
	CHECK_NAME_VALUE(doc.child(STR("node")), STR("node"), STR(""));
	CHECK(doc.child(STR("node")).child(STR("child2")) == doc.child(STR("node")).last_child());
}

TEST_XML(dom_node_attribute, "<node attr1='0' attr2='1'/>")
{
	CHECK(xml_node().attribute(STR("a")) == xml_attribute());

	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("n")) == xml_attribute());
	CHECK_NAME_VALUE(node.attribute(STR("attr1")), STR("attr1"), STR("0"));
	CHECK(node.attribute(STR("attr2")) == node.last_attribute());
}

TEST_XML(dom_node_next_previous_sibling, "<node><child1/><child2/><child3/></node>")
{
	CHECK(xml_node().next_sibling() == xml_node());
	CHECK(xml_node().next_sibling(STR("n")) == xml_node());

	CHECK(xml_node().previous_sibling() == xml_node());
	CHECK(xml_node().previous_sibling(STR("n")) == xml_node());

	xml_node child1 = doc.child(STR("node")).child(STR("child1"));
	xml_node child2 = doc.child(STR("node")).child(STR("child2"));
	xml_node child3 = doc.child(STR("node")).child(STR("child3"));

	CHECK(child1.next_sibling() == child2);
	CHECK(child3.next_sibling() == xml_node());
	
	CHECK(child1.previous_sibling() == xml_node());
	CHECK(child3.previous_sibling() == child2);
	
	CHECK(child1.next_sibling(STR("child3")) == child3);
	CHECK(child1.next_sibling(STR("child")) == xml_node());

	CHECK(child3.previous_sibling(STR("child1")) == child1);
	CHECK(child3.previous_sibling(STR("child")) == xml_node());
}

TEST_XML(dom_node_child_value, "<node><novalue/><child1>value1</child1><child2>value2<n/></child2><child3><![CDATA[value3]]></child3>value4</node>")
{
	CHECK_STRING(xml_node().child_value(), STR(""));
	CHECK_STRING(xml_node().child_value(STR("n")), STR(""));

	xml_node node = doc.child(STR("node"));

	CHECK_STRING(node.child_value(), STR("value4"));
	CHECK_STRING(node.child(STR("child1")).child_value(), STR("value1"));
	CHECK_STRING(node.child(STR("child2")).child_value(), STR("value2"));
	CHECK_STRING(node.child(STR("child3")).child_value(), STR("value3"));
	CHECK_STRING(node.child_value(STR("child3")), STR("value3"));
	CHECK_STRING(node.child_value(STR("novalue")), STR(""));
}

TEST_XML(dom_node_first_last_attribute, "<node attr1='0' attr2='1'/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.first_attribute() == node.attribute(STR("attr1")));
	CHECK(node.last_attribute() == node.attribute(STR("attr2")));

	CHECK(xml_node().first_attribute() == xml_attribute());
	CHECK(xml_node().last_attribute() == xml_attribute());

	CHECK(doc.first_attribute() == xml_attribute());
	CHECK(doc.last_attribute() == xml_attribute());
}

TEST_XML(dom_node_first_last_child, "<node><child1/><child2/></node>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.first_child() == node.child(STR("child1")));
	CHECK(node.last_child() == node.child(STR("child2")));

	CHECK(xml_node().first_child() == xml_node());
	CHECK(xml_node().last_child() == xml_node());

	CHECK(doc.first_child() == node);
	CHECK(doc.last_child() == node);
}

TEST_XML(dom_node_find_child_by_attribute, "<node><stub attr='value3' /><child1 attr='value1'/><child2 attr='value2'/><child2 attr='value3'/></node>")
{
	CHECK(xml_node().find_child_by_attribute(STR("name"), STR("attr"), STR("value")) == xml_node());
	CHECK(xml_node().find_child_by_attribute(STR("attr"), STR("value")) == xml_node());

	xml_node node = doc.child(STR("node"));

	CHECK(node.find_child_by_attribute(STR("child2"), STR("attr"), STR("value3")) == node.last_child());
	CHECK(node.find_child_by_attribute(STR("child2"), STR("attr3"), STR("value3")) == xml_node());
	CHECK(node.find_child_by_attribute(STR("attr"), STR("value2")) == node.child(STR("child2")));
	CHECK(node.find_child_by_attribute(STR("attr3"), STR("value")) == xml_node());
}

TEST(dom_node_find_child_by_attribute_null)
{
	xml_document doc;
	xml_node node0 = doc.append_child();
	xml_node node1 = doc.append_child(STR("a"));
	xml_node node2 = doc.append_child(STR("a"));
	xml_node node3 = doc.append_child(STR("a"));

	(void)node0;

	// this adds an attribute with null name and/or value in the internal representation
	node1.append_attribute(STR(""));
	node2.append_attribute(STR("id"));
	node3.append_attribute(STR("id")) = STR("1");

	// make sure find_child_by_attribute works if name/value is null
	CHECK(doc.find_child_by_attribute(STR("unknown"), STR("wrong")) == xml_node());
	CHECK(doc.find_child_by_attribute(STR("id"), STR("wrong")) == xml_node());
	CHECK(doc.find_child_by_attribute(STR("id"), STR("")) == node2);
	CHECK(doc.find_child_by_attribute(STR("id"), STR("1")) == node3);

	CHECK(doc.find_child_by_attribute(STR("a"), STR("unknown"), STR("wrong")) == xml_node());
	CHECK(doc.find_child_by_attribute(STR("a"), STR("id"), STR("wrong")) == xml_node());
	CHECK(doc.find_child_by_attribute(STR("a"), STR("id"), STR("")) == node2);
	CHECK(doc.find_child_by_attribute(STR("a"), STR("id"), STR("1")) == node3);
}

struct find_predicate_const
{
	bool result;

	find_predicate_const(bool result_): result(result_)
	{
	}

	template <typename T> bool operator()(const T&) const
	{
		return result;
	}
};

struct find_predicate_prefix
{
	const pugi::char_t* prefix;

	find_predicate_prefix(const pugi::char_t* prefix_): prefix(prefix_)
	{
	}

	template <typename T> bool operator()(const T& obj) const
	{
	#ifdef PUGIXML_WCHAR_MODE
		// can't use wcsncmp here because of a bug in DMC
		return std::basic_string<pugi::char_t>(obj.name()).compare(0, wcslen(prefix), prefix) == 0;
	#else
		return strncmp(obj.name(), prefix, strlen(prefix)) == 0;
	#endif
	}
};

TEST_XML(dom_node_find_attribute, "<node attr1='0' attr2='1'/>")
{
	CHECK(xml_node().find_attribute(find_predicate_const(true)) == xml_attribute());

	xml_node node = doc.child(STR("node"));

	CHECK(doc.find_attribute(find_predicate_const(true)) == xml_attribute());
	CHECK(node.find_attribute(find_predicate_const(true)) == node.first_attribute());
	CHECK(node.find_attribute(find_predicate_const(false)) == xml_attribute());
	CHECK(node.find_attribute(find_predicate_prefix(STR("attr2"))) == node.last_attribute());
	CHECK(node.find_attribute(find_predicate_prefix(STR("attr"))) == node.first_attribute());
}

TEST_XML(dom_node_find_child, "<node><child1/><child2/></node>")
{
	CHECK(xml_node().find_child(find_predicate_const(true)) == xml_node());

	xml_node node = doc.child(STR("node"));

	CHECK(node.child(STR("node")).child(STR("child1")).find_child(find_predicate_const(true)) == xml_node());
	CHECK(node.find_child(find_predicate_const(true)) == node.first_child());
	CHECK(node.find_child(find_predicate_const(false)) == xml_node());
	CHECK(node.find_child(find_predicate_prefix(STR("child2"))) == node.last_child());
	CHECK(node.find_child(find_predicate_prefix(STR("child"))) == node.first_child());
}

TEST_XML(dom_node_find_node, "<node><child1/><child2/></node>")
{
	CHECK(xml_node().find_node(find_predicate_const(true)) == xml_node());

	xml_node node = doc.child(STR("node"));

	CHECK(node.child(STR("node")).child(STR("child1")).find_node(find_predicate_const(true)) == xml_node());
	CHECK(node.find_node(find_predicate_const(true)) == node.first_child());
	CHECK(node.find_node(find_predicate_const(false)) == xml_node());
	CHECK(node.find_node(find_predicate_prefix(STR("child2"))) == node.last_child());
	CHECK(node.find_node(find_predicate_prefix(STR("child"))) == node.first_child());
	CHECK(doc.find_node(find_predicate_prefix(STR("child"))) == node.first_child());
	CHECK(doc.find_node(find_predicate_prefix(STR("child2"))) == node.last_child());
	CHECK(doc.find_node(find_predicate_prefix(STR("child3"))) == xml_node());
}

#ifndef PUGIXML_NO_STL
TEST_XML(dom_node_path, "<node><child1>text<child2/></child1></node>")
{
	CHECK(xml_node().path() == STR(""));
	
	CHECK(doc.path() == STR(""));
	CHECK(doc.child(STR("node")).path() == STR("/node"));
	CHECK(doc.child(STR("node")).child(STR("child1")).path() == STR("/node/child1"));
	CHECK(doc.child(STR("node")).child(STR("child1")).child(STR("child2")).path() == STR("/node/child1/child2"));
	CHECK(doc.child(STR("node")).child(STR("child1")).first_child().path() == STR("/node/child1/"));
	
	CHECK(doc.child(STR("node")).child(STR("child1")).path('\\') == STR("\\node\\child1"));
}
#endif

TEST_XML(dom_node_first_element_by_path, "<node><child1>text<child2/></child1></node>")
{
	CHECK(xml_node().first_element_by_path(STR("/")) == xml_node());
	CHECK(xml_node().first_element_by_path(STR("a")) == xml_node());
	
	CHECK(doc.first_element_by_path(STR("")) == doc);
	CHECK(doc.first_element_by_path(STR("/")) == doc);

	CHECK(doc.first_element_by_path(STR("/node/")) == doc.child(STR("node")));
	CHECK(doc.first_element_by_path(STR("node/")) == doc.child(STR("node")));
	CHECK(doc.first_element_by_path(STR("node")) == doc.child(STR("node")));
	CHECK(doc.first_element_by_path(STR("/node")) == doc.child(STR("node")));

#ifndef PUGIXML_NO_STL
	CHECK(doc.first_element_by_path(STR("/node/child1/child2")).path() == STR("/node/child1/child2"));
#endif

	CHECK(doc.first_element_by_path(STR("/node/child2")) == xml_node());
	
	CHECK(doc.first_element_by_path(STR("\\node\\child1"), '\\') == doc.child(STR("node")).child(STR("child1")));

	CHECK(doc.child(STR("node")).first_element_by_path(STR("..")) == doc);
	CHECK(doc.child(STR("node")).first_element_by_path(STR(".")) == doc.child(STR("node")));

	CHECK(doc.child(STR("node")).first_element_by_path(STR("../node/./child1/../.")) == doc.child(STR("node")));

	CHECK(doc.child(STR("node")).first_element_by_path(STR("child1")) == doc.child(STR("node")).child(STR("child1")));
	CHECK(doc.child(STR("node")).first_element_by_path(STR("child1/")) == doc.child(STR("node")).child(STR("child1")));
	CHECK(doc.child(STR("node")).first_element_by_path(STR("child")) == xml_node());
	CHECK(doc.child(STR("node")).first_element_by_path(STR("child11")) == xml_node());

	CHECK(doc.first_element_by_path(STR("//node")) == doc.child(STR("node")));
}

struct test_walker: xml_tree_walker
{
	std::basic_string<pugi::char_t> log;
	unsigned int call_count;
	unsigned int stop_count;

	test_walker(unsigned int stop_count_ = 0): call_count(0), stop_count(stop_count_)
	{
	}

	std::basic_string<pugi::char_t> depthstr() const
	{
		char buf[32];
		sprintf(buf, "%d", depth());

	#ifdef PUGIXML_WCHAR_MODE
		wchar_t wbuf[32];
		std::copy(buf, buf + strlen(buf) + 1, &wbuf[0]);

		return std::basic_string<pugi::char_t>(wbuf);
	#else
		return std::basic_string<pugi::char_t>(buf);
	#endif
	}

	virtual bool begin(xml_node& node)
	{
		log += STR("|");
		log += depthstr();
		log += STR(" <");
		log += node.name();
		log += STR("=");
		log += node.value();

		return ++call_count != stop_count && xml_tree_walker::begin(node);
	}

	virtual bool for_each(xml_node& node)
	{
		log += STR("|");
		log += depthstr();
		log += STR(" !");
		log += node.name();
		log += STR("=");
		log += node.value();

		return ++call_count != stop_count && xml_tree_walker::end(node);
	}

	virtual bool end(xml_node& node)
	{
		log += STR("|");
		log += depthstr();
		log += STR(" >");
		log += node.name();
		log += STR("=");
		log += node.value();

		return ++call_count != stop_count;
	}
};

TEST_XML(dom_node_traverse, "<node><child>text</child></node>")
{
	test_walker walker;

	CHECK(doc.traverse(walker));

	CHECK(walker.call_count == 5);
	CHECK(walker.log == STR("|-1 <=|0 !node=|1 !child=|2 !=text|-1 >="));
}

TEST_XML(dom_node_traverse_siblings, "<node><child/><child>text</child><child/></node>")
{
	test_walker walker;

	CHECK(doc.traverse(walker));

	CHECK(walker.call_count == 7);
	CHECK(walker.log == STR("|-1 <=|0 !node=|1 !child=|1 !child=|2 !=text|1 !child=|-1 >="));
}

TEST(dom_node_traverse_empty)
{
	test_walker walker;

	CHECK(xml_node().traverse(walker));

	CHECK(walker.call_count == 2);
	CHECK(walker.log == STR("|-1 <=|-1 >="));
}

TEST_XML(dom_node_traverse_child, "<node><child>text</child></node><another>node</another>")
{
	test_walker walker;

	CHECK(doc.child(STR("node")).traverse(walker));

	CHECK(walker.call_count == 4);
	CHECK(walker.log == STR("|-1 <node=|0 !child=|1 !=text|-1 >node="));
}

TEST_XML(dom_node_traverse_stop_begin, "<node><child>text</child></node>")
{
	test_walker walker(1);

	CHECK(!doc.traverse(walker));

	CHECK(walker.call_count == 1);
	CHECK(walker.log == STR("|-1 <="));
}

TEST_XML(dom_node_traverse_stop_for_each, "<node><child>text</child></node>")
{
	test_walker walker(3);

	CHECK(!doc.traverse(walker));

	CHECK(walker.call_count == 3);
	CHECK(walker.log == STR("|-1 <=|0 !node=|1 !child="));
}

TEST_XML(dom_node_traverse_stop_end, "<node><child>text</child></node>")
{
	test_walker walker(5);

	CHECK(!doc.traverse(walker));

	CHECK(walker.call_count == 5);
	CHECK(walker.log == STR("|-1 <=|0 !node=|1 !child=|2 !=text|-1 >="));
}

TEST_XML_FLAGS(dom_offset_debug, "<?xml?><!DOCTYPE><?pi?><!--comment--><node>pcdata<![CDATA[cdata]]></node>", parse_default | parse_pi | parse_comments | parse_declaration | parse_doctype)
{
	CHECK(xml_node().offset_debug() == -1);
	CHECK(doc.offset_debug() == 0);

	xml_node_iterator it = doc.begin();

	CHECK((it++)->offset_debug() == 2);
	CHECK((it++)->offset_debug() == 16);
	CHECK((it++)->offset_debug() == 19);
	CHECK((it++)->offset_debug() == 27);
	CHECK((it++)->offset_debug() == 38);

	xml_node_iterator cit = doc.child(STR("node")).begin();
	
	CHECK((cit++)->offset_debug() == 43);
	CHECK((cit++)->offset_debug() == 58);
}

TEST(dom_offset_debug_encoding)
{
	char buf[] = { 0, '<', 0, 'n', 0, '/', 0, '>' };

	xml_document doc;
	CHECK(doc.load_buffer(buf, sizeof(buf)));

	CHECK(doc.offset_debug() == 0);
	CHECK(doc.first_child().offset_debug() == 1);
}

TEST_XML(dom_offset_debug_append, "<node/>")
{
	xml_node c1 = doc.first_child();
	xml_node c2 = doc.append_child(STR("node"));
	xml_node c3 = doc.append_child(node_pcdata);

	CHECK(doc.offset_debug() == 0);
	CHECK(c1.offset_debug() == 1);
	CHECK(c2.offset_debug() == -1);
	CHECK(c3.offset_debug() == -1);

	c1.set_name(STR("nodenode"));
	CHECK(c1.offset_debug() == -1);
}

TEST_XML(dom_offset_debug_append_buffer, "<node/>")
{
	CHECK(doc.offset_debug() == 0);
	CHECK(doc.first_child().offset_debug() == 1);

	CHECK(doc.append_buffer("<node/>", 7));
	CHECK(doc.offset_debug() == -1);
	CHECK(doc.first_child().offset_debug() == -1);
	CHECK(doc.last_child().offset_debug() == -1);
}

TEST_XML(dom_internal_object, "<node attr='value'>value</node>")
{
	xml_node node = doc.child(STR("node"));
	xml_attribute attr = node.first_attribute();
	xml_node value = node.first_child();
	
	CHECK(xml_node().internal_object() == 0);
	CHECK(xml_attribute().internal_object() == 0);

    CHECK(node.internal_object() != 0);
    CHECK(value.internal_object() != 0);
    CHECK(node.internal_object() != value.internal_object());

    CHECK(attr.internal_object() != 0);

    xml_node node_copy = node;
    CHECK(node_copy.internal_object() == node.internal_object());

    xml_attribute attr_copy = attr;
    CHECK(attr_copy.internal_object() == attr.internal_object());
}

TEST_XML(dom_hash_value, "<node attr='value'>value</node>")
{
	xml_node node = doc.child(STR("node"));
	xml_attribute attr = node.first_attribute();
	xml_node value = node.first_child();
	
	CHECK(xml_node().hash_value() == 0);
	CHECK(xml_attribute().hash_value() == 0);

    CHECK(node.hash_value() != 0);
    CHECK(value.hash_value() != 0);
    CHECK(node.hash_value() != value.hash_value());

    CHECK(attr.hash_value() != 0);

    xml_node node_copy = node;
    CHECK(node_copy.hash_value() == node.hash_value());

    xml_attribute attr_copy = attr;
    CHECK(attr_copy.hash_value() == attr.hash_value());
}

TEST_XML(dom_node_named_iterator, "<node><node1><child/></node1><node2><child/><child/></node2><node3/><node4><child/><x/></node4></node>")
{
	xml_node node1 = doc.child(STR("node")).child(STR("node1"));
	xml_node node2 = doc.child(STR("node")).child(STR("node2"));
	xml_node node3 = doc.child(STR("node")).child(STR("node3"));
	xml_node node4 = doc.child(STR("node")).child(STR("node4"));

	CHECK(xml_named_node_iterator(xml_node(), STR("child")) == xml_named_node_iterator());

	xml_object_range<xml_named_node_iterator> r1 = node1.children(STR("child"));
	xml_object_range<xml_named_node_iterator> r2 = node2.children(STR("child"));
	xml_object_range<xml_named_node_iterator> r3 = node3.children(STR("child"));
	xml_object_range<xml_named_node_iterator> r4 = node4.children(STR("child"));

	CHECK(r1.begin() != r1.end());
	CHECK(*r1.begin() == node1.first_child());
	CHECK(r1.begin() == move_iter(r1.end(), -1));
	CHECK(move_iter(r1.begin(), 1) == r1.end());

	CHECK(r2.begin() != r2.end());
	CHECK(*r2.begin() == node2.first_child());
	CHECK(*move_iter(r2.begin(), 1) == node2.last_child());
	CHECK(r2.begin() == move_iter(r2.end(), -2));
	CHECK(move_iter(r2.begin(), 1) == move_iter(r2.end(), -1));
	CHECK(move_iter(r2.begin(), 2) == r2.end());

	CHECK(r3.begin() == r3.end());
	CHECK(!(r3.begin() != r3.end()));

	CHECK(r4.begin() != r4.end());
	CHECK(*r4.begin() == node4.first_child());
	CHECK(r4.begin() == move_iter(r4.end(), -1));
	CHECK(move_iter(r4.begin(), 1) == r4.end());

	xml_named_node_iterator it = r1.begin();
	xml_named_node_iterator itt = it;

	CHECK(itt == it);

	CHECK(itt++ == it);
	CHECK(itt == r1.end());

	CHECK(itt != it);
	CHECK(itt == ++it);

	CHECK(itt-- == r1.end());
	CHECK(itt == r1.begin());

	CHECK(itt->offset_debug() == 14);
}

TEST_XML(dom_node_children_attributes, "<node1 attr1='value1' attr2='value2' /><node2 />")
{
	xml_object_range<xml_node_iterator> r1 = doc.children();

	CHECK(r1.begin() == doc.begin());
	CHECK(r1.end() == doc.end());

	xml_object_range<xml_node_iterator> r2 = xml_node().children();

	CHECK(r2.begin() == xml_node_iterator());
	CHECK(r2.end() == xml_node_iterator());

	xml_node node = doc.child(STR("node1"));

	xml_object_range<xml_attribute_iterator> r3 = node.attributes();

	CHECK(r3.begin() == node.attributes_begin());
	CHECK(r3.end() == node.attributes_end());

	xml_object_range<xml_attribute_iterator> r4 = xml_node().attributes();

	CHECK(r4.begin() == xml_attribute_iterator());
	CHECK(r4.end() == xml_attribute_iterator());
}

TEST_XML(dom_unspecified_bool_coverage, "<node attr='value'>text</node>")
{
	xml_node node = doc.first_child();

	CHECK(node);
	static_cast<void (*)(xml_node***)>(node)(0);

	CHECK(node.first_attribute());
	static_cast<void (*)(xml_attribute***)>(node.first_attribute())(0);

	CHECK(node.text());
	static_cast<void (*)(xml_text***)>(node.text())(0);

#ifndef PUGIXML_NO_XPATH
	xpath_query q(STR("/node"));

	CHECK(q);
	static_cast<void (*)(xpath_query***)>(q)(0);

	xpath_node qn = q.evaluate_node(doc);

	CHECK(qn);
	static_cast<void (*)(xpath_node***)>(qn)(0);
#endif
}

#if __cplusplus >= 201103
TEST_XML(dom_ranged_for, "<node attr1='1' attr2='2'><test>3</test><fake>5</fake><test>4</test></node>")
{
	int index = 1;

	for (xml_node n: doc.children())
	{
		for (xml_attribute a: n.attributes())
		{
			CHECK(a.as_int() == index);
			index++;
		}

		for (xml_node c: n.children(STR("test")))
		{
			CHECK(c.text().as_int() == index);
			index++;
		}
	}

	CHECK(index == 5);
}
#endif

TEST_XML(dom_node_attribute_hinted, "<node attr1='1' attr2='2' attr3='3' />")
{
	xml_node node = doc.first_child();
	xml_attribute attr1 = node.attribute(STR("attr1"));
	xml_attribute attr2 = node.attribute(STR("attr2"));
	xml_attribute attr3 = node.attribute(STR("attr3"));

	xml_attribute hint;
	CHECK(!xml_node().attribute(STR("test"), hint) && !hint);

	CHECK(node.attribute(STR("attr2"), hint) == attr2 && hint == attr3);
	CHECK(node.attribute(STR("attr3"), hint) == attr3 && !hint);

	CHECK(node.attribute(STR("attr1"), hint) == attr1 && hint == attr2);
	CHECK(node.attribute(STR("attr2"), hint) == attr2 && hint == attr3);
	CHECK(node.attribute(STR("attr1"), hint) == attr1 && hint == attr2);
	CHECK(node.attribute(STR("attr1"), hint) == attr1 && hint == attr2);

	CHECK(!node.attribute(STR("attr"), hint) && hint == attr2);
}

TEST_XML(dom_as_int_overflow, "<node attr1='-2147483649' attr2='2147483648' attr3='-4294967296' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == -2147483647 - 1);
	CHECK(node.attribute(STR("attr2")).as_int() == 2147483647);
	CHECK(node.attribute(STR("attr3")).as_int() == -2147483647 - 1);
}

TEST_XML(dom_as_uint_overflow, "<node attr1='-1' attr2='4294967296' attr3='5294967295' attr4='21474836479' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_uint() == 0);
	CHECK(node.attribute(STR("attr2")).as_uint() == 4294967295u);
	CHECK(node.attribute(STR("attr3")).as_uint() == 4294967295u);
	CHECK(node.attribute(STR("attr4")).as_uint() == 4294967295u);
}

TEST_XML(dom_as_int_hex_overflow, "<node attr1='-0x80000001' attr2='0x80000000' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == -2147483647 - 1);
	CHECK(node.attribute(STR("attr2")).as_int() == 2147483647);
}

TEST_XML(dom_as_uint_hex_overflow, "<node attr1='-0x1' attr2='0x100000000' attr3='0x123456789' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_uint() == 0);
	CHECK(node.attribute(STR("attr2")).as_uint() == 4294967295u);
	CHECK(node.attribute(STR("attr3")).as_uint() == 4294967295u);
}

TEST_XML(dom_as_int_many_digits, "<node attr1='0000000000000000000000000000000000000000000000001' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == 1);
	CHECK(node.attribute(STR("attr1")).as_uint() == 1);
}

TEST_XML(dom_as_int_hex_many_digits, "<node attr1='0x0000000000000000000000000000000000000000000000001' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == 1);
	CHECK(node.attribute(STR("attr1")).as_uint() == 1);
}

#ifdef PUGIXML_HAS_LONG_LONG
TEST_XML(dom_as_llong_overflow, "<node attr1='-9223372036854775809' attr2='9223372036854775808' attr3='-18446744073709551616' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_llong() == -9223372036854775807ll - 1);
	CHECK(node.attribute(STR("attr2")).as_llong() == 9223372036854775807ll);
	CHECK(node.attribute(STR("attr3")).as_llong() == -9223372036854775807ll - 1);
}

TEST_XML(dom_as_ullong_overflow, "<node attr1='-1' attr2='18446744073709551616' attr3='28446744073709551615' attr4='166020696663385964543' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_ullong() == 0);
	CHECK(node.attribute(STR("attr2")).as_ullong() == 18446744073709551615ull);
	CHECK(node.attribute(STR("attr3")).as_ullong() == 18446744073709551615ull);
	CHECK(node.attribute(STR("attr4")).as_ullong() == 18446744073709551615ull);
}

TEST_XML(dom_as_llong_hex_overflow, "<node attr1='-0x8000000000000001' attr2='0x8000000000000000' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_llong() == -9223372036854775807ll - 1);
	CHECK(node.attribute(STR("attr2")).as_llong() == 9223372036854775807ll);
}

TEST_XML(dom_as_ullong_hex_overflow, "<node attr1='-0x1' attr2='0x10000000000000000' attr3='0x12345678923456789' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_ullong() == 0);
	CHECK(node.attribute(STR("attr2")).as_ullong() == 18446744073709551615ull);
	CHECK(node.attribute(STR("attr3")).as_ullong() == 18446744073709551615ull);
}

TEST_XML(dom_as_llong_many_digits, "<node attr1='0000000000000000000000000000000000000000000000001' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_llong() == 1);
	CHECK(node.attribute(STR("attr1")).as_ullong() == 1);
}

TEST_XML(dom_as_llong_hex_many_digits, "<node attr1='0x0000000000000000000000000000000000000000000000001' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_llong() == 1);
	CHECK(node.attribute(STR("attr1")).as_ullong() == 1);
}
#endif

TEST_XML(dom_as_int_plus, "<node attr1='+1' attr2='+0xa' />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.attribute(STR("attr1")).as_int() == 1);
	CHECK(node.attribute(STR("attr1")).as_uint() == 1);
	CHECK(node.attribute(STR("attr2")).as_int() == 10);
	CHECK(node.attribute(STR("attr2")).as_uint() == 10);

#ifdef PUGIXML_HAS_LONG_LONG
	CHECK(node.attribute(STR("attr1")).as_llong() == 1);
	CHECK(node.attribute(STR("attr1")).as_ullong() == 1);
	CHECK(node.attribute(STR("attr2")).as_llong() == 10);
	CHECK(node.attribute(STR("attr2")).as_ullong() == 10);
#endif
}
