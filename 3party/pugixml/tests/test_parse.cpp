#include "common.hpp"

#include "writer_string.hpp"

TEST(parse_pi_skip)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_declaration};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<?pi?><?pi value?>"), flags));
		CHECK(!doc.first_child());

		CHECK(doc.load_string(STR("<?pi <tag/> value?>"), flags));
		CHECK(!doc.first_child());
	}
}

TEST(parse_pi_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<?pi1?><?pi2 value?>"), parse_fragment | parse_pi));

	xml_node pi1 = doc.first_child();
	xml_node pi2 = doc.last_child();

	CHECK(pi1 != pi2);
	CHECK(pi1.type() == node_pi);
	CHECK_STRING(pi1.name(), STR("pi1"));
	CHECK_STRING(pi1.value(), STR(""));
	CHECK(pi2.type() == node_pi);
	CHECK_STRING(pi2.name(), STR("pi2"));
	CHECK_STRING(pi2.value(), STR("value"));
}

TEST(parse_pi_parse_spaces)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<?target  \r\n\t  value ?>"), parse_fragment | parse_pi));

	xml_node pi = doc.first_child();

	CHECK(pi.type() == node_pi);
	CHECK_STRING(pi.name(), STR("target"));
	CHECK_STRING(pi.value(), STR("value "));
}

TEST(parse_pi_error)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_pi};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<?"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<??"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?>"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?#?>"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name>"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name ?"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name?"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name? "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name?  "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name  "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name   "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value  "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value  ?"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value  ? "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value  ? >"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name value  ? > "), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name&"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?name&?"), flags).status == status_bad_pi);
	}
	
	CHECK(doc.load_string(STR("<?xx#?>"), parse_fragment | parse_pi).status == status_bad_pi);
	CHECK(doc.load_string(STR("<?name&?>"), parse_fragment | parse_pi).status == status_bad_pi);
	CHECK(doc.load_string(STR("<?name& x?>"), parse_fragment | parse_pi).status == status_bad_pi);
}

TEST(parse_comments_skip)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<!----><!--value-->"), parse_fragment));
	CHECK(!doc.first_child());
}

TEST(parse_comments_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<!----><!--value-->"), parse_fragment | parse_comments));

	xml_node c1 = doc.first_child();
	xml_node c2 = doc.last_child();

	CHECK(c1 != c2);
	CHECK(c1.type() == node_comment);
	CHECK_STRING(c1.name(), STR(""));
	CHECK_STRING(c1.value(), STR(""));
	CHECK(c2.type() == node_comment);
	CHECK_STRING(c2.name(), STR(""));
	CHECK_STRING(c2.value(), STR("value"));
}

TEST(parse_comments_parse_no_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<!--\r\rval1\rval2\r\nval3\nval4\r\r-->"), parse_fragment | parse_comments));

	xml_node c = doc.first_child();
	CHECK(c.type() == node_comment);
	CHECK_STRING(c.value(), STR("\r\rval1\rval2\r\nval3\nval4\r\r"));
}

TEST(parse_comments_parse_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<!--\r\rval1\rval2\r\nval3\nval4\r\r-->"), parse_fragment | parse_comments | parse_eol));

	xml_node c = doc.first_child();
	CHECK(c.type() == node_comment);
	CHECK_STRING(c.value(), STR("\n\nval1\nval2\nval3\nval4\n\n"));
}

TEST(parse_comments_error)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_comments, parse_fragment | parse_comments | parse_eol};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<!-"), flags).status == status_bad_comment);
		CHECK(doc.load_string(STR("<!--"), flags).status == status_bad_comment);
		CHECK(doc.load_string(STR("<!--v"), flags).status == status_bad_comment);
		CHECK(doc.load_string(STR("<!-->"), flags).status == status_bad_comment);
		CHECK(doc.load_string(STR("<!--->"), flags).status == status_bad_comment);
		CHECK(doc.load_string(STR("<!-- <!-- --><!- -->"), flags).status == status_bad_comment);
	}
}

TEST(parse_cdata_skip)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<![CDATA[]]><![CDATA[value]]>"), parse_fragment));
	CHECK(!doc.first_child());
}

TEST(parse_cdata_skip_contents)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node><![CDATA[]]>hello<![CDATA[value]]>, world!</node>"), parse_fragment));
	CHECK_NODE(doc, STR("<node>hello, world!</node>"));
}

TEST(parse_cdata_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<![CDATA[]]><![CDATA[value]]>"), parse_fragment | parse_cdata));

	xml_node c1 = doc.first_child();
	xml_node c2 = doc.last_child();

	CHECK(c1 != c2);
	CHECK(c1.type() == node_cdata);
	CHECK_STRING(c1.name(), STR(""));
	CHECK_STRING(c1.value(), STR(""));
	CHECK(c2.type() == node_cdata);
	CHECK_STRING(c2.name(), STR(""));
	CHECK_STRING(c2.value(), STR("value"));
}

TEST(parse_cdata_parse_no_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<![CDATA[\r\rval1\rval2\r\nval3\nval4\r\r]]>"), parse_fragment | parse_cdata));

	xml_node c = doc.first_child();
	CHECK(c.type() == node_cdata);
	CHECK_STRING(c.value(), STR("\r\rval1\rval2\r\nval3\nval4\r\r"));
}

TEST(parse_cdata_parse_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<![CDATA[\r\rval1\rval2\r\nval3\nval4\r\r]]>"), parse_fragment | parse_cdata | parse_eol));

	xml_node c = doc.first_child();
	CHECK(c.type() == node_cdata);
	CHECK_STRING(c.value(), STR("\n\nval1\nval2\nval3\nval4\n\n"));
}

TEST(parse_cdata_error)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_cdata, parse_fragment | parse_cdata | parse_eol};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<!["), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![C"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CD"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDA"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDAT"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA["), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[]"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[data"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[data]"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[data]]"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[>"), flags).status == status_bad_cdata);
		CHECK(doc.load_string(STR("<![CDATA[ <![CDATA[]]><![CDATA ]]>"), flags).status == status_bad_cdata);
	}
}

TEST(parse_ws_pcdata_skip)
{
	xml_document doc;
	CHECK(doc.load_string(STR("  "), parse_fragment));
	CHECK(!doc.first_child());

	CHECK(doc.load_string(STR("<root>  <node>  </node>  </root>"), parse_minimal));
	
	xml_node root = doc.child(STR("root"));
	
	CHECK(root.first_child() == root.last_child());
	CHECK(!root.first_child().first_child());
}

TEST(parse_ws_pcdata_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<root>  <node>  </node>  </root>"), parse_minimal | parse_ws_pcdata));

	xml_node root = doc.child(STR("root"));

	xml_node c1 = root.first_child();
	xml_node c2 = c1.next_sibling();
	xml_node c3 = c2.next_sibling();

	CHECK(c3 == root.last_child());

	CHECK(c1.type() == node_pcdata);
	CHECK_STRING(c1.value(), STR("  "));
	CHECK(c3.type() == node_pcdata);
	CHECK_STRING(c3.value(), STR("  "));

	CHECK(c2.first_child() == c2.last_child());
	CHECK(c2.first_child().type() == node_pcdata);
	CHECK_STRING(c2.first_child().value(), STR("  "));
}

static int get_tree_node_count(xml_node n)
{
    int result = 1;

    for (xml_node c = n.first_child(); c; c = c.next_sibling())
        result += get_tree_node_count(c);

    return result;
}

TEST(parse_ws_pcdata_permutations)
{
    struct test_data_t
    {
        unsigned int mask; // 1 = default flags, 2 = parse_ws_pcdata, 4 = parse_ws_pcdata_single
        const pugi::char_t* source;
        const pugi::char_t* result;
        int nodes; // negative if parsing should fail
    };

    test_data_t test_data[] =
    {
        // external pcdata should be discarded (whitespace or not)
        {7, STR("ext1<node/>"), STR("<node />"), 2},
        {7, STR("ext1<node/>ext2"), STR("<node />"), 2},
        {7, STR(" <node/>"), STR("<node />"), 2},
        {7, STR("<node/> "), STR("<node />"), 2},
        {7, STR(" <node/> "), STR("<node />"), 2},
        // inner pcdata should be preserved
        {7, STR("<node>inner</node>"), STR("<node>inner</node>"), 3},
        {7, STR("<node>inner1<child/>inner2</node>"), STR("<node>inner1<child />inner2</node>"), 5},
        {7, STR("<node>inner1<child>deep</child>inner2</node>"), STR("<node>inner1<child>deep</child>inner2</node>"), 6},
        // empty pcdata nodes should never be created
        {7, STR("<node>inner1<child></child>inner2</node>"), STR("<node>inner1<child />inner2</node>"), 5},
        {7, STR("<node><child></child>inner2</node>"), STR("<node><child />inner2</node>"), 4},
        {7, STR("<node>inner1<child></child></node>"), STR("<node>inner1<child /></node>"), 4},
        {7, STR("<node><child></child></node>"), STR("<node><child /></node>"), 3},
        // comments, pi or other nodes should not cause pcdata creation either
        {7, STR("<node><!----><child><?pi?></child><![CDATA[x]]></node>"), STR("<node><child /><![CDATA[x]]></node>"), 4},
        // leading/trailing pcdata whitespace should be preserved (note: this will change if parse_ws_pcdata_trim is introduced)
        {7, STR("<node>\t \tinner1<child> deep   </child>\t\ninner2\n\t</node>"), STR("<node>\t \tinner1<child> deep   </child>\t\ninner2\n\t</node>"), 6},
        // whitespace-only pcdata preservation depends on the parsing mode
        {1, STR("<node>\n\t<child>   </child>\n\t<child> <deep>  </deep> </child>\n\t<!---->\n\t</node>"), STR("<node><child /><child><deep /></child></node>"), 5},
        {2, STR("<node>\n\t<child>   </child>\n\t<child> <deep>  </deep> </child>\n\t<!---->\n\t</node>"), STR("<node>\n\t<child>   </child>\n\t<child> <deep>  </deep> </child>\n\t\n\t</node>"), 13},
        {4, STR("<node>\n\t<child>   </child>\n\t<child> <deep>  </deep> </child>\n\t<!---->\n\t</node>"), STR("<node><child>   </child><child><deep>  </deep></child></node>"), 7},
        // current implementation of parse_ws_pcdata_single has an unfortunate bug; reproduce it here
        {4, STR("<node>\t\t<!---->\n\n</node>"), STR("<node>\n\n</node>"), 3},
        // error case: terminate PCDATA in the middle
        {7, STR("<node>abcdef"), STR("<node>abcdef</node>"), -3},
        {5, STR("<node>      "), STR("<node />"), -2},
        {2, STR("<node>      "), STR("<node>      </node>"), -3},
        // error case: terminate PCDATA as early as possible
        {7, STR("<node>"), STR("<node />"), -2},
        {7, STR("<node>a"), STR("<node>a</node>"), -3},
        {5, STR("<node> "), STR("<node />"), -2},
        {2, STR("<node> "), STR("<node> </node>"), -3},
    };

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); ++i)
    {
        const test_data_t& td = test_data[i];

        for (int flag = 0; flag < 3; ++flag)
        {
            if (td.mask & (1 << flag))
            {
                unsigned int flags[] = {parse_default, parse_default | parse_ws_pcdata, parse_default | parse_ws_pcdata_single};

                xml_document doc;
                CHECK((td.nodes > 0) == doc.load_string(td.source, flags[flag]));
                CHECK_NODE(doc, td.result);

                int nodes = get_tree_node_count(doc);
                CHECK((td.nodes < 0 ? -td.nodes : td.nodes) == nodes);
            }
        }
    }
}

TEST(parse_ws_pcdata_fragment_permutations)
{
    struct test_data_t
    {
        unsigned int mask; // 1 = default flags, 2 = parse_ws_pcdata, 4 = parse_ws_pcdata_single
        const pugi::char_t* source;
        const pugi::char_t* result;
        int nodes; // negative if parsing should fail
    };

    test_data_t test_data[] =
    {
        // external pcdata should be preserved
        {7, STR("ext1"), STR("ext1"), 2},
        {5, STR("    "), STR(""), 1},
        {2, STR("    "), STR("    "), 2},
        {7, STR("ext1<node/>"), STR("ext1<node />"), 3},
        {7, STR("<node/>ext2"), STR("<node />ext2"), 3},
        {7, STR("ext1<node/>ext2"), STR("ext1<node />ext2"), 4},
        {7, STR("ext1<node1/>ext2<node2/>ext3"), STR("ext1<node1 />ext2<node2 />ext3"), 6},
        {5, STR(" <node/>"), STR("<node />"), 2},
        {2, STR(" <node/>"), STR(" <node />"), 3},
        {5, STR("<node/> "), STR("<node />"), 2},
        {2, STR("<node/> "), STR("<node /> "), 3},
        {5, STR(" <node/> "), STR("<node />"), 2},
        {2, STR(" <node/> "), STR(" <node /> "), 4},
        {5, STR(" <node1/> <node2/> "), STR("<node1 /><node2 />"), 3},
        {2, STR(" <node1/> <node2/> "), STR(" <node1 /> <node2 /> "), 6},
    };

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); ++i)
    {
        const test_data_t& td = test_data[i];

        for (int flag = 0; flag < 3; ++flag)
        {
            if (td.mask & (1 << flag))
            {
                unsigned int flags[] = {parse_default, parse_default | parse_ws_pcdata, parse_default | parse_ws_pcdata_single};

                xml_document doc;
                CHECK((td.nodes > 0) == doc.load_string(td.source, flags[flag] | parse_fragment));
                CHECK_NODE(doc, td.result);

                int nodes = get_tree_node_count(doc);
                CHECK((td.nodes < 0 ? -td.nodes : td.nodes) == nodes);
            }
        }
    }
}

TEST(parse_pcdata_no_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<root>\r\rval1\rval2\r\nval3\nval4\r\r</root>"), parse_minimal));

	CHECK_STRING(doc.child_value(STR("root")), STR("\r\rval1\rval2\r\nval3\nval4\r\r"));
}

TEST(parse_pcdata_eol)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<root>\r\rval1\rval2\r\nval3\nval4\r\r</root>"), parse_minimal | parse_eol));

	CHECK_STRING(doc.child_value(STR("root")), STR("\n\nval1\nval2\nval3\nval4\n\n"));
}

TEST(parse_pcdata_skip_ext)
{
	xml_document doc;
	CHECK(doc.load_string(STR("pre<root/>post"), parse_minimal));
	CHECK(doc.first_child() == doc.last_child());
	CHECK(doc.first_child().type() == node_element);
}

TEST(parse_pcdata_error)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<root>pcdata"), parse_minimal).status == status_end_element_mismatch);
}

TEST(parse_pcdata_trim)
{
    struct test_data_t
    {
        const pugi::char_t* source;
        const pugi::char_t* result;
        unsigned int flags;
    };

    test_data_t test_data[] =
    {
    	{ STR("<node> text</node>"), STR("text"), 0 },
    	{ STR("<node>\t\n text</node>"), STR("text"), 0 },
    	{ STR("<node>text </node>"), STR("text"), 0 },
    	{ STR("<node>text \t\n</node>"), STR("text"), 0 },
    	{ STR("<node>\r\n\t text \t\n\r</node>"), STR("text"), 0 },
    	{ STR(" text"), STR("text"), parse_fragment },
    	{ STR("\t\n text"), STR("text"), parse_fragment },
    	{ STR("text "), STR("text"), parse_fragment },
    	{ STR("text \t\n"), STR("text"), parse_fragment },
    	{ STR("\r\n\t text \t\n\r"), STR("text"), parse_fragment },
    	{ STR("<node>\r\n\t text \t\n\r more \r\n\t</node>"), STR("text \t\n\r more"), 0 },
    	{ STR("<node>\r\n\t text \t\n\r more \r\n\t</node>"), STR("text \t\n\n more"), parse_eol },
    	{ STR("<node>\r\n\t text \r\n\r\n\r\n\r\n\r\n\r\n\r\n more \r\n\t</node>"), STR("text \n\n\n\n\n\n\n more"), parse_eol },
    	{ STR("<node>     test&amp;&amp;&amp;&amp;&amp;&amp;&amp;    </node>"), STR("test&amp;&amp;&amp;&amp;&amp;&amp;&amp;"), 0 },
    	{ STR("<node>     test&amp;&amp;&amp;&amp;&amp;&amp;&amp;    </node>"), STR("test&&&&&&&"), parse_escapes },
        { STR("     test&amp;&amp;&amp;&amp;&amp;&amp;&amp;    "), STR("test&&&&&&&"), parse_fragment | parse_escapes },
        { STR("<node>\r\n\t text \t\n\r m&amp;&amp;e \r\n\t</node>"), STR("text \t\n\n m&&e"), parse_eol | parse_escapes }
    };

    for (size_t i = 0; i < sizeof(test_data) / sizeof(test_data[0]); ++i)
    {
        const test_data_t& td = test_data[i];

        xml_document doc;
        CHECK(doc.load_string(td.source, td.flags | parse_trim_pcdata));

        const pugi::char_t* value = doc.child(STR("node")) ? doc.child_value(STR("node")) : doc.text().get();
        CHECK_STRING(value, td.result);
    }
}

TEST(parse_pcdata_trim_empty)
{
	unsigned int flags[] = { 0, parse_ws_pcdata, parse_ws_pcdata_single, parse_ws_pcdata | parse_ws_pcdata_single };

	for (size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i)
	{
		xml_document doc;
		CHECK(doc.load_string(STR("<node>   </node>"), flags[i] | parse_trim_pcdata));

		xml_node node = doc.child(STR("node"));
		CHECK(node);
		CHECK(!node.first_child());
	}
}

TEST(parse_escapes_skip)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id='&lt;&gt;&amp;&apos;&quot;'>&lt;&gt;&amp;&apos;&quot;</node>"), parse_minimal));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("&lt;&gt;&amp;&apos;&quot;"));
}

TEST(parse_escapes_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id='&lt;&gt;&amp;&apos;&quot;'>&lt;&gt;&amp;&apos;&quot;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("<>&'\""));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("<>&'\""));
}

TEST(parse_escapes_code)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#1;&#32;&#x20;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("\01  "));
}

TEST(parse_escapes_code_exhaustive_dec)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#/;&#01;&#2;&#3;&#4;&#5;&#6;&#7;&#8;&#9;&#:;&#a;&#A;&#XA;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&#/;\x1\x2\x3\x4\x5\x6\x7\x8\x9&#:;&#a;&#A;&#XA;"));
}

TEST(parse_escapes_code_exhaustive_hex)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#x/;&#x01;&#x2;&#x3;&#x4;&#x5;&#x6;&#x7;&#x8;&#x9;&#x:;&#x@;&#xA;&#xB;&#xC;&#xD;&#xE;&#xF;&#xG;&#x`;&#xa;&#xb;&#xc;&#xd;&#xe;&#xf;&#xg;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&#x/;\x1\x2\x3\x4\x5\x6\x7\x8\x9&#x:;&#x@;\xa\xb\xc\xd\xe\xf&#xG;&#x`;\xa\xb\xc\xd\xe\xf&#xg;"));
}

TEST(parse_escapes_code_restore)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#1&#32;&#x1&#32;&#1-&#32;&#x1-&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&#1 &#x1 &#1- &#x1- "));
}

TEST(parse_escapes_char_restore)
{
	xml_document doc;

	CHECK(doc.load_string(STR("<node>&q&#32;&qu&#32;&quo&#32;&quot&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&q &qu &quo &quot "));

	CHECK(doc.load_string(STR("<node>&a&#32;&ap&#32;&apo&#32;&apos&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&a &ap &apo &apos "));

	CHECK(doc.load_string(STR("<node>&a&#32;&am&#32;&amp&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&a &am &amp "));

	CHECK(doc.load_string(STR("<node>&l&#32;&lt&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&l &lt "));

	CHECK(doc.load_string(STR("<node>&g&#32;&gt&#32;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&g &gt "));
}

TEST(parse_escapes_unicode)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#x03B3;&#x03b3;&#x24B62;</node>"), parse_minimal | parse_escapes));

#ifdef PUGIXML_WCHAR_MODE
	const pugi::char_t* v = doc.child_value(STR("node"));

	size_t wcharsize = sizeof(wchar_t);

	CHECK(v[0] == 0x3b3 && v[1] == 0x3b3 && (wcharsize == 2 ? v[2] == wchar_cast(0xd852) && v[3] == wchar_cast(0xdf62) : v[2] == wchar_cast(0x24b62)));
#else
	CHECK_STRING(doc.child_value(STR("node")), "\xce\xb3\xce\xb3\xf0\xa4\xad\xa2");
#endif
}

TEST(parse_escapes_error)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#x03g;&#ab;&quot</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&#x03g;&#ab;&quot"));

	CHECK(!doc.load_string(STR("<node id='&#x12")));
	CHECK(!doc.load_string(STR("<node id='&g")));
	CHECK(!doc.load_string(STR("<node id='&gt")));
	CHECK(!doc.load_string(STR("<node id='&l")));
	CHECK(!doc.load_string(STR("<node id='&lt")));
	CHECK(!doc.load_string(STR("<node id='&a")));
	CHECK(!doc.load_string(STR("<node id='&amp")));
	CHECK(!doc.load_string(STR("<node id='&apos")));
}

TEST(parse_escapes_code_invalid)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node>&#;&#x;&;&#x-;&#-;</node>"), parse_minimal | parse_escapes));
	CHECK_STRING(doc.child_value(STR("node")), STR("&#;&#x;&;&#x-;&#-;"));
}

TEST(parse_escapes_attribute)
{
	xml_document doc;

	for (int wnorm = 0; wnorm < 2; ++wnorm)
		for (int eol = 0; eol < 2; ++eol)
			for (int wconv = 0; wconv < 2; ++wconv)
			{
				unsigned int flags = parse_escapes;

				flags |= (wnorm ? parse_wnorm_attribute : 0);
				flags |= (eol ? parse_eol : 0);
				flags |= (wconv ? parse_wconv_attribute : 0);

				CHECK(doc.load_string(STR("<node id='&quot;'/>"), flags));
				CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("\""));
			}
}

TEST(parse_attribute_spaces)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id1='v1' id2 ='v2' id3= 'v3' id4 = 'v4' id5 \n\r\t = \r\t\n 'v5' />"), parse_minimal));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id1")).value(), STR("v1"));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id2")).value(), STR("v2"));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id3")).value(), STR("v3"));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id4")).value(), STR("v4"));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id5")).value(), STR("v5"));
}

TEST(parse_attribute_quot)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id1='v1' id2=\"v2\"/>"), parse_minimal));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id1")).value(), STR("v1"));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id2")).value(), STR("v2"));
}

TEST(parse_attribute_no_eol_no_wconv)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id=' \t\r\rval1  \rval2\r\nval3\nval4\r\r'/>"), parse_minimal));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR(" \t\r\rval1  \rval2\r\nval3\nval4\r\r"));
}

TEST(parse_attribute_eol_no_wconv)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id=' \t\r\rval1  \rval2\r\nval3\nval4\r\r'/>"), parse_minimal | parse_eol));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR(" \t\n\nval1  \nval2\nval3\nval4\n\n"));
}

TEST(parse_attribute_no_eol_wconv)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id=' \t\r\rval1  \rval2\r\nval3\nval4\r\r'/>"), parse_minimal | parse_wconv_attribute));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("    val1   val2 val3 val4  "));
}

TEST(parse_attribute_eol_wconv)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id=' \t\r\rval1  \rval2\r\nval3\nval4\r\r'/>"), parse_minimal | parse_eol | parse_wconv_attribute));
	CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("    val1   val2 val3 val4  "));
}

TEST(parse_attribute_wnorm)
{
	xml_document doc;

	for (int eol = 0; eol < 2; ++eol)
		for (int wconv = 0; wconv < 2; ++wconv)
		{
			unsigned int flags = parse_minimal | parse_wnorm_attribute | (eol ? parse_eol : 0) | (wconv ? parse_wconv_attribute : 0);
			CHECK(doc.load_string(STR("<node id=' \t\r\rval1  \rval2\r\nval3\nval4\r\r'/>"), flags));
			CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("val1 val2 val3 val4"));
		}
}

TEST(parse_attribute_variations)
{
	xml_document doc;

	for (int wnorm = 0; wnorm < 2; ++wnorm)
		for (int eol = 0; eol < 2; ++eol)
			for (int wconv = 0; wconv < 2; ++wconv)
				for (int escapes = 0; escapes < 2; ++escapes)
				{
					unsigned int flags = parse_minimal;

					flags |= (wnorm ? parse_wnorm_attribute : 0);
					flags |= (eol ? parse_eol : 0);
					flags |= (wconv ? parse_wconv_attribute : 0);
					flags |= (escapes ? parse_escapes : 0);

					CHECK(doc.load_string(STR("<node id='1'/>"), flags));
					CHECK_STRING(doc.child(STR("node")).attribute(STR("id")).value(), STR("1"));
				}
}


TEST(parse_attribute_error)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node id"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id "), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id  "), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id   "), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id/"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id?/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id=/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id='/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id=\"/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id=\"'/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id='\"/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id='\"/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node #/>"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node#/>"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node id1='1'id2='2'/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node id&='1'/>"), parse_minimal).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<node &='1'/>"), parse_minimal).status == status_bad_start_element);
}

TEST(parse_attribute_termination_error)
{
	xml_document doc;

	for (int wnorm = 0; wnorm < 2; ++wnorm)
		for (int eol = 0; eol < 2; ++eol)
			for (int wconv = 0; wconv < 2; ++wconv)
			{
				unsigned int flags = parse_minimal;

				flags |= (wnorm ? parse_wnorm_attribute : 0);
				flags |= (eol ? parse_eol : 0);
				flags |= (wconv ? parse_wconv_attribute : 0);

				CHECK(doc.load_string(STR("<node id='value"), flags).status == status_bad_attribute);
			}
}

TEST(parse_attribute_quot_inside)
{
	xml_document doc;

	for (int wnorm = 0; wnorm < 2; ++wnorm)
		for (int eol = 0; eol < 2; ++eol)
			for (int wconv = 0; wconv < 2; ++wconv)
			{
				unsigned int flags = parse_escapes;

				flags |= (wnorm ? parse_wnorm_attribute : 0);
				flags |= (eol ? parse_eol : 0);
				flags |= (wconv ? parse_wconv_attribute : 0);

				CHECK(doc.load_string(STR("<node id1='\"' id2=\"'\"/>"), flags));
				CHECK_STRING(doc.child(STR("node")).attribute(STR("id1")).value(), STR("\""));
				CHECK_STRING(doc.child(STR("node")).attribute(STR("id2")).value(), STR("'"));
			}
}

TEST(parse_tag_single)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node/><node /><node\n/>"), parse_minimal));
	CHECK_NODE(doc, STR("<node /><node /><node />"));
}

TEST(parse_tag_hierarchy)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node><n1><n2/></n1><n3><n4><n5></n5></n4></n3 \r\n></node>"), parse_minimal));
	CHECK_NODE(doc, STR("<node><n1><n2 /></n1><n3><n4><n5 /></n4></n3></node>"));
}

TEST(parse_tag_error)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<"), parse_minimal).status == status_unrecognized_tag);
	CHECK(doc.load_string(STR("<!"), parse_minimal).status == status_unrecognized_tag);
	CHECK(doc.load_string(STR("<!D"), parse_minimal).status == status_unrecognized_tag);
	CHECK(doc.load_string(STR("<#"), parse_minimal).status == status_unrecognized_tag);
	CHECK(doc.load_string(STR("<node#"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node/"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node /"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node / "), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node / >"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node/ >"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("</ node>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("</node"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("</node "), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("<node></ node>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("<node></node"), parse_minimal).status == status_bad_end_element);
	CHECK(doc.load_string(STR("<node></node "), parse_minimal).status == status_bad_end_element);
	CHECK(doc.load_string(STR("<node></nodes>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("<node>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("<node/><"), parse_minimal).status == status_unrecognized_tag);
	CHECK(doc.load_string(STR("<node attr='value'>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("</></node>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("</node>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("</>"), parse_minimal).status == status_end_element_mismatch);
	CHECK(doc.load_string(STR("<node></node v>"), parse_minimal).status == status_bad_end_element);
	CHECK(doc.load_string(STR("<node&/>"), parse_minimal).status == status_bad_start_element);
	CHECK(doc.load_string(STR("<node& v='1'/>"), parse_minimal).status == status_bad_start_element);
}

TEST(parse_declaration_cases)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<?xml?><?xmL?><?xMl?><?xML?><?Xml?><?XmL?><?XMl?><?XML?>"), parse_fragment | parse_pi));
	CHECK(!doc.first_child());
}

TEST(parse_declaration_attr_cases)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<?xml ?><?xmL ?><?xMl ?><?xML ?><?Xml ?><?XmL ?><?XMl ?><?XML ?>"), parse_fragment | parse_pi));
	CHECK(!doc.first_child());
}

TEST(parse_declaration_skip)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_pi};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<?xml?><?xml version='1.0'?>"), flags));
		CHECK(!doc.first_child());

		CHECK(doc.load_string(STR("<?xml <tag/> ?>"), flags));
		CHECK(!doc.first_child());
	}
}

TEST(parse_declaration_parse)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<?xml?><?xml version='1.0'?>"), parse_fragment | parse_declaration));

	xml_node d1 = doc.first_child();
	xml_node d2 = doc.last_child();

	CHECK(d1 != d2);
	CHECK(d1.type() == node_declaration);
	CHECK_STRING(d1.name(), STR("xml"));
	CHECK(d2.type() == node_declaration);
	CHECK_STRING(d2.name(), STR("xml"));
	CHECK_STRING(d2.attribute(STR("version")).value(), STR("1.0"));
}

TEST(parse_declaration_error)
{
	xml_document doc;

	unsigned int flag_sets[] = {parse_fragment, parse_fragment | parse_declaration};

	for (unsigned int i = 0; i < sizeof(flag_sets) / sizeof(flag_sets[0]); ++i)
	{
		unsigned int flags = flag_sets[i];

		CHECK(doc.load_string(STR("<?xml"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?xml?"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?xml>"), flags).status == status_bad_pi);
		CHECK(doc.load_string(STR("<?xml version='1>"), flags).status == status_bad_pi);
	}
	
	CHECK(doc.load_string(STR("<?xml version='1?>"), parse_fragment | parse_declaration).status == status_bad_attribute);
	CHECK(doc.load_string(STR("<foo><?xml version='1'?></foo>"), parse_fragment | parse_declaration).status == status_bad_pi);
}

TEST(parse_empty)
{
	xml_document doc;

	CHECK(doc.load_string(STR("")).status == status_no_document_element && !doc.first_child());
	CHECK(doc.load_string(STR(""), parse_fragment) && !doc.first_child());
}

TEST(parse_out_of_memory)
{
	test_runner::_memory_fail_threshold = 256;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.load_string(STR("<foo a='1'/>")).status == status_out_of_memory));
	CHECK(!doc.first_child());
}

TEST(parse_out_of_memory_halfway_node)
{
	const unsigned int count = 10000;
	static char_t text[count * 4];

	for (unsigned int i = 0; i < count; ++i)
	{
		text[4*i + 0] = '<';
		text[4*i + 1] = 'n';
		text[4*i + 2] = '/';
		text[4*i + 3] = '>';
	}

	test_runner::_memory_fail_threshold = 65536;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.load_buffer_inplace(text, count * 4).status == status_out_of_memory));
	CHECK_NODE(doc.first_child(), STR("<n />"));
}

TEST(parse_out_of_memory_halfway_attr)
{
	const unsigned int count = 10000;
	static char_t text[count * 5 + 4];

	text[0] = '<';
	text[1] = 'n';

	for (unsigned int i = 0; i < count; ++i)
	{
		text[5*i + 2] = ' ';
		text[5*i + 3] = 'a';
		text[5*i + 4] = '=';
		text[5*i + 5] = '"';
		text[5*i + 6] = '"';
	}

	text[5 * count + 2] = '/';
	text[5 * count + 3] = '>';

	test_runner::_memory_fail_threshold = 65536;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.load_buffer_inplace(text, count * 5 + 4).status == status_out_of_memory));
	CHECK_STRING(doc.first_child().name(), STR("n"));
	CHECK_STRING(doc.first_child().first_attribute().name(), STR("a"));
	CHECK_STRING(doc.first_child().last_attribute().name(), STR("a"));
}

TEST(parse_out_of_memory_conversion)
{
	test_runner::_memory_fail_threshold = 256;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.load_buffer("<foo\x90/>", 7, parse_default, encoding_latin1).status == status_out_of_memory));
	CHECK(!doc.first_child());
}

TEST(parse_out_of_memory_allocator_state_sync)
{
	const unsigned int count = 10000;
	static char_t text[count * 4];

	for (unsigned int i = 0; i < count; ++i)
	{
		text[4*i + 0] = '<';
		text[4*i + 1] = 'n';
		text[4*i + 2] = '/';
		text[4*i + 3] = '>';
	}

	test_runner::_memory_fail_threshold = 65536;

	xml_document doc;
	CHECK_ALLOC_FAIL(CHECK(doc.load_buffer_inplace(text, count * 4).status == status_out_of_memory));
	CHECK_NODE(doc.first_child(), STR("<n />"));

	test_runner::_memory_fail_threshold = 0;

	for (unsigned int j = 0; j < count; ++j)
		CHECK(doc.append_child(STR("n")));
}

static bool test_offset(const char_t* contents, unsigned int options, pugi::xml_parse_status status, ptrdiff_t offset)
{
	xml_document doc;
	xml_parse_result res = doc.load_string(contents, options);

	return res.status == status && res.offset == offset;
}

#define CHECK_OFFSET(contents, options, status, offset) CHECK(test_offset(STR(contents), options, status, offset))

TEST(parse_error_offset)
{
	CHECK_OFFSET("<node/>", parse_default, status_ok, 0);

	test_runner::_memory_fail_threshold = 1;
	CHECK_ALLOC_FAIL(CHECK_OFFSET("<node/>", parse_default, status_out_of_memory, 0));
	test_runner::_memory_fail_threshold = 0;

	CHECK_OFFSET("<3d/>", parse_default, status_unrecognized_tag, 1);
	CHECK_OFFSET(" <3d/>", parse_default, status_unrecognized_tag, 2);
	CHECK_OFFSET(" <", parse_default, status_unrecognized_tag, 1);

	CHECK_OFFSET("<?pi", parse_default, status_bad_pi, 3);
	CHECK_OFFSET("<?pi", parse_default | parse_pi, status_bad_pi, 3);
	CHECK_OFFSET("<?xml", parse_default | parse_declaration, status_bad_pi, 4);

	CHECK_OFFSET("<!----", parse_default, status_bad_comment, 5);
	CHECK_OFFSET("<!----", parse_default | parse_comments, status_bad_comment, 4);

	CHECK_OFFSET("<![CDA", parse_default, status_bad_cdata, 5);
	CHECK_OFFSET("<![CDATA[non-terminated]]", parse_default, status_bad_cdata, 9);

	CHECK_OFFSET("<!DOCTYPE doc", parse_default, status_bad_doctype, 12);
	CHECK_OFFSET("<!DOCTYPE greeting [ <!ATTLIST list type    (bullets|ordered|glossary)  \"orde", parse_default, status_bad_doctype, 76);

	CHECK_OFFSET("<node", parse_default, status_bad_start_element, 4);
	CHECK_OFFSET("<node ", parse_default, status_bad_start_element, 5);
	CHECK_OFFSET("<nod%>", parse_default, status_bad_start_element, 5);

	CHECK_OFFSET("<node a=2>", parse_default, status_bad_attribute, 8);
	CHECK_OFFSET("<node a='2>", parse_default, status_bad_attribute, 9);

	CHECK_OFFSET("<n></n $>", parse_default, status_bad_end_element, 7);
	CHECK_OFFSET("<n></n", parse_default, status_bad_end_element, 5);

	CHECK_OFFSET("<no></na>", parse_default, status_end_element_mismatch, 8);
	CHECK_OFFSET("<no></nod>", parse_default, status_end_element_mismatch, 9);
}

TEST(parse_result_default)
{
	xml_parse_result result;

	CHECK(!result);
	CHECK(result.status == status_internal_error);
	CHECK(result.offset == 0);
	CHECK(result.encoding == encoding_auto);
}

TEST(parse_bom_fragment)
{
	struct test_data_t
	{
		xml_encoding encoding;
		const char* data;
		size_t size;
		const char_t* text;
	};

	const test_data_t data[] =
	{
		{ encoding_utf8, "\xef\xbb\xbf", 3, STR("") },
		{ encoding_utf8, "\xef\xbb\xbftest", 7, STR("test") },
		{ encoding_utf16_be, "\xfe\xff", 2, STR("") },
		{ encoding_utf16_be, "\xfe\xff\x00t\x00o\x00s\x00t", 10, STR("tost") },
		{ encoding_utf16_le, "\xff\xfe", 2, STR("") },
		{ encoding_utf16_le, "\xff\xfet\x00o\x00s\x00t\x00", 10, STR("tost") },
		{ encoding_utf32_be, "\x00\x00\xfe\xff", 4, STR("") },
		{ encoding_utf32_be, "\x00\x00\xfe\xff\x00\x00\x00t\x00\x00\x00o\x00\x00\x00s\x00\x00\x00t", 20, STR("tost") },
		{ encoding_utf32_le, "\xff\xfe\x00\x00", 4, STR("") },
		{ encoding_utf32_le, "\xff\xfe\x00\x00t\x00\x00\x00o\x00\x00\x00s\x00\x00\x00t\x00\x00\x00", 20, STR("tost") },
	};

	for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i)
	{
		xml_document doc;
		CHECK(doc.load_buffer(data[i].data, data[i].size, parse_fragment, data[i].encoding));
		CHECK_STRING(doc.text().get(), data[i].text);
		CHECK(save_narrow(doc, format_no_declaration | format_raw | format_write_bom, data[i].encoding) == std::string(data[i].data, data[i].size));
	}
}

TEST(parse_bom_fragment_invalid_utf8)
{
	xml_document doc;

	CHECK(doc.load_buffer("\xef\xbb\xbb", 3, parse_fragment, encoding_utf8));

	const char_t* value = doc.text().get();

#ifdef PUGIXML_WCHAR_MODE
	CHECK(value[0] == wchar_cast(0xfefb) && value[1] == 0);
#else
	CHECK_STRING(value, "\xef\xbb\xbb");
#endif
}

TEST(parse_bom_fragment_invalid_utf16)
{
	xml_document doc;

	CHECK(doc.load_buffer("\xff\xfe", 2, parse_fragment, encoding_utf16_be));

	const char_t* value = doc.text().get();

#ifdef PUGIXML_WCHAR_MODE
	CHECK(value[0] == wchar_cast(0xfffe) && value[1] == 0);
#else
	CHECK_STRING(value, "\xef\xbf\xbe");
#endif
}

TEST(parse_bom_fragment_invalid_utf32)
{
	xml_document doc;

	CHECK(doc.load_buffer("\xff\xff\x00\x00", 4, parse_fragment, encoding_utf32_le));

	const char_t* value = doc.text().get();

#ifdef PUGIXML_WCHAR_MODE
	CHECK(value[0] == wchar_cast(0xffff) && value[1] == 0);
#else
	CHECK_STRING(value, "\xef\xbf\xbf");
#endif
}

TEST(parse_pcdata_gap_fragment)
{
	xml_document doc;
	CHECK(doc.load_string(STR("a&amp;b"), parse_fragment | parse_escapes));
	CHECK_STRING(doc.text().get(), STR("a&b"));
}

TEST(parse_name_end_eof)
{
	char_t test[] = STR("<node>");

	xml_document doc;
	CHECK(doc.load_buffer_inplace(test, 6 * sizeof(char_t)).status == status_end_element_mismatch);
	CHECK_STRING(doc.first_child().name(), STR("node"));
}

TEST(parse_close_tag_eof)
{
	char_t test1[] = STR("<node></node");
	char_t test2[] = STR("<node></nodx");

	xml_document doc;
	CHECK(doc.load_buffer_inplace(test1, 12 * sizeof(char_t)).status == status_bad_end_element);
	CHECK_STRING(doc.first_child().name(), STR("node"));

	CHECK(doc.load_buffer_inplace(test2, 12 * sizeof(char_t)).status == status_end_element_mismatch);
	CHECK_STRING(doc.first_child().name(), STR("node"));
}

TEST(parse_fuzz_doctype)
{
	unsigned char data[] =
	{
		0x3b, 0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45, 0xef, 0xbb, 0xbf, 0x3c, 0x3f, 0x78,
		0x6d, 0x6c, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x3d, 0x22, 0x31, 0x2e, 0x30, 0x22,
		0x3f, 0x3e, 0x3c, 0x21, 0x2d, 0x2d, 0x20, 0xe9, 0x80, 0xb1, 0xe5, 0xa0, 0xb1, 0xe3, 0x82, 0xb4,
		0xe3, 0x83, 0xb3, 0x20, 0xef, 0x83, 0x97, 0xe3, 0xa9, 0x2a, 0x20, 0x2d, 0x2d, 0x3e
	};

	xml_document doc;
	CHECK(doc.load_buffer(data, sizeof(data)).status == status_bad_doctype);
}
