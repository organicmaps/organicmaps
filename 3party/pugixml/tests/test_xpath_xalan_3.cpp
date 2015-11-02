#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_xalan_axes_1, "<far-north><north-north-west1/><north-north-west2/><north><near-north><far-west/><west/><near-west/><center center-attr-1='c1' center-attr-2='c2' center-attr-3='c3'><near-south-west/><near-south><south><far-south/></south></near-south><near-south-east/></center><near-east/><east/><far-east/></near-north></north><north-north-east1/><north-north-east2/></far-north>")
{
	xml_node center = doc.select_node(STR("//center")).node();

	CHECK_XPATH_NODESET(center, STR("self::*[near-south]")) % 10;
	CHECK_XPATH_NODESET(center, STR("self::*[@center-attr-2]")) % 10;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*")) % 9 % 8 % 7;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*/following-sibling::*")) % 8 % 9 % 10 % 19 % 20 % 21;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*[2]/following-sibling::*")) % 9 % 10 % 19 % 20 % 21;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*[2]/following-sibling::*[4]")) % 20;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*[2]/following-sibling::*[4]/preceding-sibling::*[5]/following-sibling::*[4]/following-sibling::*[2]")) % 21;
	CHECK_XPATH_NODESET(center, STR("following-sibling::*")) % 19 % 20 % 21;
	CHECK_XPATH_NODESET(center, STR("following-sibling::*/preceding-sibling::*")) % 7 % 8 % 9 % 10 % 19 % 20;
	CHECK_XPATH_NODESET(center, STR("following-sibling::*[2]/preceding-sibling::*")) % 19 % 10 % 9 % 8 % 7;
	CHECK_XPATH_NODESET(center, STR("following-sibling::*[2]/preceding-sibling::*[4]")) % 8;
	CHECK_XPATH_NODESET(center, STR("following-sibling::*[2]/preceding-sibling::*[4]/following-sibling::*[5]/preceding-sibling::*[4]/preceding-sibling::*[2]")) % 7;
	CHECK_XPATH_NODESET(center, STR("following::*[4]/../*[2]")) % 4;
	CHECK_XPATH_NODESET(center, STR("preceding::*[2]/../following::*")) % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("preceding::*[2]/../descendant::*[10]/following-sibling::east")) % 20;
	CHECK_XPATH_NODESET(center, STR("//*")) % 2 % 3 % 4 % 5 % 6 % 7 % 8 % 9 % 10 % 14 % 15 % 16 % 17 % 18 % 19 % 20 % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("//ancestor::*")) % 2 % 5 % 6 % 10 % 15 % 16;
	CHECK_XPATH_NODESET(center, STR("//*[count(ancestor::*) >= 2]/../parent::*")) % 2 % 5 % 6 % 10 % 15;
	CHECK_XPATH_NODESET(center, STR("//*[count(./*/*) > 0]")) % 2 % 5 % 6 % 10 % 15;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*")) % 2 % 5 % 6 % 10;
	CHECK_XPATH_NODESET(center, STR("@*/following::*")) % 14 % 15 % 16 % 17 % 18 % 19 % 20 % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/preceding::*")) % 3 % 4 % 7 % 8 % 9;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*|following-sibling::*")) % 7 % 8 % 9 % 19 % 20 % 21;
	CHECK_XPATH_NODESET(center, STR("(preceding-sibling::*|following-sibling::*)/ancestor::*[last()]/*[last()]")) % 23;
	CHECK_XPATH_NODESET(center, STR(".//near-south/preceding-sibling::*|following-sibling::east/ancestor-or-self::*[2]")) % 6 % 14;
}

TEST_XML_FLAGS(xpath_xalan_axes_2, "<far-north> Level-1<north-north-west1/><north-north-west2/><!-- Comment-2 --> Level-2<?a-pi pi-2?><north><!-- Comment-3 --> Level-3<?a-pi pi-3?><near-north><far-west/><west/><near-west/><!-- Comment-4 --> Level-4<?a-pi pi-4?><center center-attr-1='c1' center-attr-2='c2' center-attr-3='c3'><near-south-west/><!--Comment-5-->   Level-5<?a-pi pi-5?><near-south><!--Comment-6-->   Level-6<?a-pi pi-6?><south attr1='First' attr2='Last'> <far-south/></south></near-south><near-south-east/></center><near-east/><east/><far-east/></near-north></north><north-north-east1/><north-north-east2/></far-north>", parse_default | parse_comments | parse_pi)
{
	xml_node center = doc.select_node(STR("//center")).node();

	CHECK_XPATH_NODESET(center, STR("@*")) % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/child::*"));
	CHECK_XPATH_NODESET(center, STR("@*/descendant::node()"));
	CHECK_XPATH_NODESET(center, STR("@*/parent::node()")) % 20;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::node()")) % 1 % 2 % 9 % 13 % 20;
	CHECK_XPATH_NODESET(center, STR("@*/self::node()")) % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/.")) % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/descendant-or-self::node()")) % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor-or-self::node()")) % 1 % 2 % 9 % 13 % 20 % 21 % 22 % 23;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor-or-self::*")) % 2 % 9 % 13 % 20;
	CHECK_XPATH_NODESET(center, STR("@*/preceding-sibling::node()"));
	CHECK_XPATH_NODESET(center, STR("@*/following-sibling::*"));
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/preceding::*")) % 4 % 5 % 14 % 15 % 16;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/preceding::comment()")) % 6 % 10 % 17;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/preceding::text()")) % 3 % 7 % 11 % 18;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/preceding::processing-instruction()")) % 8 % 12 % 19;
	CHECK_XPATH_NODESET(center, STR("@*/following::comment()")) % 25 % 29;
	CHECK_XPATH_NODESET(center, STR("@*/following::processing-instruction()")) % 27 % 31;
	CHECK_XPATH_NODESET(center, STR("@*/following::text()")) % 26 % 30;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/preceding::node()")) % 3 % 4 % 5 % 6 % 7 % 8 % 10 % 11 % 12 % 14 % 15 % 16 % 17 % 18 % 19;
	CHECK_XPATH_NODESET(center, STR("@*/ancestor::*/near-north/*[4]/@*/following::node()")) % 24 % 25 % 26 % 27 % 28 % 29 % 30 % 31 % 32 % 35 % 36 % 37 % 38 % 39 % 40 % 41;

	CHECK_XPATH_NODESET(center, STR("(//comment())[1]/..")) % 2;
	CHECK_XPATH_NODESET(center, STR("(//attribute::*)[1]/../..")) % 13;
}

TEST_XML(xpath_xalan_axes_3, "<far-north><north><near-north><far-west/><west/><near-west/><center><near-south><south><far-south/></south></near-south></center><near-east/><east/><far-east/></near-north></north></far-north>")
{
	xml_node center = doc.select_node(STR("//center")).node();

	CHECK_XPATH_NODESET(center, STR("ancestor-or-self::*")) % 8 % 4 % 3 % 2;
	CHECK_XPATH_NODESET(center, STR("ancestor::*[3]")) % 2;
	CHECK_XPATH_NODESET(center, STR("ancestor-or-self::*[1]")) % 8;
	CHECK_XPATH_NODESET(center, STR("@*[2]"));
	CHECK_XPATH_NODESET(center, STR("child::*[2]"));
	CHECK_XPATH_NODESET(center, STR("child::near-south-west"));
	CHECK_XPATH_NODESET(center, STR("descendant::*[3]")) % 11;
	CHECK_XPATH_NODESET(center, STR("descendant::far-south")) % 11;
	CHECK_XPATH_NODESET(center, STR("descendant-or-self::*[3]")) % 10;
	CHECK_XPATH_NODESET(center, STR("descendant-or-self::far-south")) % 11;
	CHECK_XPATH_NODESET(center, STR("descendant-or-self::center")) % 8;
	CHECK_XPATH_NODESET(center, STR("following::*[4]"));
	CHECK_XPATH_NODESET(center, STR("following::out-yonder-east"));
	CHECK_XPATH_NODESET(center, STR("preceding::*[4]"));
	CHECK_XPATH_NODESET(center, STR("preceding::out-yonder-west"));
	CHECK_XPATH_NODESET(center, STR("following-sibling::*[2]")) % 13;
	CHECK_XPATH_NODESET(center, STR("following-sibling::east")) % 13;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::*[2]")) % 6;
	CHECK_XPATH_NODESET(center, STR("preceding-sibling::west")) % 6;
	CHECK_XPATH_NODESET(center, STR("parent::near-north")) % 4;
	CHECK_XPATH_NODESET(center, STR("parent::*[1]")) % 4;
	CHECK_XPATH_NODESET(center, STR("parent::foo"));
	CHECK_XPATH_NODESET(center, STR("..")) % 4;
	CHECK_XPATH_NODESET(center, STR("self::center")) % 8;
	CHECK_XPATH_NODESET(center, STR("self::*[1]")) % 8;
	CHECK_XPATH_NODESET(center, STR("self::foo"));
	CHECK_XPATH_NODESET(center, STR(".")) % 8;
	CHECK_XPATH_NODESET(center, STR("/far-north/north/near-north/center/ancestor-or-self::*")) % 8 % 4 % 3 % 2;
}

TEST_XML(xpath_xalan_axes_4, "<far-north><north><near-north><far-west/><west/><near-west/><center><near-south><south><far-south/></south></near-south></center><near-east/><east/><far-east/></near-north></north></far-north>")
{
	xml_node north = doc.select_node(STR("//north")).node();

	CHECK_XPATH_STRING(north, STR("name(/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(/descendant::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::near-north/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::near-north/descendant::far-west)"), STR("far-west"));

	CHECK_XPATH_STRING(north, STR("name(/descendant-or-self::north/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(/descendant-or-self::north/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(/descendant::near-north/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant-or-self::north/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant-or-self::north/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant::near-north/descendant-or-self::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant::near-north/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant-or-self::north/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant-or-self::north/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant::near-north/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/descendant::near-north/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/child::near-north/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::north/child::near-north/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant::near-north/descendant-or-self::near-north/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::near-north/descendant-or-self::near-north/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant::near-north/descendant::far-west/descendant-or-self::far-west)"), STR("far-west"));

	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::node()/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::node()/descendant::far-west)"), STR("far-west"));

	CHECK_XPATH_STRING(north, STR("name(/descendant-or-self::node()/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(/descendant-or-self::node()/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(/descendant::node()/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant-or-self::node()/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant-or-self::node()/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant::node()/descendant-or-self::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(self::node()/descendant::node()/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant-or-self::node()/descendant-or-self::north)"), STR("north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant-or-self::node()/child::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant::node()/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/descendant::node()/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/child::node()/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant-or-self::node()/child::node()/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant::node()/descendant-or-self::node()/descendant-or-self::near-north)"), STR("near-north"));
	CHECK_XPATH_STRING(north, STR("name(descendant::node()/descendant-or-self::node()/child::far-west)"), STR("far-west"));
	CHECK_XPATH_STRING(north, STR("name(descendant::node()/descendant::node()/descendant-or-self::far-west)"), STR("far-west"));
}

TEST_XML_FLAGS(xpath_xalan_axes_5, "<text>text</text><comment><!--comment--></comment><pi><?pi?></pi>", parse_default | parse_comments | parse_pi)
{
	CHECK_XPATH_NODESET(doc, STR("text/self::text()"));
	CHECK_XPATH_NODESET(doc, STR("comment/self::comment()"));
	CHECK_XPATH_NODESET(doc, STR("pi/self::processing-instruction()"));
}

TEST_XML(xpath_xalan_axes_6, "<doc><T>Test for source tree depth</T><a><T>A</T><b><T>B</T><c><T>C</T><d><T>D</T><e><T>E</T><f><T>F</T><g><T>G</T><h><T>H</T><i><T>I</T><j><T>J</T><k><T>K</T><l><T>L</T><m><T>M</T><n><T>N</T><o><T>O</T></o></n></m></l></k></j></i></h></g></f></e></d></c></b></a></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("//T")) % 3 % 6 % 9 % 12 % 15 % 18 % 21 % 24 % 27 % 30 % 33 % 36 % 39 % 42 % 45 % 48;
}

TEST_XML(xpath_xalan_axes_7, "<far-north><north><near-north><far-west/><west/><near-west/><center center-attr-1='c1' center-attr-2='c2' center-attr-3='c3'><near-south><south><far-south/></south></near-south></center><near-east/><east/><far-east/></near-north></north></far-north>")
{
	xml_node center = doc.select_node(STR("//center")).node();

	CHECK_XPATH_NODESET(center, STR("attribute::*[2]")) % 10;
	CHECK_XPATH_NODESET(center, STR("@*")) % 9 % 10 % 11;
	CHECK_XPATH_NODESET(center, STR("child::*/child::*")) % 13;
	CHECK_XPATH_NODESET(center, STR("child::*/descendant::*")) % 13 % 14;
	CHECK_XPATH_NODESET(center, STR("descendant::*/child::*")) % 13 % 14;
}

TEST_XML(xpath_xalan_axes_8, "<far-north><north><near-north><far-west/><west/><near-west/><center center-attr-1='c1' center-attr-2='c2' center-attr-3='c3'><near-south-east/><near-south><south><far-south/></south></near-south><near-south-west/></center><near-east/><east/><far-east/></near-north></north></far-north>")
{
	xml_node near_north = doc.select_node(STR("//near-north")).node();

	CHECK_XPATH_NODESET(near_north, STR("center//child::*")) % 12 % 13 % 14 % 15 % 16;
	CHECK_XPATH_NODESET(near_north, STR("center//descendant::*")) % 12 % 13 % 14 % 15 % 16;
	CHECK_XPATH_NODESET(near_north, STR("center/descendant::*")) % 12 % 13 % 14 % 15 % 16;
	CHECK_XPATH_NODESET(near_north, STR("center/child::*")) % 12 % 13 % 16;
	CHECK_XPATH_NODESET(near_north, STR("center//*")) % 12 % 13 % 14 % 15 % 16;
}

TEST_XML(xpath_xalan_axes_9, "<doc><foo att1='c'><foo att1='b'><foo att1='a'><baz/></foo></foo></foo><bar/></doc>")
{
	xml_node baz = doc.select_node(STR("//baz")).node();

	CHECK_XPATH_NODESET(baz, STR("ancestor-or-self::*[@att1][1]/@att1")) % 8;
    CHECK_XPATH_NODESET(baz, STR("(ancestor-or-self::*)[@att1][1]/@att1")) % 4;

	CHECK_XPATH_NODESET(baz, STR("ancestor::foo[1]/@att1")) % 8;
	CHECK_XPATH_NODESET(baz, STR("(ancestor::foo[1])/@att1")) % 8;
	CHECK_XPATH_NODESET(baz, STR("(ancestor::foo)[1]/@att1")) % 4;
	CHECK_XPATH_NODESET(baz, STR("((ancestor::foo))[1]/@att1")) % 4;
	CHECK_XPATH_NODESET(baz, STR("(((ancestor::foo)[1])/@att1)")) % 4;

	xml_node bar = doc.child(STR("doc")).child(STR("bar"));

    CHECK_XPATH_NODESET(bar, STR("preceding::foo[1]/@att1")) % 8;
	CHECK_XPATH_NODESET(bar, STR("(preceding::foo)[1]/@att1")) % 4;
}

TEST_XML(xpath_xalan_axes_10, "<doc><foo att1='c'/><foo att1='b'/><foo att1='a'/><baz/></doc>")
{
	xml_node baz = doc.child(STR("doc")).child(STR("baz"));

    CHECK_XPATH_NODESET(baz, STR("preceding-sibling::foo[1]/@att1")) % 8;
	CHECK_XPATH_NODESET(baz, STR("(preceding-sibling::foo)[1]/@att1")) % 4;
}

TEST_XML(xpath_xalan_axes_11, "<chapter title='A' x='0'><section title='A1' x='1'><subsection title='A1a' x='2'>hello</subsection><subsection title='A1b'>ahoy</subsection></section><section title='A2'><subsection title='A2a'>goodbye</subsection><subsection title='A2b'>sayonara</subsection><subsection title='A2c'>adios</subsection></section><section title='A3'><subsection title='A3a'>aloha</subsection><subsection title='A3b'><footnote x='3'>A3b-1</footnote><footnote>A3b-2</footnote></subsection><subsection title='A3c'>shalom</subsection></section></chapter>")
{
	xml_node chapter = doc.child(STR("chapter"));

    CHECK_XPATH_NUMBER(doc, STR("count(//@*)"), 16);
    CHECK_XPATH_NUMBER(doc, STR("count(//@title)"), 12);
    CHECK_XPATH_NUMBER(doc, STR("count(//section//@*)"), 14);
    CHECK_XPATH_NUMBER(doc, STR("count(//section//@title)"), 11);

	CHECK_XPATH_NUMBER(chapter, STR("count(.//@*)"), 16);
	CHECK_XPATH_NUMBER(chapter, STR("count(.//@title)"), 12);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[1]//@*)"), 5);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[1]//@title)"), 3);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[2]//@*)"), 4);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[2]//@title)"), 4);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[3]//@*)"), 5);
	CHECK_XPATH_NUMBER(chapter, STR("count(section[3]//@title)"), 4);
}

TEST_XML_FLAGS(xpath_xalan_axes_12, "<far-north><north>north-text1<near-north><far-west/><west><!-- Western comment --></west><near-west/><center>center-text1<near-south><south>south-text</south></near-south><near-south-west/>center-text2</center><near-east/><east><!-- Eastern comment --></east><far-east/></near-north>north-text2</north></far-north>", parse_default | parse_comments)
{
	CHECK_XPATH_NODESET(doc, STR("/descendant::*")) % 2 % 3 % 5 % 6 % 7 % 9 % 10 % 12 % 13 % 15 % 17 % 18 % 20;
	CHECK_XPATH_NODESET(doc, STR("far-north/..//*")) % 2 % 3 % 5 % 6 % 7 % 9 % 10 % 12 % 13 % 15 % 17 % 18 % 20;
	CHECK_XPATH_NODESET(doc, STR("far-north/north/..//*")) % 3 % 5 % 6 % 7 % 9 % 10 % 12 % 13 % 15 % 17 % 18 % 20;
	CHECK_XPATH_NODESET(doc, STR("far-north/north-yonder/..//*"));
}

TEST_XML(xpath_xalan_axes_13, "<doc att1='e'><foo att1='d'><foo att1='c'><foo att1='b'><baz att1='a'/></foo></foo></foo></doc>")
{
	xml_node d = doc.child(STR("doc"));
	xml_node baz = doc.select_node(STR("//baz")).node();

	CHECK_XPATH_NUMBER(d, STR("count(descendant-or-self::*/@att1)"), 5);
	CHECK_XPATH_NODESET(d, STR("descendant-or-self::*/@att1[last()]")) % 3 % 5 % 7 % 9 % 11;
	CHECK_XPATH_STRING(d, STR("string(descendant-or-self::*/@att1[last()])"), STR("e"));
	CHECK_XPATH_NODESET(d, STR("descendant-or-self::*[last()]/@att1")) % 11;
	CHECK_XPATH_NODESET(d, STR("(descendant-or-self::*/@att1)[last()]")) % 11;

	CHECK_XPATH_NUMBER(baz, STR("count(ancestor-or-self::*/@att1)"), 5);
    CHECK_XPATH_NODESET(baz, STR("ancestor-or-self::*/@att1[last()]")) % 3 % 5 % 7 % 9 % 11;
    CHECK_XPATH_STRING(baz, STR("string(ancestor-or-self::*/@att1[last()])"), STR("e"));
    CHECK_XPATH_NODESET(baz, STR("(ancestor-or-self::*)/@att1[last()]")) % 3 % 5 % 7 % 9 % 11;
    CHECK_XPATH_STRING(baz, STR("string((ancestor-or-self::*)/@att1[last()])"), STR("e"));
    CHECK_XPATH_NODESET(baz, STR("(ancestor-or-self::*/@att1)[last()]")) % 11;
    CHECK_XPATH_NODESET(baz, STR("(ancestor::*|self::*)/@att1[last()]")) % 3 % 5 % 7 % 9 % 11;
    CHECK_XPATH_STRING(baz, STR("string((ancestor::*|self::*)/@att1[last()])"), STR("e"));
	CHECK_XPATH_NODESET(baz, STR("((ancestor::*|self::*)/@att1)[last()]")) % 11;
}

TEST_XML_FLAGS(xpath_xalan_axes_14, "<doc><n a='v'/><?pi?><!--comment-->text<center/>text<!--comment--><?pi?><n a='v'/></doc>", parse_default | parse_comments | parse_pi)
{
	CHECK_XPATH_NODESET(doc, STR("//center/preceding::node()")) % 7 % 6 % 5 % 3;
	CHECK_XPATH_NODESET(doc, STR("//center/following::node()")) % 9 % 10 % 11 % 12;
}

TEST_XML(xpath_xalan_axes_15, "<doc><foo new='true'><baz>is new</baz></foo><foo new='true'>xyz<baz>is new but has text</baz></foo><foo new='false'><baz>is not new</baz></foo></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("//text()[ancestor::*[@new='true'][not(text())]]")) % 6;
	CHECK_XPATH_NODESET(doc, STR("//text()[ancestor::*[2][@new]]")) % 6 % 11 % 15;

	xml_node foo = doc.child(STR("doc")).child(STR("foo")).child(STR("baz")).first_child();

	CHECK_XPATH_STRING(foo, STR("name(ancestor::*[3])"), STR("doc"));
	CHECK_XPATH_STRING(foo, STR("name(ancestor::*[2])"), STR("foo"));
	CHECK_XPATH_STRING(foo, STR("name(ancestor::*[1])"), STR("baz"));
}

TEST_XML(xpath_xalan_axes_16, "<doc><child><grandchild><greatgrandchild/></grandchild><grandchild><greatgrandchild><greatgreatgreatgrandchild/></greatgrandchild><greatgrandchild/></grandchild></child><child><grandchild><greatgrandchild/><greatgrandchild/><greatgrandchild/></grandchild></child><child><grandchild><greatgrandchild/></grandchild></child><child><grandchild/></child><child><grandchild/><grandchild/></child><child/></doc>")
{
	xml_node c1 = doc.child(STR("doc")).child(STR("child")), c2 = c1.next_sibling(), c3 = c2.next_sibling(), c4 = c3.next_sibling(), c5 = c4.next_sibling(), c6 = c5.next_sibling();

	CHECK_XPATH_STRING(c1.first_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("1,2"));
	CHECK_XPATH_STRING(c1.last_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("3,4"));

	CHECK_XPATH_STRING(c2.first_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("3,4"));

	CHECK_XPATH_STRING(c3.first_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("1,2"));

	CHECK_XPATH_STRING(c4.first_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("0,1"));

	CHECK_XPATH_STRING(c5.first_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("0,1"));
	CHECK_XPATH_STRING(c5.last_child(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("0,1"));

	CHECK_XPATH_STRING(c6, STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("0,1"));

	CHECK_XPATH_STRING(xml_node(), STR("concat(count(descendant::*), ',', count(descendant-or-self::*))"), STR("0,0"));
}

TEST_XML(xpath_xalan_axes_17, "<doc><a><asub><asubsub><yy/></asubsub></asub></a><b><bsub><xx><xxchild/></xx></bsub></b><xx>here</xx><d><dsub><dsubsub><xx/></dsubsub></dsub></d><e><esub><xx><xxchild/></xx></esub><esubsib><sibchild/></esubsib></e><xx><childofxx/></xx><xx><xxsub><xxsubsub/></xxsub></xx></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("//xx/descendant::*")) % 10 % 20 % 24 % 26 % 27;
}

TEST_XML(xpath_xalan_axes_18, "<north><center center-attr='here'><south/></center></north>")
{
	xml_node center = doc.child(STR("north")).child(STR("center"));

	CHECK_XPATH_NODESET(center, STR("@*/self::node()")) % 4;
	CHECK_XPATH_NODESET(center, STR("@*/self::*")); // * tests for principal node type
	CHECK_XPATH_NODESET(center, STR("@*/self::text()"));
	CHECK_XPATH_NODESET(center, STR("@*/self::center-attr")); // * tests for principal node type
}

#endif
