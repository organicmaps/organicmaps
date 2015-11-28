#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_paths_abbrev_w3c_1, "<node><para/><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para"));
	CHECK_XPATH_NODESET(n, STR("para")) % 3 % 5;
}

TEST_XML(xpath_paths_abbrev_w3c_2, "<node><para/><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("*"));
	CHECK_XPATH_NODESET(n, STR("*")) % 3 % 4 % 5;
}

TEST_XML(xpath_paths_abbrev_w3c_3, "<node>pcdata<child/><![CDATA[cdata]]></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("text()"));
	CHECK_XPATH_NODESET(n, STR("text()")) % 3 % 5;
}

TEST_XML(xpath_paths_abbrev_w3c_4, "<node name='value' foo='bar' />")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("@name"));
	CHECK_XPATH_NODESET(n, STR("@name")) % 3;
}

TEST_XML(xpath_paths_abbrev_w3c_5, "<node name='value' foo='bar' />")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("@*"));
	CHECK_XPATH_NODESET(n, STR("@*")) % 3 % 4;
}

TEST_XML(xpath_paths_abbrev_w3c_6, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[1]"));
	CHECK_XPATH_NODESET(n, STR("para[1]")) % 3;
}

TEST_XML(xpath_paths_abbrev_w3c_7, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[last()]"));
	CHECK_XPATH_NODESET(n, STR("para[last()]")) % 6;
}

TEST_XML(xpath_paths_abbrev_w3c_8, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;

	CHECK_XPATH_NODESET(c, STR("*/para"));
	CHECK_XPATH_NODESET(doc, STR("*/para")) % 3 % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_9, "<doc><chapter/><chapter/><chapter/><chapter/><chapter><section/><section/><section/></chapter><chapter/></doc>")
{
	xml_node c;
	xml_node n = doc.child(STR("doc")).child(STR("chapter"));

	CHECK_XPATH_NODESET(c, STR("/doc/chapter[5]/section[2]"));
	CHECK_XPATH_NODESET(n, STR("/doc/chapter[5]/section[2]")) % 9;
	CHECK_XPATH_NODESET(doc, STR("/doc/chapter[5]/section[2]")) % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_10, "<chapter><para><para/><para/><foo><para/></foo></para><foo/><para/></chapter>")
{
	xml_node c;

	CHECK_XPATH_NODESET(c, STR("chapter//para"));
	CHECK_XPATH_NODESET(doc, STR("chapter//para")) % 3 % 4 % 5 % 7 % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_11, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("//para"));
	CHECK_XPATH_NODESET(n, STR("//para")) % 3 % 4 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("//para")) % 3 % 4 % 5 % 7 % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_12, "<node><olist><item/></olist><item/><olist><olist><item/><item/></olist></olist></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("//olist/item"));
	CHECK_XPATH_NODESET(n, STR("//olist/item")) % 4 % 8 % 9;
	CHECK_XPATH_NODESET(n.child(STR("olist")), STR("//olist/item")) % 4 % 8 % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_13, "<node><child/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("."));
	CHECK_XPATH_NODESET(n, STR(".")) % 2;
	CHECK_XPATH_NODESET(n.child(STR("child")), STR(".")) % 3;
}

TEST_XML(xpath_paths_abbrev_w3c_14, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR(".//para"));
	CHECK_XPATH_NODESET(n, STR(".//para")) % 3 % 4 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR(".//para")) % 4 % 5 % 7;
}

TEST_XML(xpath_paths_abbrev_w3c_15, "<node lang='en'><child/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR(".."));
	CHECK_XPATH_NODESET(n, STR("..")) % 1;
	CHECK_XPATH_NODESET(n.child(STR("child")), STR("..")) % 2;
}

TEST_XML(xpath_paths_abbrev_w3c_16, "<node lang='en'><child/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("../@lang"));
	CHECK_XPATH_NODESET(n, STR("../@lang"));
	CHECK_XPATH_NODESET(n.child(STR("child")), STR("../@lang")) % 3;
}

TEST_XML(xpath_paths_abbrev_w3c_17, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[@type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("para[@type=\"warning\"]")) % 4 % 6 % 11 % 13 % 15;
}

TEST_XML(xpath_paths_abbrev_w3c_18, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[@type=\"warning\"][5]"));
	CHECK_XPATH_NODESET(n, STR("para[@type=\"warning\"][5]")) % 15;
}

TEST_XML(xpath_paths_abbrev_w3c_19a, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[5][@type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("para[5][@type=\"warning\"]"));
}

TEST_XML(xpath_paths_abbrev_w3c_19b, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("para[5][@type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("para[5][@type=\"warning\"]")) % 9;
}

TEST_XML(xpath_paths_abbrev_w3c_20, "<node><chapter><title>foo</title></chapter><chapter><title>Introduction</title></chapter><chapter><title>introduction</title></chapter><chapter/><chapter><title>Introduction</title><title>foo</title></chapter></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("chapter[title=\"Introduction\"]"));
	CHECK_XPATH_NODESET(n, STR("chapter[title=\"Introduction\"]")) % 6 % 13;
}

TEST_XML(xpath_paths_abbrev_w3c_21, "<node><chapter><title>foo</title></chapter><chapter><title>Introduction</title></chapter><chapter><title>introduction</title></chapter><chapter/><chapter><title>Introduction</title><title>foo</title></chapter></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("chapter[title]"));
	CHECK_XPATH_NODESET(n, STR("chapter[title]")) % 3 % 6 % 9 % 13;
}

TEST_XML(xpath_paths_abbrev_w3c_22, "<node><employee/><employee secretary=''/><employee assistant=''/><employee secretary='' assistant=''/><employee assistant='' secretary=''/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("employee[@secretary and @assistant]"));
	CHECK_XPATH_NODESET(n, STR("employee[@secretary and @assistant]")) % 8 % 11;
}

#endif
