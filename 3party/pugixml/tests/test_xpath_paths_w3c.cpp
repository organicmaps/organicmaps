#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_paths_w3c_1, "<node><para/><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para"));
	CHECK_XPATH_NODESET(n, STR("child::para")) % 3 % 5;
}

TEST_XML(xpath_paths_w3c_2, "<node><para/><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::*"));
	CHECK_XPATH_NODESET(n, STR("child::*")) % 3 % 4 % 5;
}

TEST_XML(xpath_paths_w3c_3, "<node>pcdata<child/><![CDATA[cdata]]></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::text()"));
	CHECK_XPATH_NODESET(n, STR("child::text()")) % 3 % 5;
}

TEST_XML(xpath_paths_w3c_4, "<node>pcdata<child/><![CDATA[cdata]]></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::node()"));
	CHECK_XPATH_NODESET(n, STR("child::node()")) % 3 % 4 % 5;
}

TEST_XML(xpath_paths_w3c_5, "<node name='value' foo='bar' />")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("attribute::name"));
	CHECK_XPATH_NODESET(n, STR("attribute::name")) % 3;
}

TEST_XML(xpath_paths_w3c_6, "<node name='value' foo='bar' />")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("attribute::*"));
	CHECK_XPATH_NODESET(n, STR("attribute::*")) % 3 % 4;
}

TEST_XML(xpath_paths_w3c_7, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("descendant::para"));
	CHECK_XPATH_NODESET(n, STR("descendant::para")) % 3 % 4 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("descendant::para")) % 4 % 5 % 7;
}

TEST_XML(xpath_paths_w3c_8, "<node><div><font><div><div/></div></font></div></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("ancestor::div"));
	CHECK_XPATH_NODESET(n.child(STR("div")).child(STR("font")).child(STR("div")).child(STR("div")), STR("ancestor::div")) % 5 % 3;
}

TEST_XML(xpath_paths_w3c_9, "<node><div><font><div><div/></div></font></div></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("ancestor-or-self::div"));
	CHECK_XPATH_NODESET(n.child(STR("div")).child(STR("font")).child(STR("div")).child(STR("div")), STR("ancestor-or-self::div")) % 6 % 5 % 3;
}

TEST_XML(xpath_paths_w3c_10, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("descendant-or-self::para"));
	CHECK_XPATH_NODESET(n, STR("descendant-or-self::para")) % 3 % 4 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("descendant-or-self::para")) % 3 % 4 % 5 % 7;
}

TEST_XML(xpath_paths_w3c_11, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("self::para"));
	CHECK_XPATH_NODESET(n, STR("self::para"));
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("self::para")) % 3;
}

TEST_XML(xpath_paths_w3c_12, "<chapter><para><para/><para/><foo><para/></foo></para><foo/><para/></chapter>")
{
	xml_node c;

	CHECK_XPATH_NODESET(c, STR("child::chapter/descendant::para"));
	CHECK_XPATH_NODESET(doc, STR("child::chapter/descendant::para")) % 3 % 4 % 5 % 7 % 9;
}

TEST_XML(xpath_paths_w3c_13, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;

	CHECK_XPATH_NODESET(c, STR("child::*/child::para"));
	CHECK_XPATH_NODESET(doc, STR("child::*/child::para")) % 3 % 9;
}

TEST_XML(xpath_paths_w3c_14, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("/"));

	CHECK_XPATH_NODESET(doc, STR("/")) % 1;
	CHECK_XPATH_NODESET(n, STR("/")) % 1;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("/")) % 1;
}

TEST_XML(xpath_paths_w3c_15, "<node><para><para/><para/><foo><para/></foo></para><foo/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("/descendant::para"));
	CHECK_XPATH_NODESET(n, STR("/descendant::para")) % 3 % 4 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(n.child(STR("para")), STR("/descendant::para")) % 3 % 4 % 5 % 7 % 9;
}

TEST_XML(xpath_paths_w3c_16, "<node><olist><item/></olist><item/><olist><olist><item/><item/></olist></olist></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("/descendant::olist/child::item"));
	CHECK_XPATH_NODESET(n, STR("/descendant::olist/child::item")) % 4 % 8 % 9;
	CHECK_XPATH_NODESET(n.child(STR("olist")), STR("/descendant::olist/child::item")) % 4 % 8 % 9;
}

TEST_XML(xpath_paths_w3c_17, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()=1]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()=1]")) % 3;
}

TEST_XML(xpath_paths_w3c_18, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()=last()]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()=last()]")) % 6;
}

TEST_XML(xpath_paths_w3c_19, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()=last()-1]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()=last()-1]")) % 5;
}

TEST_XML(xpath_paths_w3c_20, "<node><para/><para/><para/><para/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()>1]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()>1]")) % 4 % 5 % 6;
}

TEST_XML(xpath_paths_w3c_21, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(c, STR("following-sibling::chapter[position()=1]"));
	CHECK_XPATH_NODESET(n, STR("following-sibling::chapter[position()=1]")) % 6;
}

TEST_XML(xpath_paths_w3c_22, "<node><chapter/><chapter/><chapter/><chapter/><chapter/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node")).child(STR("chapter")).next_sibling().next_sibling();

	CHECK_XPATH_NODESET(c, STR("preceding-sibling::chapter[position()=1]"));
	CHECK_XPATH_NODESET(n, STR("preceding-sibling::chapter[position()=1]")) % 4;
}

TEST_XML(xpath_paths_w3c_23, "<node><figure><figure/><figure/><foo><figure/></foo></figure><foo/><figure/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("/descendant::figure[position()=4]"));
	CHECK_XPATH_NODESET(n, STR("/descendant::figure[position()=4]")) % 7;
	CHECK_XPATH_NODESET(n.child(STR("figure")), STR("/descendant::figure[position()=4]")) % 7;
}

TEST_XML(xpath_paths_w3c_24, "<doc><chapter/><chapter/><chapter/><chapter/><chapter><section/><section/><section/></chapter><chapter/></doc>")
{
	xml_node c;
	xml_node n = doc.child(STR("doc")).child(STR("chapter"));

	CHECK_XPATH_NODESET(c, STR("/child::doc/child::chapter[position()=5]/child::section[position()=2]"));
	CHECK_XPATH_NODESET(n, STR("/child::doc/child::chapter[position()=5]/child::section[position()=2]")) % 9;
	CHECK_XPATH_NODESET(doc, STR("/child::doc/child::chapter[position()=5]/child::section[position()=2]")) % 9;
}

TEST_XML(xpath_paths_w3c_25, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[attribute::type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("child::para[attribute::type=\"warning\"]")) % 4 % 6 % 11 % 13 % 15;
}

TEST_XML(xpath_paths_w3c_26, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[attribute::type=\"warning\"][position()=5]"));
	CHECK_XPATH_NODESET(n, STR("child::para[attribute::type=\"warning\"][position()=5]")) % 15;
}

TEST_XML(xpath_paths_w3c_27a, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='error'/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()=5][attribute::type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()=5][attribute::type=\"warning\"]"));
}

TEST_XML(xpath_paths_w3c_27b, "<node><para/><para type='warning'/><para type='warning'/><para/><para type='warning'/><para type='warning'/><para type='warning'/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::para[position()=5][attribute::type=\"warning\"]"));
	CHECK_XPATH_NODESET(n, STR("child::para[position()=5][attribute::type=\"warning\"]")) % 9;
}

TEST_XML(xpath_paths_w3c_28, "<node><chapter><title>foo</title></chapter><chapter><title>Introduction</title></chapter><chapter><title>introduction</title></chapter><chapter/><chapter><title>Introduction</title><title>foo</title></chapter></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::chapter[child::title='Introduction']"));
	CHECK_XPATH_NODESET(n, STR("child::chapter[child::title='Introduction']")) % 6 % 13;
}

TEST_XML(xpath_paths_w3c_29, "<node><chapter><title>foo</title></chapter><chapter><title>Introduction</title></chapter><chapter><title>introduction</title></chapter><chapter/><chapter><title>Introduction</title><title>foo</title></chapter></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::chapter[child::title]"));
	CHECK_XPATH_NODESET(n, STR("child::chapter[child::title]")) % 3 % 6 % 9 % 13;
}

TEST_XML(xpath_paths_w3c_30, "<node><abstract/><chapter/><chapter/><references/><appendix/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::*[self::chapter or self::appendix]"));
	CHECK_XPATH_NODESET(n, STR("child::*[self::chapter or self::appendix]")) % 4 % 5 % 7;
}

TEST_XML(xpath_paths_w3c_31a, "<node><abstract/><chapter/><chapter/><references/><appendix/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::*[self::chapter or self::appendix][position()=last()]"));
	CHECK_XPATH_NODESET(n, STR("child::*[self::chapter or self::appendix][position()=last()]")) % 7;
}

TEST_XML(xpath_paths_w3c_31b, "<node><abstract/><chapter/><chapter/><references/><appendix/><chapter/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_NODESET(c, STR("child::*[self::chapter or self::appendix][position()=last()]"));
	CHECK_XPATH_NODESET(n, STR("child::*[self::chapter or self::appendix][position()=last()]")) % 8;
}

#endif
