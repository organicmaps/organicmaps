#include "common.hpp"

#include <limits>
#include <string>

#include <math.h>
#include <string.h>

TEST_XML(dom_attr_assign, "<node/>")
{
	xml_node node = doc.child(STR("node"));

	node.append_attribute(STR("attr1")) = STR("v1");
	xml_attribute() = STR("v1");

	node.append_attribute(STR("attr2")) = -2147483647;
	node.append_attribute(STR("attr3")) = -2147483647 - 1;
	xml_attribute() = -2147483647 - 1;

	node.append_attribute(STR("attr4")) = 4294967295u;
	node.append_attribute(STR("attr5")) = 4294967294u;
	xml_attribute() = 4294967295u;

	node.append_attribute(STR("attr6")) = 0.5;
	xml_attribute() = 0.5;

	node.append_attribute(STR("attr7")) = 0.25f;
	xml_attribute() = 0.25f;

	node.append_attribute(STR("attr8")) = true;
	xml_attribute() = true;

	CHECK_NODE(node, STR("<node attr1=\"v1\" attr2=\"-2147483647\" attr3=\"-2147483648\" attr4=\"4294967295\" attr5=\"4294967294\" attr6=\"0.5\" attr7=\"0.25\" attr8=\"true\" />"));
}

TEST_XML(dom_attr_set_name, "<node attr='value' />")
{
	xml_attribute attr = doc.child(STR("node")).attribute(STR("attr"));

	CHECK(attr.set_name(STR("n")));
	CHECK(!xml_attribute().set_name(STR("n")));

	CHECK_NODE(doc, STR("<node n=\"value\" />"));
}

TEST_XML(dom_attr_set_value, "<node/>")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.append_attribute(STR("attr1")).set_value(STR("v1")));
	CHECK(!xml_attribute().set_value(STR("v1")));

	CHECK(node.append_attribute(STR("attr2")).set_value(-2147483647));
	CHECK(node.append_attribute(STR("attr3")).set_value(-2147483647 - 1));
	CHECK(!xml_attribute().set_value(-2147483647));

	CHECK(node.append_attribute(STR("attr4")).set_value(4294967295u));
	CHECK(node.append_attribute(STR("attr5")).set_value(4294967294u));
	CHECK(!xml_attribute().set_value(4294967295u));

	CHECK(node.append_attribute(STR("attr6")).set_value(0.5));
	CHECK(!xml_attribute().set_value(0.5));

	CHECK(node.append_attribute(STR("attr7")).set_value(0.25f));
	CHECK(!xml_attribute().set_value(0.25f));

	CHECK(node.append_attribute(STR("attr8")).set_value(true));
	CHECK(!xml_attribute().set_value(true));

	CHECK_NODE(node, STR("<node attr1=\"v1\" attr2=\"-2147483647\" attr3=\"-2147483648\" attr4=\"4294967295\" attr5=\"4294967294\" attr6=\"0.5\" attr7=\"0.25\" attr8=\"true\" />"));
}

#ifdef PUGIXML_HAS_LONG_LONG
TEST_XML(dom_attr_assign_llong, "<node/>")
{
	xml_node node = doc.child(STR("node"));
	
	node.append_attribute(STR("attr1")) = -9223372036854775807ll;
	node.append_attribute(STR("attr2")) = -9223372036854775807ll - 1;
	xml_attribute() = -9223372036854775807ll - 1;

	node.append_attribute(STR("attr3")) = 18446744073709551615ull;
	node.append_attribute(STR("attr4")) = 18446744073709551614ull;
	xml_attribute() = 18446744073709551615ull;

	CHECK_NODE(node, STR("<node attr1=\"-9223372036854775807\" attr2=\"-9223372036854775808\" attr3=\"18446744073709551615\" attr4=\"18446744073709551614\" />"));
}

TEST_XML(dom_attr_set_value_llong, "<node/>")
{
	xml_node node = doc.child(STR("node"));
	
	CHECK(node.append_attribute(STR("attr1")).set_value(-9223372036854775807ll));
	CHECK(node.append_attribute(STR("attr2")).set_value(-9223372036854775807ll - 1));
	CHECK(!xml_attribute().set_value(-9223372036854775807ll - 1));

	CHECK(node.append_attribute(STR("attr3")).set_value(18446744073709551615ull));
	CHECK(node.append_attribute(STR("attr4")).set_value(18446744073709551614ull));
	CHECK(!xml_attribute().set_value(18446744073709551615ull));

	CHECK_NODE(node, STR("<node attr1=\"-9223372036854775807\" attr2=\"-9223372036854775808\" attr3=\"18446744073709551615\" attr4=\"18446744073709551614\" />"));
}
#endif

TEST_XML(dom_attr_assign_large_number_float, "<node attr='' />")
{
	xml_node node = doc.child(STR("node"));

	node.attribute(STR("attr")) = std::numeric_limits<float>::max();

	CHECK(test_node(node, STR("<node attr=\"3.40282347e+038\" />"), STR(""), pugi::format_raw) ||
		  test_node(node, STR("<node attr=\"3.40282347e+38\" />"), STR(""), pugi::format_raw));
}

TEST_XML(dom_attr_assign_large_number_double, "<node attr='' />")
{
	xml_node node = doc.child(STR("node"));

	node.attribute(STR("attr")) = std::numeric_limits<double>::max();

	// Borland C does not print double values with enough precision
#ifdef __BORLANDC__
	CHECK_NODE(node, STR("<node attr=\"1.7976931348623156e+308\" />"));
#else
	CHECK_NODE(node, STR("<node attr=\"1.7976931348623157e+308\" />"));
#endif
}

TEST_XML(dom_node_set_name, "<node>text</node>")
{
	CHECK(doc.child(STR("node")).set_name(STR("n")));
	CHECK(!doc.child(STR("node")).first_child().set_name(STR("n")));
	CHECK(!xml_node().set_name(STR("n")));

	CHECK_NODE(doc, STR("<n>text</n>"));
}

TEST_XML(dom_node_set_value, "<node>text</node>")
{
	CHECK(doc.child(STR("node")).first_child().set_value(STR("no text")));
	CHECK(!doc.child(STR("node")).set_value(STR("no text")));
	CHECK(!xml_node().set_value(STR("no text")));

	CHECK_NODE(doc, STR("<node>no text</node>"));
}

TEST_XML(dom_node_set_value_allocated, "<node>text</node>")
{
	CHECK(doc.child(STR("node")).first_child().set_value(STR("no text")));
	CHECK(!doc.child(STR("node")).set_value(STR("no text")));
	CHECK(!xml_node().set_value(STR("no text")));
	CHECK(doc.child(STR("node")).first_child().set_value(STR("no text at all")));

	CHECK_NODE(doc, STR("<node>no text at all</node>"));
}

TEST_XML(dom_node_prepend_attribute, "<node><child/></node>")
{
	CHECK(xml_node().prepend_attribute(STR("a")) == xml_attribute());
	CHECK(doc.prepend_attribute(STR("a")) == xml_attribute());
	
	xml_attribute a1 = doc.child(STR("node")).prepend_attribute(STR("a1"));
	CHECK(a1);
	a1 = STR("v1");

	xml_attribute a2 = doc.child(STR("node")).prepend_attribute(STR("a2"));
	CHECK(a2 && a1 != a2);
	a2 = STR("v2");

	xml_attribute a3 = doc.child(STR("node")).child(STR("child")).prepend_attribute(STR("a3"));
	CHECK(a3 && a1 != a3 && a2 != a3);
	a3 = STR("v3");

	CHECK_NODE(doc, STR("<node a2=\"v2\" a1=\"v1\"><child a3=\"v3\" /></node>"));
}

TEST_XML(dom_node_append_attribute, "<node><child/></node>")
{
	CHECK(xml_node().append_attribute(STR("a")) == xml_attribute());
	CHECK(doc.append_attribute(STR("a")) == xml_attribute());
	
	xml_attribute a1 = doc.child(STR("node")).append_attribute(STR("a1"));
	CHECK(a1);
	a1 = STR("v1");

	xml_attribute a2 = doc.child(STR("node")).append_attribute(STR("a2"));
	CHECK(a2 && a1 != a2);
	a2 = STR("v2");

	xml_attribute a3 = doc.child(STR("node")).child(STR("child")).append_attribute(STR("a3"));
	CHECK(a3 && a1 != a3 && a2 != a3);
	a3 = STR("v3");

	CHECK_NODE(doc, STR("<node a1=\"v1\" a2=\"v2\"><child a3=\"v3\" /></node>"));
}

TEST_XML(dom_node_insert_attribute_after, "<node a1='v1'><child a2='v2'/></node>")
{
	CHECK(xml_node().insert_attribute_after(STR("a"), xml_attribute()) == xml_attribute());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	CHECK(node.insert_attribute_after(STR("a"), xml_attribute()) == xml_attribute());
	CHECK(node.insert_attribute_after(STR("a"), a2) == xml_attribute());
	
	xml_attribute a3 = node.insert_attribute_after(STR("a3"), a1);
	CHECK(a3 && a3 != a2 && a3 != a1);
	a3 = STR("v3");

	xml_attribute a4 = node.insert_attribute_after(STR("a4"), a1);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);
	a4 = STR("v4");

	xml_attribute a5 = node.insert_attribute_after(STR("a5"), a3);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);
	a5 = STR("v5");

	CHECK(child.insert_attribute_after(STR("a"), a4) == xml_attribute());

	CHECK_NODE(doc, STR("<node a1=\"v1\" a4=\"v4\" a3=\"v3\" a5=\"v5\"><child a2=\"v2\" /></node>"));
}

TEST_XML(dom_node_insert_attribute_before, "<node a1='v1'><child a2='v2'/></node>")
{
	CHECK(xml_node().insert_attribute_before(STR("a"), xml_attribute()) == xml_attribute());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	CHECK(node.insert_attribute_before(STR("a"), xml_attribute()) == xml_attribute());
	CHECK(node.insert_attribute_before(STR("a"), a2) == xml_attribute());
	
	xml_attribute a3 = node.insert_attribute_before(STR("a3"), a1);
	CHECK(a3 && a3 != a2 && a3 != a1);
	a3 = STR("v3");

	xml_attribute a4 = node.insert_attribute_before(STR("a4"), a1);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);
	a4 = STR("v4");

	xml_attribute a5 = node.insert_attribute_before(STR("a5"), a3);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);
	a5 = STR("v5");

	CHECK(child.insert_attribute_before(STR("a"), a4) == xml_attribute());

	CHECK_NODE(doc, STR("<node a5=\"v5\" a3=\"v3\" a4=\"v4\" a1=\"v1\"><child a2=\"v2\" /></node>"));
}

TEST_XML(dom_node_prepend_copy_attribute, "<node a1='v1'><child a2='v2'/><child/></node>")
{
	CHECK(xml_node().prepend_copy(xml_attribute()) == xml_attribute());
	CHECK(xml_node().prepend_copy(doc.child(STR("node")).attribute(STR("a1"))) == xml_attribute());
	CHECK(doc.prepend_copy(doc.child(STR("node")).attribute(STR("a1"))) == xml_attribute());
	CHECK(doc.child(STR("node")).prepend_copy(xml_attribute()) == xml_attribute());
	
	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	xml_attribute a3 = node.prepend_copy(a1);
	CHECK(a3 && a3 != a2 && a3 != a1);

	xml_attribute a4 = node.prepend_copy(a2);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);

	xml_attribute a5 = node.last_child().prepend_copy(a1);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);

	CHECK_NODE(doc, STR("<node a2=\"v2\" a1=\"v1\" a1=\"v1\"><child a2=\"v2\" /><child a1=\"v1\" /></node>"));

	a3.set_name(STR("a3"));
	a3 = STR("v3");
	
	a4.set_name(STR("a4"));
	a4 = STR("v4");
	
	a5.set_name(STR("a5"));
	a5 = STR("v5");
	
	CHECK_NODE(doc, STR("<node a4=\"v4\" a3=\"v3\" a1=\"v1\"><child a2=\"v2\" /><child a5=\"v5\" /></node>"));
}

TEST_XML(dom_node_append_copy_attribute, "<node a1='v1'><child a2='v2'/><child/></node>")
{
	CHECK(xml_node().append_copy(xml_attribute()) == xml_attribute());
	CHECK(xml_node().append_copy(doc.child(STR("node")).attribute(STR("a1"))) == xml_attribute());
	CHECK(doc.append_copy(doc.child(STR("node")).attribute(STR("a1"))) == xml_attribute());
	CHECK(doc.child(STR("node")).append_copy(xml_attribute()) == xml_attribute());
	
	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	xml_attribute a3 = node.append_copy(a1);
	CHECK(a3 && a3 != a2 && a3 != a1);

	xml_attribute a4 = node.append_copy(a2);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);

	xml_attribute a5 = node.last_child().append_copy(a1);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);

	CHECK_NODE(doc, STR("<node a1=\"v1\" a1=\"v1\" a2=\"v2\"><child a2=\"v2\" /><child a1=\"v1\" /></node>"));

	a3.set_name(STR("a3"));
	a3 = STR("v3");
	
	a4.set_name(STR("a4"));
	a4 = STR("v4");
	
	a5.set_name(STR("a5"));
	a5 = STR("v5");
	
	CHECK_NODE(doc, STR("<node a1=\"v1\" a3=\"v3\" a4=\"v4\"><child a2=\"v2\" /><child a5=\"v5\" /></node>"));
}

TEST_XML(dom_node_insert_copy_after_attribute, "<node a1='v1'><child a2='v2'/></node>")
{
	CHECK(xml_node().insert_copy_after(xml_attribute(), xml_attribute()) == xml_attribute());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	CHECK(node.insert_copy_after(a1, xml_attribute()) == xml_attribute());
	CHECK(node.insert_copy_after(xml_attribute(), a1) == xml_attribute());
	CHECK(node.insert_copy_after(a2, a2) == xml_attribute());
	
	xml_attribute a3 = node.insert_copy_after(a1, a1);
	CHECK(a3 && a3 != a2 && a3 != a1);

	xml_attribute a4 = node.insert_copy_after(a2, a1);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);

	xml_attribute a5 = node.insert_copy_after(a4, a1);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);

	CHECK(child.insert_copy_after(a4, a4) == xml_attribute());

	CHECK_NODE(doc, STR("<node a1=\"v1\" a2=\"v2\" a2=\"v2\" a1=\"v1\"><child a2=\"v2\" /></node>"));

	a3.set_name(STR("a3"));
	a3 = STR("v3");
	
	a4.set_name(STR("a4"));
	a4 = STR("v4");
	
	a5.set_name(STR("a5"));
	a5 = STR("v5");
	
	CHECK_NODE(doc, STR("<node a1=\"v1\" a5=\"v5\" a4=\"v4\" a3=\"v3\"><child a2=\"v2\" /></node>"));
}

TEST_XML(dom_node_insert_copy_before_attribute, "<node a1='v1'><child a2='v2'/></node>")
{
	CHECK(xml_node().insert_copy_before(xml_attribute(), xml_attribute()) == xml_attribute());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	xml_attribute a1 = node.attribute(STR("a1"));
	xml_attribute a2 = child.attribute(STR("a2"));

	CHECK(node.insert_copy_before(a1, xml_attribute()) == xml_attribute());
	CHECK(node.insert_copy_before(xml_attribute(), a1) == xml_attribute());
	CHECK(node.insert_copy_before(a2, a2) == xml_attribute());
	
	xml_attribute a3 = node.insert_copy_before(a1, a1);
	CHECK(a3 && a3 != a2 && a3 != a1);

	xml_attribute a4 = node.insert_copy_before(a2, a1);
	CHECK(a4 && a4 != a3 && a4 != a2 && a4 != a1);

	xml_attribute a5 = node.insert_copy_before(a4, a1);
	CHECK(a5 && a5 != a4 && a5 != a3 && a5 != a2 && a5 != a1);

	CHECK(child.insert_copy_before(a4, a4) == xml_attribute());

	CHECK_NODE(doc, STR("<node a1=\"v1\" a2=\"v2\" a2=\"v2\" a1=\"v1\"><child a2=\"v2\" /></node>"));

	a3.set_name(STR("a3"));
	a3 = STR("v3");
	
	a4.set_name(STR("a4"));
	a4 = STR("v4");
	
	a5.set_name(STR("a5"));
	a5 = STR("v5");
	
	CHECK_NODE(doc, STR("<node a3=\"v3\" a4=\"v4\" a5=\"v5\" a1=\"v1\"><child a2=\"v2\" /></node>"));
}

TEST_XML(dom_node_remove_attribute, "<node a1='v1' a2='v2' a3='v3'><child a4='v4'/></node>")
{
	CHECK(!xml_node().remove_attribute(STR("a")));
	CHECK(!xml_node().remove_attribute(xml_attribute()));
	
	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(!node.remove_attribute(STR("a")));
	CHECK(!node.remove_attribute(xml_attribute()));
	CHECK(!node.remove_attribute(child.attribute(STR("a4"))));

	CHECK_NODE(doc, STR("<node a1=\"v1\" a2=\"v2\" a3=\"v3\"><child a4=\"v4\" /></node>"));

	CHECK(node.remove_attribute(STR("a1")));
	CHECK(node.remove_attribute(node.attribute(STR("a3"))));
	CHECK(child.remove_attribute(STR("a4")));

	CHECK_NODE(doc, STR("<node a2=\"v2\"><child /></node>"));
}

TEST_XML(dom_node_prepend_child, "<node>foo<child/></node>")
{
	CHECK(xml_node().prepend_child() == xml_node());
	CHECK(doc.child(STR("node")).first_child().prepend_child() == xml_node());
	CHECK(doc.prepend_child(node_document) == xml_node());
	CHECK(doc.prepend_child(node_null) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).prepend_child();
	CHECK(n1);
	CHECK(n1.set_name(STR("n1")));

	xml_node n2 = doc.child(STR("node")).prepend_child();
	CHECK(n2 && n1 != n2);
	CHECK(n2.set_name(STR("n2")));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).prepend_child(node_pcdata);
	CHECK(n3 && n1 != n3 && n2 != n3);
	CHECK(n3.set_value(STR("n3")));
	
	xml_node n4 = doc.prepend_child(node_comment);
	CHECK(n4 && n1 != n4 && n2 != n4 && n3 != n4);
	CHECK(n4.set_value(STR("n4")));

	CHECK_NODE(doc, STR("<!--n4--><node><n2 /><n1 />foo<child>n3</child></node>"));
}

TEST_XML(dom_node_append_child, "<node>foo<child/></node>")
{
	CHECK(xml_node().append_child() == xml_node());
	CHECK(doc.child(STR("node")).first_child().append_child() == xml_node());
	CHECK(doc.append_child(node_document) == xml_node());
	CHECK(doc.append_child(node_null) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).append_child();
	CHECK(n1);
	CHECK(n1.set_name(STR("n1")));

	xml_node n2 = doc.child(STR("node")).append_child();
	CHECK(n2 && n1 != n2);
	CHECK(n2.set_name(STR("n2")));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).append_child(node_pcdata);
	CHECK(n3 && n1 != n3 && n2 != n3);
	CHECK(n3.set_value(STR("n3")));
	
	xml_node n4 = doc.append_child(node_comment);
	CHECK(n4 && n1 != n4 && n2 != n4 && n3 != n4);
	CHECK(n4.set_value(STR("n4")));

	CHECK_NODE(doc, STR("<node>foo<child>n3</child><n1 /><n2 /></node><!--n4-->"));
}

TEST_XML(dom_node_insert_child_after, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_child_after(node_element, xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_child_after(node_element, xml_node()) == xml_node());
	CHECK(doc.insert_child_after(node_document, xml_node()) == xml_node());
	CHECK(doc.insert_child_after(node_null, xml_node()) == xml_node());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(node.insert_child_after(node_element, node) == xml_node());
	CHECK(child.insert_child_after(node_element, node) == xml_node());
	
	xml_node n1 = node.insert_child_after(node_element, child);
	CHECK(n1 && n1 != node && n1 != child);
	CHECK(n1.set_name(STR("n1")));

	xml_node n2 = node.insert_child_after(node_element, child);
	CHECK(n2 && n2 != node && n2 != child && n2 != n1);
	CHECK(n2.set_name(STR("n2")));

	xml_node n3 = node.insert_child_after(node_pcdata, n2);
	CHECK(n3 && n3 != node && n3 != child && n3 != n1 && n3 != n2);
	CHECK(n3.set_value(STR("n3")));

	xml_node n4 = node.insert_child_after(node_pi, node.first_child());
	CHECK(n4 && n4 != node && n4 != child && n4 != n1 && n4 != n2 && n4 != n3);
	CHECK(n4.set_name(STR("n4")));

	CHECK(child.insert_child_after(node_element, n3) == xml_node());

	CHECK_NODE(doc, STR("<node>foo<?n4?><child /><n2 />n3<n1 /></node>"));
}

TEST_XML(dom_node_insert_child_before, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_child_before(node_element, xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_child_before(node_element, xml_node()) == xml_node());
	CHECK(doc.insert_child_before(node_document, xml_node()) == xml_node());
	CHECK(doc.insert_child_before(node_null, xml_node()) == xml_node());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(node.insert_child_before(node_element, node) == xml_node());
	CHECK(child.insert_child_before(node_element, node) == xml_node());
	
	xml_node n1 = node.insert_child_before(node_element, child);
	CHECK(n1 && n1 != node && n1 != child);
	CHECK(n1.set_name(STR("n1")));

	xml_node n2 = node.insert_child_before(node_element, child);
	CHECK(n2 && n2 != node && n2 != child && n2 != n1);
	CHECK(n2.set_name(STR("n2")));

	xml_node n3 = node.insert_child_before(node_pcdata, n2);
	CHECK(n3 && n3 != node && n3 != child && n3 != n1 && n3 != n2);
	CHECK(n3.set_value(STR("n3")));

	xml_node n4 = node.insert_child_before(node_pi, node.first_child());
	CHECK(n4 && n4 != node && n4 != child && n4 != n1 && n4 != n2 && n4 != n3);
	CHECK(n4.set_name(STR("n4")));

	CHECK(child.insert_child_before(node_element, n3) == xml_node());

	CHECK_NODE(doc, STR("<node><?n4?>foo<n1 />n3<n2 /><child /></node>"));
}

TEST_XML(dom_node_prepend_child_name, "<node>foo<child/></node>")
{
	CHECK(xml_node().prepend_child(STR("")) == xml_node());
	CHECK(doc.child(STR("node")).first_child().prepend_child(STR("")) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).prepend_child(STR("n1"));
	CHECK(n1);

	xml_node n2 = doc.child(STR("node")).prepend_child(STR("n2"));
	CHECK(n2 && n1 != n2);

	CHECK_NODE(doc, STR("<node><n2 /><n1 />foo<child /></node>"));
}

TEST_XML(dom_node_append_child_name, "<node>foo<child/></node>")
{
	CHECK(xml_node().append_child(STR("")) == xml_node());
	CHECK(doc.child(STR("node")).first_child().append_child(STR("")) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).append_child(STR("n1"));
	CHECK(n1);

	xml_node n2 = doc.child(STR("node")).append_child(STR("n2"));
	CHECK(n2 && n1 != n2);

	CHECK_NODE(doc, STR("<node>foo<child /><n1 /><n2 /></node>"));
}

TEST_XML(dom_node_insert_child_after_name, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_child_after(STR(""), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_child_after(STR(""), xml_node()) == xml_node());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(node.insert_child_after(STR(""), node) == xml_node());
	CHECK(child.insert_child_after(STR(""), node) == xml_node());
	
	xml_node n1 = node.insert_child_after(STR("n1"), child);
	CHECK(n1 && n1 != node && n1 != child);

	xml_node n2 = node.insert_child_after(STR("n2"), child);
	CHECK(n2 && n2 != node && n2 != child && n2 != n1);

	CHECK(child.insert_child_after(STR(""), n2) == xml_node());

	CHECK_NODE(doc, STR("<node>foo<child /><n2 /><n1 /></node>"));
}

TEST_XML(dom_node_insert_child_before_name, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_child_before(STR(""), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_child_before(STR(""), xml_node()) == xml_node());

	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(node.insert_child_before(STR(""), node) == xml_node());
	CHECK(child.insert_child_before(STR(""), node) == xml_node());
	
	xml_node n1 = node.insert_child_before(STR("n1"), child);
	CHECK(n1 && n1 != node && n1 != child);

	xml_node n2 = node.insert_child_before(STR("n2"), child);
	CHECK(n2 && n2 != node && n2 != child && n2 != n1);

	CHECK(child.insert_child_before(STR(""), n2) == xml_node());

	CHECK_NODE(doc, STR("<node>foo<n1 /><n2 /><child /></node>"));
}

TEST_XML(dom_node_remove_child, "<node><n1/><n2/><n3/><child><n4/></child></node>")
{
	CHECK(!xml_node().remove_child(STR("a")));
	CHECK(!xml_node().remove_child(xml_node()));
	
	xml_node node = doc.child(STR("node"));
	xml_node child = node.child(STR("child"));

	CHECK(!node.remove_child(STR("a")));
	CHECK(!node.remove_child(xml_node()));
	CHECK(!node.remove_child(child.child(STR("n4"))));

	CHECK_NODE(doc, STR("<node><n1 /><n2 /><n3 /><child><n4 /></child></node>"));

	CHECK(node.remove_child(STR("n1")));
	CHECK(node.remove_child(node.child(STR("n3"))));
	CHECK(child.remove_child(STR("n4")));

	CHECK_NODE(doc, STR("<node><n2 /><child /></node>"));
}

TEST_XML(dom_node_remove_child_complex, "<node id='1'><n1 id1='1' id2='2'/><n2/><n3/><child><n4/></child></node>")
{
	CHECK(doc.child(STR("node")).remove_child(STR("n1")));

	CHECK_NODE(doc, STR("<node id=\"1\"><n2 /><n3 /><child><n4 /></child></node>"));

	CHECK(doc.remove_child(STR("node")));

	CHECK_NODE(doc, STR(""));
}

TEST_XML(dom_node_remove_child_complex_allocated, "<node id='1'><n1 id1='1' id2='2'/><n2/><n3/><child><n4/></child></node>")
{
	doc.append_copy(doc.child(STR("node")));

	CHECK(doc.remove_child(STR("node")));
	CHECK(doc.remove_child(STR("node")));

	CHECK_NODE(doc, STR(""));
}

TEST_XML(dom_node_prepend_copy, "<node>foo<child/></node>")
{
	CHECK(xml_node().prepend_copy(xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().prepend_copy(doc.child(STR("node"))) == xml_node());
	CHECK(doc.prepend_copy(doc) == xml_node());
	CHECK(doc.prepend_copy(xml_node()) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).prepend_copy(doc.child(STR("node")).first_child());
	CHECK(n1);
	CHECK_STRING(n1.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foofoo<child /></node>"));

	xml_node n2 = doc.child(STR("node")).prepend_copy(doc.child(STR("node")).child(STR("child")));
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.name(), STR("child"));
	CHECK_NODE(doc, STR("<node><child />foofoo<child /></node>"));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).prepend_copy(doc.child(STR("node")).first_child().next_sibling());
	CHECK(n3 && n3 != n1 && n3 != n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>foo</child>foofoo<child /></node>"));
}

TEST_XML(dom_node_append_copy, "<node>foo<child/></node>")
{
	CHECK(xml_node().append_copy(xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().append_copy(doc.child(STR("node"))) == xml_node());
	CHECK(doc.append_copy(doc) == xml_node());
	CHECK(doc.append_copy(xml_node()) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).append_copy(doc.child(STR("node")).first_child());
	CHECK(n1);
	CHECK_STRING(n1.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child />foo</node>"));

	xml_node n2 = doc.child(STR("node")).append_copy(doc.child(STR("node")).child(STR("child")));
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.name(), STR("child"));
	CHECK_NODE(doc, STR("<node>foo<child />foo<child /></node>"));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).append_copy(doc.child(STR("node")).first_child());
	CHECK(n3 && n3 != n1 && n3 != n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child>foo</child>foo<child /></node>"));
}

TEST_XML(dom_node_insert_copy_after, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_copy_after(xml_node(), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_copy_after(doc.child(STR("node")), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_copy_after(doc, doc) == xml_node());
	CHECK(doc.insert_copy_after(xml_node(), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_copy_after(doc.child(STR("node")), xml_node()) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).insert_copy_after(doc.child(STR("node")).child(STR("child")), doc.child(STR("node")).first_child());
	CHECK(n1);
	CHECK_STRING(n1.name(), STR("child"));
	CHECK_NODE(doc, STR("<node>foo<child /><child /></node>"));

	xml_node n2 = doc.child(STR("node")).insert_copy_after(doc.child(STR("node")).first_child(), doc.child(STR("node")).last_child());
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child /><child />foo</node>"));

	xml_node n3 = doc.child(STR("node")).insert_copy_after(doc.child(STR("node")).first_child(), doc.child(STR("node")).first_child());
	CHECK(n3 && n3 != n1 && n3 != n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foofoo<child /><child />foo</node>"));
}

TEST_XML(dom_node_insert_copy_before, "<node>foo<child/></node>")
{
	CHECK(xml_node().insert_copy_before(xml_node(), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_copy_before(doc.child(STR("node")), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_copy_before(doc, doc) == xml_node());
	CHECK(doc.insert_copy_before(xml_node(), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_copy_before(doc.child(STR("node")), xml_node()) == xml_node());
	
	xml_node n1 = doc.child(STR("node")).insert_copy_before(doc.child(STR("node")).child(STR("child")), doc.child(STR("node")).first_child());
	CHECK(n1);
	CHECK_STRING(n1.name(), STR("child"));
	CHECK_NODE(doc, STR("<node><child />foo<child /></node>"));

	xml_node n2 = doc.child(STR("node")).insert_copy_before(doc.child(STR("node")).first_child(), doc.child(STR("node")).last_child());
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.name(), STR("child"));
	CHECK_NODE(doc, STR("<node><child />foo<child /><child /></node>"));

	xml_node n3 = doc.child(STR("node")).insert_copy_before(doc.child(STR("node")).first_child().next_sibling(), doc.child(STR("node")).first_child());
	CHECK(n3 && n3 != n1 && n3 != n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child />foo<child /><child /></node>"));
}

TEST_XML(dom_node_copy_recursive, "<node>foo<child/></node>")
{
	doc.child(STR("node")).append_copy(doc.child(STR("node")));
	CHECK_NODE(doc, STR("<node>foo<child /><node>foo<child /></node></node>"));
}

TEST_XML(dom_node_copy_crossdoc, "<node/>")
{
	xml_document newdoc;
	newdoc.append_copy(doc.child(STR("node")));
	CHECK_NODE(doc, STR("<node />"));
	CHECK_NODE(newdoc, STR("<node />"));
}

TEST_XML(dom_node_copy_crossdoc_attribute, "<node attr='value'/>")
{
	xml_document newdoc;
	newdoc.append_child(STR("copy")).append_copy(doc.child(STR("node")).attribute(STR("attr")));
	CHECK_NODE(doc, STR("<node attr=\"value\" />"));
	CHECK_NODE(newdoc, STR("<copy attr=\"value\" />"));
}

TEST_XML_FLAGS(dom_node_copy_types, "<?xml version='1.0'?><!DOCTYPE id><root><?pi value?><!--comment--><node id='1'>pcdata<![CDATA[cdata]]></node></root>", parse_full)
{
	doc.append_copy(doc.child(STR("root")));
	CHECK_NODE(doc, STR("<?xml version=\"1.0\"?><!DOCTYPE id><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root>"));

	doc.insert_copy_before(doc.first_child(), doc.first_child());
	CHECK_NODE(doc, STR("<?xml version=\"1.0\"?><?xml version=\"1.0\"?><!DOCTYPE id><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root>"));

	doc.insert_copy_after(doc.first_child().next_sibling().next_sibling(), doc.first_child());
	CHECK_NODE(doc, STR("<?xml version=\"1.0\"?><!DOCTYPE id><?xml version=\"1.0\"?><!DOCTYPE id><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root><root><?pi value?><!--comment--><node id=\"1\">pcdata<![CDATA[cdata]]></node></root>"));
}

TEST(dom_node_declaration_name)
{
	xml_document doc;
	doc.append_child(node_declaration);

	// name 'xml' is auto-assigned
	CHECK(doc.first_child().type() == node_declaration);
	CHECK_STRING(doc.first_child().name(), STR("xml"));

	doc.insert_child_after(node_declaration, doc.first_child());
	doc.insert_child_before(node_declaration, doc.first_child());
	doc.prepend_child(node_declaration);

	CHECK_NODE(doc, STR("<?xml?><?xml?><?xml?><?xml?>"));
}

TEST(dom_node_declaration_attributes)
{
    xml_document doc;
    xml_node node = doc.append_child(node_declaration);
    node.append_attribute(STR("version")) = STR("1.0");
    node.append_attribute(STR("encoding")) = STR("utf-8");

    CHECK_NODE(doc, STR("<?xml version=\"1.0\" encoding=\"utf-8\"?>"));
}

TEST(dom_node_declaration_top_level)
{
	xml_document doc;
	doc.append_child().set_name(STR("node"));

	xml_node node = doc.first_child();
	node.append_child(node_pcdata).set_value(STR("text"));

	CHECK(node.insert_child_before(node_declaration, node.first_child()) == xml_node());
	CHECK(node.insert_child_after(node_declaration, node.first_child()) == xml_node());
	CHECK(node.append_child(node_declaration) == xml_node());

	CHECK_NODE(doc, STR("<node>text</node>"));

	CHECK(doc.insert_child_before(node_declaration, node));
	CHECK(doc.insert_child_after(node_declaration, node));
	CHECK(doc.append_child(node_declaration));

	CHECK_NODE(doc, STR("<?xml?><node>text</node><?xml?><?xml?>"));
}

TEST(dom_node_declaration_copy)
{
	xml_document doc;
	doc.append_child(node_declaration);

	doc.append_child().set_name(STR("node"));

	doc.last_child().append_copy(doc.first_child());

	CHECK_NODE(doc, STR("<?xml?><node />"));
}

TEST(dom_string_out_of_memory)
{
	const unsigned int length = 65536;
	static char_t string[length + 1];

	for (unsigned int i = 0; i < length; ++i) string[i] = 'a';
	string[length] = 0;

	xml_document doc;
	xml_node node = doc.append_child();
	xml_attribute attr = node.append_attribute(STR("a"));
	xml_node text = node.append_child(node_pcdata);

	// no value => long value
	test_runner::_memory_fail_threshold = 32;

	CHECK_ALLOC_FAIL(CHECK(!node.set_name(string)));
	CHECK_ALLOC_FAIL(CHECK(!text.set_value(string)));
	CHECK_ALLOC_FAIL(CHECK(!attr.set_name(string)));
	CHECK_ALLOC_FAIL(CHECK(!attr.set_value(string)));

	// set some names/values
	test_runner::_memory_fail_threshold = 0;

	node.set_name(STR("n"));
	attr.set_value(STR("v"));
	text.set_value(STR("t"));

	// some value => long value
	test_runner::_memory_fail_threshold = 32;

	CHECK_ALLOC_FAIL(CHECK(!node.set_name(string)));
	CHECK_ALLOC_FAIL(CHECK(!text.set_value(string)));
	CHECK_ALLOC_FAIL(CHECK(!attr.set_name(string)));
	CHECK_ALLOC_FAIL(CHECK(!attr.set_value(string)));

	// check that original state was preserved
	test_runner::_memory_fail_threshold = 0;

	CHECK_NODE(doc, STR("<n a=\"v\">t</n>"));
}

TEST(dom_node_out_of_memory)
{
	test_runner::_memory_fail_threshold = 65536;

	// exhaust memory limit
	xml_document doc;

	xml_node n = doc.append_child();
	CHECK(n.set_name(STR("n")));

	xml_attribute a = n.append_attribute(STR("a"));
	CHECK(a);

	CHECK_ALLOC_FAIL(while (n.append_child(node_comment)) { /* nop */ });
	CHECK_ALLOC_FAIL(while (n.append_attribute(STR("b"))) { /* nop */ });

	// verify all node modification operations
	CHECK_ALLOC_FAIL(CHECK(!n.append_child()));
	CHECK_ALLOC_FAIL(CHECK(!n.prepend_child()));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_child_after(node_element, n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_child_before(node_element, n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.append_attribute(STR(""))));
	CHECK_ALLOC_FAIL(CHECK(!n.prepend_attribute(STR(""))));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_attribute_after(STR(""), a)));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_attribute_before(STR(""), a)));

	// verify node copy operations
	CHECK_ALLOC_FAIL(CHECK(!n.append_copy(n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.prepend_copy(n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_copy_after(n.first_child(), n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_copy_before(n.first_child(), n.first_child())));
	CHECK_ALLOC_FAIL(CHECK(!n.append_copy(a)));
	CHECK_ALLOC_FAIL(CHECK(!n.prepend_copy(a)));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_copy_after(a, a)));
	CHECK_ALLOC_FAIL(CHECK(!n.insert_copy_before(a, a)));
}

TEST(dom_node_memory_limit)
{
	const unsigned int length = 65536;
	static char_t string[length + 1];

	for (unsigned int i = 0; i < length; ++i) string[i] = 'a';
	string[length] = 0;

	test_runner::_memory_fail_threshold = 32768 * 2 + sizeof(string);

	xml_document doc;

	for (int j = 0; j < 32; ++j)
	{
		CHECK(doc.append_child().set_name(string));
		CHECK(doc.remove_child(doc.first_child()));
	}
}

TEST(dom_node_memory_limit_pi)
{
	const unsigned int length = 65536;
	static char_t string[length + 1];

	for (unsigned int i = 0; i < length; ++i) string[i] = 'a';
	string[length] = 0;

	test_runner::_memory_fail_threshold = 32768 * 2 + sizeof(string);

	xml_document doc;

	for (int j = 0; j < 32; ++j)
	{
		CHECK(doc.append_child(node_pi).set_value(string));
		CHECK(doc.remove_child(doc.first_child()));
	}
}

TEST(dom_node_doctype_top_level)
{
	xml_document doc;
	doc.append_child().set_name(STR("node"));

	xml_node node = doc.first_child();
	node.append_child(node_pcdata).set_value(STR("text"));

	CHECK(node.insert_child_before(node_doctype, node.first_child()) == xml_node());
	CHECK(node.insert_child_after(node_doctype, node.first_child()) == xml_node());
	CHECK(node.append_child(node_doctype) == xml_node());

	CHECK_NODE(doc, STR("<node>text</node>"));

	CHECK(doc.insert_child_before(node_doctype, node));
	CHECK(doc.insert_child_after(node_doctype, node));
	CHECK(doc.append_child(node_doctype));

	CHECK_NODE(doc, STR("<!DOCTYPE><node>text</node><!DOCTYPE><!DOCTYPE>"));
}

TEST(dom_node_doctype_copy)
{
	xml_document doc;
	doc.append_child(node_doctype);

	doc.append_child().set_name(STR("node"));

	doc.last_child().append_copy(doc.first_child());

	CHECK_NODE(doc, STR("<!DOCTYPE><node />"));
}

TEST(dom_node_doctype_value)
{
    xml_document doc;
    xml_node node = doc.append_child(node_doctype);

    CHECK(node.type() == node_doctype);
    CHECK_STRING(node.value(), STR(""));
    CHECK_NODE(node, STR("<!DOCTYPE>"));

    CHECK(node.set_value(STR("id [ foo ]")));
    CHECK_NODE(node, STR("<!DOCTYPE id [ foo ]>"));
}

TEST_XML(dom_node_append_buffer_native, "<node>test</node>")
{
	xml_node node = doc.child(STR("node"));

	const char_t data1[] = STR("<child1 id='1' /><child2>text</child2>");
	const char_t data2[] = STR("<child3 />");

	CHECK(node.append_buffer(data1, sizeof(data1)));
	CHECK(node.append_buffer(data2, sizeof(data2)));
	CHECK(node.append_buffer(data1, sizeof(data1)));
	CHECK(node.append_buffer(data2, sizeof(data2)));
	CHECK(node.append_buffer(data2, sizeof(data2)));

	CHECK_NODE(doc, STR("<node>test<child1 id=\"1\" /><child2>text</child2><child3 /><child1 id=\"1\" /><child2>text</child2><child3 /><child3 /></node>"));
}

TEST_XML(dom_node_append_buffer_convert, "<node>test</node>")
{
	xml_node node = doc.child(STR("node"));

	const char data[] = {0, 0, 0, '<', 0, 0, 0, 'n', 0, 0, 0, '/', 0, 0, 0, '>'};

	CHECK(node.append_buffer(data, sizeof(data)));
	CHECK(node.append_buffer(data, sizeof(data), parse_default, encoding_utf32_be));

	CHECK_NODE(doc, STR("<node>test<n /><n /></node>"));
}


TEST_XML(dom_node_append_buffer_remove, "<node>test</node>")
{
	xml_node node = doc.child(STR("node"));

	const char data1[] = "<child1 id='1' /><child2>text</child2>";
	const char data2[] = "<child3 />";

	CHECK(node.append_buffer(data1, sizeof(data1)));
	CHECK(node.append_buffer(data2, sizeof(data2)));
	CHECK(node.append_buffer(data1, sizeof(data1)));
	CHECK(node.append_buffer(data2, sizeof(data2)));

	CHECK_NODE(doc, STR("<node>test<child1 id=\"1\" /><child2>text</child2><child3 /><child1 id=\"1\" /><child2>text</child2><child3 /></node>"));

	while (node.remove_child(STR("child2"))) {}

	CHECK_NODE(doc, STR("<node>test<child1 id=\"1\" /><child3 /><child1 id=\"1\" /><child3 /></node>"));

	while (node.remove_child(STR("child1"))) {}

	CHECK_NODE(doc, STR("<node>test<child3 /><child3 /></node>"));

	while (node.remove_child(STR("child3"))) {}

	CHECK_NODE(doc, STR("<node>test</node>"));

	CHECK(doc.remove_child(STR("node")));

	CHECK(!doc.first_child());
}

TEST(dom_node_append_buffer_empty_document)
{
	xml_document doc;

	const char data[] = "<child1 id='1' /><child2>text</child2>";

	doc.append_buffer(data, sizeof(data));

	CHECK_NODE(doc, STR("<child1 id=\"1\" /><child2>text</child2>"));
}

TEST_XML(dom_node_append_buffer_invalid_type, "<node>test</node>")
{
	const char data[] = "<child1 id='1' /><child2>text</child2>";

	CHECK(xml_node().append_buffer(data, sizeof(data)).status == status_append_invalid_root);
	CHECK(doc.first_child().first_child().append_buffer(data, sizeof(data)).status == status_append_invalid_root);
}

TEST_XML(dom_node_append_buffer_close_external, "<node />")
{
	xml_node node = doc.child(STR("node"));

	const char data[] = "<child1 /></node><child2 />";

	CHECK(node.append_buffer(data, sizeof(data)).status == status_end_element_mismatch);
	CHECK_NODE(doc, STR("<node><child1 /></node>"));

	CHECK(node.append_buffer(data, sizeof(data)).status == status_end_element_mismatch);
	CHECK_NODE(doc, STR("<node><child1 /><child1 /></node>"));
}

TEST(dom_node_append_buffer_out_of_memory_extra)
{
	test_runner::_memory_fail_threshold = 1;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.append_buffer("<n/>", 4).status == status_out_of_memory));
	CHECK(!doc.first_child());
}

TEST(dom_node_append_buffer_out_of_memory_buffer)
{
	test_runner::_memory_fail_threshold = 32768 + 128;

	char data[128] = {0};

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.append_buffer(data, sizeof(data)).status == status_out_of_memory));
	CHECK(!doc.first_child());
}

TEST(dom_node_append_buffer_out_of_memory_nodes)
{
	unsigned int count = 4000;
	std::basic_string<char_t> data;

	for (unsigned int i = 0; i < count; ++i)
		data += STR("<a/>");

	test_runner::_memory_fail_threshold = 32768 + 128 + data.length() * sizeof(char_t) + 32;

#ifdef PUGIXML_COMPACT
	// ... and some space for hash table
	test_runner::_memory_fail_threshold += 2048;
#endif

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.append_buffer(data.c_str(), data.length() * sizeof(char_t), parse_fragment).status == status_out_of_memory));

	unsigned int valid = 0;

	for (xml_node n = doc.first_child(); n; n = n.next_sibling())
	{
		CHECK_STRING(n.name(), STR("a"));
		valid++;
	}

	CHECK(valid > 0 && valid < count);
}

TEST(dom_node_append_buffer_out_of_memory_name)
{
	test_runner::_memory_fail_threshold = 32768 + 4096;

	char data[4096] = {0};

	xml_document doc;
	CHECK(doc.append_child(STR("root")));
	CHECK_ALLOC_FAIL(CHECK(doc.first_child().append_buffer(data, sizeof(data)).status == status_out_of_memory));
	CHECK_STRING(doc.first_child().name(), STR("root"));
}

TEST_XML(dom_node_append_buffer_fragment, "<node />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.append_buffer("1", 1).status == status_no_document_element);
	CHECK_NODE(doc, STR("<node>1</node>"));

	CHECK(node.append_buffer("2", 1, parse_fragment));
	CHECK_NODE(doc, STR("<node>12</node>"));

	CHECK(node.append_buffer("3", 1).status == status_no_document_element);
	CHECK_NODE(doc, STR("<node>123</node>"));

	CHECK(node.append_buffer("4", 1, parse_fragment));
	CHECK_NODE(doc, STR("<node>1234</node>"));
}

TEST_XML(dom_node_append_buffer_empty, "<node />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(node.append_buffer("", 0).status == status_no_document_element);
	CHECK(node.append_buffer("", 0, parse_fragment).status == status_ok);

	CHECK(node.append_buffer(0, 0).status == status_no_document_element);
	CHECK(node.append_buffer(0, 0, parse_fragment).status == status_ok);

	CHECK_NODE(doc, STR("<node />"));
}

TEST_XML(dom_node_prepend_move, "<node>foo<child/></node>")
{
	xml_node child = doc.child(STR("node")).child(STR("child"));

	CHECK(xml_node().prepend_move(xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().prepend_move(child) == xml_node());
	CHECK(doc.prepend_move(doc) == xml_node());
	CHECK(doc.prepend_move(xml_node()) == xml_node());

	xml_node n1 = doc.child(STR("node")).prepend_move(doc.child(STR("node")).first_child());
	CHECK(n1 && n1 == doc.child(STR("node")).first_child());
	CHECK_STRING(n1.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child /></node>"));

	xml_node n2 = doc.child(STR("node")).prepend_move(doc.child(STR("node")).child(STR("child")));
	CHECK(n2 && n2 != n1 && n2 == child);
	CHECK_STRING(n2.name(), STR("child"));
	CHECK_NODE(doc, STR("<node><child />foo</node>"));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).prepend_move(doc.child(STR("node")).first_child().next_sibling());
	CHECK(n3 && n3 == n1 && n3 != n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>foo</child></node>"));
}

TEST_XML(dom_node_append_move, "<node>foo<child/></node>")
{
	xml_node child = doc.child(STR("node")).child(STR("child"));

	CHECK(xml_node().append_move(xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().append_move(child) == xml_node());
	CHECK(doc.append_move(doc) == xml_node());
	CHECK(doc.append_move(xml_node()) == xml_node());

	xml_node n1 = doc.child(STR("node")).append_move(doc.child(STR("node")).first_child());
	CHECK(n1 && n1 == doc.child(STR("node")).last_child());
	CHECK_STRING(n1.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child />foo</node>"));

	xml_node n2 = doc.child(STR("node")).append_move(doc.child(STR("node")).last_child());
	CHECK(n2 && n2 == n1);
	CHECK_STRING(n2.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child />foo</node>"));

	xml_node n3 = doc.child(STR("node")).child(STR("child")).append_move(doc.child(STR("node")).last_child());
	CHECK(n3 && n3 == n1 && n3 == n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>foo</child></node>"));
}

TEST_XML(dom_node_insert_move_after, "<node>foo<child>bar</child></node>")
{
	xml_node child = doc.child(STR("node")).child(STR("child"));

	CHECK(xml_node().insert_move_after(xml_node(), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_move_after(doc.child(STR("node")), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_move_after(doc, doc) == xml_node());
	CHECK(doc.insert_move_after(xml_node(), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_move_after(doc.child(STR("node")), xml_node()) == xml_node());

	xml_node n1 = doc.child(STR("node")).insert_move_after(child, doc.child(STR("node")).first_child());
	CHECK(n1 && n1 == child);
	CHECK_STRING(n1.name(), STR("child"));
	CHECK_NODE(doc, STR("<node>foo<child>bar</child></node>"));

	xml_node n2 = doc.child(STR("node")).insert_move_after(doc.child(STR("node")).first_child(), child);
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>bar</child>foo</node>"));

	xml_node n3 = child.insert_move_after(doc.child(STR("node")).last_child(), child.first_child());
	CHECK(n3 && n3 != n1 && n3 == n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>barfoo</child></node>"));
}

TEST_XML(dom_node_insert_move_before, "<node>foo<child>bar</child></node>")
{
	xml_node child = doc.child(STR("node")).child(STR("child"));

	CHECK(xml_node().insert_move_before(xml_node(), xml_node()) == xml_node());
	CHECK(doc.child(STR("node")).first_child().insert_move_before(doc.child(STR("node")), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_move_before(doc, doc) == xml_node());
	CHECK(doc.insert_move_before(xml_node(), doc.child(STR("node"))) == xml_node());
	CHECK(doc.insert_move_before(doc.child(STR("node")), xml_node()) == xml_node());

	xml_node n1 = doc.child(STR("node")).insert_move_before(child, doc.child(STR("node")).first_child());
	CHECK(n1 && n1 == child);
	CHECK_STRING(n1.name(), STR("child"));
	CHECK_NODE(doc, STR("<node><child>bar</child>foo</node>"));

	xml_node n2 = doc.child(STR("node")).insert_move_before(doc.child(STR("node")).last_child(), child);
	CHECK(n2 && n2 != n1);
	CHECK_STRING(n2.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node>foo<child>bar</child></node>"));

	xml_node n3 = child.insert_move_before(doc.child(STR("node")).first_child(), child.first_child());
	CHECK(n3 && n3 != n1 && n3 == n2);
	CHECK_STRING(n3.value(), STR("foo"));
	CHECK_NODE(doc, STR("<node><child>foobar</child></node>"));
}

TEST_XML(dom_node_move_recursive, "<root><node>foo<child/></node></root>")
{
	xml_node root = doc.child(STR("root"));
	xml_node node = root.child(STR("node"));
	xml_node foo = node.first_child();
	xml_node child = node.last_child();

	CHECK(node.prepend_move(node) == xml_node());
	CHECK(node.prepend_move(root) == xml_node());

	CHECK(node.append_move(node) == xml_node());
	CHECK(node.append_move(root) == xml_node());

	CHECK(node.insert_move_before(node, foo) == xml_node());
	CHECK(node.insert_move_before(root, foo) == xml_node());

	CHECK(node.insert_move_after(node, foo) == xml_node());
	CHECK(node.insert_move_after(root, foo) == xml_node());

	CHECK(child.append_move(node) == xml_node());

	CHECK_NODE(doc, STR("<root><node>foo<child /></node></root>"));
}

TEST_XML(dom_node_move_marker, "<node />")
{
	xml_node node = doc.child(STR("node"));

	CHECK(doc.insert_move_before(node, node) == xml_node());
	CHECK(doc.insert_move_after(node, node) == xml_node());

	CHECK_NODE(doc, STR("<node />"));
}

TEST_XML(dom_node_move_crossdoc, "<node/>")
{
	xml_document newdoc;
	CHECK(newdoc.append_move(doc.child(STR("node"))) == xml_node());
	CHECK_NODE(newdoc, STR(""));
}

TEST_XML(dom_node_move_tree, "<root><n1 a1='v1'><c1/>t1</n1><n2 a2='v2'><c2/>t2</n2><n3 a3='v3'><c3/>t3</n3><n4 a4='v4'><c4/>t4</n4></root>")
{
	xml_node root = doc.child(STR("root"));
	xml_node n1 = root.child(STR("n1"));
	xml_node n2 = root.child(STR("n2"));
	xml_node n3 = root.child(STR("n3"));
	xml_node n4 = root.child(STR("n4"));

	// n2 n1 n3 n4
	CHECK(n2 == root.prepend_move(n2));

	// n2 n3 n4 n1
	CHECK(n1 == root.append_move(n1));

	// n2 n4 n3 n1
	CHECK(n4 == root.insert_move_before(n4, n3));

	// n2 n4 n1 + n3
	CHECK(n3 == doc.insert_move_after(n3, root));

	CHECK_NODE(doc, STR("<root><n2 a2=\"v2\"><c2 />t2</n2><n4 a4=\"v4\"><c4 />t4</n4><n1 a1=\"v1\"><c1 />t1</n1></root><n3 a3=\"v3\"><c3 />t3</n3>"));

	CHECK(n1 == root.child(STR("n1")));
	CHECK(n2 == root.child(STR("n2")));
	CHECK(n3 == doc.child(STR("n3")));
	CHECK(n4 == root.child(STR("n4")));
}

TEST(dom_node_copy_stackless)
{
	unsigned int count = 20000;
	std::basic_string<char_t> data;

	for (unsigned int i = 0; i < count; ++i)
		data += STR("<a>");

	data += STR("text");

	for (unsigned int j = 0; j < count; ++j)
		data += STR("</a>");

	xml_document doc;
	CHECK(doc.load_string(data.c_str()));

	xml_document copy;
	CHECK(copy.append_copy(doc.first_child()));

	CHECK_NODE(doc, data.c_str());
}

TEST(dom_node_copy_copyless)
{
	std::basic_string<char_t> data;
	data += STR("<node>");
	for (int i = 0; i < 10000; ++i)
		data += STR("pcdata");
	data += STR("<?name value?><child attr1=\"\" attr2=\"value2\" /></node>");

	std::basic_string<char_t> datacopy = data;

	// the document is parsed in-place so there should only be 1 page worth of allocations
	test_runner::_memory_fail_threshold = 32768 + 128;

#ifdef PUGIXML_COMPACT
	// ... and some space for hash table
	test_runner::_memory_fail_threshold += 2048;
#endif

	xml_document doc;
	CHECK(doc.load_buffer_inplace(&datacopy[0], datacopy.size() * sizeof(char_t), parse_full));

	// this copy should share all string storage; since there are not a lot of nodes we should not have *any* allocations here (everything will fit in the same page in the document)
	xml_node copy = doc.append_copy(doc.child(STR("node")));
	xml_node copy2 = doc.append_copy(copy);

	CHECK_NODE(copy, data.c_str());
	CHECK_NODE(copy2, data.c_str());
}

TEST(dom_node_copy_copyless_mix)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>pcdata<?name value?><child attr1=\"\" attr2=\"value2\" /></node>"), parse_full));

	xml_node child = doc.child(STR("node")).child(STR("child"));

	child.set_name(STR("copychild"));
	child.attribute(STR("attr2")).set_name(STR("copyattr2"));
	child.attribute(STR("attr1")).set_value(STR("copyvalue1"));

	std::basic_string<char_t> data;
	for (int i = 0; i < 10000; ++i)
		data += STR("pcdata");

	doc.child(STR("node")).text().set(data.c_str());

	xml_node copy = doc.append_copy(doc.child(STR("node")));
	xml_node copy2 = doc.append_copy(copy);

	std::basic_string<char_t> dataxml;
	dataxml += STR("<node>");
	dataxml += data;
	dataxml += STR("<?name value?><copychild attr1=\"copyvalue1\" copyattr2=\"value2\" /></node>");

	CHECK_NODE(copy, dataxml.c_str());
	CHECK_NODE(copy2, dataxml.c_str());
}

TEST_XML(dom_node_copy_copyless_taint, "<node attr=\"value\" />")
{
	xml_node node = doc.child(STR("node"));
	xml_node copy = doc.append_copy(node);

	CHECK_NODE(doc, STR("<node attr=\"value\" /><node attr=\"value\" />"));

	node.set_name(STR("nod1"));

	CHECK_NODE(doc, STR("<nod1 attr=\"value\" /><node attr=\"value\" />"));

	xml_node copy2 = doc.append_copy(copy);

	CHECK_NODE(doc, STR("<nod1 attr=\"value\" /><node attr=\"value\" /><node attr=\"value\" />"));

	copy.attribute(STR("attr")).set_value(STR("valu2"));

	CHECK_NODE(doc, STR("<nod1 attr=\"value\" /><node attr=\"valu2\" /><node attr=\"value\" />"));

	copy2.attribute(STR("attr")).set_name(STR("att3"));

	CHECK_NODE(doc, STR("<nod1 attr=\"value\" /><node attr=\"valu2\" /><node att3=\"value\" />"));
}

TEST(dom_node_copy_attribute_copyless)
{
	std::basic_string<char_t> data;
	data += STR("<node attr=\"");
	for (int i = 0; i < 10000; ++i)
		data += STR("data");
	data += STR("\" />");

	std::basic_string<char_t> datacopy = data;

	// the document is parsed in-place so there should only be 1 page worth of allocations
	test_runner::_memory_fail_threshold = 32768 + 128;

#ifdef PUGIXML_COMPACT
	// ... and some space for hash table
	test_runner::_memory_fail_threshold += 2048;
#endif

	xml_document doc;
	CHECK(doc.load_buffer_inplace(&datacopy[0], datacopy.size() * sizeof(char_t), parse_full));

	// this copy should share all string storage; since there are not a lot of nodes we should not have *any* allocations here (everything will fit in the same page in the document)
	xml_node copy1 = doc.append_child(STR("node"));
	copy1.append_copy(doc.first_child().first_attribute());

	xml_node copy2 = doc.append_child(STR("node"));
	copy2.append_copy(copy1.first_attribute());

	CHECK_NODE(copy1, data.c_str());
	CHECK_NODE(copy2, data.c_str());
}

TEST_XML(dom_node_copy_attribute_copyless_taint, "<node attr=\"value\" />")
{
	xml_node node = doc.child(STR("node"));
	xml_attribute attr = node.first_attribute();

	xml_node copy1 = doc.append_child(STR("copy1"));
	xml_node copy2 = doc.append_child(STR("copy2"));
	xml_node copy3 = doc.append_child(STR("copy3"));

	CHECK_NODE(doc, STR("<node attr=\"value\" /><copy1 /><copy2 /><copy3 />"));

	copy1.append_copy(attr);

	CHECK_NODE(doc, STR("<node attr=\"value\" /><copy1 attr=\"value\" /><copy2 /><copy3 />"));

	attr.set_name(STR("att1"));
	copy2.append_copy(attr);

	CHECK_NODE(doc, STR("<node att1=\"value\" /><copy1 attr=\"value\" /><copy2 att1=\"value\" /><copy3 />"));

	copy1.first_attribute().set_value(STR("valu2"));
	copy3.append_copy(copy1.first_attribute());

	CHECK_NODE(doc, STR("<node att1=\"value\" /><copy1 attr=\"valu2\" /><copy2 att1=\"value\" /><copy3 attr=\"valu2\" />"));
}

TEST_XML(dom_node_copy_out_of_memory_node, "<node><child1 /><child2 /><child3>text1<child4 />text2</child3></node>")
{
	test_runner::_memory_fail_threshold = 32768 * 2 + 4096;

	xml_document copy;
	CHECK_ALLOC_FAIL(for (int i = 0; i < 1000; ++i) copy.append_copy(doc.first_child()));
}

TEST_XML(dom_node_copy_out_of_memory_attr, "<node attr1='' attr2='' attr3='' attr4='' attr5='' attr6='' attr7='' attr8='' attr9='' attr10='' attr11='' attr12='' attr13='' attr14='' attr15='' />")
{
	test_runner::_memory_fail_threshold = 32768 * 2 + 4096;

	xml_document copy;
	CHECK_ALLOC_FAIL(for (int i = 0; i < 1000; ++i) copy.append_copy(doc.first_child()));
}

TEST_XML(dom_node_remove_deallocate, "<node attr='value'>text</node>")
{
	xml_node node = doc.child(STR("node"));

	xml_attribute attr = node.attribute(STR("attr"));
	attr.set_name(STR("longattr"));
	attr.set_value(STR("longvalue"));

	node.set_name(STR("longnode"));
	node.text().set(STR("longtext"));

	node.remove_attribute(attr);
	doc.remove_child(node);

	CHECK_NODE(doc, STR(""));
}

TEST_XML(dom_node_set_deallocate, "<node attr='value'>text</node>")
{
	xml_node node = doc.child(STR("node"));

	xml_attribute attr = node.attribute(STR("attr"));

	attr.set_name(STR("longattr"));
	attr.set_value(STR("longvalue"));
	node.set_name(STR("longnode"));

	attr.set_name(STR(""));
	attr.set_value(STR(""));
	node.set_name(STR(""));
	node.text().set(STR(""));

	CHECK_NODE(doc, STR("<:anonymous :anonymous=\"\"></:anonymous>"));
}

TEST(dom_node_copy_declaration_empty_name)
{
	xml_document doc1;
	xml_node decl1 = doc1.append_child(node_declaration);
	decl1.set_name(STR(""));

	xml_document doc2;
	xml_node decl2 = doc2.append_copy(decl1);

	CHECK_STRING(decl2.name(), STR(""));
}

template <typename T> bool fp_equal(T lhs, T rhs)
{
	// Several compilers compare float/double values on x87 stack without proper rounding
	// This causes roundtrip tests to fail, although they correctly preserve the data.
#if (defined(_MSC_VER) && _MSC_VER < 1400) || defined(__MWERKS__)
	return memcmp(&lhs, &rhs, sizeof(T)) == 0;
#else
	return lhs == rhs;
#endif
}

TEST(dom_fp_roundtrip_min_max_float)
{
	xml_document doc;
	xml_node node = doc.append_child(STR("node"));
	xml_attribute attr = node.append_attribute(STR("attr"));

	node.text().set(std::numeric_limits<float>::min());
	CHECK(fp_equal(node.text().as_float(), std::numeric_limits<float>::min()));

	attr.set_value(std::numeric_limits<float>::max());
	CHECK(fp_equal(attr.as_float(), std::numeric_limits<float>::max()));
}

TEST(dom_fp_roundtrip_min_max_double)
{
	xml_document doc;
	xml_node node = doc.append_child(STR("node"));
	xml_attribute attr = node.append_attribute(STR("attr"));

	attr.set_value(std::numeric_limits<double>::min());
	CHECK(fp_equal(attr.as_double(), std::numeric_limits<double>::min()));

	node.text().set(std::numeric_limits<double>::max());
	CHECK(fp_equal(node.text().as_double(), std::numeric_limits<double>::max()));
}

const double fp_roundtrip_base[] =
{
	0.31830988618379067154,
	0.43429448190325182765,
	0.57721566490153286061,
	0.69314718055994530942,
	0.70710678118654752440,
	0.78539816339744830962,
};

TEST(dom_fp_roundtrip_float)
{
	xml_document doc;

	for (int e = -125; e <= 128; ++e)
	{
		for (size_t i = 0; i < sizeof(fp_roundtrip_base) / sizeof(fp_roundtrip_base[0]); ++i)
		{
			float value = static_cast<float>(ldexp(fp_roundtrip_base[i], e));

			doc.text().set(value);
			CHECK(fp_equal(doc.text().as_float(), value));
		}
	}
}

// Borland C does not print double values with enough precision
#ifndef __BORLANDC__
TEST(dom_fp_roundtrip_double)
{
	xml_document doc;

	for (int e = -1021; e <= 1024; ++e)
	{
		for (size_t i = 0; i < sizeof(fp_roundtrip_base) / sizeof(fp_roundtrip_base[0]); ++i)
		{
		#if (defined(_MSC_VER) && _MSC_VER < 1400) || defined(__MWERKS__)
			// Not all runtime libraries guarantee roundtripping for denormals
			if (e == -1021 && fp_roundtrip_base[i] < 0.5)
				continue;
		#endif

		#ifdef __DMC__
			// Digital Mars C does not roundtrip on exactly one combination
			if (e == -12 && i == 1)
				continue;
		#endif

			double value = ldexp(fp_roundtrip_base[i], e);

			doc.text().set(value);
			CHECK(fp_equal(doc.text().as_double(), value));
		}
	}
}
#endif
