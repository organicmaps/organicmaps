#ifndef PUGIXML_NO_XPATH

#include "common.hpp"

#include <string>

TEST(xpath_literal_parse)
{
	xml_node c;
	CHECK_XPATH_STRING(c, STR("'a\"b'"), STR("a\"b"));
	CHECK_XPATH_STRING(c, STR("\"a'b\""), STR("a'b"));
	CHECK_XPATH_STRING(c, STR("\"\""), STR(""));
	CHECK_XPATH_STRING(c, STR("\'\'"), STR(""));
}

TEST(xpath_literal_error)
{
	CHECK_XPATH_FAIL(STR("\""));
	CHECK_XPATH_FAIL(STR("\"foo"));
	CHECK_XPATH_FAIL(STR("\'"));
	CHECK_XPATH_FAIL(STR("\'bar"));
}

TEST(xpath_number_parse)
{
	xml_node c;
	CHECK_XPATH_NUMBER(c, STR("0"), 0);
	CHECK_XPATH_NUMBER(c, STR("123"), 123);
	CHECK_XPATH_NUMBER(c, STR("123.456"), 123.456);
	CHECK_XPATH_NUMBER(c, STR(".123"), 0.123);
	CHECK_XPATH_NUMBER(c, STR("123.4567890123456789012345"), 123.4567890123456789012345);
	CHECK_XPATH_NUMBER(c, STR("123."), 123);
}

TEST(xpath_number_error)
{
	CHECK_XPATH_FAIL(STR("123a"));
	CHECK_XPATH_FAIL(STR("123.a"));
	CHECK_XPATH_FAIL(STR(".123a"));
}

TEST(xpath_variables)
{
	CHECK_XPATH_FAIL(STR("$var")); // no variable var
	CHECK_XPATH_FAIL(STR("$1"));
	CHECK_XPATH_FAIL(STR("$"));
}

TEST(xpath_empty_expression)
{
	CHECK_XPATH_FAIL(STR(""));
}

TEST(xpath_lexer_error)
{
	CHECK_XPATH_FAIL(STR("!"));
	CHECK_XPATH_FAIL(STR("&"));
}

TEST(xpath_unmatched_braces)
{
	CHECK_XPATH_FAIL(STR("node["));
	CHECK_XPATH_FAIL(STR("node[1"));
	CHECK_XPATH_FAIL(STR("node[]]"));
	CHECK_XPATH_FAIL(STR("node("));
	CHECK_XPATH_FAIL(STR("node(()"));
	CHECK_XPATH_FAIL(STR("(node)[1"));
	CHECK_XPATH_FAIL(STR("(1"));
}

TEST(xpath_incorrect_step)
{
	CHECK_XPATH_FAIL(STR("child::1"));
	CHECK_XPATH_FAIL(STR("something::*"));
	CHECK_XPATH_FAIL(STR("a::*"));
	CHECK_XPATH_FAIL(STR("c::*"));
	CHECK_XPATH_FAIL(STR("d::*"));
	CHECK_XPATH_FAIL(STR("f::*"));
	CHECK_XPATH_FAIL(STR("n::*"));
	CHECK_XPATH_FAIL(STR("p::*"));
}

TEST(xpath_semantics_error)
{
	CHECK_XPATH_FAIL(STR("1[1]"));
	CHECK_XPATH_FAIL(STR("1 | 1"));
}

TEST(xpath_semantics_posinv) // coverage for contains()
{
	xpath_query(STR("(node)[substring(1, 2, 3)]"));
	xpath_query(STR("(node)[concat(1, 2, 3, 4)]"));
	xpath_query(STR("(node)[count(foo)]"));
	xpath_query(STR("(node)[local-name()]"));
	xpath_query(STR("(node)[(node)[1]]"));
}

TEST(xpath_parse_paths_valid)
{
    const char_t* paths[] =
	{
		// From Jaxen tests
		STR("foo[.='bar']"), STR("foo[.!='bar']"), STR("/"), STR("*"), STR("//foo"), STR("/*"), STR("/."), STR("/foo[/bar[/baz]]"),
		STR("/foo/bar/baz[(1 or 2) + 3 * 4 + 8 and 9]"), STR("/foo/bar/baz"), STR("(.)[1]"), STR("self::node()"), STR("."), STR("count(/)"),
		STR("foo[1]"), STR("/baz[(1 or 2) + 3 * 4 + 8 and 9]"), STR("foo/bar[/baz[(1 or 2) - 3 mod 4 + 8 and 9 div 8]]"),
		STR("foo/bar/yeah:baz[a/b/c and toast]"), STR("/foo/bar[../x='123']"), STR("/foo[@bar='1234']"), STR("foo|bar"),
		STR("/foo|/bar[@id='1234']"), STR("count(//author/attribute::*)"), STR("/child::node()/child::node()[@id='_13563275']"),
		STR("10 + (count(descendant::author) * 5)"), STR("10 + count(descendant::author) * 5"), STR("2 + (2 * 5)"), STR("//foo:bar"),
		STR("count(//author)+5"), STR("count(//author)+count(//author/attribute::*)"), STR("/foo/bar[@a='1' and @c!='2']"),
		STR("12 + (count(//author)+count(//author/attribute::*)) div 2"), STR("text()[.='foo']"), STR("/*/*[@id='123']")
        STR("/foo/bar[@a='1' and @b='2']"), STR("/foo/bar[@a='1' and @b!='2']"), STR("//attribute::*[.!='crunchy']"),
        STR("'//*[contains(string(text()),\"yada yada\")]'"),

		// From ajaxslt tests
		STR("@*"), STR("@*|node()"), STR("/descendant-or-self::div"), STR("/div"), STR("//div"), STR("/descendant-or-self::node()/child::para"),
		STR("substring('12345', 0, 3)"), STR("//title | //link"), STR("x//title"), STR("x/title"), STR("id('a')//title"), STR("//*[@about]"),
		STR("count(descendant::*)"), STR("count(descendant::*) + count(ancestor::*)"), STR("@*|text()"), STR("*|/"), STR("source|destination"),
		STR("page != 'to' and page != 'from'"), STR("substring-after(icon/@image, '/mapfiles/marker')"), STR("substring-before(str, c)"), STR("page = 'from'"),
		STR("segments/@time"), STR("child::para"), STR("child::*"), STR("child::text()"), STR("child::node()"), STR("attribute::name"), STR("attribute::*"),
		STR("descendant::para"), STR("ancestor::div"), STR("ancestor-or-self::div"), STR("descendant-or-self::para"), STR("self::para"), STR("child::*/child::para"),
		STR("concat(substring-before(@image,'marker'),'icon',substring-after(@image,'marker'))"), STR("/"), STR("/descendant::para"), STR("/descendant::olist/child::item"),
		STR("child::para[position()=1]"), STR("child::para[position()=last()]"), STR("child::para[position()=last()-1]"), STR("child::para[position()>1]"),
		STR("following-sibling::chapter[position()=1]"), STR("preceding-sibling::chapter[position()=1]"), STR("/descendant::figure[position()=42]"),
		STR("/child::doc/child::chapter[position()=5]/child::section[position()=2]"), STR("child::chapter/descendant::para"), STR("child::para[attribute::type='warning']"),
		STR("child::para[attribute::type='warning'][position()=5]"), STR("child::para[position()=5][attribute::type='warning']"), STR("child::chapter[child::title='Introduction']"),
		STR("child::chapter[child::title]"), STR("child::*[self::chapter or self::appendix]"), STR("child::*[self::chapter or self::appendix][position()=last()]"),
		STR("count(//*[id='u1']|//*[id='u2'])"), STR("count(//*[id='u1']|//*[class='u'])"), STR("count(//*[class='u']|//*[class='u'])"), STR("count(//*[class='u']|//*[id='u1'])"),
		STR("count(//*[@id='self']/ancestor-or-self::*)"), STR("count(//*[@id='self']/ancestor::*)"), STR("count(//*[@id='self']/attribute::*)"), STR("count(//*[@id='self']/child::*)"),
		STR("count(//*[@id='self']/descendant-or-self::*)"), STR("count(//*[@id='self']/descendant::*)"), STR("count(//*[@id='self']/following-sibling::*)"),
		STR("count(//*[@id='self']/following::*)"), STR("//*[@id='self']/parent::*/@id"), STR("count(//*[@id='self']/preceding-sibling::*)"),
		STR("count(//*[@id='self']/preceding::*)"), STR("//*[@id='self']/self::*/@id"), STR("id('nested1')/div[1]//input[2]"), STR("id('foo')//div[contains(@id, 'useful')]//input"),
		STR("(//table[@class='stylee'])//th[text()='theHeaderText']/../td"), STR("address"), STR("address=string(/page/user/defaultlocation)"), STR("count-of-snippet-of-url = 0"),
		STR("daddr"), STR("form"), STR("form = 'from'"), STR("form = 'to'"), STR("form='near'"), STR("home"), STR("i"), STR("i > page and i < page + range"),
		STR("i < page and i >= page - range"), STR("i < @max"), STR("i <= page"), STR("i + 1"), STR("i = page"), STR("i = 1"), STR("info = position() or (not(info) and position() = 1)"),
		STR("is-first-order"), STR("is-first-order and snippets-exist"), STR("more"), STR("more > 0"), STR("near-point"), STR("page"), STR("page != 'from'"), STR("page != 'to'"),
		STR("page != 'to' and page != 'from'"), STR("page > 1"), STR("page = 'basics'"), STR("page = 'details'"), STR("page = 'from'"), STR("page = 'to'"), STR("page='from'"),
		STR("page='to'"), STR("r >= 0.5"), STR("r >= 1"), STR("r - 0"), STR("r - 1"), STR("r - 2"), STR("r - 3"), STR("r - 4"), STR("saddr"), STR("sources"), STR("sources[position() < details]"),
		STR("src"), STR("str"), STR("\"'\""), STR("(//location[string(info/references/reference[1]/url)=string(current-url)]/info/references/reference[1])[1]"),
		STR("(not(count-of-snippet-of-url = 0) and (position() = 1) or not(current-url = //locations/location[position() = last-pos]//reference[1]/url))"),
		STR("(not(info) and position() = 1) or info = position()"), STR("."), STR("../@arg0"), STR("../@filterpng"), STR("/page/@filterpng"), STR("4"), STR("@attribution"),
		STR("@id"), STR("@max > @num"), STR("@meters > 16093"), STR("@name"), STR("@start div @num + 1"), STR("@url"), STR("ad"), STR("address/line"), STR("adsmessage"),
		STR("attr"), STR("boolean(location[@id='near'][icon/@image])"), STR("bubble/node()"), STR("calltoaction/node()"), STR("category"), STR("contains(str, c)"),
		STR("count(//location[string(info/references/reference[1]/url)=string(current-url)]//snippet)"), STR("count(//snippet)"), STR("count(attr)"), STR("count(location)"),
		STR("count(structured/source) > 1"), STR("description/node()"), STR("destination"), STR("destinationAddress"), STR("domain"), STR("false()"), STR("icon/@class != 'noicon'"),
		STR("icon/@image"), STR("info"), STR("info/address/line"), STR("info/distance"), STR("info/distance and near-point"), STR("info/distance and info/phone and near-point"),
		STR("info/distance or info/phone"), STR("info/panel/node()"), STR("info/phone"), STR("info/references/reference[1]"), STR("info/references/reference[1]/snippet"),
		STR("info/references/reference[1]/url"), STR("info/title"), STR("info/title/node()"), STR("line"), STR("location"), STR("location[@id!='near']"), STR("location[@id='near'][icon/@image]"),
		STR("location[position() > umlocations div 2]"), STR("location[position() <= numlocations div 2]"), STR("locations"), STR("locations/location"), STR("near"), STR("node()"),
		STR("not(count-of-snippets = 0)"), STR("not(form = 'from')"), STR("not(form = 'near')"), STR("not(form = 'to')"), STR("not(../@page)"), STR("not(structured/source)"), STR("notice"),
		STR("number(../@info)"), STR("number(../@items)"), STR("number(/page/@linewidth)"), STR("page/ads"), STR("page/directions"), STR("page/error"), STR("page/overlay"),
		STR("page/overlay/locations/location"), STR("page/refinements"), STR("page/request/canonicalnear"), STR("page/request/near"), STR("page/request/query"), STR("page/spelling/suggestion"),
		STR("page/user/defaultlocation"), STR("phone"), STR("position()"), STR("position() != 1"), STR("position() != last()"), STR("position() > 1"), STR("position() < details"),
		STR("position()-1"), STR("query"), STR("references/@total"), STR("references/reference"), STR("references/reference/domain"), STR("references/reference/url"),
		STR("reviews/@positive div (reviews/@positive + reviews/@negative) * 5"), STR("reviews/@positive div (reviews/@positive + reviews/@negative) * (5)"), STR("reviews/@total"),
		STR("reviews/@total > 1"), STR("reviews/@total > 5"), STR("reviews/@total = 1"), STR("segments/@distance"), STR("segments/@time"), STR("segments/segment"), STR("shorttitle/node()"),
		STR("snippet"), STR("snippet/node()"), STR("source"), STR("sourceAddress"), STR("sourceAddress and destinationAddress"), STR("string(../@daddr)"), STR("string(../@form)"),
		STR("string(../@page)"), STR("string(../@saddr)"), STR("string(info/title)"), STR("string(page/request/canonicalnear) != ''"), STR("string(page/request/near) != ''"),
		STR("string-length(address) > linewidth"), STR("structured/@total - details"), STR("structured/source"), STR("structured/source[@name]"), STR("substring(address, 1, linewidth - 3)"),
		STR("substring-after(str, c)"), STR("substring-after(icon/@image, '/mapfiles/marker')"), STR("substring-before(str, c)"), STR("tagline/node()"), STR("targetedlocation"),
		STR("title"), STR("title/node()"), STR("true()"), STR("url"), STR("visibleurl"), STR("id(\"level10\")/ancestor::SPAN"), STR("id(\"level10\")/ancestor-or-self::SPAN"), STR("//attribute::*"),
        STR("child::HTML/child::BODY/child::H1"), STR("descendant::node()"), STR("descendant-or-self::SPAN"), STR("id(\"first\")/following::text()"), STR("id(\"first\")/following-sibling::node()"),
        STR("id(\"level10\")/parent::node()"), STR("id(\"last\")/preceding::text()"), STR("id(\"last\")/preceding-sibling::node()"), STR("/HTML/BODY/H1/self::node()"), STR("//*[@name]"),
        STR("id(\"pet\")/SELECT[@name=\"species\"]/OPTION[@selected]/@value"), STR("descendant::INPUT[@name=\"name\"]/@value"), STR("id(\"pet\")/INPUT[@name=\"gender\" and @checked]/@value"),
        STR("//TEXTAREA[@name=\"description\"]/text()"), STR("id(\"div1\")|id(\"div2\")|id(\"div3 div4 div5\")"), STR("//LI[1]"), STR("//LI[last()]/text()"), STR("//LI[position() mod 2]/@class"),
        STR("//text()[.=\"foo\"]"), STR("descendant-or-self::SPAN[position() > 2]"), STR("descendant::*[contains(@class,\" fruit \")]"),

		// ajaxslt considers this path invalid, however I believe it's valid as per spec
		STR("***"),

		// Oasis MSFT considers this path invalid, however I believe it's valid as per spec
		STR("**..**"),

		// Miscellaneous
		STR("..***..***.***.***..***..***..")
    };

	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
	{
		xpath_query q(paths[i]);
	}
}

#if defined(PUGIXML_WCHAR_MODE) || !defined(PUGIXML_NO_STL)
TEST(xpath_parse_paths_valid_unicode)
{
    // From ajaxslt
	const wchar_t* paths[] =
	{
	#ifdef U_LITERALS
		L"/descendant-or-self::\u90e8\u5206", L"//\u90e8\u5206", L"substring('\uff11\uff12\uff13\uff14\uff15', 0, 3)", L"//\u30bf\u30a4\u30c8\u30eb | //\u30ea\u30f3\u30af",
		L"\u8b0e//\u30bf\u30a4\u30c8\u30eb", L"//*[@\u30c7\u30b9\u30c6\u30a3\u30cd\u30a4\u30b7\u30e7\u30f3]", L"\u30da\u30fc\u30b8 = '\u304b\u3089'",
		L"concat(substring-before(@\u30a4\u30e1\u30fc\u30b8,'\u76ee\u5370'),'\u30a2\u30a4\u30b3\u30f3',substring-after(@\u30a4\u30e1\u30fc\u30b8,'\u76ee\u5370'))",
		L"\u30bd\u30fc\u30b9|\u30c7\u30b9\u30c6\u30a3\u30cd\u30a4\u30b7\u30e7\u30f3", L"\u30da\u30fc\u30b8 != '\u307e\u3067' and \u30da\u30fc\u30b8 != '\u304b\u3089'",
		L"substring-after(\u30a2\u30a4\u30b3\u30f3/@\u30a4\u30e1\u30fc\u30b8, '/\u5730\u56f3\u30d5\u30a1\u30a4\u30eb/\u76ee\u5370')", L"child::\u6bb5\u843d",
		L"substring-before(\u6587\u5b57\u5217, \u6587\u5b57)", L"\u30bb\u30b0\u30e1\u30f3\u30c8/@\u6642\u523b", L"attribute::\u540d\u524d", L"descendant::\u6bb5\u843d",
		L"ancestor::\u90e8\u5206", L"ancestor-or-self::\u90e8\u5206", L"descendant-or-self::\u6bb5\u843d", L"self::\u6bb5\u843d", L"child::\u7ae0/descendant::\u6bb5\u843d",
		L"child::*/child::\u6bb5\u843d", L"/descendant::\u6bb5\u843d", L"/descendant::\u9806\u5e8f\u30ea\u30b9\u30c8/child::\u9805\u76ee", L"child::\u6bb5\u843d[position()=1]",
		L"child::\u6bb5\u843d[position()=last()]", L"child::\u6bb5\u843d[position()=last()-1]", L"child::\u6bb5\u843d[position()>1]", L"following-sibling::\u7ae0[position()=1]",
		L"preceding-sibling::\u7ae0[position()=1]", L"/descendant::\u56f3\u8868[position()=42]", L"/child::\u6587\u66f8/child::\u7ae0[position()=5]/child::\u7bc0[position()=2]",
		L"child::\u6bb5\u843d[attribute::\u30bf\u30a4\u30d7='\u8b66\u544a']", L"child::\u6bb5\u843d[attribute::\u30bf\u30a4\u30d7='\u8b66\u544a'][position()=5]",
		L"child::\u6bb5\u843d[position()=5][attribute::\u30bf\u30a4\u30d7='\u8b66\u544a']", L"child::\u7ae0[child::\u30bf\u30a4\u30c8\u30eb='\u306f\u3058\u3081\u306b']",
		L"child::\u7ae0[child::\u30bf\u30a4\u30c8\u30eb]", L"child::*[self::\u7ae0 or self::\u4ed8\u9332]", L"child::*[self::\u7ae0 or self::\u4ed8\u9332][position()=last()]",
	#else
		L"/descendant-or-self::\x90e8\x5206", L"//\x90e8\x5206", L"substring('\xff11\xff12\xff13\xff14\xff15', 0, 3)", L"//\x30bf\x30a4\x30c8\x30eb | //\x30ea\x30f3\x30af",
		L"\x8b0e//\x30bf\x30a4\x30c8\x30eb", L"//*[@\x30c7\x30b9\x30c6\x30a3\x30cd\x30a4\x30b7\x30e7\x30f3]", L"\x30da\x30fc\x30b8 = '\x304b\x3089'",
		L"concat(substring-before(@\x30a4\x30e1\x30fc\x30b8,'\x76ee\x5370'),'\x30a2\x30a4\x30b3\x30f3',substring-after(@\x30a4\x30e1\x30fc\x30b8,'\x76ee\x5370'))",
		L"\x30bd\x30fc\x30b9|\x30c7\x30b9\x30c6\x30a3\x30cd\x30a4\x30b7\x30e7\x30f3", L"\x30da\x30fc\x30b8 != '\x307e\x3067' and \x30da\x30fc\x30b8 != '\x304b\x3089'",
		L"substring-after(\x30a2\x30a4\x30b3\x30f3/@\x30a4\x30e1\x30fc\x30b8, '/\x5730\x56f3\x30d5\x30a1\x30a4\x30eb/\x76ee\x5370')", L"child::\x6bb5\x843d",
		L"substring-before(\x6587\x5b57\x5217, \x6587\x5b57)", L"\x30bb\x30b0\x30e1\x30f3\x30c8/@\x6642\x523b", L"attribute::\x540d\x524d", L"descendant::\x6bb5\x843d",
		L"ancestor::\x90e8\x5206", L"ancestor-or-self::\x90e8\x5206", L"descendant-or-self::\x6bb5\x843d", L"self::\x6bb5\x843d", L"child::\x7ae0/descendant::\x6bb5\x843d",
		L"child::*/child::\x6bb5\x843d", L"/descendant::\x6bb5\x843d", L"/descendant::\x9806\x5e8f\x30ea\x30b9\x30c8/child::\x9805\x76ee", L"child::\x6bb5\x843d[position()=1]",
		L"child::\x6bb5\x843d[position()=last()]", L"child::\x6bb5\x843d[position()=last()-1]", L"child::\x6bb5\x843d[position()>1]", L"following-sibling::\x7ae0[position()=1]",
		L"preceding-sibling::\x7ae0[position()=1]", L"/descendant::\x56f3\x8868[position()=42]", L"/child::\x6587\x66f8/child::\x7ae0[position()=5]/child::\x7bc0[position()=2]",
		L"child::\x6bb5\x843d[attribute::\x30bf\x30a4\x30d7='\x8b66\x544a']", L"child::\x6bb5\x843d[attribute::\x30bf\x30a4\x30d7='\x8b66\x544a'][position()=5]",
		L"child::\x6bb5\x843d[position()=5][attribute::\x30bf\x30a4\x30d7='\x8b66\x544a']", L"child::\x7ae0[child::\x30bf\x30a4\x30c8\x30eb='\x306f\x3058\x3081\x306b']",
		L"child::\x7ae0[child::\x30bf\x30a4\x30c8\x30eb]", L"child::*[self::\x7ae0 or self::\x4ed8\x9332]", L"child::*[self::\x7ae0 or self::\x4ed8\x9332][position()=last()]",
	#endif
	};

	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
	{
	#if defined(PUGIXML_WCHAR_MODE)
		xpath_query q(paths[i]);
	#elif !defined(PUGIXML_NO_STL)
		std::basic_string<char> path_utf8 = pugi::as_utf8(paths[i]);
		xpath_query q(path_utf8.c_str());
	#endif
	}
}
#endif

TEST(xpath_parse_invalid)
{
    const char_t* paths[] =
	{
		// From Jaxen tests
		STR("//:p"), STR("/foo/bar/"), STR("12 + (count(//author)+count(//author/attribute::*)) / 2"), STR("id()/2"), STR("+"),
		STR("///triple slash"), STR("/numbers numbers"), STR("/a/b[c > d]efg"), STR("/inv/child::"), STR("/invoice/@test[abcd"),
		STR("/invoice/@test[abcd > x"), STR("string-length('a"), STR("/descendant::()"), STR("(1 + 1"), STR("!false()"),
		STR("$author"), STR("10 + $foo"), STR("$foo:bar"), STR("$varname[@a='1']"), STR("foo/$variable/foo"),
		STR(".[1]"), STR("chyld::foo"), STR("foo/tacos()"), STR("foo/tacos()"), STR("/foo/bar[baz"), STR("//"), STR("*:foo"),
        STR("/cracker/cheese[(mold > 1) and (sense/taste"),

		// From xpath-as3 tests
		STR("a b"), STR("//self::node())"), STR("/x/y[contains(self::node())"), STR("/x/y[contains(self::node()]"), STR("///"), STR("text::a"),

		// From haXe-xpath tests
		STR("|/gjs"), STR("+3"), STR("/html/body/p != ---'div'/a"), STR(""), STR("@"), STR("#akf"), STR(",")

		// Miscellaneous
		STR("..."), STR("...."), STR("**"), STR("****"), STR("******"), STR("..***..***.***.***..***..***..*"), STR("/[1]")
	};

	for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
	{
		CHECK_XPATH_FAIL(paths[i]);
	}
}

TEST_XML(xpath_parse_absolute, "<div><s/></div>")
{
	CHECK_XPATH_NODESET(doc, STR("/")) % 1;

	CHECK_XPATH_NODESET(doc, STR("/div/s")) % 3;
	CHECK_XPATH_NODESET(doc, STR("/ div /s")) % 3;
	CHECK_XPATH_FAIL(STR("/ div 5"));

	CHECK_XPATH_NODESET(doc, STR("/*/s")) % 3;
	CHECK_XPATH_NODESET(doc, STR("/ * /s")) % 3;
	CHECK_XPATH_FAIL(STR("/ * 5"));

	CHECK_XPATH_NODESET(doc, STR("/*[/]")) % 2;
}

TEST(xpath_parse_out_of_memory_first_page)
{
	test_runner::_memory_fail_threshold = 1;

	CHECK_ALLOC_FAIL(CHECK_XPATH_FAIL(STR("1")));
}

TEST(xpath_parse_out_of_memory_second_page_node)
{
	test_runner::_memory_fail_threshold = 8192;

	CHECK_ALLOC_FAIL(CHECK_XPATH_FAIL(STR("1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1")));
}

TEST(xpath_parse_out_of_memory_string_to_number)
{
	test_runner::_memory_fail_threshold = 4096 + 128;

	CHECK_ALLOC_FAIL(CHECK_XPATH_FAIL(STR("0.11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111")));
}

TEST(xpath_parse_qname_error)
{
	CHECK_XPATH_FAIL(STR("foo: bar"));
	CHECK_XPATH_FAIL(STR("foo :bar"));
	CHECK_XPATH_FAIL(STR("foo: *"));
	CHECK_XPATH_FAIL(STR("foo :*"));
	CHECK_XPATH_FAIL(STR(":*"));
	CHECK_XPATH_FAIL(STR(":bar"));
	CHECK_XPATH_FAIL(STR(":"));
}

TEST(xpath_parse_result_default)
{
	xpath_parse_result result;

	CHECK(!result);
	CHECK(result.error != 0);
	CHECK(result.offset == 0);
}

#endif
