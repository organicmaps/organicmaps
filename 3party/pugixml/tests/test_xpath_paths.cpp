#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_paths_axes_child, "<node attr='value'><child attr='value'><subchild/></child><another/><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	CHECK_XPATH_NODESET(c, STR("child:: node()"));

	CHECK_XPATH_NODESET(n, STR("child:: node()")) % 4 % 7 % 8; // child, another, last
	CHECK_XPATH_NODESET(n, STR("another/child:: node()"));

	CHECK_XPATH_NODESET(n, STR("@attr/child::node()"));
	CHECK_XPATH_NODESET(na, STR("child::node()"));
}

TEST_XML(xpath_paths_axes_descendant, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	CHECK_XPATH_NODESET(c, STR("descendant:: node()"));

	CHECK_XPATH_NODESET(n, STR("descendant:: node()")) % 4 % 6 % 7 % 8 % 9; // child, subchild, another, subchild, last
	CHECK_XPATH_NODESET(doc, STR("descendant:: node()")) % 2 % 4 % 6 % 7 % 8 % 9; // node, child, subchild, another, subchild, last
	CHECK_XPATH_NODESET(n, STR("another/descendant:: node()")) % 8; // subchild
	CHECK_XPATH_NODESET(n, STR("last/descendant:: node()"));

	CHECK_XPATH_NODESET(n, STR("@attr/descendant::node()"));
	CHECK_XPATH_NODESET(na, STR("descendant::node()"));
}

TEST_XML(xpath_paths_axes_parent, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	CHECK_XPATH_NODESET(c, STR("parent:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("parent:: node()")) % 2; // node
	CHECK_XPATH_NODESET(n, STR("child/subchild/parent:: node()")) % 4; // child
	CHECK_XPATH_NODESET(n, STR("@attr/parent:: node()")) % 2; // node
	CHECK_XPATH_NODESET(n, STR("parent:: node()")) % 1; // root
	CHECK_XPATH_NODESET(doc, STR("parent:: node()"));

	CHECK_XPATH_NODESET(na, STR("parent:: node()")) % 2; // node
}

TEST_XML(xpath_paths_axes_ancestor, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.child(STR("child")).attribute(STR("attr")), n.child(STR("child")));

	CHECK_XPATH_NODESET(c, STR("ancestor:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("ancestor:: node()")) % 2 % 1; // node, root
	CHECK_XPATH_NODESET(n, STR("child/subchild/ancestor:: node()")) % 4 % 2 % 1; // child, node, root
	CHECK_XPATH_NODESET(n, STR("child/@attr/ancestor:: node()")) % 4 % 2 % 1; // child, node, root
	CHECK_XPATH_NODESET(n, STR("ancestor:: node()")) % 1; // root
	CHECK_XPATH_NODESET(doc, STR("ancestor:: node()"));

	CHECK_XPATH_NODESET(na, STR("ancestor:: node()")) % 4 % 2 % 1; // child, node, root
}

TEST_XML(xpath_paths_axes_following_sibling, "<node attr1='value' attr2='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr1")), n);

	CHECK_XPATH_NODESET(c, STR("following-sibling:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("following-sibling:: node()")) % 8 % 10; // another, last
	CHECK_XPATH_NODESET(n.child(STR("last")), STR("following-sibling:: node()"));

	CHECK_XPATH_NODESET(n, STR("@attr1/following-sibling:: node()")); // attributes are not siblings
	CHECK_XPATH_NODESET(na, STR("following-sibling:: node()")); // attributes are not siblings
}

TEST_XML(xpath_paths_axes_preceding_sibling, "<node attr1='value' attr2='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr2")), n);

	CHECK_XPATH_NODESET(c, STR("preceding-sibling:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("preceding-sibling:: node()"));
	CHECK_XPATH_NODESET(n.child(STR("last")), STR("preceding-sibling:: node()")) % 8 % 5; // another, child

	CHECK_XPATH_NODESET(n, STR("@attr2/following-sibling:: node()")); // attributes are not siblings
	CHECK_XPATH_NODESET(na, STR("following-sibling:: node()")); // attributes are not siblings
}

TEST_XML(xpath_paths_axes_following, "<node attr1='value' attr2='value'><child attr='value'><subchild/></child><another><subchild/></another><almost/><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr1")), n);

	CHECK_XPATH_NODESET(c, STR("following:: node()"));

	CHECK_XPATH_NODESET(n, STR("following:: node()")); // no descendants
	CHECK_XPATH_NODESET(n.child(STR("child")), STR("following:: node()")) % 8 % 9 % 10 % 11; // another, subchild, almost, last
	CHECK_XPATH_NODESET(n.child(STR("child")).child(STR("subchild")), STR("following:: node()")) % 8 % 9 % 10 % 11; // another, subchild, almost, last
	CHECK_XPATH_NODESET(n.child(STR("last")), STR("following:: node()"));

	CHECK_XPATH_NODESET(n, STR("@attr1/following::node()")) % 5 % 7 % 8 % 9 % 10 % 11; // child, subchild, another, subchild, almost, last - because @/following
	CHECK_XPATH_NODESET(n, STR("child/@attr/following::node()")) % 7 % 8 % 9 % 10 % 11; // subchild, another, subchild, almost, last
	CHECK_XPATH_NODESET(na, STR("following::node()")) % 5 % 7 % 8 % 9 % 10 % 11; // child, subchild, another, subchild, almost, last - because @/following
}

TEST_XML(xpath_paths_axes_preceding, "<node attr1='value' attr2='value'><child attr='value'><subchild/></child><another><subchild id='1'/></another><almost/><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.child(STR("child")).attribute(STR("attr")), n.child(STR("child")));

	CHECK_XPATH_NODESET(c, STR("preceding:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("preceding:: node()")); // no ancestors
	CHECK_XPATH_NODESET(n.child(STR("last")), STR("preceding:: node()")) % 11 % 9 % 8 % 7 % 5; // almost, subchild, another, subchild, child
	CHECK_XPATH_NODESET(n.child(STR("another")).child(STR("subchild")), STR("preceding:: node()")) % 7 % 5; // subchild, child
	CHECK_XPATH_NODESET(n, STR("preceding:: node()"));

	CHECK_XPATH_NODESET(n, STR("child/@attr/preceding::node()")); // no ancestors
	CHECK_XPATH_NODESET(n, STR("//subchild[@id]/@id/preceding::node()")) % 7 % 5; // subchild, child
	CHECK_XPATH_NODESET(na, STR("preceding::node()")); // no ancestors
}

TEST_XML(xpath_paths_axes_attribute, "<node attr1='value' attr2='value'><child attr='value'><subchild/></child><another xmlns:foo='bar'><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr1")), n);

	CHECK_XPATH_NODESET(c, STR("attribute:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("attribute:: node()")) % 6; // child/@attr
	CHECK_XPATH_NODESET(n.child(STR("last")), STR("attribute:: node()"));
	CHECK_XPATH_NODESET(n, STR("attribute:: node()")) % 3 % 4; // node/@attr1 node/@attr2
	CHECK_XPATH_NODESET(doc, STR("descendant-or-self:: node()/attribute:: node()")) % 3 % 4 % 6; // all attributes
	CHECK_XPATH_NODESET(n.child(STR("another")), STR("attribute:: node()")); // namespace nodes are not attributes

	CHECK_XPATH_NODESET(n, STR("@attr1/attribute::node()"));
	CHECK_XPATH_NODESET(na, STR("attribute::node()"));
}

TEST_XML(xpath_paths_axes_namespace, "<node xmlns:foo='bar' attr='value'/>")
{
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	// namespace nodes are not supported
	CHECK_XPATH_NODESET(n, STR("namespace:: node()"));
	CHECK_XPATH_NODESET(n, STR("@attr/attribute::node()"));
	CHECK_XPATH_NODESET(na, STR("attribute::node()"));
}

TEST_XML(xpath_paths_axes_self, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	CHECK_XPATH_NODESET(c, STR("self:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("self:: node()")) % 4; // child
	CHECK_XPATH_NODESET(n, STR("self:: node()")) % 2; // node
	CHECK_XPATH_NODESET(n, STR("child/self:: node()")) % 4; // child
	CHECK_XPATH_NODESET(n, STR("child/@attr/self:: node()")) % 5; // @attr
	CHECK_XPATH_NODESET(doc, STR("self:: node()")) % 1; // root
	CHECK_XPATH_NODESET(na, STR("self:: node()")) % 3; // @attr
}

TEST_XML(xpath_paths_axes_descendant_or_self, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.child(STR("child")).attribute(STR("attr")), n.child(STR("child")));

	CHECK_XPATH_NODESET(c, STR("descendant-or-self:: node()"));

	CHECK_XPATH_NODESET(n, STR("descendant-or-self:: node()")) % 2 % 4 % 6 % 7 % 8 % 9; // node, child, subchild, another, subchild, last
	CHECK_XPATH_NODESET(doc, STR("descendant-or-self:: node()")) % 1 % 2 % 4 % 6 % 7 % 8 % 9; // root, node, child, subchild, another, subchild, last
	CHECK_XPATH_NODESET(n, STR("another/descendant-or-self:: node()")) % 7 % 8; // another, subchild
	CHECK_XPATH_NODESET(n, STR("last/descendant-or-self:: node()")) % 9; // last

	CHECK_XPATH_NODESET(n, STR("child/@attr/descendant-or-self::node()")) % 5; // @attr
	CHECK_XPATH_NODESET(na, STR("descendant-or-self::node()")) % 5; // @attr
}

TEST_XML(xpath_paths_axes_ancestor_or_self, "<node attr='value'><child attr='value'><subchild/></child><another><subchild/></another><last/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.child(STR("child")).attribute(STR("attr")), n.child(STR("child")));

	CHECK_XPATH_NODESET(c, STR("ancestor-or-self:: node()"));

	CHECK_XPATH_NODESET(n.child(STR("child")), STR("ancestor-or-self:: node()")) % 4 % 2 % 1; // child, node, root
	CHECK_XPATH_NODESET(n, STR("child/subchild/ancestor-or-self:: node()")) % 6 % 4 % 2 % 1; // subchild, child, node, root
	CHECK_XPATH_NODESET(n, STR("child/@attr/ancestor-or-self:: node()")) % 5 % 4 % 2 % 1; // @attr, child, node, root
	CHECK_XPATH_NODESET(n, STR("ancestor-or-self:: node()")) % 2 % 1; // root, node
	CHECK_XPATH_NODESET(doc, STR("ancestor-or-self:: node()")) % 1; // root
	CHECK_XPATH_NODESET(n, STR("ancestor-or-self:: node()")) % 2 % 1; // root, node
	CHECK_XPATH_NODESET(n, STR("last/ancestor-or-self::node()")) % 9 % 2 % 1; // root, node, last
	CHECK_XPATH_NODESET(na, STR("ancestor-or-self:: node()")) % 5 % 4 % 2 % 1; // @attr, child, node, root
}

TEST_XML(xpath_paths_axes_abbrev, "<node attr='value'><foo/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// @ axis
	CHECK_XPATH_NODESET(c, STR("@attr"));
	CHECK_XPATH_NODESET(n, STR("@attr")) % 3;

	// no axis - child implied
	CHECK_XPATH_NODESET(c, STR("foo"));
	CHECK_XPATH_NODESET(n, STR("foo")) % 4;
	CHECK_XPATH_NODESET(doc, STR("node()")) % 2;

	// @ axis should disable all other axis specifiers
	CHECK_XPATH_FAIL(STR("@child::foo"));
	CHECK_XPATH_FAIL(STR("@attribute::foo"));
}

TEST_XML(xpath_paths_nodetest_all, "<node a1='v1' x:a2='v2'><c1/><x:c2/><c3/><x:c4/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("*"));
	CHECK_XPATH_NODESET(c, STR("child::*"));

	CHECK_XPATH_NODESET(n, STR("*")) % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(n, STR("child::*")) % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(n, STR("attribute::*")) % 3 % 4;
}

TEST_XML_FLAGS(xpath_paths_nodetest_name, "<node a1='v1' x:a2='v2'><c1/><x:c2/><c3/><x:c4/><?c1?></node>", parse_default | parse_pi)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("c1"));
	CHECK_XPATH_NODESET(c, STR("child::c1"));

	CHECK_XPATH_NODESET(n, STR("c1")) % 5;
	CHECK_XPATH_NODESET(n, STR("x:c2")) % 6;

	CHECK_XPATH_NODESET(n, STR("child::c1")) % 5;
	CHECK_XPATH_NODESET(n, STR("child::x:c2")) % 6;

	CHECK_XPATH_NODESET(n, STR("attribute::a1")) % 3;
	CHECK_XPATH_NODESET(n, STR("attribute::x:a2")) % 4;
	CHECK_XPATH_NODESET(n, STR("@x:a2")) % 4;
}

TEST_XML(xpath_paths_nodetest_all_in_namespace, "<node a1='v1' x:a2='v2'><c1/><x:c2/><c3/><x:c4/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("x:*"));
	CHECK_XPATH_NODESET(c, STR("child::x:*"));

	CHECK_XPATH_NODESET(n, STR("x:*")) % 6 % 8;
	CHECK_XPATH_NODESET(n, STR("child::x:*")) % 6 % 8;

	CHECK_XPATH_NODESET(n, STR("attribute::x:*")) % 4;
	CHECK_XPATH_NODESET(n, STR("@x:*")) % 4;

	CHECK_XPATH_FAIL(STR(":*"));
	CHECK_XPATH_FAIL(STR("@:*"));
}

TEST_XML_FLAGS(xpath_paths_nodetest_type, "<node attr='value'>pcdata<child/><?pi1 value?><?pi2 value?><!--comment--><![CDATA[cdata]]></node>", parse_default | parse_pi | parse_comments)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// check on empty nodes
	CHECK_XPATH_NODESET(c, STR("node()"));
	CHECK_XPATH_NODESET(c, STR("text()"));
	CHECK_XPATH_NODESET(c, STR("comment()"));
	CHECK_XPATH_NODESET(c, STR("processing-instruction()"));
	CHECK_XPATH_NODESET(c, STR("processing-instruction('foobar')"));

	// child axis
	CHECK_XPATH_NODESET(n, STR("node()")) % 4 % 5 % 6 % 7 % 8 % 9;
	CHECK_XPATH_NODESET(n, STR("text()")) % 4 % 9;
	CHECK_XPATH_NODESET(n, STR("comment()")) % 8;
	CHECK_XPATH_NODESET(n, STR("processing-instruction()")) % 6 % 7;
	CHECK_XPATH_NODESET(n, STR("processing-instruction('pi2')")) % 7;

	// attribute axis
	CHECK_XPATH_NODESET(n, STR("@node()")) % 3;
	CHECK_XPATH_NODESET(n, STR("@text()"));
	CHECK_XPATH_NODESET(n, STR("@comment()"));
	CHECK_XPATH_NODESET(n, STR("@processing-instruction()"));
	CHECK_XPATH_NODESET(n, STR("@processing-instruction('pi2')"));

	// incorrect 'argument' number
	CHECK_XPATH_FAIL(STR("node('')"));
	CHECK_XPATH_FAIL(STR("text('')"));
	CHECK_XPATH_FAIL(STR("comment('')"));
	CHECK_XPATH_FAIL(STR("processing-instruction(1)"));
	CHECK_XPATH_FAIL(STR("processing-instruction('', '')"));
	CHECK_XPATH_FAIL(STR("processing-instruction(concat('a', 'b'))"));
}

TEST_XML_FLAGS(xpath_paths_nodetest_principal, "<node attr='value'>pcdata<child/><?pi1 value?><?pi2 value?><!--comment--><![CDATA[cdata]]></node><abra:cadabra abra:arba=''/>", parse_default | parse_pi | parse_comments)
{
	// node() test is true for any node type
	CHECK_XPATH_NODESET(doc, STR("//node()")) % 2 % 4 % 5 % 6 % 7 % 8 % 9 % 10;
	CHECK_XPATH_NODESET(doc, STR("//attribute::node()")) % 3 % 11;
	CHECK_XPATH_NODESET(doc, STR("//attribute::node()/ancestor-or-self::node()")) % 1 % 2 % 3 % 10 % 11;

	// name test is true only for node with principal node type (depends on axis)
	CHECK_XPATH_NODESET(doc, STR("node/child::child")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::attr")) % 3;
	CHECK_XPATH_NODESET(doc, STR("node/child::pi1"));
	CHECK_XPATH_NODESET(doc, STR("node/child::attr"));
	CHECK_XPATH_NODESET(doc, STR("node/child::child/self::child")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::attr/self::attr")); // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("node/child::child/ancestor-or-self::child")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::attr/ancestor-or-self::attr")); // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("node/child::child/descendant-or-self::child")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::attr/descendant-or-self::attr")); // attribute is not of element type

	// any name test is true only for node with principal node type (depends on axis)
	CHECK_XPATH_NODESET(doc, STR("node/child::*")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::*")) % 3;
	CHECK_XPATH_NODESET(doc, STR("node/child::*/self::*")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::*/self::*")); // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("node/child::*/ancestor-or-self::*")) % 5 % 2;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::*/ancestor-or-self::*")) % 2; // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("node/child::*/descendant-or-self::*")) % 5;
	CHECK_XPATH_NODESET(doc, STR("node/attribute::*/descendant-or-self::*")); // attribute is not of element type

	// namespace test is true only for node with principal node type (depends on axis)
	CHECK_XPATH_NODESET(doc, STR("child::abra:*")) % 10;
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/attribute::abra:*")) % 11;
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/self::abra:*")) % 10;
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/attribute::abra:*/self::abra:*")); // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/ancestor-or-self::abra:*")) % 10;
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/attribute::abra:*/ancestor-or-self::abra:*")) % 10; // attribute is not of element type
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/descendant-or-self::abra:*")) % 10;
	CHECK_XPATH_NODESET(doc, STR("child::abra:*/attribute::abra:*/descendant-or-self::abra:*")); // attribute is not of element type
}

TEST_XML(xpath_paths_absolute, "<node attr='value'><foo><foo/><foo/></foo></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));
	xpath_node na(n.attribute(STR("attr")), n);

	CHECK_XPATH_NODESET(c, STR("/foo"));
	CHECK_XPATH_NODESET(n, STR("/foo"));
	CHECK_XPATH_NODESET(n, STR("/node/foo")) % 4;
	CHECK_XPATH_NODESET(n.child(STR("foo")), STR("/node/foo")) % 4;
	CHECK_XPATH_NODESET(na, STR("/node/foo")) % 4;

	CHECK_XPATH_NODESET(c, STR("/"));
	CHECK_XPATH_NODESET(n, STR("/")) % 1;
	CHECK_XPATH_NODESET(n.child(STR("foo")), STR("/")) % 1;
	CHECK_XPATH_NODESET(na, STR("/")) % 1;
}

TEST_XML(xpath_paths_step_abbrev, "<node><foo/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("."));
	CHECK_XPATH_NODESET(c, STR(".."));

	CHECK_XPATH_NODESET(n, STR(".")) % 2;
	CHECK_XPATH_NODESET(n, STR("..")) % 1;
	CHECK_XPATH_NODESET(n, STR("../node")) % 2;
	CHECK_XPATH_NODESET(n.child(STR("foo")), STR("..")) % 2;

	CHECK_XPATH_FAIL(STR(".node"));
	CHECK_XPATH_FAIL(STR("..node"));
}

TEST_XML(xpath_paths_relative_abbrev, "<node><foo><foo/><foo/></foo></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("foo//bar"));

	CHECK_XPATH_NODESET(n, STR("foo/foo")) % 4 % 5;
	CHECK_XPATH_NODESET(n, STR("foo//foo")) % 4 % 5;
	CHECK_XPATH_NODESET(n, STR(".//foo")) % 3 % 4 % 5;
}

TEST_XML(xpath_paths_absolute_abbrev, "<node><foo><foo/><foo/></foo></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("//bar"));

	CHECK_XPATH_NODESET(n, STR("//foo")) % 3 % 4 % 5;
	CHECK_XPATH_NODESET(n.child(STR("foo")), STR("//foo")) % 3 % 4 % 5;
	CHECK_XPATH_NODESET(doc, STR("//foo")) % 3 % 4 % 5;
}

TEST_XML(xpath_paths_predicate_boolean, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[position()=1]")) % 6;
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[position()=2]")) % 7;
	CHECK_XPATH_NODESET(n, STR("preceding-sibling::chapter[position()=1]")) % 4;
	CHECK_XPATH_NODESET(n, STR("preceding-sibling::chapter[position()=2]")) % 3;
}

TEST_XML(xpath_paths_predicate_number, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[1]")) % 6;
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[2]")) % 7;
	CHECK_XPATH_NODESET(n, STR("preceding-sibling::chapter[1]")) % 4;
	CHECK_XPATH_NODESET(n, STR("preceding-sibling::chapter[2]")) % 3;
}

TEST_XML(xpath_paths_predicate_number_boundary, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("node/chapter[0.999999999999999]"));
	CHECK_XPATH_NODESET(doc, STR("node/chapter[1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("node/chapter[1.000000000000001]"));
	CHECK_XPATH_NODESET(doc, STR("node/chapter[1.999999999999999]"));
	CHECK_XPATH_NODESET(doc, STR("node/chapter[2]")) % 4;
	CHECK_XPATH_NODESET(doc, STR("node/chapter[2.000000000000001]"));
	CHECK_XPATH_NODESET(doc, STR("node/chapter[4.999999999999999]"));
	CHECK_XPATH_NODESET(doc, STR("node/chapter[5]")) % 7;
	CHECK_XPATH_NODESET(doc, STR("node/chapter[5.000000000000001]"));
}

TEST_XML(xpath_paths_predicate_number_out_of_range, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[0]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[-1]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[-1000000000000]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[-1 div 0]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[1000000000000]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[1 div 0]"));

#ifndef MSVC6_NAN_BUG
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[0 div 0]"));
#endif
}

TEST_XML(xpath_paths_predicate_constant_boolean, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	xpath_variable_set set;
	set.set(STR("true"), true);
	set.set(STR("false"), false);

	CHECK_XPATH_NODESET_VAR(n, STR("following-sibling::chapter[$false]"), &set);
	CHECK_XPATH_NODESET_VAR(n, STR("following-sibling::chapter[$true]"), &set) % 6 % 7;
}

TEST_XML(xpath_paths_predicate_position_eq, "<node><chapter/><chapter/><chapter>3</chapter><chapter/><chapter/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("node/chapter[position()=1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("node/chapter[position()=2+2]")) % 7;
	CHECK_XPATH_NODESET(doc, STR("node/chapter[position()=last()]")) % 8;

#ifndef MSVC6_NAN_BUG
	CHECK_XPATH_NODESET(doc, STR("node/chapter[position()=string()]")) % 5;
#endif
}

TEST_XML(xpath_paths_predicate_several, "<node><employee/><employee secretary=''/><employee assistant=''/><employee secretary='' assistant=''/><employee assistant='' secretary=''/></node>")
{
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(n, STR("employee")) % 3 % 4 % 6 % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("employee[@secretary]")) % 4 % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("employee[@assistant]")) % 6 % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("employee[@secretary][@assistant]")) % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("employee[@assistant][@secretary]")) % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("employee[@secretary and @assistant]")) % 8 % 11;
}

TEST_XML(xpath_paths_predicate_filter_boolean, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(n, STR("(following-sibling::chapter)[position()=1]")) % 6;
	CHECK_XPATH_NODESET(n, STR("(following-sibling::chapter)[position()=2]")) % 7;
	CHECK_XPATH_NODESET(n, STR("(preceding-sibling::chapter)[position()=1]")) % 3;
	CHECK_XPATH_NODESET(n, STR("(preceding-sibling::chapter)[position()=2]")) % 4;
}

TEST_XML(xpath_paths_predicate_filter_number, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(n, STR("(following-sibling::chapter)[1]")) % 6;
	CHECK_XPATH_NODESET(n, STR("(following-sibling::chapter)[2]")) % 7;
	CHECK_XPATH_NODESET(n, STR("(preceding-sibling::chapter)[1]")) % 3;
	CHECK_XPATH_NODESET(n, STR("(preceding-sibling::chapter)[2]")) % 4;
}

TEST_XML(xpath_paths_predicate_filter_posinv, "<node><employee/><employee secretary=''/><employee assistant=''/><employee secretary='' assistant=''/><employee assistant='' secretary=''/></node>")
{
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(n, STR("employee")) % 3 % 4 % 6 % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("(employee[@secretary])[@assistant]")) % 8 % 11;
	CHECK_XPATH_NODESET(n, STR("((employee)[@assistant])[@secretary]")) % 8 % 11;
}

TEST_XML(xpath_paths_step_compose, "<node><foo><foo/><foo/></foo><foo/></node>")
{
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(n, STR("(.)/foo")) % 3 % 6;
	CHECK_XPATH_NODESET(n, STR("(.)//foo")) % 3 % 4 % 5 % 6;
	CHECK_XPATH_NODESET(n, STR("(./..)//*")) % 2 % 3 % 4 % 5 % 6;

	CHECK_XPATH_FAIL(STR("(1)/foo"));
	CHECK_XPATH_FAIL(STR("(1)//foo"));
}

TEST_XML(xpath_paths_descendant_double_slash_w3c, "<node><para><para/><para/><para><para/></para></para><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//para")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//para[1]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[1]")) % 3;
}

TEST_XML(xpath_paths_needs_sorting, "<node><child/><child/><child><subchild/><subchild/></child></node>")
{
    CHECK_XPATH_NODESET(doc, STR("(node/child/subchild)[2]")) % 7;
}

TEST_XML(xpath_paths_descendant_filters, "<node><para><para/><para/><para><para/></para></para><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//para[1]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[true()][1]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[true()][1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[1][true()]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[1][true()]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[1][2]"));
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[1][2]"));
	CHECK_XPATH_NODESET(doc, STR("//para[true()]")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[true()]")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//para[position()=1][true()]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[position()=1][true()]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[true()][position()=1]")) % 3 % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant::para[true()][position()=1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//node()[self::para]")) % 3 % 4 % 5 % 6 % 7 % 8;
}

TEST_XML(xpath_paths_descendant_optimize, "<node><para><para/><para/><para><para/></para></para><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//para")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("/descendant-or-self::node()/child::para")) % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("/descendant-or-self::node()[name()='para']/child::para")) % 4 % 5 % 6 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant-or-self::node()[name()='para']/child::para[1]")) % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("/descendant-or-self::node()[3]/child::para")) % 4 % 5 % 6;
}

TEST_XML(xpath_paths_descendant_optimize_axes, "<node><para><para/><para/><para><para/></para></para><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//.")) % 1 % 2 % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//descendant::*")) % 2 % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//descendant-or-self::*")) % 2 % 3 % 4 % 5 % 6 % 7 % 8;

	CHECK_XPATH_NODESET(doc, STR("//..")) % 1 % 2 % 3 % 6;
	CHECK_XPATH_NODESET(doc, STR("//ancestor::*")) % 2 % 3 % 6;
	CHECK_XPATH_NODESET(doc, STR("//ancestor-or-self::*")) % 2 % 3 % 4 % 5 % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//preceding-sibling::*")) % 3 % 4 % 5;
	CHECK_XPATH_NODESET(doc, STR("//following-sibling::*")) % 5 % 6 % 8;
	CHECK_XPATH_NODESET(doc, STR("//preceding::*")) % 3 % 4 % 5 % 6 % 7;
	CHECK_XPATH_NODESET(doc, STR("//following::*")) % 5 % 6 % 7 % 8;
}

TEST_XML(xpath_paths_descendant_optimize_last, "<node><para><para/><para/><para><para/></para></para><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//para[last()]")) % 6 % 7 % 8;
	CHECK_XPATH_NODESET(doc, STR("//para[last() = 1]")) % 7;
}

TEST_XML(xpath_paths_precision, "<node><para/><para/><para/><para/><para/></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//para[1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[3 div 3]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[6 div 3 - 1]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("//para[6 * (1 div 3) - 1]")) % 3;
}

TEST_XML(xpath_paths_unsorted_child, "<node><foo><bar/></foo><node><foo><bar/></foo></node><foo><bar/></foo></node>")
{
	CHECK_XPATH_NODESET(doc, STR("//node/foo")) % 3 % 6 % 8;
	CHECK_XPATH_NODESET(doc, STR("//node/foo/bar")) % 4 % 7 % 9;

	xpath_node_set ns = doc.select_nodes(STR("//node/foo/bar"));
	CHECK(ns.type() == xpath_node_set::type_unsorted);

	xpath_node_set nss = ns;
	nss.sort();

	CHECK(ns[0] == nss[0]);
	CHECK(ns[1] == nss[2]);
	CHECK(ns[2] == nss[1]);
}

TEST_XML(xpath_paths_optimize_compare_attribute, "<node id='1' /><node id='2' /><node xmlns='3' />")
{
	CHECK_XPATH_NODESET(doc, STR("node[@id = '1']")) % 2;
	CHECK_XPATH_NODESET(doc, STR("node[@id = '2']")) % 4;
	CHECK_XPATH_NODESET(doc, STR("node[@id = 2]")) % 4;
	CHECK_XPATH_NODESET(doc, STR("node[@id[. > 3] = '2']"));
	CHECK_XPATH_NODESET(doc, STR("node['1' = @id]")) % 2;

	xpath_variable_set set;
	set.set(STR("var1"), STR("2"));
	set.set(STR("var2"), 2.0);

	CHECK_XPATH_NODESET_VAR(doc, STR("node[@id = $var1]"), &set) % 4;
	CHECK_XPATH_NODESET_VAR(doc, STR("node[@id = $var2]"), &set) % 4;

	CHECK_XPATH_NODESET(doc, STR("node[@xmlns = '3']"));
}

TEST_XML(xpath_paths_optimize_step_once, "<node><para1><para2/><para3/><para4><para5 attr5=''/></para4></para1><para6/></node>")
{
    CHECK_XPATH_BOOLEAN(doc, STR("node//para2/following::*"), true);
    CHECK_XPATH_BOOLEAN(doc, STR("node//para6/following::*"), false);

    CHECK_XPATH_STRING(doc, STR("name(node//para2/following::*)"), STR("para3"));
    CHECK_XPATH_STRING(doc, STR("name(node//para6/following::*)"), STR(""));

    CHECK_XPATH_BOOLEAN(doc, STR("node//para1/preceding::*"), false);
    CHECK_XPATH_BOOLEAN(doc, STR("node//para6/preceding::*"), true);

    CHECK_XPATH_STRING(doc, STR("name(node//para1/preceding::*)"), STR(""));
    CHECK_XPATH_STRING(doc, STR("name(node//para6/preceding::*)"), STR("para1"));

    CHECK_XPATH_BOOLEAN(doc, STR("node//para6/preceding::para4"), true);

    CHECK_XPATH_BOOLEAN(doc, STR("//@attr5/ancestor-or-self::*"), true);
    CHECK_XPATH_BOOLEAN(doc, STR("//@attr5/ancestor::*"), true);

    CHECK_XPATH_BOOLEAN(doc, STR("//@attr5/following::para6"), true);
    CHECK_XPATH_STRING(doc, STR("name(//@attr5/following::para6)"), STR("para6"));

    CHECK_XPATH_BOOLEAN(doc, STR("//para5/ancestor-or-self::*"), true);
    CHECK_XPATH_BOOLEAN(doc, STR("//para5/ancestor::*"), true);

    CHECK_XPATH_BOOLEAN(doc, STR("//@attr5/ancestor-or-self::node()"), true);
}

TEST_XML(xpath_paths_null_nodeset_entries, "<node attr='value'/>")
{
    xpath_node nodes[] =
    {
        xpath_node(doc.first_child()),
        xpath_node(xml_node()),
        xpath_node(doc.first_child().first_attribute(), doc.first_child()),
        xpath_node(xml_attribute(), doc.first_child()),
        xpath_node(xml_attribute(), xml_node()),
    };

    xpath_node_set ns(nodes, nodes + sizeof(nodes) / sizeof(nodes[0]));

    xpath_variable_set vars;
    vars.set(STR("x"), ns);

    xpath_node_set rs = xpath_query(STR("$x/."), &vars).evaluate_node_set(xml_node());

    CHECK(rs.size() == 2);
    CHECK(rs[0] == nodes[0]);
    CHECK(rs[1] == nodes[2]);
}
#endif
