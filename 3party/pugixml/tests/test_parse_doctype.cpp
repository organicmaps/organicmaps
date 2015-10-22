#define _CRT_SECURE_NO_WARNINGS

#include "common.hpp"

#include <string.h>
#include <wchar.h>
#include <string>

static xml_parse_result load_concat(xml_document& doc, const char_t* a, const char_t* b = STR(""), const char_t* c = STR(""))
{
	char_t buffer[768];

#ifdef PUGIXML_WCHAR_MODE
	wcscpy(buffer, a);
	wcscat(buffer, b);
	wcscat(buffer, c);
#else
	strcpy(buffer, a);
	strcat(buffer, b);
	strcat(buffer, c);
#endif

	return doc.load_string(buffer, parse_fragment);
}

static bool test_doctype_wf(const char_t* decl)
{
	xml_document doc;

	// standalone
	if (!load_concat(doc, decl) || !doc.first_child().empty()) return false;

	// pcdata pre/postfix
	if (!load_concat(doc, STR("a"), decl) || !test_node(doc, STR("a"), STR(""), format_raw)) return false;
	if (!load_concat(doc, decl, STR("b")) || !test_node(doc, STR("b"), STR(""), format_raw)) return false;
	if (!load_concat(doc, STR("a"), decl, STR("b")) || !test_node(doc, STR("ab"), STR(""), format_raw)) return false;

	// node pre/postfix
	if (!load_concat(doc, STR("<nodea/>"), decl) || !test_node(doc, STR("<nodea />"), STR(""), format_raw)) return false;
	if (!load_concat(doc, decl, STR("<nodeb/>")) || !test_node(doc, STR("<nodeb />"), STR(""), format_raw)) return false;
	if (!load_concat(doc, STR("<nodea/>"), decl, STR("<nodeb/>")) || !test_node(doc, STR("<nodea /><nodeb />"), STR(""), format_raw)) return false;

    // check load-store contents preservation
    CHECK(doc.load_string(decl, parse_doctype | parse_fragment));
    CHECK_NODE(doc, decl);

	return true;
}

static bool test_doctype_nwf(const char_t* decl)
{
	xml_document doc;

	// standalone
	if (load_concat(doc, decl).status != status_bad_doctype) return false;

	// pcdata postfix
	if (load_concat(doc, decl, STR("b")).status != status_bad_doctype) return false;

	// node postfix
	if (load_concat(doc, decl, STR("<nodeb/>")).status != status_bad_doctype) return false;

	return true;
}

#define TEST_DOCTYPE_WF(contents) CHECK(test_doctype_wf(STR(contents)))
#define TEST_DOCTYPE_NWF(contents) CHECK(test_doctype_nwf(STR(contents)))

TEST(parse_doctype_skip)
{
	TEST_DOCTYPE_WF("<!DOCTYPE doc>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc SYSTEM 'foo'>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc SYSTEM \"foo\">");
	TEST_DOCTYPE_WF("<!DOCTYPE doc PUBLIC \"foo\" 'bar'>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc PUBLIC \"foo'\">");
	TEST_DOCTYPE_WF("<!DOCTYPE doc SYSTEM 'foo' [<!ELEMENT foo 'ANY'>]>");
}

TEST(parse_doctype_error)
{
	TEST_DOCTYPE_NWF("<!DOCTYPE");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM 'foo");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM \"foo");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc PUBLIC \"foo\" 'bar");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc PUBLIC \"foo'\"");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM 'foo' [<!ELEMENT foo 'ANY");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM 'foo' [<!ELEMENT foo 'ANY'>");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM 'foo' [<!ELEMENT foo 'ANY'>]");
	TEST_DOCTYPE_NWF("<!DOCTYPE doc SYSTEM 'foo' [<!ELEMENT foo 'ANY'>] ");
}

// Examples from W3C recommendations
TEST(parse_doctype_w3c_wf)
{
	TEST_DOCTYPE_WF("<!DOCTYPE greeting SYSTEM \"hello.dtd\">");
	TEST_DOCTYPE_WF("<!DOCTYPE greeting [ <!ELEMENT greeting (#PCDATA)> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary)  \"ordered\"> <!ATTLIST form method  CDATA   #FIXED \"POST\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE greeting [ <!ENTITY % draft 'INCLUDE' > <!ENTITY % final 'IGNORE' > <![%draft;[ <!ELEMENT book (comments*, title, body, supplements?)> ]]> <![%final;[ <!ELEMENT book (title, body, supplements?)> ]]>]>");
	TEST_DOCTYPE_WF("<!DOCTYPE greeting [ <!ENTITY open-hatch PUBLIC \"-//Textuality//TEXT Standard open-hatch boilerplate//EN\" \"http://www.textuality.com/boilerplate/OpenHatch.xml\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE greeting [ <!ENTITY EndAttr \"27'\" > ]>");
}

TEST(parse_doctype_w3c_nwf)
{
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting SYSTEM \"hello.dtd>");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting SYSTEM");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ELEMENT greeting (#PCDATA)> ]");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ELEMENT greeting (#PCDATA)>");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ELEMENT greeting (#PCDATA");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ ");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary)  \"ordered\"> ]");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary)  \"ordered\">");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary)  \"orde");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary) ");
	TEST_DOCTYPE_NWF("<!DOCTYPE greeting [ <!ENTITY open-hatch PUBLIC \"-//Textuality//TEXT Standard open-hatch boilerplate//EN\" \"http://www.textuality.com/boilerplate/OpenHatch.x");
}

// Examples from xmlsuite
TEST(parse_doctype_xmlconf_eduni_1)
{
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!NOTATION gif SYSTEM \"file:///usr/X11R6/bin/xv\"> <!ENTITY declared SYSTEM \"xyzzy\" NDATA gif> <!ATTLIST foo bar ENTITY \"7\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!NOTATION gif SYSTEM \"file:///usr/X11R6/bin/xv\"> <!ENTITY declared SYSTEM \"xyzzy\" NDATA gif> <!ATTLIST foo bar ENTITY \"undeclared\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY % e SYSTEM \"E60.ent\"> %e; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY e \"an &unparsed; entity\"> <!NOTATION gif SYSTEM \"file:///usr/X11R6/bin/xv\"> <!ENTITY unparsed SYSTEM \"xyzzy\" NDATA gif> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo bar CDATA #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo xml:lang CDATA #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY e SYSTEM \"E38.ent\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo SYSTEM \"E36.dtd\">");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ELEMENT bar (foo|foo)> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!NOTATION one SYSTEM \"file:///usr/bin/awk\"> <!ATTLIST foo bar NOTATION (one|one) #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo bar (one|one) #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo xml:lang NMTOKEN #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY gt \">\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo bar NMTOKENS #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY % pe SYSTEM \"subdir1/E18-pe\"> %pe; %intpe; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (PCDATA|foo)*> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (foo*)> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (foo*)> <!ENTITY space \"&#38;#32;\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (foo*)> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (foo*)> <!ENTITY space \" \"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo EMPTY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo EMPTY> <!ENTITY empty \"\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <![INCLUDE[<!ATTLIST foo bar CDATA #IMPLIED>]]> <![IGNORE[some junk]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY % pe \"hello\"> <!-- If forward were expanded when ent was declared, we were get an error, but it is bypassed and not expanded until ent is used in the instance --> <!ENTITY ent \"%pe; &#33; &forward;\"> <!ENTITY forward \"goodbye\"> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY % e \"bar CDATA #IMPLIED>\"> <!ATTLIST foo %e;");
	TEST_DOCTYPE_NWF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY % e \"bar CDATA #IMPLIED>\"> <!ATTLIST foo %e; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ENTITY ent SYSTEM 'E18-ent'> ]>");
}

TEST(parse_doctype_xmlconf_eduni_2)
{
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ENTITY % pe \"<!ENTITY ent1 'text'>\"> %pe; <!ELEMENT foo ANY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ENTITY ent SYSTEM \"ent\"> <!ELEMENT foo ANY> <!ATTLIST foo a CDATA \"contains &ent; reference\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo id ID #IMPLIED> <!ATTLIST foo ref IDREF \"undef\"> <!ATTLIST foo ent ENTITY \"undef\"> <!-- can't test NOTATION attribute, because if it's undeclared then we'll get an error for one of the enumerated values being undeclared. --> <!ENTITY ent SYSTEM \"foo\" NDATA not> <!NOTATION not SYSTEM \"not\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo EMPTY> <!ATTLIST foo a (one|two|three) \"four\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo a NOTATION (not) \"not2\"> <!NOTATION not SYSTEM \"not\"> <!NOTATION not2 SYSTEM \"not2\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo EMPTY> <!ATTLIST foo a NMTOKENS \"34+\"> ]>");
}

TEST(parse_doctype_xmlconf_eduni_3)
{
	TEST_DOCTYPE_WF("<!DOCTYPE animal [ <!ELEMENT animal ANY> <?_\xd9\x9f an only legal per 5th edition extender #x65f in PITarget ?> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root ANY> <!ELEMENT \xc2\xb7_BadName EMPTY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ENTITY e \"<X&#xe5c;></X&#xe5c;>\"> ]>");
}

TEST(parse_doctype_xmlconf_eduni_4)
{
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY a:b \"bogus\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo xmlns:a CDATA #IMPLIED xmlns:b NMTOKEN #IMPLIED xmlns:c CDATA #IMPLIED> <!ELEMENT bar ANY> <!ATTLIST bar a:attr CDATA #IMPLIED b:attr CDATA #IMPLIED c:attr CDATA #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo xmlns:a CDATA #IMPLIED xmlns:b CDATA #IMPLIED xmlns:c CDATA #IMPLIED> <!ELEMENT bar ANY> <!ATTLIST bar a:attr CDATA #IMPLIED b:attr CDATA #IMPLIED c:attr CDATA #IMPLIED> <!ENTITY tilde \"~\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT xmlns:foo EMPTY> ]>");
}

TEST(parse_doctype_xmlconf_eduni_5)
{
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ENTITY e \"&#x0c;\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo (foo*)> <!ENTITY e \"abc&#x85;def\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE foo [ <!ELEMENT foo ANY> <!ATTLIST foo bar  NMTOKENS #IMPLIED> <!ENTITY val \"abc&#x85;def\"> ]>");
}

TEST(parse_doctype_xmlconf_ibm_1)
{
	TEST_DOCTYPE_WF("<!DOCTYPE animal SYSTEM \"ibm32i04.dtd\" [ <!ATTLIST animal xml:space (default|preserve) 'preserve'> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (a,b)> <!ELEMENT a EMPTY> <!ELEMENT b (#PCDATA|c)* > <!ELEMENT c ANY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (PCDATA|b)* > <!ELEMENT b (#PCDATA) > <!ATTLIST b attr1 CDATA #REQUIRED> <!ATTLIST b attr2 (abc|def) \"abc\"> <!ATTLIST b attr3 CDATA #FIXED \"fixed\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE test [ <!ELEMENT test ANY> <!ELEMENT landscape EMPTY> <!ENTITY parsedentity1 SYSTEM \"ibm56iv01.xml\"> <!ENTITY parsedentity2 SYSTEM \"ibm56iv02.xml\"> <!ATTLIST landscape sun ENTITIES #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE test [ <!ELEMENT test ANY> <!ELEMENT landscape EMPTY> <!ENTITY image1 SYSTEM \"d:\\testspec\\images\\sunset.gif\" NDATA gif> <!ENTITY image2 SYSTEM \"d:\\testspec\\images\\frontpag.gif\" NDATA gif> <!ATTLIST landscape sun ENTITIES #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE tokenizer [ <!ELEMENT tokenizer ANY> <!ATTLIST tokenizer UniqueName ID #FIXED \"AC1999\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE test [ <!ELEMENT test ANY> <!ELEMENT blob (#PCDATA)> <!NOTATION base64 SYSTEM \"mimecode\"> <!NOTATION uuencode SYSTEM \"uudecode\"> <!ATTLIST blob content-encoding NOTATION (base64|uuencode|raw|ascii) #REQUIRED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE test [ <!ELEMENT test ANY> <!ELEMENT a EMPTY> <!ELEMENT nametoken EMPTY> <!ATTLIST nametoken namevalue NMTOKEN \"@#$\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)* > <!ENTITY % pe1 SYSTEM \"ibm68i04.ent\"> %pe1; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT a EMPTY> <!ATTLIST a attr1 CDATA \"&ge1;\"> <!--* GE reference in attr default before declaration *--> <!ENTITY ge1 \"abcdef\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ENTITY ge1 \"abcdef\"> <!ELEMENT a EMPTY> <!ATTLIST a attr1 CDATA  \"&ge1;\"> <!ENTITY % pe2 \"<!ATTLIST a attr2 CDATA #IMPLIED>\"> %pe3; <!--* PE reference in above doesn't match declaration *--> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!ATTLIST root att CDATA #IMPLIED> <!ENTITY % pe1 '<!ATTLIST root att2 CDATA \"&ge1;\">'> <!ENTITY ge1 \"attdefaultvalue\" > %pe1; <!--* notation JPGformat not declared *--> <!ENTITY ge2  SYSTEM \"image.jpg\" NDATA JPGformat> ]>");
}

TEST(parse_doctype_xmlconf_ibm_2)
{
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY gewithElemnetDecl \"<!ELEMENT bogus ANY>\"> <!ATTLIST student att1 CDATA #REQUIRED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY gewithlt \"abcd&#x26;&#x23;x3c;\"> <!ATTLIST student att1 CDATA #REQUIRED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY gewithlt \"abcd&#x3c;\"> <!ATTLIST student att1 CDATA #REQUIRED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE 5A_name_starts_with_digit [ <!ELEMENT 5A_name_starts_with_digit EMPTY> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY FullName \"Snow&Man\"> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY FullName \"Snow\"Man\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ATTLIST student first CDATA #REQUIRED middle CDATA #IMPLIED last CDATA #IMPLIED > <!ENTITY myfirst \"Snow\"> <!ENTITY mymiddle \"I\"> <!ENTITY mylast \"Man\"> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student SYSTEM 'student.DTD [ <!ELEMENT student (#PCDATA)> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY info PUBLIC '..\\info.dtd> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY info PUBLIC '..\\info'.dtd'> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY info PUBLIC \"..\\info.dtd> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ENTITY info PUBLIC \"This is a {test} \" \"student.dtd\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE aniaml [ <!ELEMENT animal ANY> <!ENTITY generalE \"leopard\"> &generalE; <!ENTITY % parameterE \"<!ELEMENT leopard EMPTY>\"> %parameterE; ] animal>");
	TEST_DOCTYPE_WF("<!DOCTYPE animal SYSTEM \"ibm28an01.dtd\" [ <!ELEMENT animal (cat|tiger|leopard)+> <!NOTATION animal_class SYSTEM \"ibm29v01.txt\"> <!ELEMENT cat ANY> <!ENTITY forcat \"This is a small cat\"> <!ELEMENT tiger (#PCDATA)> <!ELEMENT small EMPTY> <!ELEMENT big EMPTY> <!ATTLIST tiger color CDATA #REQUIRED> <?sound \"This is a PI\" ?> <!-- This is a comment --> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE animal [ <!ELEMENT animal ANY> <!ENTITY % parameterE \"cat SYSTEM\"> <!NOTATION %parameterE; \"cat.txt\"> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE animal [ <!ELEMENT animal ANY> <!ENTITY % parameterE \"A music file -->\"> <!-- Parameter reference appears inside a comment in DTD --> <!-- This is %parameterE; ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE animal [ <!ELEMENT animal ANY> <!ENTITY % parameterE \"A music file ?>\"> <?music %parameterE; ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE animal [ <!ELEMENT animal ANY> <!ENTITY % parameterE \"leopard EMPTY>\"> <!ELEMENT %parameterE; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root ANY> <!ATTLIST root attr1 CDATA #IMPLIED> <!ATTLIST root attr2 CDATA #IMPLIED> <!ENTITY withlt \"have <lessthan> inside\"> <!ENTITY aIndirect \"&withlt;\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!--* Mising Name S contentspec in elementdecl *--> <!ELEMENT > ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!ELEMENT a ANY> <!ELEMENT b ANY> <!--* extra separator in seq *--> <!ELEMENT aElement ((a|b),,a)? > ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!ELEMENT a ANY> <!--* Missing white space before Name in AttDef *--> <!ATTLIST a attr1 CDATA \"default\"attr2 ID #required> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE test [ <!ELEMENT test ANY> <!ELEMENT one EMPTY> <!ELEMENT two EMPTY> <!NOTATION this SYSTEM \"alpha\"> <!ATTLIST three attr NOTATION (\"this\") #IMPLIED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!-- DTD for Production 62--> <![ include [ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]]> <!--Negative test with pattern1 of P62--> <!--include(Case sensitive)--> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![[INCLUDE[ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]]> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <?[INCLUDE[ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![[ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![INCLUDE <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> [INCLUDE ]]> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <![ INCLUDE [ <!ELEMENT tiger EMPTY> <!ELEMENT animal ANY> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <![INCLUDE[ ]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!--* PE referenced before declared, against WFC: entity declared --> %paaa; <!ENTITY % paaa \"<!ATTLIST root att CDATA #IMPLIED>\"> <!ENTITY aaa \"aString\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!ATTLIST root att CDATA #IMPLIED> <!--* missing space  *--> <!ENTITY% paaa \"<!-- comments -->\"> %paaa; ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT root (#PCDATA)> <!ATTLIST root att CDATA #IMPLIED> <!--* missing closing bracket  *--> <!ENTITY % paaa \"<!-- comments -->\" %paaa; ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root PUBLIC \"-//W3C//DTD//EN\"\"empty.dtd\" [ <!ELEMENT root (#PCDATA)> <!ATTLIST root att CDATA #IMPLIED> ]>");
}

TEST(parse_doctype_xmlconf_ibm_3)
{
	TEST_DOCTYPE_WF("<!DOCTYPE animal [ <!ELEMENT animal (cat|tiger|leopard)+> <!ELEMENT cat EMPTY> <!ELEMENT tiger (#PCDATA)> <!ELEMENT leopard ANY> <!ELEMENT small EMPTY> <!ELEMENT big EMPTY> <!ATTLIST tiger color CDATA #REQUIRED> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE book [ <!ELEMENT book ANY> <!-- This test case covers     legal character ranges plus discrete legal characters for production 02. --> <?NAME target ?> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ATTLIST student first CDATA #REQUIRED middle CDATA #IMPLIED last CDATA #REQUIRED > <!ENTITY myfirst \"Snow\"> <!ENTITY mymiddle \"I\"> <!ENTITY mylast 'Man &myfirst; and &myfirst; mymiddle;.'> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student SYSTEM 'student.dtd'[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!ENTITY unref SYSTEM \"\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student PUBLIC \"\" \"student.dtd\"[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student PUBLIC '' 'student.dtd'[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student PUBLIC \"The big ' in it\" \"student.dtd\"[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student PUBLIC 'The latest version' 'student.dtd'[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student PUBLIC \"#x20 #xD #xA abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ -'()+,./:=?;!*#@$_% \" \"student.dtd\"[ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!----> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <!---> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <?pi?> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <?> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE student [ <!ELEMENT student (#PCDATA)> <?MyInstruct AVOID ? BEFORE > IN PI ?> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE animal SYSTEM \"ibm28v02.dtd\" [ <!NOTATION animal_class SYSTEM \"ibm28v02.txt\"> <!ENTITY forcat \"This is a small cat\"> <!ELEMENT tiger (#PCDATA)> <!ENTITY % make_small \"<!ELEMENT small EMPTY>\"> <!ENTITY % make_leopard_element \"<!ELEMENT leopard ANY>\"> <!ENTITY % make_attlist \"<!ATTLIST tiger color CDATA #REQUIRED>\"> %make_leopard_element; <!ELEMENT cat ANY> %make_small; <!ENTITY % make_big \"<!ELEMENT big EMPTY>\"> %make_big; %make_attlist; <?sound \"This is a PI\" ?> <!-- This is a valid test file for p28 --> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE animal [ <![INCLUDE[ <!ENTITY % rootElement \"<!ELEMENT animal ANY>\"> ]]> %rootElement; <!-- Following is a makupdecl --> <!ENTITY % make_tiger_element \"<!ELEMENT tiger EMPTY>\"> %make_tiger_element; <![IGNORE[ <!ELEMENT animal EMPTY> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT root (a,b)> <!ELEMENT a EMPTY> <!ELEMENT b (#PCDATA|c)* > <!ELEMENT c ANY> <!ENTITY inContent \"<b>General entity reference in element content</b>\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT a EMPTY> <!ELEMENT b (#PCDATA|c)* > <!ELEMENT c ANY> <!--* PE replace Text have both parentheses *--> <!ENTITY % seq1 \"(a,b,c)\"> <!ELEMENT child1 %seq1; > <!--* Another legal PE replace Text *--> <!ENTITY % seq2 \"a,b\"> <!ELEMENT child2 (%seq2;,c) > ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![IGNORE[ Everything is ignored within an ignored section, except the sub-section delimiters '<![' and ']]>'.  These must be balanced <!ok ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![IGNORE[ Everything is ignored within an ignored section, except the sub-section delimiters '<![' and ']]>'.  These must be balanced <![ <!ELEMENT animal EMPTY> ]]> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![IGNORE[ begin Everything is ignored within an ignored section, except the sub-section delimiters '<![' and ']]>'.  These must be balanced <![ <!ELEMENT animal EMPTY> ]]> nesting <![ <!ELEMENT tiger (#PCDATA)> ]]> nesting again <![ <!ELEMENT abc ANY> ]]> end ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!DOCTYPE root SYSTEM \"ibm69v01.dtd\" [ <!ELEMENT root (#PCDATA|a)* > <!ENTITY % pe1 \"<!-- comment in PE -->\"> %pe1; ]> ]>");
}

TEST(parse_doctype_xmlconf_oasis_1)
{
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <!ENTITY % ent1 \"\"> <!ENTITY ent2 \"text2\"> <!ENTITY % ent3 \"<!-- <!DOCTYPE <!ELEMENT <? '''&#34;&ent2; %ent1;\"> <!ENTITY % ent4 '\"\"&#x27;&#39;\"'> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![INCLUDE[ <!ENTITY % rootel \"<!ELEMENT doc EMPTY>\"> ]]> %rootel; <!ATTLIST doc att CDATA #IMPLIED> <![IGNORE[ <!ELEMENT doc (a)> ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![INCLUDE[<![INCLUDE[ <![IGNORE[ ignored ]]> <!ELEMENT doc EMPTY> ]]>]]> <![IGNORE[ ignored ]]> <![IGNORE[ <!ELEMENT doc ignored ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <![INCLUDE[ <![ INCLUDE [ <!ELEMENT doc EMPTY> <![IGNORE[asdfasdf]]> ]]>]]> <![INCLUDE[]]> <![INCLUDE[ ]]> <![INCLUDE[ ]]>  ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <![IGNORE[<![]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <![IGNORE[ <![INCLUDE[ <!ELEMENT doc ]]>]]> <![ IGNORE [ ]]> <![IGNORE[]]> <![IGNORE[ ]]> <![IGNORE[ ]]> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <![IGNORE[ <![ starts must balance ]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <![IGNORE[ Everything is ignored within an ignored section, except the sub-section delimiters '<![' and ']]>'.  These must be balanced, but it is no section keyword is required: <![]]> <![DUNNO[ ]]> <![INCLUDE[ asdfasdfasdf <!OK ]]> ] ]> ]] > ]]> <![IGNORE[ < ![ <! [ <![]]>]]> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ELEMENT doc EMPTY> <!NOTATION not1 SYSTEM \"a%a&b&#0<!ELEMENT<!--<?</>?>/\''\"> <!NOTATION not2 SYSTEM 'a b\"\"\"'> <!NOTATION not3 SYSTEM \"\"> <!NOTATION not4 SYSTEM ''> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ELEMENT doc EMPTY> <!NOTATION not1 PUBLIC \"<\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ELEMENT doc EMPTY> <!NOTATION not1 PUBLIC \"a b cdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"> <!NOTATION not2 PUBLIC '0123456789-()+,./:=?;!*#@$_%'> <!NOTATION not3 PUBLIC \"0123456789-()+,.'/:=?;!*#@$_%\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc SYSTEM \"p31pass1.dtd\" [<!ELEMENT doc EMPTY>]>");

    // not actually a doctype :)
    xml_document doc;
    CHECK(doc.load_string(STR("<!--a <!DOCTYPE <?- ]]>-<[ CDATA [ \"- -'- -<doc>--> <!---->"), parse_full | parse_fragment) && doc.first_child().type() == node_comment && doc.last_child().type() == node_comment && doc.first_child().next_sibling() == doc.last_child());
	CHECK(doc.load_string(STR("<?xmla <!DOCTYPE <[ CDATA [</doc> &a%b&#c?>"), parse_full | parse_fragment) && doc.first_child().type() == node_pi && doc.first_child() == doc.last_child());
}

TEST(parse_doctype_xmlconf_xmltest_1)
{
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <![ INCLUDE [ <!ELEMENT doc (#PCDATA)> ]> ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc (#PCDATA)> <![ IGNORE [");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc (#PCDATA)> <![ IGNORE [ ]>");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc (#PCDATA)> <![ INCLUDE [");
	TEST_DOCTYPE_NWF("<!DOCTYPE root [ <!ELEMENT doc (#PCDATA)> <![ INCLUDE [ ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE root [ <!ELEMENT doc EMPTY> <!ENTITY % e \"<!--\"> %e; -->");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!NOTATION foo PUBLIC \"[\" \"null.ent\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ELEMENT doc (#PCDATA)> <!ATTLIST doc a CDATA #IMPLIED> <!ENTITY e '\"'> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ENTITY e \"<foo a='&#38;'></foo>\"> ]>");
	TEST_DOCTYPE_WF("<!DOCTYPE doc [ <!ELEMENT doc (#PCDATA)> <!ENTITY e \"<![CDATA[Tim & Michael]]>\"> ]>");
}

TEST_XML_FLAGS(parse_doctype_value, "<!DOCTYPE doc [ <!ELEMENT doc (#PCDATA)> <!ENTITY e \"<![CDATA[Tim & Michael]]>\"> ]>", parse_fragment | parse_doctype)
{
    xml_node n = doc.first_child();

    CHECK(n.type() == node_doctype);
    CHECK_STRING(n.value(), STR("doc [ <!ELEMENT doc (#PCDATA)> <!ENTITY e \"<![CDATA[Tim & Michael]]>\"> ]"));
}

TEST(parse_doctype_error_toplevel)
{
    xml_document doc;
    CHECK(doc.load_string(STR("<node><!DOCTYPE></node>")).status == status_bad_doctype);
    CHECK(doc.load_string(STR("<node><!DOCTYPE></node>"), parse_doctype).status == status_bad_doctype);
}

TEST(parse_doctype_error_ignore)
{
    xml_document doc;
	CHECK(doc.load_string(STR("<!DOCTYPE root [ <![IGNORE[ ")).status == status_bad_doctype);
	CHECK(doc.load_string(STR("<!DOCTYPE root [ <![IGNORE[ "), parse_doctype).status == status_bad_doctype);
	CHECK(doc.load_string(STR("<!DOCTYPE root [ <![IGNORE[ <![INCLUDE[")).status == status_bad_doctype);
	CHECK(doc.load_string(STR("<!DOCTYPE root [ <![IGNORE[ <![INCLUDE["), parse_doctype).status == status_bad_doctype);
}

TEST(parse_doctype_stackless_group)
{
	std::basic_string<char_t> str;

	int count = 100000;

	str += STR("<!DOCTYPE ");

	for (int i = 0; i < count; ++i)
		str += STR("<!G ");

	for (int j = 0; j < count; ++j)
		str += STR(">");

	str += STR(">");

	xml_document doc;
	CHECK(doc.load_string(str.c_str(), parse_fragment));
}

TEST(parse_doctype_stackless_ignore)
{
	std::basic_string<char_t> str;

	int count = 100000;

	str += STR("<!DOCTYPE ");

	for (int i = 0; i < count; ++i)
		str += STR("<![IGNORE[ ");

	for (int j = 0; j < count; ++j)
		str += STR("]]>");

	str += STR(">");

	xml_document doc;
	CHECK(doc.load_string(str.c_str(), parse_fragment));
}
