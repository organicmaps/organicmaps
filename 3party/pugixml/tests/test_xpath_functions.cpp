#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST_XML(xpath_number_number, "<node>123</node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node")).first_child();

	// number with 0 arguments
	CHECK_XPATH_NUMBER_NAN(c, STR("number()"));
	CHECK_XPATH_NUMBER(n, STR("number()"), 123);

	// number with 1 string argument
	CHECK_XPATH_NUMBER(c, STR("number(' -123.456 ')"), -123.456);
	CHECK_XPATH_NUMBER(c, STR("number(' -123.')"), -123);
	CHECK_XPATH_NUMBER(c, STR("number('123.')"), 123);
	CHECK_XPATH_NUMBER(c, STR("number('.56')"), 0.56);
	CHECK_XPATH_NUMBER(c, STR("number('123 ')"), 123);
	CHECK_XPATH_NUMBER_NAN(c, STR("number('foobar')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('f1')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('1f')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('1.f')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('1.0f')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('123 f')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('.')"));

	// number with 1 bool argument
	CHECK_XPATH_NUMBER(c, STR("number(true())"), 1);
	CHECK_XPATH_NUMBER(c, STR("number(false())"), 0);

	// number with 1 node set argument
	CHECK_XPATH_NUMBER(n, STR("number(.)"), 123);

	// number with 1 number argument
	CHECK_XPATH_NUMBER(c, STR("number(1)"), 1);

	// number with 2 arguments
	CHECK_XPATH_FAIL(STR("number(1, 2)"));
}

TEST_XML(xpath_number_sum, "<node>123<child>789</child></node><node/>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// sum with 0 arguments
	CHECK_XPATH_FAIL(STR("sum()"));

	// sum with 1 argument
	CHECK_XPATH_NUMBER(c, STR("sum(.)"), 0);
	CHECK_XPATH_NUMBER(n, STR("sum(.)"), 123789); // 123 .. 789

	CHECK_XPATH_NUMBER(n, STR("sum(./descendant-or-self::node())"), 125490); // node + 123 + child + 789 = 123789 + 123 + 789 + 789 = 125490
	CHECK_XPATH_NUMBER(n, STR("sum(.//node())"), 1701); // 123 + child + 789 = 123 + 789 + 789
	CHECK_XPATH_NUMBER_NAN(doc.last_child(), STR("sum(.)"));

	// sum with 2 arguments
	CHECK_XPATH_FAIL(STR("sum(1, 2)"));

	// sum with 1 non-node-set argument
	CHECK_XPATH_FAIL(STR("sum(1)"));
}

TEST(xpath_number_floor)
{
	xml_node c;

	// floor with 0 arguments
	CHECK_XPATH_FAIL(STR("floor()"));

	// floor with 1 argument
	CHECK_XPATH_NUMBER(c, STR("floor(0)"), 0);
	CHECK_XPATH_NUMBER(c, STR("floor(1.2)"), 1);
	CHECK_XPATH_NUMBER(c, STR("floor(1)"), 1);
	CHECK_XPATH_NUMBER(c, STR("floor(-1.2)"), -2);
	CHECK_XPATH_NUMBER_NAN(c, STR("floor(string('nan'))"));
	CHECK_XPATH_STRING(c, STR("string(floor(1 div 0))"), STR("Infinity"));
	CHECK_XPATH_STRING(c, STR("string(floor(-1 div 0))"), STR("-Infinity"));

	// floor with 2 arguments
	CHECK_XPATH_FAIL(STR("floor(1, 2)"));

	// floor with argument 0 should return 0
	CHECK_XPATH_STRING(c, STR("string(1 div floor(0))"), STR("Infinity"));

	// floor with argument -0 should return -0
#if !(defined(__APPLE__) && defined(__MACH__)) // MacOS X gcc 4.0.1 implements floor incorrectly (floor never returns -0)
	CHECK_XPATH_STRING(c, STR("string(1 div floor(-0))"), STR("-Infinity"));
#endif
}

TEST(xpath_number_ceiling)
{
	xml_node c;

	// ceiling with 0 arguments
	CHECK_XPATH_FAIL(STR("ceiling()"));

	// ceiling with 1 argument
	CHECK_XPATH_NUMBER(c, STR("ceiling(0)"), 0);
	CHECK_XPATH_NUMBER(c, STR("ceiling(1.2)"), 2);
	CHECK_XPATH_NUMBER(c, STR("ceiling(1)"), 1);
	CHECK_XPATH_NUMBER(c, STR("ceiling(-1.2)"), -1);
	CHECK_XPATH_NUMBER_NAN(c, STR("ceiling(string('nan'))"));
	CHECK_XPATH_STRING(c, STR("string(ceiling(1 div 0))"), STR("Infinity"));
	CHECK_XPATH_STRING(c, STR("string(ceiling(-1 div 0))"), STR("-Infinity"));

	// ceiling with 2 arguments
	CHECK_XPATH_FAIL(STR("ceiling(1, 2)"));

	// ceiling with argument 0 should return 0
	CHECK_XPATH_STRING(c, STR("string(1 div ceiling(0))"), STR("Infinity"));

	// ceiling with argument in range (-1, -0] should result in minus zero
#if !(defined(__APPLE__) && defined(__MACH__)) && !defined(__CLR_VER) // MacOS X gcc 4.0.1 and x64 CLR implement ceil incorrectly (ceil never returns -0)
	CHECK_XPATH_STRING(c, STR("string(1 div ceiling(-0))"), STR("-Infinity"));
	CHECK_XPATH_STRING(c, STR("string(1 div ceiling(-0.1))"), STR("-Infinity"));
#endif
}

TEST(xpath_number_round)
{
	xml_node c;

	// round with 0 arguments
	CHECK_XPATH_FAIL(STR("round()"));

	// round with 1 argument
	CHECK_XPATH_NUMBER(c, STR("round(1.2)"), 1);
	CHECK_XPATH_NUMBER(c, STR("round(1.5)"), 2);
	CHECK_XPATH_NUMBER(c, STR("round(1.8)"), 2);
	CHECK_XPATH_NUMBER(c, STR("round(1)"), 1);
	CHECK_XPATH_NUMBER(c, STR("round(-1.2)"), -1);
	CHECK_XPATH_NUMBER(c, STR("round(-1.5)"), -1);
	CHECK_XPATH_NUMBER(c, STR("round(-1.6)"), -2);
	CHECK_XPATH_NUMBER_NAN(c, STR("round(string('nan'))"));
	CHECK_XPATH_STRING(c, STR("string(round(1 div 0))"), STR("Infinity"));
	CHECK_XPATH_STRING(c, STR("string(round(-1 div 0))"), STR("-Infinity"));

	// round with 2 arguments
	CHECK_XPATH_FAIL(STR("round(1, 2)"));

	// round with argument in range [-0.5, -0] should result in minus zero
	CHECK_XPATH_STRING(c, STR("string(1 div round(0))"), STR("Infinity"));

#if !(defined(__APPLE__) && defined(__MACH__)) && !defined(__CLR_VER) // MacOS X gcc 4.0.1 and x64 CLR implement ceil incorrectly (ceil never returns -0)
	CHECK_XPATH_STRING(c, STR("string(1 div round(-0.5))"), STR("-Infinity"));
	CHECK_XPATH_STRING(c, STR("string(1 div round(-0))"), STR("-Infinity"));
	CHECK_XPATH_STRING(c, STR("string(1 div round(-0.1))"), STR("-Infinity"));
#endif
}

TEST_XML(xpath_boolean_boolean, "<node />")
{
	xml_node c;

	// boolean with 0 arguments
	CHECK_XPATH_FAIL(STR("boolean()"));

	// boolean with 1 number argument
	CHECK_XPATH_BOOLEAN(c, STR("boolean(0)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(1)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(-1)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(0.1)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(number('nan'))"), false);

	// boolean with 1 string argument
	CHECK_XPATH_BOOLEAN(c, STR("boolean('x')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean('')"), false);

	// boolean with 1 node set argument
	CHECK_XPATH_BOOLEAN(c, STR("boolean(.)"), false);
	CHECK_XPATH_BOOLEAN(doc, STR("boolean(.)"), true);
	CHECK_XPATH_BOOLEAN(doc, STR("boolean(foo)"), false);

	// boolean with 2 arguments
	CHECK_XPATH_FAIL(STR("boolean(1, 2)"));
}

TEST(xpath_boolean_not)
{
	xml_node c;

	// not with 0 arguments
	CHECK_XPATH_FAIL(STR("not()"));

	// not with 1 argument
	CHECK_XPATH_BOOLEAN(c, STR("not(true())"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(false())"), true);

	// boolean with 2 arguments
	CHECK_XPATH_FAIL(STR("not(1, 2)"));
}

TEST(xpath_boolean_true)
{
	xml_node c;

	// true with 0 arguments
	CHECK_XPATH_BOOLEAN(c, STR("true()"), true);

	// true with 1 argument
	CHECK_XPATH_FAIL(STR("true(1)"));
}

TEST(xpath_boolean_false)
{
	xml_node c;

	// false with 0 arguments
	CHECK_XPATH_BOOLEAN(c, STR("false()"), false);

	// false with 1 argument
	CHECK_XPATH_FAIL(STR("false(1)"));
}

TEST_XML(xpath_boolean_lang, "<node xml:lang='en'><child xml:lang='zh-UK'><subchild attr=''/></child></node><foo><bar/></foo>")
{
	xml_node c;

	// lang with 0 arguments
	CHECK_XPATH_FAIL(STR("lang()"));

	// lang with 1 argument, no language
	CHECK_XPATH_BOOLEAN(c, STR("lang('en')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("foo")), STR("lang('en')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("foo")), STR("lang('')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("foo")).child(STR("bar")), STR("lang('en')"), false);

	// lang with 1 argument, same language/prefix
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")), STR("lang('en')"), true);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")), STR("lang('zh-uk')"), true);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")), STR("lang('zh')"), true);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")).child(STR("subchild")), STR("lang('zh')"), true);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")).child(STR("subchild")), STR("lang('ZH')"), true);

	// lang with 1 argument, different language/prefix
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")), STR("lang('')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")), STR("lang('e')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")), STR("lang('en')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")), STR("lang('zh-gb')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")), STR("lang('r')"), false);
	CHECK_XPATH_BOOLEAN(doc.child(STR("node")).child(STR("child")).child(STR("subchild")), STR("lang('en')"), false);

	// lang with 1 attribute argument
	CHECK_XPATH_NODESET(doc, STR("//@*[lang('en')]"));

	// lang with 2 arguments
	CHECK_XPATH_FAIL(STR("lang(1, 2)"));
}

TEST_XML(xpath_string_string, "<node>123<child id='1'>789</child><child><subchild><![CDATA[200]]></subchild></child>100</node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// string with 0 arguments
	CHECK_XPATH_STRING(c, STR("string()"), STR(""));
	CHECK_XPATH_STRING(n.child(STR("child")), STR("string()"), STR("789"));

	// string with 1 node-set argument
	CHECK_XPATH_STRING(n, STR("string(child)"), STR("789"));
	CHECK_XPATH_STRING(n, STR("string(child/@id)"), STR("1"));
	CHECK_XPATH_STRING(n, STR("string(.)"), STR("123789200100"));

	// string with 1 number argument
	CHECK_XPATH_STRING(c, STR("string(0 div 0)"), STR("NaN"));
	CHECK_XPATH_STRING(c, STR("string(0)"), STR("0"));
	CHECK_XPATH_STRING(c, STR("string(-0)"), STR("0"));
	CHECK_XPATH_STRING(c, STR("string(1 div 0)"), STR("Infinity"));
	CHECK_XPATH_STRING(c, STR("string(-1 div -0)"), STR("Infinity"));
	CHECK_XPATH_STRING(c, STR("string(-1 div 0)"), STR("-Infinity"));
	CHECK_XPATH_STRING(c, STR("string(1 div -0)"), STR("-Infinity"));
	CHECK_XPATH_STRING(c, STR("string(1234567)"), STR("1234567"));
	CHECK_XPATH_STRING(c, STR("string(-1234567)"), STR("-1234567"));
	CHECK_XPATH_STRING(c, STR("string(1234.5678)"), STR("1234.5678"));
	CHECK_XPATH_STRING(c, STR("string(-1234.5678)"), STR("-1234.5678"));
	CHECK_XPATH_STRING(c, STR("string(0.5678)"), STR("0.5678"));
	CHECK_XPATH_STRING(c, STR("string(-0.5678)"), STR("-0.5678"));
	CHECK_XPATH_STRING(c, STR("string(0.0)"), STR("0"));
	CHECK_XPATH_STRING(c, STR("string(-0.0)"), STR("0"));

	// string with 1 boolean argument
	CHECK_XPATH_STRING(c, STR("string(true())"), STR("true"));
	CHECK_XPATH_STRING(c, STR("string(false())"), STR("false"));

	// string with 1 string argument
	CHECK_XPATH_STRING(c, STR("string('abc')"), STR("abc"));

	// string with 2 arguments
	CHECK_XPATH_FAIL(STR("string(1, 2)"));
}

TEST(xpath_string_concat)
{
	xml_node c;

	// concat with 0 arguments
	CHECK_XPATH_FAIL(STR("concat()"));

	// concat with 1 argument
	CHECK_XPATH_FAIL(STR("concat('')"));

	// concat with exactly 2 arguments
	CHECK_XPATH_STRING(c, STR("concat('prev','next')"), STR("prevnext"));
	CHECK_XPATH_STRING(c, STR("concat('','next')"), STR("next"));
	CHECK_XPATH_STRING(c, STR("concat('prev','')"), STR("prev"));

	// concat with 3 or more arguments
	CHECK_XPATH_STRING(c, STR("concat('a', 'b', 'c')"), STR("abc"));
	CHECK_XPATH_STRING(c, STR("concat('a', 'b', 'c', 'd')"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("concat('a', 'b', 'c', 'd', 'e')"), STR("abcde"));
	CHECK_XPATH_STRING(c, STR("concat('a', 'b', 'c', 'd', 'e', 'f')"), STR("abcdef"));
	CHECK_XPATH_STRING(c, STR("concat('a', 'b', 'c', 'd', 'e', 'f', 'g')"), STR("abcdefg"));
	CHECK_XPATH_STRING(c, STR("concat(1, 2, 3, 4, 5, 6, 7, 8)"), STR("12345678"));
}

TEST(xpath_string_starts_with)
{
	xml_node c;

	// starts-with with 0 arguments
	CHECK_XPATH_FAIL(STR("starts-with()"));

	// starts-with with 1 argument
	CHECK_XPATH_FAIL(STR("starts-with('a')"));

	// starts-with with 2 arguments
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('abc', '')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('abc', 'a')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('abc', 'abc')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('abc', 'abcd')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('bc', 'c')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('', 'c')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("starts-with('', '')"), true);

	// starts-with with 3 arguments
	CHECK_XPATH_FAIL(STR("starts-with('a', 'b', 'c')"));
}

TEST(xpath_string_contains)
{
	xml_node c;

	// contains with 0 arguments
	CHECK_XPATH_FAIL(STR("contains()"));

	// contains with 1 argument
	CHECK_XPATH_FAIL(STR("contains('a')"));

	// contains with 2 arguments
	CHECK_XPATH_BOOLEAN(c, STR("contains('abc', '')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("contains('abc', 'a')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("contains('abc', 'abc')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("contains('abcd', 'bc')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("contains('abc', 'abcd')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("contains('b', 'bc')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("contains('', 'c')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("contains('', '')"), true);

	// contains with 3 arguments
	CHECK_XPATH_FAIL(STR("contains('a', 'b', 'c')"));
}

TEST(xpath_string_substring_before)
{
	xml_node c;

	// substring-before with 0 arguments
	CHECK_XPATH_FAIL(STR("substring-before()"));

	// substring-before with 1 argument
	CHECK_XPATH_FAIL(STR("substring-before('a')"));

	// substring-before with 2 arguments
	CHECK_XPATH_STRING(c, STR("substring-before('abc', 'abc')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-before('abc', 'a')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-before('abc', 'cd')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-before('abc', 'b')"), STR("a"));
	CHECK_XPATH_STRING(c, STR("substring-before('abc', 'c')"), STR("ab"));
	CHECK_XPATH_STRING(c, STR("substring-before('abc', '')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-before('', '')"), STR(""));

	// substring-before with 2 arguments, from W3C standard
	CHECK_XPATH_STRING(c, STR("substring-before(\"1999/04/01\",\"/\")"), STR("1999"));

	// substring-before with 3 arguments
	CHECK_XPATH_FAIL(STR("substring-before('a', 'b', 'c')"));
}

TEST(xpath_string_substring_after)
{
	xml_node c;

	// substring-after with 0 arguments
	CHECK_XPATH_FAIL(STR("substring-after()"));

	// substring-after with 1 argument
	CHECK_XPATH_FAIL(STR("substring-after('a')"));

	// substring-after with 2 arguments
	CHECK_XPATH_STRING(c, STR("substring-after('abc', 'abc')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-after('abc', 'a')"), STR("bc"));
	CHECK_XPATH_STRING(c, STR("substring-after('abc', 'cd')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-after('abc', 'b')"), STR("c"));
	CHECK_XPATH_STRING(c, STR("substring-after('abc', 'c')"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring-after('abc', '')"), STR("abc"));
	CHECK_XPATH_STRING(c, STR("substring-after('', '')"), STR(""));

	// substring-before with 2 arguments, from W3C standard
	CHECK_XPATH_STRING(c, STR("substring-after(\"1999/04/01\",\"/\")"), STR("04/01"));
	CHECK_XPATH_STRING(c, STR("substring-after(\"1999/04/01\",\"19\")"), STR("99/04/01"));

	// substring-after with 3 arguments
	CHECK_XPATH_FAIL(STR("substring-after('a', 'b', 'c')"));
}

TEST_XML(xpath_string_substring_after_heap, "<node>foo<child/>bar</node>")
{
    CHECK_XPATH_STRING(doc, STR("substring-after(node, 'fo')"), STR("obar"));
    CHECK_XPATH_STRING(doc, STR("substring-after(node, 'fooba')"), STR("r"));
    CHECK_XPATH_STRING(doc, STR("substring-after(node, 'foobar')"), STR(""));
}

TEST(xpath_string_substring)
{
	xml_node c;

	// substring with 0 arguments
	CHECK_XPATH_FAIL(STR("substring()"));

	// substring with 1 argument
	CHECK_XPATH_FAIL(STR("substring('')"));

	// substring with 2 arguments
	CHECK_XPATH_STRING(c, STR("substring('abcd', 2)"), STR("bcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1.1)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1.5)"), STR("bcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1.8)"), STR("bcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 10)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 0)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -1 div 0)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1 div 0)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 0 div 0)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('', 1)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('', 0)"), STR(""));
    CHECK_XPATH_STRING(c, STR("substring(substring('internalexternalcorrect substring',9),9)"), STR("correct substring"));

	// substring with 3 arguments
	CHECK_XPATH_STRING(c, STR("substring('abcd', 2, 1)"), STR("b"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 2, 2)"), STR("bc"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1, 0)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1, 0.4)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1, 0.5)"), STR("a"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 10, -5)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 0, -1)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 100)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 101)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 102)"), STR("a"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 103)"), STR("ab"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 104)"), STR("abc"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 105)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 106)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -100, 1 div 0)"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("substring('abcd', -1 div 0, 4)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 1 div 0, 0 div 0)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('abcd', 0 div 0, 1)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('', 1, 2)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('', 0, 0)"), STR(""));

	// substring with 3 arguments, from W3C standard
	CHECK_XPATH_STRING(c, STR("substring('12345', 1.5, 2.6)"), STR("234"));
	CHECK_XPATH_STRING(c, STR("substring('12345', 0, 3)"), STR("12"));
	CHECK_XPATH_STRING(c, STR("substring('12345', 0 div 0, 3)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('12345', 1, 0 div 0)"), STR(""));
	CHECK_XPATH_STRING(c, STR("substring('12345', -42, 1 div 0)"), STR("12345"));
	CHECK_XPATH_STRING(c, STR("substring('12345', -1 div 0, 1 div 0)"), STR(""));

	// substring with 4 arguments
	CHECK_XPATH_FAIL(STR("substring('', 1, 2, 3)"));
}

TEST_XML(xpath_string_substring_heap, "<node>foo<child/>bar</node>")
{
    CHECK_XPATH_STRING(doc, STR("substring(node, 3)"), STR("obar"));
    CHECK_XPATH_STRING(doc, STR("substring(node, 6)"), STR("r"));
    CHECK_XPATH_STRING(doc, STR("substring(node, 7)"), STR(""));
}

TEST_XML(xpath_string_string_length, "<node>123</node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// string-length with 0 arguments
	CHECK_XPATH_NUMBER(c, STR("string-length()"), 0);
	CHECK_XPATH_NUMBER(n, STR("string-length()"), 3);

	// string-length with 1 argument
	CHECK_XPATH_NUMBER(c, STR("string-length('')"), 0);
	CHECK_XPATH_NUMBER(c, STR("string-length('a')"), 1);
	CHECK_XPATH_NUMBER(c, STR("string-length('abcdef')"), 6);

	// string-length with 2 arguments
	CHECK_XPATH_FAIL(STR("string-length(1, 2)"));
}

TEST_XML_FLAGS(xpath_string_normalize_space, "<node> \t\r\rval1  \rval2\r\nval3\nval4\r\r</node>", parse_minimal)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// normalize-space with 0 arguments
	CHECK_XPATH_STRING(c, STR("normalize-space()"), STR(""));
	CHECK_XPATH_STRING(n, STR("normalize-space()"), STR("val1 val2 val3 val4"));

	// normalize-space with 1 argument
	CHECK_XPATH_STRING(c, STR("normalize-space('')"), STR(""));
	CHECK_XPATH_STRING(c, STR("normalize-space('abcd')"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("normalize-space(' \r\nabcd')"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("normalize-space('abcd \n\r')"), STR("abcd"));
	CHECK_XPATH_STRING(c, STR("normalize-space('ab\r\n\tcd')"), STR("ab cd"));
	CHECK_XPATH_STRING(c, STR("normalize-space('ab    cd')"), STR("ab cd"));
	CHECK_XPATH_STRING(c, STR("normalize-space('\07')"), STR("\07"));

	// normalize-space with 2 arguments
	CHECK_XPATH_FAIL(STR("normalize-space(1, 2)"));
}

TEST(xpath_string_translate)
{
	xml_node c;

	// translate with 0 arguments
	CHECK_XPATH_FAIL(STR("translate()"));

	// translate with 1 argument
	CHECK_XPATH_FAIL(STR("translate('a')"));

	// translate with 2 arguments
	CHECK_XPATH_FAIL(STR("translate('a', 'b')"));

	// translate with 3 arguments
	CHECK_XPATH_STRING(c, STR("translate('abc', '', '')"), STR("abc"));
	CHECK_XPATH_STRING(c, STR("translate('abc', '', 'foo')"), STR("abc"));
	CHECK_XPATH_STRING(c, STR("translate('abc', 'ab', 'ba')"), STR("bac"));
	CHECK_XPATH_STRING(c, STR("translate('abc', 'ab', 'f')"), STR("fc"));
	CHECK_XPATH_STRING(c, STR("translate('abc', 'aabb', '1234')"), STR("13c"));
	CHECK_XPATH_STRING(c, STR("translate('', 'abc', 'bac')"), STR(""));

	// translate with 3 arguments, from W3C standard
	CHECK_XPATH_STRING(c, STR("translate('bar','abc','ABC')"), STR("BAr"));
	CHECK_XPATH_STRING(c, STR("translate('--aaa--','abc-','ABC')"), STR("AAA"));

	// translate with 4 arguments
	CHECK_XPATH_FAIL(STR("translate('a', 'b', 'c', 'd')"));
}

TEST(xpath_string_translate_table)
{
	xml_node c;

	CHECK_XPATH_STRING(c, STR("translate('abcd\xe9 ', 'abc', 'ABC')"), STR("ABCd\xe9 "));
	CHECK_XPATH_STRING(c, STR("translate('abcd\xe9 ', 'abc\xe9', 'ABC!')"), STR("ABCd! "));
	CHECK_XPATH_STRING(c, STR("translate('abcde', concat('abc', 'd'), 'ABCD')"), STR("ABCDe"));
	CHECK_XPATH_STRING(c, STR("translate('abcde', 'abcd', concat('ABC', 'D'))"), STR("ABCDe"));
}

TEST(xpath_string_translate_remove)
{
	xml_node c;

	CHECK_XPATH_STRING(c, STR("translate('000000755', '0', '')"), STR("755"));
	CHECK_XPATH_STRING(c, STR("translate('000000755', concat('0', ''), '')"), STR("755"));
}

TEST_XML(xpath_nodeset_last, "<node><c1/><c1/><c2/><c3/><c3/><c3/><c3/></node>")
{
	xml_node n = doc.child(STR("node"));

	// last with 0 arguments
	CHECK_XPATH_NUMBER(n, STR("last()"), 1);
	CHECK_XPATH_NODESET(n, STR("c1[last() = 1]"));
	CHECK_XPATH_NODESET(n, STR("c1[last() = 2]")) % 3 % 4; // c1, c1
	CHECK_XPATH_NODESET(n, STR("c2/preceding-sibling::node()[last() = 2]")) % 4 % 3; // c1, c1

	// last with 1 argument
	CHECK_XPATH_FAIL(STR("last(c)"));
}

TEST_XML(xpath_nodeset_position, "<node><c1/><c1/><c2/><c3/><c3/><c3/><c3/></node>")
{
	xml_node n = doc.child(STR("node"));

	// position with 0 arguments
	CHECK_XPATH_NUMBER(n, STR("position()"), 1);
	CHECK_XPATH_NODESET(n, STR("c1[position() = 0]"));
	CHECK_XPATH_NODESET(n, STR("c1[position() = 1]")) % 3;
	CHECK_XPATH_NODESET(n, STR("c1[position() = 2]")) % 4;
	CHECK_XPATH_NODESET(n, STR("c1[position() = 3]"));
	CHECK_XPATH_NODESET(n, STR("c2/preceding-sibling::node()[position() = 1]")) % 4;
	CHECK_XPATH_NODESET(n, STR("c2/preceding-sibling::node()[position() = 2]")) % 3;

	// position with 1 argument
	CHECK_XPATH_FAIL(STR("position(c)"));
}

TEST_XML(xpath_nodeset_count, "<node><c1/><c1/><c2/><c3/><c3/><c3/><c3/></node>")
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// count with 0 arguments
	CHECK_XPATH_FAIL(STR("count()"));

	// count with 1 non-node-set argument
	CHECK_XPATH_FAIL(STR("count(1)"));
	CHECK_XPATH_FAIL(STR("count(true())"));
	CHECK_XPATH_FAIL(STR("count('')"));

	// count with 1 node-set argument
	CHECK_XPATH_NUMBER(c, STR("count(.)"), 0);
	CHECK_XPATH_NUMBER(n, STR("count(.)"), 1);
	CHECK_XPATH_NUMBER(n, STR("count(c1)"), 2);
	CHECK_XPATH_NUMBER(n, STR("count(c2)"), 1);
	CHECK_XPATH_NUMBER(n, STR("count(c3)"), 4);
	CHECK_XPATH_NUMBER(n, STR("count(c4)"), 0);

	// count with 2 arguments
	CHECK_XPATH_FAIL(STR("count(x, y)"));
}

TEST_XML(xpath_nodeset_id, "<node id='foo'/>")
{
	xml_node n = doc.child(STR("node"));

	// id with 0 arguments
	CHECK_XPATH_FAIL(STR("id()"));

	// id with 1 argument - no DTD => no id
	CHECK_XPATH_NODESET(n, STR("id('foo')"));

	// id with 2 arguments
	CHECK_XPATH_FAIL(STR("id(1, 2)"));
}

TEST_XML_FLAGS(xpath_nodeset_local_name, "<node xmlns:foo='http://foo'><c1>text</c1><c2 xmlns:foo='http://foo2' foo:attr='value'><foo:child/></c2><c3 xmlns='http://def' attr='value'><child/></c3><c4><?target stuff?></c4></node>", parse_default | parse_pi)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// local-name with 0 arguments
	CHECK_XPATH_STRING(c, STR("local-name()"), STR(""));
	CHECK_XPATH_STRING(n, STR("local-name()"), STR("node"));

	// local-name with 1 non-node-set argument
	CHECK_XPATH_FAIL(STR("local-name(1)"));

	// local-name with 1 node-set argument
	CHECK_XPATH_STRING(n, STR("local-name(c1)"), STR("c1"));
	CHECK_XPATH_STRING(n, STR("local-name(c2/node())"), STR("child"));
	CHECK_XPATH_STRING(n, STR("local-name(c2/attribute::node())"), STR("attr"));
	CHECK_XPATH_STRING(n, STR("local-name(c1/node())"), STR(""));
	CHECK_XPATH_STRING(n, STR("local-name(c4/node())"), STR("target"));
	CHECK_XPATH_STRING(n, STR("local-name(c1/following-sibling::node())"), STR("c2"));
	CHECK_XPATH_STRING(n, STR("local-name(c4/preceding-sibling::node())"), STR("c1"));

	// local-name with 2 arguments
	CHECK_XPATH_FAIL(STR("local-name(c1, c2)"));
}

TEST_XML_FLAGS(xpath_nodeset_namespace_uri, "<node xmlns:foo='http://foo'><c1>text</c1><c2 xmlns:foo='http://foo2' foo:attr='value'><foo:child/></c2><c3 xmlns='http://def' attr='value'><child/></c3><c4><?target stuff?></c4><c5><foo:child/></c5><c6 bar:attr=''/><c7><node foo:attr=''/></c7></node>", parse_default | parse_pi)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// namespace-uri with 0 arguments
	CHECK_XPATH_STRING(c, STR("namespace-uri()"), STR(""));
	CHECK_XPATH_STRING(n.child(STR("c2")).child(STR("foo:child")), STR("namespace-uri()"), STR("http://foo2"));

	// namespace-uri with 1 non-node-set argument
	CHECK_XPATH_FAIL(STR("namespace-uri(1)"));

	// namespace-uri with 1 node-set argument
	CHECK_XPATH_STRING(n, STR("namespace-uri(c1)"), STR(""));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c5/child::node())"), STR("http://foo"));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c2/attribute::node())"), STR("http://foo2"));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c2/child::node())"), STR("http://foo2"));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c1/child::node())"), STR(""));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c4/child::node())"), STR(""));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c3)"), STR("http://def"));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c3/@attr)"), STR("")); // the namespace name for an unprefixed attribute name always has no value (Namespaces in XML 1.0)
	CHECK_XPATH_STRING(n, STR("namespace-uri(c3/child::node())"), STR("http://def"));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c6/@bar:attr)"), STR(""));
	CHECK_XPATH_STRING(n, STR("namespace-uri(c7/node/@foo:attr)"), STR("http://foo"));

	// namespace-uri with 2 arguments
	CHECK_XPATH_FAIL(STR("namespace-uri(c1, c2)"));
}

TEST_XML_FLAGS(xpath_nodeset_name, "<node xmlns:foo='http://foo'><c1>text</c1><c2 xmlns:foo='http://foo2' foo:attr='value'><foo:child/></c2><c3 xmlns='http://def' attr='value'><child/></c3><c4><?target stuff?></c4></node>", parse_default | parse_pi)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	// name with 0 arguments
	CHECK_XPATH_STRING(c, STR("name()"), STR(""));
	CHECK_XPATH_STRING(n, STR("name()"), STR("node"));

	// name with 1 non-node-set argument
	CHECK_XPATH_FAIL(STR("name(1)"));

	// name with 1 node-set argument
	CHECK_XPATH_STRING(n, STR("name(c1)"), STR("c1"));
	CHECK_XPATH_STRING(n, STR("name(c2/node())"), STR("foo:child"));
	CHECK_XPATH_STRING(n, STR("name(c2/attribute::node())"), STR("foo:attr"));
	CHECK_XPATH_STRING(n, STR("name(c1/node())"), STR(""));
	CHECK_XPATH_STRING(n, STR("name(c4/node())"), STR("target"));
	CHECK_XPATH_STRING(n, STR("name(c1/following-sibling::node())"), STR("c2"));
	CHECK_XPATH_STRING(n, STR("name(c4/preceding-sibling::node())"), STR("c1"));

	// name with 2 arguments
	CHECK_XPATH_FAIL(STR("name(c1, c2)"));
}

TEST(xpath_function_arguments)
{
	xml_node c;

	// conversion to string
	CHECK_XPATH_NUMBER(c, STR("string-length(12)"), 2);

	// conversion to number
	CHECK_XPATH_NUMBER(c, STR("round('1.2')"), 1);
	CHECK_XPATH_NUMBER(c, STR("round('1.7')"), 2);

	// conversion to boolean
	CHECK_XPATH_BOOLEAN(c, STR("not('1')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not('')"), true);

	// conversion to node set
	CHECK_XPATH_FAIL(STR("sum(1)"));

	// expression evaluation
	CHECK_XPATH_NUMBER(c, STR("round((2 + 2 * 2) div 4)"), 2);

	// empty expressions
	CHECK_XPATH_FAIL(STR("round(,)"));
	CHECK_XPATH_FAIL(STR("substring(,)"));
	CHECK_XPATH_FAIL(STR("substring('a',)"));
	CHECK_XPATH_FAIL(STR("substring(,'a')"));

	// extra commas
	CHECK_XPATH_FAIL(STR("round(,1)"));
	CHECK_XPATH_FAIL(STR("round(1,)"));

	// lack of commas
	CHECK_XPATH_FAIL(STR("substring(1 2)"));

	// whitespace after function name
	CHECK_XPATH_BOOLEAN(c, STR("true ()"), true);

	// too many arguments
	CHECK_XPATH_FAIL(STR("round(1, 2, 3, 4, 5, 6)"));
}

TEST_XML_FLAGS(xpath_string_value, "<node><c1>pcdata</c1><c2><child/></c2><c3 attr='avalue'/><c4><?target pivalue?></c4><c5><!--comment--></c5><c6><![CDATA[cdata]]></c6></node>", parse_default | parse_pi | parse_comments)
{
	xml_node c;
	xml_node n = doc.child(STR("node"));

	CHECK_XPATH_STRING(c, STR("string()"), STR(""));
	CHECK_XPATH_STRING(doc, STR("string()"), STR("pcdatacdata"));
	CHECK_XPATH_STRING(n, STR("string()"), STR("pcdatacdata"));
	CHECK_XPATH_STRING(n, STR("string(c1/node())"), STR("pcdata"));
	CHECK_XPATH_STRING(n, STR("string(c2/node())"), STR(""));
	CHECK_XPATH_STRING(n, STR("string(c3/@attr)"), STR("avalue"));
	CHECK_XPATH_STRING(n, STR("string(c4/node())"), STR("pivalue"));
	CHECK_XPATH_STRING(n, STR("string(c5/node())"), STR("comment"));
	CHECK_XPATH_STRING(n, STR("string(c6/node())"), STR("cdata"));
}

TEST(xpath_string_value_empty)
{
	xml_document doc;
	doc.append_child(node_pcdata).set_value(STR("head"));
	doc.append_child(node_pcdata);
	doc.append_child(node_pcdata).set_value(STR("tail"));

	CHECK_XPATH_STRING(doc, STR("string()"), STR("headtail"));
}

TEST_XML(xpath_string_concat_translate, "<node>foobar</node>")
{
	CHECK_XPATH_STRING(doc, STR("concat('a', 'b', 'c', translate(node, 'o', 'a'), 'd')"), STR("abcfaabard"));
}

#endif
