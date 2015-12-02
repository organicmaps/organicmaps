#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

TEST(xpath_xalan_boolean_1)
{
	xml_node c;

	CHECK_XPATH_BOOLEAN(c, STR("true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true() and true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true() or true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(true())"), false);
	CHECK_XPATH_BOOLEAN(c, STR("boolean('')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1>2"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1>=2"), false);
	CHECK_XPATH_BOOLEAN(c, STR("false()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1=2"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1 = 1.00"), true);
	CHECK_XPATH_BOOLEAN(c, STR("0 = -0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1 = '001'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true()='0'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false()=''"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true()=2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false()=0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false() and false()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'foo' and 'fop'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true() and false()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("false() and true()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'1' and '0'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true() or false()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false() or true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false() or false()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("0 or ''"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(false())"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(false() = false())"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(true() = false())"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not('')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not('0')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("boolean('0')"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(0)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(-0)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(1)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(1 div 0)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("boolean(0 div 0)"), false);
}

TEST_XML(xpath_xalan_boolean_2, "<doc/>")
{
	CHECK_XPATH_BOOLEAN(doc, STR("boolean(doc)"), true);
	CHECK_XPATH_BOOLEAN(doc, STR("boolean(foo)"), false);
}

TEST(xpath_xalan_boolean_3)
{
	xml_node c;

	CHECK_XPATH_BOOLEAN(c, STR("1>1"), false);
	CHECK_XPATH_BOOLEAN(c, STR("2>1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1<2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1<1"), false);
	CHECK_XPATH_BOOLEAN(c, STR("2<1"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'2'>'1'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("0 > -0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("2>=2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2>=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1<=2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1<=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2<=1"), false);
	CHECK_XPATH_BOOLEAN(c, STR("false() and 1 div 0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("true() or 1 div 0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1!=1"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1!=2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1!=1.00"), false);
	CHECK_XPATH_BOOLEAN(c, STR("false()!=true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true()!=false()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("false()!=false()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'ace' != 'ace'"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'ace' != 'abc'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("'H' != '  H'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("'H' != 'H  '"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1.9999999 < 2.0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2.0000001 < 2.0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1.9999999 < 2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2 < 2.0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("'001' = 1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("0=false()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("'0'=true()"), true);
}

TEST_XML(xpath_xalan_boolean_4, "<avj><a>foo</a><b>bar</b><c>foobar</c><d>foo</d></avj>")
{
	CHECK_XPATH_BOOLEAN(doc, STR("avj/*='foo'"), true);
	CHECK_XPATH_BOOLEAN(doc, STR("not(avj/*='foo')"), false);
	CHECK_XPATH_BOOLEAN(doc, STR("avj/*!='foo'"), true);
	CHECK_XPATH_BOOLEAN(doc, STR("not(avj/*!='foo')"), false);

	CHECK_XPATH_BOOLEAN(doc, STR("avj/k='foo'"), false);
	CHECK_XPATH_BOOLEAN(doc, STR("not(avj/k='foo')"), true);
	CHECK_XPATH_BOOLEAN(doc, STR("avj/k!='foo'"), false);
	CHECK_XPATH_BOOLEAN(doc, STR("not(avj/k!='foo')"), true);
}

TEST_XML(xpath_xalan_boolean_5, "<doc><j l='12' w='33'>first</j><j l='17' w='45'>second</j><j l='16' w='78'>third</j><j l='12' w='33'>fourth</j></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] = j[@w='33']"), true);
	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] = j[@l='17']"), false);
	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] = j[.='first' or @w='45']"), true);

	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] != j[@w='33']"), true);
	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] != j[@l='17']"), true);
	CHECK_XPATH_BOOLEAN(c, STR("j[@l='12'] != j[.='first' or @w='45']"), true);
	CHECK_XPATH_BOOLEAN(c, STR("j[@l='16'] != j[@w='78']"), false);
}

TEST_XML(xpath_xalan_boolean_6, "<doc><avj><good><b>12</b><c>34</c><d>56</d><e>78</e></good></avj></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_BOOLEAN(c, STR("avj/good/*=34"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/good/*=34)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("avj/good/*!=34"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/good/*!=34)"), false);

	CHECK_XPATH_BOOLEAN(c, STR("34=avj/good/*"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(34=avj/good/*)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("34!=avj/good/*"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(34!=avj/good/*)"), false);
}

TEST_XML(xpath_xalan_boolean_7, "<doc><avj><bool><b>true</b><c></c><d>false?</d><e>1</e><f>0</f></bool></avj></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_BOOLEAN(c, STR("avj/bool/*=true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/bool/*=true())"), false);
	CHECK_XPATH_BOOLEAN(c, STR("avj/bool/*!=true()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/bool/*!=true())"), true);

	CHECK_XPATH_BOOLEAN(c, STR("true()=avj/bool/*"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(true()=avj/bool/*)"), false);
	CHECK_XPATH_BOOLEAN(c, STR("true()!=avj/bool/*"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(true()!=avj/bool/*)"), true);

	CHECK_XPATH_BOOLEAN(c, STR("avj/none/*=true()"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/none/*=true())"), true);
	CHECK_XPATH_BOOLEAN(c, STR("avj/none/*!=true()"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(avj/none/*!=true())"), false);

	CHECK_XPATH_BOOLEAN(c, STR("true()=avj/none/*"), false);
	CHECK_XPATH_BOOLEAN(c, STR("not(true()=avj/none/*)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("true()!=avj/none/*"), true);
	CHECK_XPATH_BOOLEAN(c, STR("not(true()!=avj/none/*)"), false);
}

TEST_XML(xpath_xalan_conditional, "<letters>b</letters>")
{
	xml_node c;

	CHECK_XPATH_BOOLEAN(c, STR("(round(3.7) > 3)"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2 > 1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("9 mod 3 = 0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("'a'='a'"), true);
	CHECK_XPATH_BOOLEAN(c, STR("2+2=4"), true);

	xml_node b = doc.child(STR("letters")).first_child();

	CHECK_XPATH_BOOLEAN(b, STR(".='b'"), true);
	CHECK_XPATH_BOOLEAN(b, STR("name(..)='letters'"), true);
}

TEST_XML(xpath_xalan_math_1, "<a>3</a>")
{
	xml_node c;

	CHECK_XPATH_NUMBER(c, STR("number('1')"), 1);
	CHECK_XPATH_NUMBER(c, STR("floor(0.0)"), 0);
	CHECK_XPATH_NUMBER(c, STR("ceiling(0.0)"), 0);
	CHECK_XPATH_NUMBER(c, STR("round(0.0)"), 0);
	CHECK_XPATH_NUMBER(c, STR("2*3"), 6);
	CHECK_XPATH_NUMBER(c, STR("3+6"), 9);
	CHECK_XPATH_NUMBER(c, STR("3-1"), 2);
	CHECK_XPATH_NUMBER_NAN(doc, STR("a-1")); // a-1 is a name test, not arithmetic expression
	CHECK_XPATH_NUMBER(doc, STR("a -1"), 2);
	CHECK_XPATH_NUMBER(c, STR("6 div 2"), 3);
	CHECK_XPATH_NUMBER(c, STR("5 mod 2"), 1);
	CHECK_XPATH_NUMBER_NAN(c, STR("number(n)"));
	CHECK_XPATH_NUMBER(c, STR("number(2)"), 2);
	CHECK_XPATH_NUMBER(c, STR("number('3')"), 3);
	CHECK_XPATH_NUMBER_NAN(c, STR("number('')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('abc')"));
	CHECK_XPATH_BOOLEAN(c, STR("number(string(1.0))=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("number(true())=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("number(false())=0"), true);

#ifndef MSVC6_NAN_BUG
	CHECK_XPATH_BOOLEAN(c, STR("number('xxx')=number('xxx')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("number('xxx')=0"), false);
#endif

	CHECK_XPATH_NUMBER(doc, STR("floor(a)"), 3);
	CHECK_XPATH_NUMBER(c, STR("floor(1.9)"), 1);
	CHECK_XPATH_NUMBER(c, STR("floor(2.999999)"), 2);
	CHECK_XPATH_NUMBER(c, STR("floor(-1.5)"), -2);
	CHECK_XPATH_BOOLEAN(c, STR("floor(1)=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("floor(1.9)=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("floor(-1.5)=-2"), true);
	CHECK_XPATH_NUMBER(doc, STR("ceiling(a)"), 3);
	CHECK_XPATH_NUMBER(c, STR("ceiling(1.54)"), 2);
	CHECK_XPATH_NUMBER(c, STR("ceiling(2.999999)"), 3);
	CHECK_XPATH_NUMBER(c, STR("ceiling(3.000001)"), 4);
	CHECK_XPATH_BOOLEAN(c, STR("ceiling(1)=1"), true);
	CHECK_XPATH_BOOLEAN(c, STR("ceiling(1.1)=2"), true);
	CHECK_XPATH_BOOLEAN(c, STR("ceiling(-1.5)=-1"), true);
}

TEST_XML(xpath_xalan_math_2, "<a>3</a>")
{
	xml_node c;

	CHECK_XPATH_NUMBER(doc, STR("round(a)"), 3);
	CHECK_XPATH_NUMBER(c, STR("round(1.24)"), 1);
	CHECK_XPATH_NUMBER(c, STR("round(2.999999)"), 3);
	CHECK_XPATH_NUMBER(c, STR("round(3.000001)"), 3);
	CHECK_XPATH_NUMBER(c, STR("round(1.1)"), 1);
	CHECK_XPATH_NUMBER(c, STR("round(-1.1)"), -1);
	CHECK_XPATH_NUMBER(c, STR("round(1.9)"), 2);
	CHECK_XPATH_NUMBER(c, STR("round(-1.9)"), -2);
	CHECK_XPATH_NUMBER(c, STR("round(1.5)"), 2);
	CHECK_XPATH_NUMBER(c, STR("round(-1.5)"), -1);
	CHECK_XPATH_NUMBER(c, STR("round(1.4999999)"), 1);
	CHECK_XPATH_NUMBER(c, STR("round(-1.4999999)"), -1);
	CHECK_XPATH_NUMBER(c, STR("round(1.5000001)"), 2);
	CHECK_XPATH_NUMBER(c, STR("round(-1.5000001)"), -2);
}

TEST_XML(xpath_xalan_math_3, "<doc><n v='1'/><n>2</n><n v='3'/><n>4</n><n v='5'>5</n><e>17</e><e>-5</e><e>8</e><e>-37</e></doc>")
{
	CHECK_XPATH_NUMBER(doc, STR("sum(doc/x)"), 0);
	CHECK_XPATH_NUMBER_NAN(doc, STR("sum(doc/n)"));
	CHECK_XPATH_NUMBER(doc, STR("sum(doc/n[text()])"), 11);
	CHECK_XPATH_NUMBER(doc, STR("sum(doc/n/@v)"), 9);
	CHECK_XPATH_NUMBER(doc, STR("sum(doc/e)"), -17);
}

TEST_XML(xpath_xalan_math_4, "<doc><n1 a='1'>2</n1><n2 a='2'>3</n2><n1-n2>123</n1-n2><n-1>72</n-1><n-2>12</n-2><div a='2'>5</div><mod a='5'>2</mod></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NUMBER(c, STR("n1*n2"), 6);
	CHECK_XPATH_NUMBER(c, STR("n1/@a*n2/@a"), 2);
	CHECK_XPATH_NUMBER(c, STR("(n1/@a)*(n2/@a)"), 2);
	CHECK_XPATH_NUMBER(c, STR("n1+n2"), 5);
	CHECK_XPATH_NUMBER(c, STR("n1/@a+n2/@a"), 3);
	CHECK_XPATH_NUMBER(c, STR("(n1/@a)+(n2/@a)"), 3);
	CHECK_XPATH_NUMBER(c, STR("1-2"), -1);
	CHECK_XPATH_NUMBER(c, STR("n1 - n2"), -1);
	CHECK_XPATH_NUMBER(c, STR("n1-n2"), 123);
	CHECK_XPATH_NUMBER(c, STR("n-1 - n-2"), 60);
	CHECK_XPATH_NUMBER(c, STR("n-1 -n-2"), 60);
	CHECK_XPATH_NUMBER(c, STR("7+-3"), 4);
	CHECK_XPATH_NUMBER(c, STR("n-1+-n-2"), 60);
	CHECK_XPATH_NUMBER(c, STR("7 - -3"), 10);
	CHECK_XPATH_NUMBER(c, STR("n-1 - -n-2"), 84);
	CHECK_XPATH_NUMBER(c, STR("-7 --3"), -4);
	CHECK_XPATH_NUMBER(c, STR("-n-1 --n-2"), -60);

	CHECK_XPATH_FAIL(STR("+7"));
	CHECK_XPATH_FAIL(STR("7++3"));
	CHECK_XPATH_FAIL(STR("7-+3"));

	CHECK_XPATH_NUMBER(c, STR("6 div -2"), -3);
	CHECK_XPATH_NUMBER(c, STR("n1 div n2"), 2.0 / 3.0);
	CHECK_XPATH_NUMBER(c, STR("div div mod"), 2.5);
	CHECK_XPATH_NUMBER(c, STR("div/@a div mod/@a"), 0.4);

	CHECK_XPATH_BOOLEAN(c, STR("1 div -0 = 2 div -0"), true);
	CHECK_XPATH_BOOLEAN(c, STR("1 div -0 = 1 div 0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("1 div -0 = -1 div 0"), true);

#ifndef MSVC6_NAN_BUG
	CHECK_XPATH_BOOLEAN(c, STR("0 div 0 >= 0"), false);
	CHECK_XPATH_BOOLEAN(c, STR("0 div 0 < 0"), false);
#endif

	CHECK_XPATH_NUMBER(c, STR("n1 mod n2"), 2);
	CHECK_XPATH_NUMBER(c, STR("div mod mod"), 1);
	CHECK_XPATH_NUMBER(c, STR("div/@a mod mod/@a"), 2);

	CHECK_XPATH_BOOLEAN(c, STR("(5 mod 2 = 1) and (5 mod -2 = 1) and (-5 mod 2 = -1) and (-5 mod -2 = -1)"), true);
}

TEST(xpath_xalan_math_5)
{
	xml_node c;

	CHECK_XPATH_NUMBER(c, STR("(((((('3'+5)*(3)+((('2')+2)*('1' - 6)))-('4' - '2'))+(-(4-6)))))"), 4);
	CHECK_XPATH_NUMBER(c, STR("1*1*2*2*2*3*3*1*1*1*0.5*0.5"), 18);
	CHECK_XPATH_NUMBER(c, STR("1440 div 2 div 2 div 6"), 60);
	CHECK_XPATH_NUMBER(c, STR("1440 div 2 div 2 div 6 div 10"), 6);
	CHECK_XPATH_NUMBER(c, STR("1440 div 2 div 2 div 6 div 10 div 3"), 2);
	CHECK_XPATH_NUMBER(c, STR("(1*2*3*4*5*6)div 2 div 6 div 10 div 3"), 2);
	CHECK_XPATH_NUMBER_NAN(c, STR("(2 + number('xxx'))"));
	CHECK_XPATH_NUMBER_NAN(c, STR("2 * -number('xxx')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("2 - number('xxx')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('xxx') - 3"));
	CHECK_XPATH_NUMBER_NAN(c, STR("2 div number('xxx')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('xxx') div 3"));

#ifndef __BORLANDC__ // BCC fmod does not propagate NaN correctly
	CHECK_XPATH_NUMBER_NAN(c, STR("2 mod number('xxx')"));
	CHECK_XPATH_NUMBER_NAN(c, STR("number('xxx') mod 3"));
#endif

	CHECK_XPATH_NUMBER_NAN(c, STR("floor(number('xxx'))"));
	CHECK_XPATH_NUMBER_NAN(c, STR("ceiling(number('xxx'))"));
	CHECK_XPATH_NUMBER_NAN(c, STR("round(number('xxx'))"));
	CHECK_XPATH_NUMBER(c, STR("10+5+25+20+15+50+35+40"), 200);
	CHECK_XPATH_NUMBER(c, STR("100-9-7-4-17-18-5"), 40);
	CHECK_XPATH_NUMBER(c, STR("3*2+5*4-4*2-1"), 17);
    CHECK_XPATH_NUMBER(c, STR("6*5-8*2+5*2"), 24);
    CHECK_XPATH_NUMBER(c, STR("10*5-4*2+6*1 -3*3"), 39);

    CHECK_XPATH_NUMBER(c, STR("(24 div 3 +2) div (40 div 8 -3)"), 5);
    CHECK_XPATH_NUMBER(c, STR("80 div 2 + 12 div 2 - 4 div 2"), 44);
    CHECK_XPATH_NUMBER(c, STR("70 div 10 - 18 div 6 + 10 div 2"), 9);

    CHECK_XPATH_NUMBER(c, STR("48 mod 17 - 2 mod 9 + 13 mod 5"), 15);
    CHECK_XPATH_NUMBER(c, STR("56 mod round(5*2+1.444) - 6 mod 4 + 7 mod 4"), 2);
    CHECK_XPATH_NUMBER(c, STR("(77 mod 10 + 5 mod 8) mod 10"), 2);
}

TEST_XML(xpath_xalan_math_6, "<doc><n1>3</n1><n2>7</n2><n3>x</n3></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NUMBER(c, STR("-(n1|n2)"), -3);
	CHECK_XPATH_NUMBER(c, STR("-(n2|n1)"), -3);
	CHECK_XPATH_BOOLEAN(c, STR("contains(number(n1), 'NaN')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("contains(number(n3), 'NaN')"), true);
}

TEST_XML(xpath_xalan_math_7, "<doc><n1>3</n1><n2>7</n2><n3>x</n3></doc>")
{
	xml_node c = doc.child(STR("doc"));

	CHECK_XPATH_NUMBER(c, STR("-(n1|n2)"), -3);
	CHECK_XPATH_NUMBER(c, STR("-(n2|n1)"), -3);
	CHECK_XPATH_BOOLEAN(c, STR("contains(number(n1), 'NaN')"), false);
	CHECK_XPATH_BOOLEAN(c, STR("contains(number(n3), 'NaN')"), true);
}

TEST_XML(xpath_xalan_math_8, "<k>0.0004</k>")
{
	CHECK_XPATH_NUMBER(doc, STR("number(1.75)"), 1.75);
	CHECK_XPATH_NUMBER(doc, STR("number(7 div 4)"), 1.75);
	CHECK_XPATH_BOOLEAN(doc, STR("(number(1.75) = (7 div 4))"), true);
	CHECK_XPATH_NUMBER(doc, STR("number(0.109375 * 16)"), 1.75);
	CHECK_XPATH_BOOLEAN(doc, STR("(number(1.75) = (0.109375 * 16))"), true);
	CHECK_XPATH_NUMBER(doc, STR("number(k)"), 0.0004);
	CHECK_XPATH_NUMBER(doc, STR("number(4 div 10000)"), 0.0004);

	// +0 works around extended precision in div on x86 (this is needed for some configurations in MinGW 3.4)
	CHECK_XPATH_BOOLEAN(doc, STR("(number(k) = (4 div 10000 + 0))"), true); 
	CHECK_XPATH_NUMBER(doc, STR("number(0.0001 * 4)"), 0.0004);
	CHECK_XPATH_BOOLEAN(doc, STR("(number(k) = (0.0001 * 4))"), true);
}

TEST(xpath_xalan_math_9)
{
	xml_node c;

	CHECK_XPATH_STRING(c, STR("string(number('0.0'))"), STR("0"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0'))"), STR("0"));

	CHECK_XPATH_STRING(c, STR("string(number('0.4'))"), STR("0.4"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.4'))"), STR("-0.4"));

	CHECK_XPATH_STRING(c, STR("string(number('4.0'))"), STR("4"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('4.0'))"), STR("-4"));

	CHECK_XPATH_STRING(c, STR("string(number('0.04'))"), STR("0.04"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.04'))"), STR("-0.04"));

	CHECK_XPATH_STRING(c, STR("string(number('0.004'))"), STR("0.004"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.004'))"), STR("-0.004"));

	CHECK_XPATH_STRING(c, STR("string(number('0.0004'))"), STR("0.0004"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0004'))"), STR("-0.0004"));

	CHECK_XPATH_STRING(c, STR("string(number('0.0000000000001'))"), STR("0.0000000000001"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0000000000001'))"), STR("-0.0000000000001"));

	CHECK_XPATH_STRING(c, STR("string(number('0.0000000000000000000000000001'))"), STR("0.0000000000000000000000000001"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0000000000000000000000000001'))"), STR("-0.0000000000000000000000000001"));

	CHECK_XPATH_STRING(c, STR("string(number('0.0000000000001000000000000001'))"), STR("0.0000000000001000000000000001"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0000000000001000000000000001'))"), STR("-0.0000000000001000000000000001"));

	CHECK_XPATH_STRING(c, STR("string(number('0.0012'))"), STR("0.0012"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.0012'))"), STR("-0.0012"));

	CHECK_XPATH_STRING(c, STR("string(number('0.012'))"), STR("0.012"));
	CHECK_XPATH_STRING(c, STR("string(-1 * number('0.012'))"), STR("-0.012"));
}

#endif
