#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_xalan_select_1, "<doc><a><b attr='test'/></a><c><d><e/></d></c></doc>")
{
	CHECK_XPATH_STRING(doc, STR("/doc/a/b/@attr"), STR("test"));
}

TEST_XML(xpath_xalan_select_2, "<doc><do do='-do-'>do</do><re>re</re><mi mi1='-mi1-' mi2='mi2'>mi</mi><fa fa='-fa-'>fa<so so='-so-'>so<la>la<ti>ti</ti>do</la></so></fa><Gsharp so='so+'>G#</Gsharp><Aflat><natural><la>A</la></natural>Ab</Aflat><Bflat>Bb</Bflat><Csharp><natural>C</natural>C#<doublesharp>D</doublesharp></Csharp></doc>")
{
	xml_node c = doc.child(STR("doc"));

	// This should come out fasolatido:
	CHECK_XPATH_NODESET(c, STR("fa")) % 12;
	// This should come out doremifasolatido:
	CHECK_XPATH_NODESET(c, STR("mi | do | fa | re")) % 3 % 6 % 8 % 12;
	// This should come out do-do-remi-mi1-mi2fasolatido-fa--so-:
	CHECK_XPATH_NODESET(c, STR("mi[@mi2='mi2'] | do | fa/so/@so | fa | mi/@* | re | fa/@fa | do/@do")) % 3 % 4 % 6 % 8 % 9 % 10 % 12 % 13 % 16;
	// This should come out solatidoG#:
	CHECK_XPATH_NODESET(c, STR(".//*[@so]")) % 15 % 23;
	// This should come out relatidoABb:
	CHECK_XPATH_NODESET(c, STR("*//la | //Bflat | re")) % 6 % 18 % 28 % 31;
	// This should come out domitiACD:
	CHECK_XPATH_NODESET(c, STR("fa/../mi | Aflat/natural/la | Csharp//* | /doc/do | *//ti")) % 3 % 8 % 20 % 28 % 34 % 37;
}

TEST_XML(xpath_xalan_select_3, "<doc><sub1><child1>preceding sibling number 1</child1><child2>current node</child2><child3>following sibling number 3</child3></sub1><sub2><c>cousin 1</c><c>cousin 2</c><child3>cousin 3</child3></sub2></doc>")
{
	CHECK_XPATH_NODESET(doc.child(STR("doc")).child(STR("sub1")).child(STR("child2")), STR("preceding-sibling::child1|//child3")) % 4 % 8 % 15;
}

TEST_XML(xpath_xalan_select_4, "<doc><child>bad1<sub>bad2</sub></child><c>bad3<sub>bad4</sub></c><sub>OK<nogo>bad5</nogo></sub></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("child::sub")) % 11;
	CHECK_XPATH_NODESET(c, STR("child ::sub")) % 11;
	CHECK_XPATH_NODESET(c, STR("child:: sub")) % 11;
	CHECK_XPATH_NODESET(c, STR("child :: sub")) % 11;
}

TEST_XML_FLAGS(xpath_xalan_select_5, "<doc>bad0<!-- Good --><comment>bad1<sub>bad2</sub></comment></doc>", parse_default | parse_comments)
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("comment()")) % 4;
	CHECK_XPATH_NODESET(c, STR("comment ()")) % 4;
	CHECK_XPATH_NODESET(c, STR("comment ( ) ")) % 4;
	CHECK_XPATH_NUMBER(c, STR("string-length()"), 12);
	CHECK_XPATH_NUMBER(c, STR("string-length ()"), 12);
	CHECK_XPATH_NUMBER(c, STR("string-length ( ) "), 12);
}

TEST_XML(xpath_xalan_select_6, "<div div='20' div-5='12'>9</div>")
{
	xml_node c = doc.child(STR("div"));

	CHECK_XPATH_NUMBER(doc, STR("div +3"), 12);
	CHECK_XPATH_NUMBER(doc, STR("* +3"), 12);
	CHECK_XPATH_NUMBER(c, STR("@div - 5"), 15);
	CHECK_XPATH_NUMBER(c, STR("@div -5"), 15);
	CHECK_XPATH_NUMBER(c, STR("@div-5"), 12);
	CHECK_XPATH_NUMBER(c, STR("@*-5"), 15);
	CHECK_XPATH_NUMBER(doc, STR("16-div"), 7);
	CHECK_XPATH_NUMBER(doc, STR("25-*"), 16);
	CHECK_XPATH_NUMBER(doc, STR("54 div*"), 6);
	CHECK_XPATH_NUMBER(doc, STR("(* - 4) div 2"), 2.5);
	CHECK_XPATH_NUMBER(doc, STR("' 6 ' div 2"), 3);
	CHECK_XPATH_NUMBER(doc, STR("' 6 '*div"), 54);
	CHECK_XPATH_NUMBER(doc, STR("5.*."), 45);
	CHECK_XPATH_NUMBER(doc, STR("5.+."), 14);
}

TEST_XML(xpath_xalan_select_7, "<doc div='20'><div>9</div><attribute>8</attribute></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NUMBER(c, STR("attribute :: div"), 20);
	CHECK_XPATH_NUMBER(c, STR("attribute :: *"), 20);
	CHECK_XPATH_NUMBER(c, STR("attribute*(div - 4)"), 40);
	CHECK_XPATH_NUMBER(c, STR("(* - 4)**"), 45);
}

TEST_XML(xpath_xalan_select_8, "<doc><a>x<div>7</div></a><a>y<div>9</div></a><a>z<div>5</div></a></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/a[div=9]")) % 7;
}

TEST_XML(xpath_xalan_select_9, "<doc><a s='v'><b>7</b><c>3</c></a><a s='w'><b>7</b><c>9</c></a><a s='x'><b>9</b><c>2</c></a><a s='y'><b>9</b><c>9</c></a><a s='z'><b>2</b><c>0</c></a></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/a[*=9]")) % 9 % 15 % 21;
}

TEST_XML(xpath_xalan_select_10, "<doc><sub1><child1>child1</child1></sub1><sub2><child2>child2</child2></sub2><sub3><child3/></sub3></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("/doc/sub1/child1|/doc/sub2/child2")) % 4 % 7;
	CHECK_XPATH_NODESET(doc.child(STR("doc")), STR("sub1/child1|/doc/sub2/child2")) % 4 % 7;
	CHECK_XPATH_NODESET(doc.child(STR("doc")), STR("sub1/child1|sub2/child2")) % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("//self::child1|//self::child2")) % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("//child1|//child2")) % 4 % 7;
	CHECK_XPATH_NODESET(doc, STR("//child1|//child2|//child3")) % 4 % 7 % 10;
}

TEST_XML(xpath_xalan_select_11, "<doc><sub1 pos='1'><child1>descendant number 1</child1></sub1><sub2 pos='2'><child1>descendant number 2</child1></sub2></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("//child1/ancestor::sub1|//child1/ancestor::sub2")) % 3 % 7;
}

TEST_XML(xpath_xalan_select_12, "<doc><sub pos='1'><child>child number 1</child><sub-sub pos='1sub'><child>grandchild number 1</child></sub-sub></sub><sub0 pos='2-no'><child>child number 2</child><sub pos='2.5'><child>grandchild number 2</child></sub></sub0><sub pos='3'><child>child number 3</child><subno pos='3.5-no'><child>grandchild number 3</child></subno></sub><sub0 pos='4-no'><child>child number 4</child><sub-sub pos='4sub'><child>grandchild number 4</child></sub-sub></sub0></doc>")
{
    CHECK_XPATH_NODESET(doc, STR("//child/ancestor-or-self::sub | //child/ancestor-or-self::sub-sub")) % 3 % 7 % 15 % 19 % 31;
}

TEST_XML(xpath_xalan_select_13, "<doc><book><author><name real='no'>Carmelo Montanez</name><chapters>Nine</chapters><bibliography></bibliography></author></book><book><author><name real='na'>David Marston</name><chapters>Seven</chapters><bibliography></bibliography></author></book><book><author><name real='yes'>Mary Brady</name><chapters>Ten</chapters><bibliography><author><name>Lynne Rosenthal</name><chapters>Five</chapters></author></bibliography></author></book></doc>")
{
	CHECK_XPATH_NODESET(doc, STR("doc/book/author[name/@real='no']|doc/book/author[name/@real='yes']")) % 4 % 20;
	CHECK_XPATH_NODESET(doc, STR("doc/book/author[(name/@real='no' and position()=1)]|doc/book/author[(name/@real='yes' and position()=last())]")) % 4 % 20;
	CHECK_XPATH_NODESET(doc, STR("doc/book/author[name='Mary Brady']|doc/book/author[name/@real='no']")) % 4 % 20;
	CHECK_XPATH_NODESET(doc, STR("doc/book/author/name|doc/book/author/bibliography/author/name")) % 5 % 13 % 21 % 28;
	CHECK_XPATH_NODESET(doc, STR("doc/book/author/name|doc/book/author/bibliography/author/chapters")) % 5 % 13 % 21 % 30;
	CHECK_XPATH_NODESET(doc, STR("doc/book/author/name|doc/book/author/noElement")) % 5 % 13 % 21;
	CHECK_XPATH_NODESET(doc, STR("//noChild1|//noChild2"));
}

TEST_XML(xpath_xalan_select_14, "<doc><sub1 pos='1'><child1>child number 1</child1></sub1><sub2 pos='2'><child2>child number 2</child2></sub2><sub3/></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("child::sub1|child::sub2")) % 3 % 7;
	CHECK_XPATH_NODESET(c, STR("descendant::child1|descendant::child2")) % 5 % 9;
	CHECK_XPATH_NODESET(c, STR("descendant-or-self::sub1|descendant-or-self::sub2")) % 3 % 7;
	CHECK_XPATH_NODESET(c.child(STR("sub2")), STR("preceding-sibling::sub1|following-sibling::sub3")) % 3 % 11;
}

TEST_XML(xpath_xalan_select_15, "<doc><child>Selection of this child is an error.</child><child high='3'>Selection of this child is an error.</child><child wide='4'>Selection of this child is an error.</child><child wide='4' high='3'>Selection of this child is an error.</child><child wide='3'>E</child><child wide='3' high='3'>F</child><child wide='3' deep='3'>G</child><child wide='4' deep='2'>Selection of this child is an error.</child><child wide='4' deep='2' high='3'>Selection of this child is an error.</child><child wide='3' deep='2'>J</child><child wide='3' deep='3' high='3'>K</child><child deep='2'>Selection of this child is an error.</child><child deep='2' high='3'>Selection of this child is an error.</child><child deep='3'>N</child><child deep='3' high='3'>O</child><child wide='4' deep='3'>P</child></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NODESET(c, STR("child[@wide='3']|child[@deep='3']")) % 15 % 18 % 22 % 35 % 39 % 51 % 54 % 58;
	CHECK_XPATH_NODESET(c, STR("child[@deep='3']|child[@wide='3']")) % 15 % 18 % 22 % 35 % 39 % 51 % 54 % 58;
}

TEST_XML(xpath_xalan_select_16, "<doc><a squish='light' squash='butternut'>1</a><a squeesh='' squish='extreme'>2</a><a squash='butternut' squeesh=''>3</a><a squish='heavy' squash='sport' squeesh=''>4</a></doc>")
{
	CHECK_XPATH_NUMBER(doc, STR("count(doc/a/attribute::*)"), 9);
	CHECK_XPATH_NUMBER(doc, STR("count(//@*)"), 9);
	CHECK_XPATH_NUMBER(doc, STR("count(//@squish)"), 3);
}

TEST_XML(xpath_xalan_select_17, "<directions><north><dup1/><dup2/><south/><east/><west/></north><north1/><north2><dup1/><dup2/><dup3/><dup4/></north2><north3><dup1/><dup2/><south-north/><east-north/><west-north/></north3><south/><east><dup1/><dup2/><north-east/><south-east/><west-east/></east><west/></directions>")
{
	xml_node c = doc.child(STR("directions"));

    CHECK_XPATH_NODESET(c, STR("north/* | north/dup1 | north/dup2")) % 4 % 5 % 6 % 7 % 8;
    CHECK_XPATH_NODESET(c, STR("north/dup2 | north/dup1 | north/*")) % 4 % 5 % 6 % 7 % 8;
    CHECK_XPATH_NODESET(c, STR("//north/dup2 | south/preceding-sibling::*[4]/* | north/dup1 | north/*")) % 4 % 5 % 6 % 7 % 8;
    CHECK_XPATH_NODESET(c, STR("north/dup2 | south/preceding-sibling::*[4]/* | north/*")) % 4 % 5 % 6 % 7 % 8;
}

TEST_XML(xpath_xalan_select_18, "<para><font color='red'>Hello</font><font color='green'>There</font><font color='blue'>World</font></para>")
{
    CHECK_XPATH_NODESET(doc, STR("/para/font[@color='green']")) % 6;
    CHECK_XPATH_NODESET(doc.child(STR("para")), STR("/para/font[@color='green']")) % 6;
    CHECK_XPATH_NODESET(doc.child(STR("para")).last_child(), STR("/para/font[@color='green']")) % 6;
}

TEST_XML_FLAGS(xpath_xalan_select_19, "<doc>1<a>in-a</a>2<!-- upper comment --><b>3<bb>4<bbb>5</bbb>6</bb>7</b><!-- lower comment -->8<c>in-c</c>9<?pi?></doc>", parse_default | parse_comments | parse_pi)
{
	CHECK_XPATH_NODESET(doc, STR("//*")) % 2 % 4 % 8 % 10 % 12 % 18;
	CHECK_XPATH_NODESET(doc, STR("//node()")) % 2 % 3 % 4 % 5 % 6 % 7 % 8 % 9 % 10 % 11 % 12 % 13 % 14 % 15 % 16 % 17 % 18 % 19 % 20 % 21;
	CHECK_XPATH_NODESET(doc, STR("//text()")) % 3 % 5 % 6 % 9 % 11 % 13 % 14 % 15 % 17 % 19 % 20;
	CHECK_XPATH_NODESET(doc, STR("//comment()")) % 7 % 16;
	CHECK_XPATH_NODESET(doc, STR("//processing-instruction()")) % 21;
}

TEST_XML(xpath_xalan_bugzilla_1, "<report><colData colId='F'>1</colData><colData colId='L'>5</colData><colData colId='F'>1</colData><colData colId='L'>5</colData><colData colId='L'>2</colData><colData colId='F'>2</colData><colData colId='L'>5</colData><colData colId='F'>2</colData></report>")
{
	CHECK_XPATH_NODESET(doc, STR("/report/colData[@colId='F' and not(.=preceding::colData)]")) % 3;
}

TEST(xpath_xalan_error_boolean)
{
	CHECK_XPATH_FAIL(STR("nt(true())"));
	CHECK_XPATH_FAIL(STR("not(troo())"));
	CHECK_XPATH_FAIL(STR("troo() and (2 = 2)"));
	CHECK_XPATH_FAIL(STR("troo() or (2 = 2)"));
	CHECK_XPATH_FAIL(STR("2 = troo()"));
	CHECK_XPATH_FAIL(STR("boolean(troo())"));
	CHECK_XPATH_FAIL(STR("true(doc)"));
	CHECK_XPATH_FAIL(STR("false(doc)"));
	CHECK_XPATH_FAIL(STR("not()"));
	CHECK_XPATH_FAIL(STR("not(false(), doc)"));
	CHECK_XPATH_FAIL(STR("boolean()"));
	CHECK_XPATH_FAIL(STR("boolean(false(), doc)"));
	CHECK_XPATH_FAIL(STR("lang()"));
	CHECK_XPATH_FAIL(STR("lang('en','us')"));
}

TEST(xpath_xalan_error_conditional)
{
	CHECK_XPATH_FAIL(STR(""));
	CHECK_XPATH_FAIL(STR("@name='John' | @name='Joe'"));
	CHECK_XPATH_FAIL(STR("\x95not(name(.)='')"));
}

TEST(xpath_xalan_error_match)
{
	CHECK_XPATH_FAIL(STR("//"));
	CHECK_XPATH_FAIL(STR("section1|"));
	CHECK_XPATH_FAIL(STR("|section1"));
}

TEST(xpath_xalan_error_math)
{
	CHECK_XPATH_FAIL(STR("6 quo 4"));
	CHECK_XPATH_FAIL(STR("-troo()"));
	CHECK_XPATH_FAIL(STR("number(troo())"));
	CHECK_XPATH_FAIL(STR("5 * troo()"));
	CHECK_XPATH_FAIL(STR("12 div troo()"));
	CHECK_XPATH_FAIL(STR("number(8,doc)"));
	CHECK_XPATH_FAIL(STR("sum(doc, 8)"));
	CHECK_XPATH_FAIL(STR("sum()"));
	CHECK_XPATH_FAIL(STR("floor(8,7)"));
	CHECK_XPATH_FAIL(STR("floor()"));
	CHECK_XPATH_FAIL(STR("ceiling(8,7)"));
	CHECK_XPATH_FAIL(STR("ceiling()"));
	CHECK_XPATH_FAIL(STR("round(8,7)"));
	CHECK_XPATH_FAIL(STR("round()"));
}

TEST(xpath_xalan_error_namespace)
{
	CHECK_XPATH_FAIL(STR("local-name(baz2:b,..)"));
	CHECK_XPATH_FAIL(STR("namespace-uri(baz2:b,..)"));
	CHECK_XPATH_FAIL(STR("name(a,b)"));
	CHECK_XPATH_FAIL(STR(":foo"));
	CHECK_XPATH_FAIL(STR("*:foo"));
}

TEST(xpath_xalan_error_position)
{
	CHECK_XPATH_FAIL(STR("*[last(*,2)]"));
	CHECK_XPATH_FAIL(STR("position(b)=1"));
	CHECK_XPATH_FAIL(STR("count()"));
	CHECK_XPATH_FAIL(STR("count(*,4)"));
	CHECK_XPATH_FAIL(STR("position()=last(a)"));
}

TEST(xpath_xalan_error_select)
{
	CHECK_XPATH_FAIL(STR(""));
	CHECK_XPATH_FAIL(STR("count(troo())"));
	CHECK_XPATH_FAIL(STR("c::sub"));
	CHECK_XPATH_FAIL(STR("c()"));
	CHECK_XPATH_FAIL(STR("(* - 4) foo 2"));
	CHECK_XPATH_FAIL(STR("5 . + *"));
	CHECK_XPATH_FAIL(STR("4/."));
	CHECK_XPATH_FAIL(STR("true()/."));
	CHECK_XPATH_FAIL(STR("item//[@type='x']"));
	CHECK_XPATH_FAIL(STR("//"));
	CHECK_XPATH_FAIL(STR("item//"));
	CHECK_XPATH_FAIL(STR("count(//)"));
	CHECK_XPATH_FAIL(STR("substring-after(//,'0')"));
	CHECK_XPATH_FAIL(STR("//+17"));
	CHECK_XPATH_FAIL(STR("//|subitem"));
	CHECK_XPATH_FAIL(STR("..[near-north]"));
}

TEST(xpath_xalan_error_string)
{
	CHECK_XPATH_FAIL(STR("string(troo())"));
	CHECK_XPATH_FAIL(STR("string-length(troo())"));
	CHECK_XPATH_FAIL(STR("normalize-space(a,'\t\r\n ab    cd  ')"));
	CHECK_XPATH_FAIL(STR("contains('ENCYCLOPEDIA')"));
	CHECK_XPATH_FAIL(STR("contains('ENCYCLOPEDIA','LOPE',doc)"));
	CHECK_XPATH_FAIL(STR("starts-with('ENCYCLOPEDIA')"));
	CHECK_XPATH_FAIL(STR("starts-with('ENCYCLOPEDIA','LOPE',doc)"));
	CHECK_XPATH_FAIL(STR("substring-before('ENCYCLOPEDIA')"));
	CHECK_XPATH_FAIL(STR("substring-before('ENCYCLOPEDIA','LOPE',doc)"));
	CHECK_XPATH_FAIL(STR("substring-after('ENCYCLOPEDIA')"));
	CHECK_XPATH_FAIL(STR("substring-after('ENCYCLOPEDIA','LOPE',doc)"));
	CHECK_XPATH_FAIL(STR("substring('ENCYCLOPEDIA')"));
	CHECK_XPATH_FAIL(STR("substring('ENCYCLOPEDIA',4,5,2)"));
	CHECK_XPATH_FAIL(STR("concat('x')"));
	CHECK_XPATH_FAIL(STR("string-length('ENCYCLOPEDIA','PEDI')"));
	CHECK_XPATH_FAIL(STR("translate('bar','abc')"));
	CHECK_XPATH_FAIL(STR("translate('bar','abc','ABC','output')"));
	CHECK_XPATH_FAIL(STR("string(22,44)"));
	CHECK_XPATH_FAIL(STR("concat(/*)"));
}

#endif
