#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_xalan_position_1, "<doc><a>1</a><a>2</a><a>3</a><a>4</a></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_BOOLEAN(c, STR("position()=1"), true);
	CHECK_XPATH_NODESET(c, STR("*[position()=4]")) % 9;
}

TEST_XML_FLAGS(xpath_xalan_position_2, "<doc><a test='true'><num>1</num></a><a><num>1191</num></a><a><num>263</num></a><a test='true'><num>2</num></a><a><num>827</num></a><a><num>256</num></a><a test='true'><num>3</num></a><a test='true'><num>4<x/>5</num></a><?pi?><?pi?><!--comment--><!--comment--></doc>", parse_default | parse_comments | parse_pi)
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("*[@test and position()=8]")) % 27;
	CHECK_XPATH_NODESET(c, STR("*[@test][position()=4]/num")) % 29;
	CHECK_XPATH_NUMBER(c, STR("count(*)"), 8);
	CHECK_XPATH_NODESET(c, STR("*[last()=position()]")) % 27;
	CHECK_XPATH_NODESET(c, STR("a[position()=2]")) % 7;
	CHECK_XPATH_NODESET(c, STR("a[3]/../a[position()=4]/num/../@test")) % 14;
	CHECK_XPATH_BOOLEAN(c, STR("not(position()=last())"), false);
	CHECK_XPATH_BOOLEAN(c, STR("position()=2"), false);
	CHECK_XPATH_BOOLEAN(c, STR("last()=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("last()+2=3"), true);
	CHECK_XPATH_NODESET(c, STR("a[position()=5 mod 3]")) % 7;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[position()=1]")) % 6 % 9 % 12 % 16 % 19 % 22 % 26 % 30;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[position()=2]")) % 32;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[position()=last()]")) % 6 % 9 % 12 % 16 % 19 % 22 % 26 % 32;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[1]")) % 6 % 9 % 12 % 16 % 19 % 22 % 26 % 30;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[2]")) % 32;
	CHECK_XPATH_NODESET(c, STR("a/num/text()[last()]")) % 6 % 9 % 12 % 16 % 19 % 22 % 26 % 32;
	CHECK_XPATH_NODESET(c, STR("a[floor(last() div 3)]")) % 7;
	CHECK_XPATH_NODESET(c, STR("a[ceiling(last() div 3)]")) % 10;
	CHECK_XPATH_NODESET(c, STR("a[round(last() div 3)]")) % 10;
	CHECK_XPATH_NODESET(c, STR("a[last() div 3]"));
	CHECK_XPATH_NODESET(c, STR("a[last() div 2]")) % 13;
	CHECK_XPATH_NODESET(c, STR("a[3]/../a[position()>=2 and position()<=4]")) % 7 % 10 % 13;
	CHECK_XPATH_NUMBER(c, STR("count(a[position()>=2 and position()<=4]/num)"), 3);
	CHECK_XPATH_NUMBER(c, STR("count(a/@*)"), 4);
	CHECK_XPATH_NUMBER(c, STR("count(a/attribute::*)"), 4);
	CHECK_XPATH_NODESET(c, STR("*[not(@test)][position()=last()]")) % 20;
	CHECK_XPATH_NODESET(c, STR("*[not(@test)][last()]")) % 20;
	CHECK_XPATH_NODESET(c, STR("a[3-2]")) % 3;
	CHECK_XPATH_NODESET(c, STR("a[0]"));
	CHECK_XPATH_NODESET(c, STR("a[9]"));
	CHECK_XPATH_NODESET(c, STR("a['3']")) % 3 % 7 % 10 % 13 % 17 % 20 % 23 % 27;
	CHECK_XPATH_NODESET(c, STR("a[number('3')]")) % 10;
	CHECK_XPATH_NODESET(c, STR("processing-instruction()[2]")) % 34;
	CHECK_XPATH_NODESET(c, STR("processing-instruction('pi')[2]")) % 34;
	CHECK_XPATH_NODESET(c, STR("comment()[2]")) % 36;
    CHECK_XPATH_NODESET(c, STR("a/*[last()]")) % 5 % 8 % 11 % 15 % 18 % 21 % 25 % 29;
    CHECK_XPATH_NODESET(c, STR("a/child::*[last()]")) % 5 % 8 % 11 % 15 % 18 % 21 % 25 % 29;
    CHECK_XPATH_NODESET(c, STR("a/descendant::*[last()]")) % 5 % 8 % 11 % 15 % 18 % 21 % 25 % 31;
    CHECK_XPATH_NODESET(c, STR("a/child::node()[last()]")) % 5 % 8 % 11 % 15 % 18 % 21 % 25 % 29;
    CHECK_XPATH_NODESET(c, STR("a/descendant::text()[last()]")) % 6 % 9 % 12 % 16 % 19 % 22 % 26 % 32;
    CHECK_XPATH_NODESET(c, STR("child::comment()[last()]")) % 36;
}

TEST_XML(xpath_xalan_position_3, "<article class='whitepaper' status='Note'><articleinfo><title>AAA</title><section id='info'><title>BBB</title><para>About this article</para><section revisionflag='added'><title>CCC</title><para>This is the section titled 'ZZZ'.</para><ednote who='KKK'><title>DDD</title><para>Don't worry.</para><section revisionflag='added'><title>EEE</title><para>This is the deep subsection.</para></section></ednote></section></section></articleinfo></article>")
{
	CHECK_XPATH_NODESET(doc, STR("(article//section/title|/articleinfo/title|article/section/para)[last()]")) % 28;
	CHECK_XPATH_NODESET(doc, STR("(article//section/title|/articleinfo/title|article/section/para)[1]")) % 10;
	CHECK_XPATH_NUMBER(doc, STR("count(article/articleinfo/section[last()])"), 1);
	CHECK_XPATH_NUMBER(doc, STR("count(article/articleinfo/section[last()][title='BBB'])"), 1);
}

TEST_XML(xpath_xalan_position_4, "<chapter><section><footnote>hello</footnote></section><section><footnote>goodbye</footnote><footnote>sayonara</footnote></section><section><footnote>aloha</footnote></section></chapter>")
{
	CHECK_XPATH_NODESET(doc, STR("chapter//footnote[1]")) % 4 % 7 % 12;
}

TEST_XML(xpath_xalan_position_5, "<chapter><section><footnote>hello</footnote><footnote>ahoy</footnote></section><section><footnote>goodbye</footnote><footnote>sayonara</footnote><footnote>adios</footnote></section><section><footnote>aloha</footnote><subsection><footnote>shalom</footnote><footnote>yo</footnote></subsection><footnote>ciao</footnote></section></chapter>")
{
	CHECK_XPATH_NODESET(doc, STR("chapter//footnote[2]")) % 6 % 11 % 21 % 23;
	CHECK_XPATH_NODESET(doc, STR("(chapter//footnote)[2]")) % 6;
	CHECK_XPATH_NODESET(doc, STR("(child::chapter/descendant-or-self::node())/footnote[2]")) % 6 % 11 % 21 % 23;
	CHECK_XPATH_NODESET(doc, STR("chapter/descendant::footnote[6]")) % 16;
	CHECK_XPATH_NODESET(doc, STR("chapter/descendant::footnote[6][1][last()]")) % 16;
}

TEST_XML_FLAGS(xpath_xalan_position_6, "<node attr='value'>pcdata<child/><?pi1 value?><?pi2 value?><!--comment--><![CDATA[cdata]]></node>", parse_default | parse_pi | parse_comments)
{
	CHECK_XPATH_NUMBER(doc, STR("count(/node/@attr/ancestor-or-self::node())"), 3);
	CHECK_XPATH_NUMBER(doc, STR("count(/node/text()/ancestor-or-self::node())"), 4);
	CHECK_XPATH_NUMBER(doc, STR("count(/node/processing-instruction()/ancestor-or-self::node())"), 4);
	CHECK_XPATH_NUMBER(doc, STR("count(/node/processing-instruction('pi1')/ancestor-or-self::node())"), 3);
	CHECK_XPATH_NUMBER(doc, STR("count(/node/comment()/ancestor-or-self::node())"), 3);
}

TEST_XML(xpath_xalan_position_7, "<chapter title='A'><section title='A1'><subsection title='A1a'>hello</subsection><subsection title='A1b'>ahoy</subsection></section><section title='A2'><subsection title='A2a'>goodbye</subsection><subsection title='A2b'>sayonara</subsection><subsection title='A2c'>adios</subsection></section><section title='A3'><subsection title='A3a'>aloha</subsection><subsection title='A3b'><footnote>A3b-1</footnote><footnote>A3b-2</footnote></subsection><subsection title='A3c'>shalom</subsection></section></chapter>")
{
	CHECK_XPATH_NODESET(doc, STR("chapter/section//@title[7]"));
	CHECK_XPATH_NODESET(doc, STR("(chapter/section//@title)[7]")) % 21;
}

TEST_XML(xpath_xalan_match_1, "<root><x spot='a' num='1'/><x spot='b' num='2'/><x spot='c' num='3'/><x spot='d' num='4'/><x spot='e' num='5'/><x spot='f' num='6'/><x spot='g' num='7'/><x spot='h' num='8'/><x spot='i' num='9'/><x spot='j' num='10'/><x spot='k' num='11'/><x spot='l' num='12'/></root>")
{
	xml_node c = doc.child(STR("root"));

	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2)=1][position() > 3]")) % 21 % 27 % 33;
	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2)=1][position() > 3][position()=2]")) % 27;
	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2) > 0][position() > 3][2]")) % 27;
	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2)=1][position() > 3][last()]")) % 33;
	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2)=1][@num > 5][last()]")) % 33;
	CHECK_XPATH_NODESET(c, STR("x[(@num mod 3)=2][position() > 2][last()]")) % 33;
	CHECK_XPATH_NODESET(c, STR("x[(position() mod 2)=1][2][@num < 10]")) % 9;
	CHECK_XPATH_NODESET(c, STR("x[(((((2*10)-4)+9) div 5) mod 3)]")) % 6;
}


TEST_XML(xpath_xalan_match_2, "<doc><l1><v2>doc-l1-v2</v2><x2>doc-l1-x2</x2><l2><v3>doc-l1-l2-v3</v3><w3>doc-l1-l2-w3</w3><x3>doc-l1-l2-x3</x3><y3>doc-l1-l2-y3</y3><l3><v4>doc-l1-l2-l3-v4</v4><x4>doc-l1-l2-l3-x4</x4></l3></l2></l1></doc>")
{
	CHECK_XPATH_STRING(doc, STR("doc/l1/v2"), STR("doc-l1-v2"));
	CHECK_XPATH_STRING(doc, STR("doc/child::l1/x2"), STR("doc-l1-x2"));
	CHECK_XPATH_STRING(doc, STR("doc/l1//v3"), STR("doc-l1-l2-v3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2/w3"), STR("doc-l1-l2-w3"));
	CHECK_XPATH_STRING(doc, STR("doc/child::l1//x3"), STR("doc-l1-l2-x3"));
	CHECK_XPATH_STRING(doc, STR("doc//child::l2/y3"), STR("doc-l1-l2-y3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2//v4"), STR("doc-l1-l2-l3-v4"));
	CHECK_XPATH_STRING(doc, STR("doc//child::l2//x4"), STR("doc-l1-l2-l3-x4"));

	CHECK_XPATH_STRING(doc, STR("doc/l1/v2"), STR("doc-l1-v2"));
	CHECK_XPATH_STRING(doc, STR("doc/l1/child::x2"), STR("doc-l1-x2"));
	CHECK_XPATH_STRING(doc, STR("doc/l1//v3"), STR("doc-l1-l2-v3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2/w3"), STR("doc-l1-l2-w3"));
	CHECK_XPATH_STRING(doc, STR("doc/l1//child::x3"), STR("doc-l1-l2-x3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2/child::y3"), STR("doc-l1-l2-y3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2//v4"), STR("doc-l1-l2-l3-v4"));
	CHECK_XPATH_STRING(doc, STR("doc//l2//child::x4"), STR("doc-l1-l2-l3-x4"));

	CHECK_XPATH_STRING(doc, STR("doc/l1/v2"), STR("doc-l1-v2"));
	CHECK_XPATH_STRING(doc, STR("doc/child::l1/child::x2"), STR("doc-l1-x2"));
	CHECK_XPATH_STRING(doc, STR("doc/l1//v3"), STR("doc-l1-l2-v3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2/w3"), STR("doc-l1-l2-w3"));
	CHECK_XPATH_STRING(doc, STR("doc/child::l1//child::x3"), STR("doc-l1-l2-x3"));
	CHECK_XPATH_STRING(doc, STR("doc//child::l2/child::y3"), STR("doc-l1-l2-y3"));
	CHECK_XPATH_STRING(doc, STR("doc//l2//v4"), STR("doc-l1-l2-l3-v4"));
	CHECK_XPATH_STRING(doc, STR("doc//child::l2//child::x4"), STR("doc-l1-l2-l3-x4"));
}

TEST_XML(xpath_xalan_match_3, "<doc><child><child-foo><name id='1'>John Doe</name><child><name id='2'>Jane Doe</name></child></child-foo></child></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/child/*[starts-with(name(),'child-')]//name")) % 5 % 9;
	CHECK_XPATH_NODESET(doc, STR("//@*")) % 6 % 10;
}

TEST_XML(xpath_xalan_expression_1, "<doc><para id='1' xml:lang='en'>en</para><div xml:lang='en'><para>en</para></div><para id='3' xml:lang='EN'>EN</para><para id='4' xml:lang='en-us'>en-us</para></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/para[@id='1' and lang('en')]")) % 3;
	CHECK_XPATH_NODESET(doc, STR("doc/para[@id='4' and lang('en')]")) % 15;
	CHECK_XPATH_NODESET(doc, STR("doc/div/para[lang('en')]")) % 9;
	CHECK_XPATH_NODESET(doc, STR("doc/para[@id='3' and lang('en')]")) % 11;
	CHECK_XPATH_NODESET(doc, STR("//para[lang('en')]/ancestor-or-self::*[@xml:lang]/@xml:lang")) % 5 % 8 % 13 % 17;
}

TEST_XML(xpath_xalan_predicate_1, "<doc><a>1</a><a>2</a><a>3</a><a>4</a></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("a[true()=4]")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[true()='stringwithchars']")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[true()=following-sibling::*]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[true()=preceding-sibling::*]")) % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[3=following-sibling::*]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[0 < true()]")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a['3.5' < 4]")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[3 < following-sibling::*]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*>3]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[3 > following-sibling::*]")) % 3;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*<3]")) % 3;
	CHECK_XPATH_NODESET(c, STR("a[1 < 2 < 3]")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[1 < 3 < 2]")) % 3 % 5 % 7 % 9;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*=true()]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[false()!=following-sibling::*]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*!=false()]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*=3]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[3=following-sibling::*]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[4!=following-sibling::*]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*!=4]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[3>=following-sibling::*]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[3<=following-sibling::*]")) % 3 % 5 % 7;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*<=3]")) % 3 % 5;
	CHECK_XPATH_NODESET(c, STR("a[following-sibling::*>=3]")) % 3 % 5 % 7;
}

TEST_XML(xpath_xalan_predicate_2, "<foo><bar a='0' b='0' c='0' d='0' seq='0'/><bar a='0' b='0' c='0' d='1' seq='1'/><bar a='0' b='0' c='1' d='0' seq='2'/><bar a='0' b='0' c='1' d='1' seq='3'/><bar a='0' b='1' c='0' d='0' seq='4'/><bar a='0' b='1' c='0' d='1' seq='5'/><bar a='0' b='1' c='1' d='0' seq='6'/><bar a='0' b='1' c='1' d='1' seq='7'/><bar a='1' b='0' c='0' d='0' seq='8'/><bar a='1' b='0' c='0' d='1' seq='9'/><bar a='1' b='0' c='1' d='0' seq='a'/><bar a='1' b='0' c='1' d='1' seq='b'/><bar a='1' b='1' c='0' d='0' seq='c'/><bar a='1' b='1' c='0' d='1' seq='d'/><bar a='1' b='1' c='1' d='0' seq='e'/><bar a='1' b='1' c='1' d='1' seq='f'/></foo>")
{
	xml_node c = doc.child(STR("foo"));

	CHECK_XPATH_NODESET(c, STR("bar[@a='1' and @b='1']")) % 75 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[(@a='1' or @b='1') and @c='1']")) % 39 % 45 % 63 % 69 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[@a='1' and (@b='1' or @c='1') and @d='1']")) % 69 % 81 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[@a='1' and @b='1' or @c='1' and @d='1']")) % 21 % 45 % 69 % 75 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[(@a='1' and @b='1') or (@c='1' and @d='1')]")) % 21 % 45 % 69 % 75 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[@a='1' or (@b='1' and @c='1') or @d='1']")) % 9 % 21 % 33 % 39 % 45 % 51 % 57 % 63 % 69 % 75 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[(@a='1' or @b='1') and (@c='1' or @d='1')]")) % 33 % 39 % 45 % 57 % 63 % 69 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[@a='1' or @b='1' and @c='1' or @d='1']")) % 9 % 21 % 33 % 39 % 45 % 51 % 57 % 63 % 69 % 75 % 81 % 87 % 93;
	CHECK_XPATH_NODESET(c, STR("bar[@a='1' or @b='1' or @c='1']")) % 15 % 21 % 27 % 33 % 39 % 45 % 51 % 57 % 63 % 69 % 75 % 81 % 87 % 93;
}

TEST_XML(xpath_xalan_predicate_3, "<doc><a>1</a><a ex=''>2</a><a ex='value'>3</a><a why=''>4</a><a why='value'>5</a></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NUMBER(c, STR("count(a[@ex])"), 2);
	CHECK_XPATH_NUMBER(c, STR("count(a[@ex=''])"), 1);
	CHECK_XPATH_NUMBER(c, STR("count(a[string-length(@ex)=0])"), 4);
	CHECK_XPATH_NUMBER(c, STR("count(a[@ex!=''])"), 1);
	CHECK_XPATH_NUMBER(c, STR("count(a[string-length(@ex) > 0])"), 1);
	CHECK_XPATH_NUMBER(c, STR("count(a[not(@ex)])"), 3);
	CHECK_XPATH_NUMBER(c, STR("count(a[not(@ex='')])"), 4);
	CHECK_XPATH_NUMBER(c, STR("count(a[not(string-length(@ex)=0)])"), 1);
	CHECK_XPATH_NUMBER(c, STR("count(a[@why='value'])"), 1);
	CHECK_XPATH_NUMBER(c, STR("count(a[@why!='value'])"), 1);
}

TEST_XML(xpath_xalan_predicate_4, "<table><tr><td>1.1</td><td>1.2</td></tr><tr><td>2.1</td><td>2.2</td><td>2.3</td></tr><tr><td>3.1</td><td>3.2<td>3.2.1</td></td></tr><tr><td>4<td>4.1<td>4.1.1</td></td></td></tr><tr><td>5.1</td><td>5.2</td><td>5.3</td><td>5.4</td></tr><tr><ta/><td>6.1</td><td>6.2</td></tr><tr><ta/><td>7.1</td><td>7.2</td><td>7.3</td></tr><tr><ta/><td>8.1</td><td>8.2</td><td>8.3</td><td>8.4</td></tr></table>")
{
	CHECK_XPATH_NUMBER(doc, STR("count(//tr)"), 8);
	CHECK_XPATH_NUMBER(doc, STR("count(//tr[count(./td)=3])"), 2);
}

TEST_XML(xpath_xalan_predicate_5, "<doc><element1>Wrong node selected!!</element1><element1>Test executed successfully</element1><element1>Wrong node selected!!</element1></doc>")
{
	CHECK_XPATH_STRING(doc, STR("doc/element1[(((((2*10)-4)+9) div 5) mod 3 )]"), STR("Test executed successfully"));
	CHECK_XPATH_STRING(doc, STR("doc/element1[(((((2*10)-4)+9) div 5) mod floor(3))]"), STR("Test executed successfully"));
	CHECK_XPATH_STRING(doc, STR("doc/element1[floor(2)]"), STR("Test executed successfully"));
}

TEST_XML(xpath_xalan_predicate_6, "<doc><a>1</a><a>2<achild>target</achild></a><a>3</a><a>4</a></doc>")
{
	CHECK_XPATH_STRING(doc, STR("doc/a['target'=descendant::*]"), STR("2target"));
	CHECK_XPATH_STRING(doc, STR("doc/a[descendant::*='target']"), STR("2target"));
}

TEST_XML(xpath_xalan_predicate_7, "<doc><a>1</a><a>2<achild>target</achild></a><a>3</a><a>4<achild>missed</achild></a></doc>")
{
	CHECK_XPATH_STRING(doc, STR("doc/a['target'!=descendant::*]"), STR("4missed"));
	CHECK_XPATH_STRING(doc, STR("doc/a[descendant::*!='target']"), STR("4missed"));
}

TEST_XML(xpath_xalan_predicate_8, "<doc><foo><bar attr='1'>this</bar><bar attr='2'>2</bar><bar attr='3'>3</bar></foo><foo><bar attr='4'>this</bar><bar attr='5'>this</bar><bar1 attr='6'>that</bar1></foo><foo><bar attr='7'><baz attr='a'>hello</baz><baz attr='b'>goodbye</baz></bar><bar2 attr='8'>this</bar2><bar2 attr='9'>that</bar2></foo><foo><bar attr='10'>this</bar><bar attr='11'><baz attr='a'>hello</baz><baz attr='b'>goodbye</baz></bar><bar attr='12'>other</bar></foo></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("foo[(bar[2])='this']")) % 13;
	CHECK_XPATH_NODESET(c, STR("foo[(bar[(baz[2])='goodbye'])]")) % 23 % 38;
	CHECK_XPATH_NODESET(c, STR("foo[(bar[2][(baz[2])='goodbye'])]")) % 38;
}

TEST_XML(xpath_xalan_predicate_9, "<doc><a><asub><asubsub/></asub></a><b><bsub><foo><child/></foo></bsub></b><c>f-inside</c><d><dsub><dsubsub><foundnode/></dsubsub></dsub></d><e>f-inside<esub>f-inside</esub><esubsib>f-inside</esubsib>f-inside</e><f><fsub/></f></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/*[starts-with(name(.),'f')]")) % 23;
	CHECK_XPATH_NODESET(doc, STR("//*[starts-with(name(.),'f')]")) % 8 % 15 % 23 % 24;
}

TEST_XML(xpath_xalan_predicate_10, "<doc><element1>Text from first element<child1>Text from child1 of first element</child1><child2>Text from child2 of first element</child2></element1><element2>Text from second element<child1>Text from child1 of second element</child1><child2 attr1='yes'>Text from child2 of second element (correct execution)</child2></element2></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_STRING(c, STR("//child2[ancestor::element2]"), STR("Text from child2 of second element (correct execution)"));
	CHECK_XPATH_STRING(c, STR("//child2[ancestor-or-self::element2]"), STR("Text from child2 of second element (correct execution)"));
	CHECK_XPATH_STRING(c, STR("//child2[attribute::attr1]"), STR("Text from child2 of second element (correct execution)"));
}

TEST_XML(xpath_xalan_predicate_11, "<doc><a squish='heavy' squash='butternut'>1</a><a squish='heavy' squeesh='virus'>2</a><a squash='butternut' squeesh='virus'>3</a><a squish='heavy'>4</a><a squeesh='virus'>5</a><a squash='butternut'>6</a></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("a[@squeesh or (@squish and @squash)]")) % 3 % 7 % 11 % 18;
	CHECK_XPATH_NODESET(c, STR("a[(@squeesh or @squish) and @squash]")) % 3 % 11;
	CHECK_XPATH_NODESET(c, STR("a[@squeesh or @squish and @squash]")) % 3 % 7 % 11 % 18;
}

TEST_XML(xpath_xalan_predicate_12, "<doc><a>1</a><a>2<achild>target</achild></a><a>3</a><a>target</a></doc>")
{
	CHECK_XPATH_STRING(doc, STR("doc/a[following-sibling::*=descendant::*]"), STR("2target"));
}

TEST_XML(xpath_xalan_predicate_13, "<doc><a squish='heavy'>1</a><a>2<achild>target</achild></a><a>3</a></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/a[('target'=descendant::*) or @squish]")) % 3 % 6;
	CHECK_XPATH_NODESET(doc, STR("doc/a[not(('target'=descendant::*) or @squish)]")) % 10;
}

TEST_XML(xpath_xalan_predicate_14, "<doc><a squish='heavy'>1</a><a>2<achild size='large'>child2</achild></a><a>3</a><a attrib='present'>4<achild>child4</achild></a></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/a[not(@*)]")) % 6 % 11;
}

TEST_XML(xpath_xalan_predicate_15, "<doc><a><asub><asubsub/></asub></a><b><bsub>x</bsub></b><c>inside</c><d><dsub><q><foundnode/></q></dsub></d></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/descendant::*[string-length(name(.))=1]")) % 3 % 6 % 9 % 11 % 13;
}

#endif
