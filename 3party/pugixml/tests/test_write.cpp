#include "common.hpp"

#include "writer_string.hpp"

#include <string>
#include <sstream>
#include <stdexcept>

TEST_XML(write_simple, "<node attr='1'><child>text</child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\n<child>text</child>\n</node>\n"), STR(""), 0);
}

TEST_XML(write_raw, "<node attr='1'><child>text</child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\"><child>text</child></node>"), STR(""), format_raw);
}

TEST_XML(write_indent, "<node attr='1'><child><sub>text</sub></child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\n\t<child>\n\t\t<sub>text</sub>\n\t</child>\n</node>\n"), STR("\t"), format_indent);
}

TEST_XML(write_indent_attributes, "<node attr='1' other='2'><child><sub>text</sub></child></node>")
{
	CHECK_NODE_EX(doc, STR("<node\n\tattr=\"1\"\n\tother=\"2\">\n\t<child>\n\t\t<sub>text</sub>\n\t</child>\n</node>\n"), STR("\t"), format_indent_attributes);
}

TEST_XML(write_indent_attributes_empty_element, "<node attr='1' other='2' />")
{
	CHECK_NODE_EX(doc, STR("<node\n\tattr=\"1\"\n\tother=\"2\" />\n"), STR("\t"), format_indent_attributes);
}

TEST_XML_FLAGS(write_indent_attributes_declaration, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><node attr='1' other='2' />", parse_full)
{
	CHECK_NODE_EX(doc, STR("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<node\n\tattr=\"1\"\n\tother=\"2\" />\n"), STR("\t"), format_indent_attributes);
}

TEST_XML(write_indent_attributes_raw, "<node attr='1' other='2'><child><sub>text</sub></child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\" other=\"2\"><child><sub>text</sub></child></node>"), STR("\t"), format_indent_attributes | format_raw);
}

TEST_XML(write_indent_attributes_empty_indent, "<node attr='1' other='2'><child><sub>text</sub></child></node>")
{
	CHECK_NODE_EX(doc, STR("<node\nattr=\"1\"\nother=\"2\">\n<child>\n<sub>text</sub>\n</child>\n</node>\n"), STR(""), format_indent_attributes);
}

TEST_XML(write_pcdata, "<node attr='1'><child><sub/>text</child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\n\t<child>\n\t\t<sub />text</child>\n</node>\n"), STR("\t"), format_indent);
}

TEST_XML_FLAGS(write_cdata, "<![CDATA[value]]>", parse_cdata | parse_fragment)
{
	CHECK_NODE(doc, STR("<![CDATA[value]]>"));
	CHECK_NODE_EX(doc, STR("<![CDATA[value]]>"), STR(""), 0);
}

TEST_XML_FLAGS(write_cdata_empty, "<![CDATA[]]>", parse_cdata | parse_fragment)
{
	CHECK_NODE(doc, STR("<![CDATA[]]>"));
	CHECK_NODE_EX(doc, STR("<![CDATA[]]>"), STR(""), 0);
}

TEST_XML_FLAGS(write_cdata_escape, "<![CDATA[value]]>", parse_cdata | parse_fragment)
{
	CHECK_NODE(doc, STR("<![CDATA[value]]>"));

	doc.first_child().set_value(STR("1]]>2]]>3"));
	CHECK_NODE(doc, STR("<![CDATA[1]]]]><![CDATA[>2]]]]><![CDATA[>3]]>"));
}

TEST_XML(write_cdata_inner, "<node><![CDATA[value]]></node>")
{
	CHECK_NODE(doc, STR("<node><![CDATA[value]]></node>"));
	CHECK_NODE_EX(doc, STR("<node><![CDATA[value]]></node>\n"), STR(""), 0);
}

TEST(write_cdata_null)
{
	xml_document doc;
	doc.append_child(node_cdata);
	doc.append_child(STR("node")).append_child(node_cdata);

	CHECK_NODE(doc, STR("<![CDATA[]]><node><![CDATA[]]></node>"));
}

TEST_XML_FLAGS(write_comment, "<!--text-->", parse_comments | parse_fragment)
{
	CHECK_NODE(doc, STR("<!--text-->"));
	CHECK_NODE_EX(doc, STR("<!--text-->\n"), STR(""), 0);
}

TEST(write_comment_invalid)
{
	xml_document doc;
	xml_node child = doc.append_child(node_comment);

	CHECK_NODE(doc, STR("<!---->"));

	child.set_value(STR("-"));
	CHECK_NODE(doc, STR("<!--- -->"));

	child.set_value(STR("--"));
	CHECK_NODE(doc, STR("<!--- - -->"));

	child.set_value(STR("---"));
	CHECK_NODE(doc, STR("<!--- - - -->"));

	child.set_value(STR("-->"));
	CHECK_NODE(doc, STR("<!--- ->-->"));

	child.set_value(STR("-->-"));
	CHECK_NODE(doc, STR("<!--- ->- -->"));
}

TEST(write_comment_null)
{
	xml_document doc;
	doc.append_child(node_comment);

	CHECK_NODE(doc, STR("<!---->"));
}

TEST_XML_FLAGS(write_pi, "<?name value?>", parse_pi | parse_fragment)
{
	CHECK_NODE(doc, STR("<?name value?>"));
	CHECK_NODE_EX(doc, STR("<?name value?>\n"), STR(""), 0);
}

TEST(write_pi_null)
{
	xml_document doc;
	xml_node node = doc.append_child(node_pi);

	CHECK_NODE(doc, STR("<?:anonymous?>"));

	node.set_value(STR("value"));

	CHECK_NODE(doc, STR("<?:anonymous value?>"));
}

TEST(write_pi_invalid)
{
	xml_document doc;
	xml_node node = doc.append_child(node_pi);

	node.set_name(STR("test"));
	node.set_value(STR("?"));

	CHECK_NODE(doc, STR("<?test ?") STR("?>"));

	node.set_value(STR("?>"));

	CHECK_NODE(doc, STR("<?test ? >?>"));

	node.set_value(STR("<?foo?>"));

	CHECK_NODE(doc, STR("<?test <?foo? >?>"));
}

TEST_XML_FLAGS(write_declaration, "<?xml version='2.0'?>", parse_declaration | parse_fragment)
{
	CHECK_NODE(doc, STR("<?xml version=\"2.0\"?>"));
	CHECK_NODE_EX(doc, STR("<?xml version=\"2.0\"?>\n"), STR(""), 0);
}

TEST_XML_FLAGS(write_doctype, "<!DOCTYPE id [ foo ]>", parse_doctype | parse_fragment)
{
	CHECK_NODE(doc, STR("<!DOCTYPE id [ foo ]>"));
	CHECK_NODE_EX(doc, STR("<!DOCTYPE id [ foo ]>\n"), STR(""), 0);
}

TEST(write_doctype_null)
{
	xml_document doc;
	doc.append_child(node_doctype);

	CHECK_NODE(doc, STR("<!DOCTYPE>"));
}

TEST_XML(write_escape, "<node attr=''>text</node>")
{
	doc.child(STR("node")).attribute(STR("attr")) = STR("<>'\"&\x04\r\n\t");
	doc.child(STR("node")).first_child().set_value(STR("<>'\"&\x04\r\n\t"));

	CHECK_NODE(doc, STR("<node attr=\"&lt;&gt;'&quot;&amp;&#04;&#13;&#10;\t\">&lt;&gt;'\"&amp;&#04;\r\n\t</node>"));
}

TEST_XML(write_escape_unicode, "<node attr='&#x3c00;'/>")
{
#ifdef PUGIXML_WCHAR_MODE
	#ifdef U_LITERALS
		CHECK_NODE(doc, STR("<node attr=\"\u3c00\" />"));
	#else
		CHECK_NODE(doc, STR("<node attr=\"\x3c00\" />"));
	#endif
#else
	CHECK_NODE(doc, STR("<node attr=\"\xe3\xb0\x80\" />"));
#endif
}

TEST_XML(write_no_escapes, "<node attr=''>text</node>")
{
	doc.child(STR("node")).attribute(STR("attr")) = STR("<>'\"&\x04\r\n\t");
	doc.child(STR("node")).first_child().set_value(STR("<>'\"&\x04\r\n\t"));

	CHECK_NODE_EX(doc, STR("<node attr=\"<>'\"&\x04\r\n\t\"><>'\"&\x04\r\n\t</node>"), STR(""), format_raw | format_no_escapes);
}

struct test_writer: xml_writer
{
	std::basic_string<pugi::char_t> contents;

	virtual void write(const void* data, size_t size)
	{
		CHECK(size % sizeof(pugi::char_t) == 0);
		contents.append(static_cast<const pugi::char_t*>(data), size / sizeof(pugi::char_t));
	}
};

TEST_XML(write_print_writer, "<node/>")
{
	test_writer writer;
	doc.print(writer, STR(""), format_default, get_native_encoding());

	CHECK(writer.contents == STR("<node />\n"));
}

#ifndef PUGIXML_NO_STL
TEST_XML(write_print_stream, "<node/>")
{
	std::ostringstream oss;
	doc.print(oss, STR(""), format_default, encoding_utf8);

	CHECK(oss.str() == "<node />\n");
}

TEST_XML(write_print_stream_encode, "<n/>")
{
	std::ostringstream oss;
	doc.print(oss, STR(""), format_default, encoding_utf16_be);

	CHECK(oss.str() == std::string("\x00<\x00n\x00 \x00/\x00>\x00\n", 12));
}

TEST_XML(write_print_stream_wide, "<node/>")
{
	std::basic_ostringstream<wchar_t> oss;
	doc.print(oss, STR(""), format_default, encoding_utf8);

	CHECK(oss.str() == L"<node />\n");
}
#endif

TEST_XML(write_huge_chunk, "<node/>")
{
	std::basic_string<pugi::char_t> name(10000, STR('n'));
	doc.child(STR("node")).set_name(name.c_str());

	test_writer writer;
	doc.print(writer, STR(""), format_default, get_native_encoding());

	CHECK(writer.contents == STR("<") + name + STR(" />\n"));
}

TEST(write_encodings)
{
	static char s_utf8[] = "<\x54\xC2\xA2\xE2\x82\xAC\xF0\xA4\xAD\xA2/>";

	xml_document doc;
	CHECK(doc.load_buffer(s_utf8, sizeof(s_utf8), parse_default, encoding_utf8));

	CHECK(write_narrow(doc, format_default, encoding_utf8) == "<\x54\xC2\xA2\xE2\x82\xAC\xF0\xA4\xAD\xA2 />\n");

	CHECK(test_write_narrow(doc, format_default, encoding_utf32_le, "<\x00\x00\x00\x54\x00\x00\x00\xA2\x00\x00\x00\xAC\x20\x00\x00\x62\x4B\x02\x00 \x00\x00\x00/\x00\x00\x00>\x00\x00\x00\n\x00\x00\x00", 36));
	CHECK(test_write_narrow(doc, format_default, encoding_utf32_be, "\x00\x00\x00<\x00\x00\x00\x54\x00\x00\x00\xA2\x00\x00\x20\xAC\x00\x02\x4B\x62\x00\x00\x00 \x00\x00\x00/\x00\x00\x00>\x00\x00\x00\n", 36));
	CHECK(write_narrow(doc, format_default, encoding_utf32) == write_narrow(doc, format_default, is_little_endian() ? encoding_utf32_le : encoding_utf32_be));

	CHECK(test_write_narrow(doc, format_default, encoding_utf16_le, "<\x00\x54\x00\xA2\x00\xAC\x20\x52\xd8\x62\xdf \x00/\x00>\x00\n\x00", 20));
	CHECK(test_write_narrow(doc, format_default, encoding_utf16_be, "\x00<\x00\x54\x00\xA2\x20\xAC\xd8\x52\xdf\x62\x00 \x00/\x00>\x00\n", 20));
	CHECK(write_narrow(doc, format_default, encoding_utf16) == write_narrow(doc, format_default, is_little_endian() ? encoding_utf16_le : encoding_utf16_be));

	size_t wcharsize = sizeof(wchar_t);
	std::basic_string<wchar_t> v = write_wide(doc, format_default, encoding_wchar);

	if (wcharsize == 4)
	{
		CHECK(v.size() == 9 && v[0] == '<' && v[1] == 0x54 && v[2] == 0xA2 && v[3] == 0x20AC && v[4] == wchar_cast(0x24B62) && v[5] == ' ' && v[6] == '/' && v[7] == '>' && v[8] == '\n');
	}
	else
	{
		CHECK(v.size() == 10 && v[0] == '<' && v[1] == 0x54 && v[2] == 0xA2 && v[3] == 0x20AC && v[4] == wchar_cast(0xd852) && v[5] == wchar_cast(0xdf62) && v[6] == ' ' && v[7] == '/' && v[8] == '>' && v[9] == '\n');
	}

    CHECK(test_write_narrow(doc, format_default, encoding_latin1, "<\x54\xA2?? />\n", 9));
}

#ifdef PUGIXML_WCHAR_MODE
TEST(write_encoding_huge)
{
	const unsigned int N = 16000;

	// make a large utf16 name consisting of 6-byte char pairs (6 does not divide internal buffer size, so will need split correction)
	std::string s_utf16 = std::string("\x00<", 2);

	for (unsigned int i = 0; i < N; ++i) s_utf16 += "\x20\xAC\xd8\x52\xdf\x62";

	s_utf16 += std::string("\x00/\x00>", 4);

	xml_document doc;
	CHECK(doc.load_buffer(&s_utf16[0], s_utf16.length(), parse_default, encoding_utf16_be));

	std::string s_utf8 = "<";

	for (unsigned int j = 0; j < N; ++j) s_utf8 += "\xE2\x82\xAC\xF0\xA4\xAD\xA2";

	s_utf8 += " />\n";

	CHECK(test_write_narrow(doc, format_default, encoding_utf8, s_utf8.c_str(), s_utf8.length()));
}

TEST(write_encoding_huge_invalid)
{
	size_t wcharsize = sizeof(wchar_t);

	if (wcharsize == 2)
	{
		const unsigned int N = 16000;

		// make a large utf16 name consisting of leading surrogate chars
		std::basic_string<wchar_t> s_utf16;

		for (unsigned int i = 0; i < N; ++i) s_utf16 += static_cast<wchar_t>(0xd852);

		xml_document doc;
		doc.append_child().set_name(s_utf16.c_str());

		CHECK(test_write_narrow(doc, format_default, encoding_utf8, "< />\n", 5));
	}
}
#else
TEST(write_encoding_huge)
{
	const unsigned int N = 16000;

	// make a large utf8 name consisting of 3-byte chars (3 does not divide internal buffer size, so will need split correction)
	std::string s_utf8 = "<";

	for (unsigned int i = 0; i < N; ++i) s_utf8 += "\xE2\x82\xAC";

	s_utf8 += "/>";

	xml_document doc;
	CHECK(doc.load_buffer(&s_utf8[0], s_utf8.length(), parse_default, encoding_utf8));

	std::string s_utf16 = std::string("\x00<", 2);

	for (unsigned int j = 0; j < N; ++j) s_utf16 += "\x20\xAC";

	s_utf16 += std::string("\x00 \x00/\x00>\x00\n", 8);

	CHECK(test_write_narrow(doc, format_default, encoding_utf16_be, s_utf16.c_str(), s_utf16.length()));
}

TEST(write_encoding_huge_invalid)
{
	const unsigned int N = 16000;

	// make a large utf8 name consisting of non-leading chars
	std::string s_utf8;

	for (unsigned int i = 0; i < N; ++i) s_utf8 += "\x82";

	xml_document doc;
	doc.append_child().set_name(s_utf8.c_str());

	std::string s_utf16 = std::string("\x00<\x00 \x00/\x00>\x00\n", 10);

	CHECK(test_write_narrow(doc, format_default, encoding_utf16_be, s_utf16.c_str(), s_utf16.length()));
}
#endif

TEST(write_unicode_escape)
{
	char s_utf8[] = "<\xE2\x82\xAC \xC2\xA2='\"\xF0\xA4\xAD\xA2&#x0a;\"'>&amp;\x14\xF0\xA4\xAD\xA2&lt;</\xE2\x82\xAC>";

	xml_document doc;
	CHECK(doc.load_buffer(s_utf8, sizeof(s_utf8), parse_default, encoding_utf8));

	CHECK(write_narrow(doc, format_default, encoding_utf8) == "<\xE2\x82\xAC \xC2\xA2=\"&quot;\xF0\xA4\xAD\xA2&#10;&quot;\">&amp;&#20;\xF0\xA4\xAD\xA2&lt;</\xE2\x82\xAC>\n");
}

#ifdef PUGIXML_WCHAR_MODE
static bool test_write_unicode_invalid(const wchar_t* name, const char* expected)
{
	xml_document doc;
	doc.append_child(node_pcdata).set_value(name);

	return write_narrow(doc, format_raw, encoding_utf8) == expected;
}

TEST(write_unicode_invalid_utf16)
{
	size_t wcharsize = sizeof(wchar_t);

	if (wcharsize == 2)
	{
		// check non-terminated degenerate handling
	#ifdef U_LITERALS
		CHECK(test_write_unicode_invalid(L"a\uda1d", "a"));
		CHECK(test_write_unicode_invalid(L"a\uda1d_", "a_"));
	#else
		CHECK(test_write_unicode_invalid(L"a\xda1d", "a"));
		CHECK(test_write_unicode_invalid(L"a\xda1d_", "a_"));
	#endif

		// check incorrect leading code
	#ifdef U_LITERALS
		CHECK(test_write_unicode_invalid(L"a\ude24", "a"));
		CHECK(test_write_unicode_invalid(L"a\ude24_", "a_"));
	#else
		CHECK(test_write_unicode_invalid(L"a\xde24", "a"));
		CHECK(test_write_unicode_invalid(L"a\xde24_", "a_"));
	#endif
	}
}
#else
static bool test_write_unicode_invalid(const char* name, const wchar_t* expected)
{
	xml_document doc;
	doc.append_child(node_pcdata).set_value(name);

	return write_wide(doc, format_raw, encoding_wchar) == expected;
}

TEST(write_unicode_invalid_utf8)
{
	// invalid 1-byte input
	CHECK(test_write_unicode_invalid("a\xb0", L"a"));
	CHECK(test_write_unicode_invalid("a\xb0_", L"a_"));

	// invalid 2-byte input
	CHECK(test_write_unicode_invalid("a\xc0", L"a"));
	CHECK(test_write_unicode_invalid("a\xd0", L"a"));
	CHECK(test_write_unicode_invalid("a\xc0_", L"a_"));
	CHECK(test_write_unicode_invalid("a\xd0_", L"a_"));

	// invalid 3-byte input
	CHECK(test_write_unicode_invalid("a\xe2\x80", L"a"));
	CHECK(test_write_unicode_invalid("a\xe2", L"a"));
	CHECK(test_write_unicode_invalid("a\xe2\x80_", L"a_"));
	CHECK(test_write_unicode_invalid("a\xe2_", L"a_"));

	// invalid 4-byte input
	CHECK(test_write_unicode_invalid("a\xf2\x97\x98", L"a"));
	CHECK(test_write_unicode_invalid("a\xf2\x97", L"a"));
	CHECK(test_write_unicode_invalid("a\xf2", L"a"));
	CHECK(test_write_unicode_invalid("a\xf2\x97\x98_", L"a_"));
	CHECK(test_write_unicode_invalid("a\xf2\x97_", L"a_"));
	CHECK(test_write_unicode_invalid("a\xf2_", L"a_"));

	// invalid 5-byte input
	CHECK(test_write_unicode_invalid("a\xf8_", L"a_"));
}
#endif

TEST(write_no_name_element)
{
	xml_document doc;
	xml_node root = doc.append_child();
	root.append_child();
	root.append_child().append_child(node_pcdata).set_value(STR("text"));

	CHECK_NODE(doc, STR("<:anonymous><:anonymous /><:anonymous>text</:anonymous></:anonymous>"));
	CHECK_NODE_EX(doc, STR("<:anonymous>\n\t<:anonymous />\n\t<:anonymous>text</:anonymous>\n</:anonymous>\n"), STR("\t"), format_default);
}

TEST(write_no_name_pi)
{
	xml_document doc;
	doc.append_child(node_pi);

	CHECK_NODE(doc, STR("<?:anonymous?>"));
}

TEST(write_no_name_attribute)
{
	xml_document doc;
	doc.append_child().set_name(STR("root"));
	doc.child(STR("root")).append_attribute(STR(""));

	CHECK_NODE(doc, STR("<root :anonymous=\"\" />"));
}

TEST(write_print_empty)
{
	test_writer writer;
	xml_node().print(writer);
}

#ifndef PUGIXML_NO_STL
TEST(write_print_stream_empty)
{
	std::ostringstream oss;
	xml_node().print(oss);
}

TEST(write_print_stream_empty_wide)
{
	std::basic_ostringstream<wchar_t> oss;
	xml_node().print(oss);
}
#endif

TEST(write_stackless)
{
	unsigned int count = 20000;
	std::basic_string<pugi::char_t> data;

	for (unsigned int i = 0; i < count; ++i)
		data += STR("<a>");

	data += STR("text");

	for (unsigned int j = 0; j < count; ++j)
		data += STR("</a>");

	xml_document doc;
	CHECK(doc.load_string(data.c_str()));

	CHECK_NODE(doc, data.c_str());
}

TEST_XML(write_indent_custom, "<node attr='1'><child><sub>text</sub></child></node>")
{
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\n<child>\n<sub>text</sub>\n</child>\n</node>\n"), STR(""), format_indent);
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\nA<child>\nAA<sub>text</sub>\nA</child>\n</node>\n"), STR("A"), format_indent);
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\nAB<child>\nABAB<sub>text</sub>\nAB</child>\n</node>\n"), STR("AB"), format_indent);
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\nABC<child>\nABCABC<sub>text</sub>\nABC</child>\n</node>\n"), STR("ABC"), format_indent);
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\nABCD<child>\nABCDABCD<sub>text</sub>\nABCD</child>\n</node>\n"), STR("ABCD"), format_indent);
	CHECK_NODE_EX(doc, STR("<node attr=\"1\">\nABCDE<child>\nABCDEABCDE<sub>text</sub>\nABCDE</child>\n</node>\n"), STR("ABCDE"), format_indent);
}

TEST(write_pcdata_null)
{
	xml_document doc;
	doc.append_child(STR("node")).append_child(node_pcdata);

	CHECK_NODE(doc, STR("<node></node>"));
	CHECK_NODE_EX(doc, STR("<node></node>\n"), STR("\t"), format_indent);

	doc.first_child().append_child(node_pcdata);

	CHECK_NODE_EX(doc, STR("<node></node>\n"), STR("\t"), format_indent);
}

TEST(write_pcdata_whitespace_fixedpoint)
{
	const char_t* data = STR("<node>  test  <child>\n  <sub/>\n   </child>\n</node>");

	static const unsigned int flags_parse[] =
	{
		0,
		parse_ws_pcdata,
		parse_ws_pcdata_single,
		parse_trim_pcdata
	};

	static const unsigned int flags_format[] =
	{
		0,
		format_raw,
		format_indent
	};

	for (unsigned int i = 0; i < sizeof(flags_parse) / sizeof(flags_parse[0]); ++i)
	{
		xml_document doc;
		CHECK(doc.load_string(data, flags_parse[i]));

		for (unsigned int j = 0; j < sizeof(flags_format) / sizeof(flags_format[0]); ++j)
		{
			std::string saved = write_narrow(doc, flags_format[j], encoding_auto);

			xml_document rdoc;
			CHECK(rdoc.load_buffer(&saved[0], saved.size(), flags_parse[i]));

			std::string rsaved = write_narrow(rdoc, flags_format[j], encoding_auto);

			CHECK(saved == rsaved);
		}
	}
}

TEST_XML_FLAGS(write_mixed, "<node><child1/><child2>pre<![CDATA[data]]>mid<!--comment--><test/>post<?pi value?>fin</child2><child3/></node>", parse_full)
{
	CHECK_NODE(doc, STR("<node><child1 /><child2>pre<![CDATA[data]]>mid<!--comment--><test />post<?pi value?>fin</child2><child3 /></node>"));
	CHECK_NODE_EX(doc, STR("<node>\n<child1 />\n<child2>pre<![CDATA[data]]>mid<!--comment-->\n<test />post<?pi value?>fin</child2>\n<child3 />\n</node>\n"), STR("\t"), 0);
	CHECK_NODE_EX(doc, STR("<node>\n\t<child1 />\n\t<child2>pre<![CDATA[data]]>mid<!--comment-->\n\t\t<test />post<?pi value?>fin</child2>\n\t<child3 />\n</node>\n"), STR("\t"), format_indent);
}

#ifndef PUGIXML_NO_EXCEPTIONS
struct throwing_writer: pugi::xml_writer
{
	virtual void write(const void*, size_t)
	{
		throw std::runtime_error("write failed");
	}
};

TEST_XML(write_throw_simple, "<node><child/></node>")
{
	try
	{
		throwing_writer w;
		doc.print(w);

		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (std::runtime_error&)
	{
	}
}

TEST_XML(write_throw_encoding, "<node><child/></node>")
{
	try
	{
		throwing_writer w;
		doc.print(w, STR("\t"), format_default, encoding_utf32_be);

		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (std::runtime_error&)
	{
	}
}
#endif
