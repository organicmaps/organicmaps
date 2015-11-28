#ifndef PUGIXML_NO_XPATH

#include <string.h> // because Borland's STL is braindead, we have to include <string.h> _before_ <string> in order to get memcmp

#include "common.hpp"

#include "helpers.hpp"

#include <string>
#include <vector>

TEST_XML(xpath_api_select_nodes, "<node><head/><foo/><foo/><tail/></node>")
{
	xpath_node_set ns1 = doc.select_nodes(STR("node/foo"));

	xpath_query q(STR("node/foo"));
	xpath_node_set ns2 = doc.select_nodes(q);

	xpath_node_set_tester(ns1, "ns1") % 4 % 5;
	xpath_node_set_tester(ns2, "ns2") % 4 % 5;
}

TEST_XML(xpath_api_select_node, "<node><head/><foo id='1'/><foo/><tail/></node>")
{
	xpath_node n1 = doc.select_node(STR("node/foo"));

	xpath_query q(STR("node/foo"));
	xpath_node n2 = doc.select_node(q);

	CHECK(n1.node().attribute(STR("id")).as_int() == 1);
	CHECK(n2.node().attribute(STR("id")).as_int() == 1);

	xpath_node n3 = doc.select_node(STR("node/bar"));
	
	CHECK(!n3);

	xpath_node n4 = doc.select_node(STR("node/head/following-sibling::foo"));
	xpath_node n5 = doc.select_node(STR("node/tail/preceding-sibling::foo"));
	
	CHECK(n4.node().attribute(STR("id")).as_int() == 1);
	CHECK(n5.node().attribute(STR("id")).as_int() == 1);
}

TEST_XML(xpath_api_node_bool_ops, "<node attr='value'/>")
{
	generic_bool_ops_test(doc.select_node(STR("node")));
	generic_bool_ops_test(doc.select_node(STR("node/@attr")));
}

TEST_XML(xpath_api_node_eq_ops, "<node attr='value'/>")
{
	generic_eq_ops_test(doc.select_node(STR("node")), doc.select_node(STR("node/@attr")));
}

TEST_XML(xpath_api_node_accessors, "<node attr='value'/>")
{
	xpath_node null;
	xpath_node node = doc.select_node(STR("node"));
	xpath_node attr = doc.select_node(STR("node/@attr"));

	CHECK(!null.node());
	CHECK(!null.attribute());
	CHECK(!null.parent());

	CHECK(node.node() == doc.child(STR("node")));
	CHECK(!node.attribute());
	CHECK(node.parent() == doc);

	CHECK(!attr.node());
	CHECK(attr.attribute() == doc.child(STR("node")).attribute(STR("attr")));
	CHECK(attr.parent() == doc.child(STR("node")));
}

inline void xpath_api_node_accessors_helper(const xpath_node_set& set)
{
	CHECK(set.size() == 2);
	CHECK(set.type() == xpath_node_set::type_sorted);
	CHECK(!set.empty());
	CHECK_STRING(set[0].node().name(), STR("foo"));
	CHECK_STRING(set[1].node().name(), STR("foo"));
	CHECK(set.first() == set[0]);
	CHECK(set.begin() + 2 == set.end());
	CHECK(set.begin()[0] == set[0] && set.begin()[1] == set[1]);
}

TEST_XML(xpath_api_nodeset_accessors, "<node><foo/><foo/></node>")
{
	xpath_node_set null;
	CHECK(null.size() == 0);
	CHECK(null.type() == xpath_node_set::type_unsorted);
	CHECK(null.empty());
	CHECK(!null.first());
	CHECK(null.begin() == null.end());

	xpath_node_set set = doc.select_nodes(STR("node/foo"));
	xpath_api_node_accessors_helper(set);

	xpath_node_set copy = set;
	xpath_api_node_accessors_helper(copy);

	xpath_node_set assigned;
	assigned = set;
	xpath_api_node_accessors_helper(assigned);

	xpath_node_set nullcopy = null;
}

TEST_XML(xpath_api_nodeset_copy, "<node><foo/><foo/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/foo"));

	xpath_node_set copy1 = set;
	CHECK(copy1.size() == 2);
	CHECK_STRING(copy1[0].node().name(), STR("foo"));

	xpath_node_set copy2;
	copy2 = set;
	CHECK(copy2.size() == 2);
	CHECK_STRING(copy2[0].node().name(), STR("foo"));

	xpath_node_set copy3;
	copy3 = set;
	copy3 = copy3;
	CHECK(copy3.size() == 2);
	CHECK_STRING(copy3[0].node().name(), STR("foo"));

	xpath_node_set copy4;
	copy4 = set;
	copy4 = copy1;
	CHECK(copy4.size() == 2);
	CHECK_STRING(copy4[0].node().name(), STR("foo"));

	xpath_node_set copy5;
	copy5 = set;
	copy5 = xpath_node_set();
	CHECK(copy5.size() == 0);
}

TEST(xpath_api_nodeset_copy_empty)
{
    xpath_node_set set;
    xpath_node_set set2 = set;
    xpath_node_set set3;
    set3 = set;
}

TEST_XML(xpath_api_evaluate, "<node attr='3'/>")
{
	xpath_query q(STR("node/@attr"));

	CHECK(q.evaluate_boolean(doc));
	CHECK(q.evaluate_number(doc) == 3);

	char_t string[3];
	CHECK(q.evaluate_string(string, 3, doc) == 2 && string[0] == '3' && string[1] == 0);

#ifndef PUGIXML_NO_STL
	CHECK(q.evaluate_string(doc) == STR("3"));
#endif

	xpath_node_set ns = q.evaluate_node_set(doc);
	CHECK(ns.size() == 1 && ns[0].attribute() == doc.child(STR("node")).attribute(STR("attr")));

	xpath_node nr = q.evaluate_node(doc);
	CHECK(nr.attribute() == doc.child(STR("node")).attribute(STR("attr")));
}

TEST_XML(xpath_api_evaluate_attr, "<node attr='3'/>")
{
	xpath_query q(STR("."));
	xpath_node n(doc.child(STR("node")).attribute(STR("attr")), doc.child(STR("node")));

	CHECK(q.evaluate_boolean(n));
	CHECK(q.evaluate_number(n) == 3);

	char_t string[3];
	CHECK(q.evaluate_string(string, 3, n) == 2 && string[0] == '3' && string[1] == 0);

#ifndef PUGIXML_NO_STL
	CHECK(q.evaluate_string(n) == STR("3"));
#endif

	xpath_node_set ns = q.evaluate_node_set(n);
	CHECK(ns.size() == 1 && ns[0] == n);

	xpath_node nr = q.evaluate_node(n);
	CHECK(nr == n);
}

#ifdef PUGIXML_NO_EXCEPTIONS
TEST_XML(xpath_api_evaluate_fail, "<node attr='3'/>")
{
	xpath_query q(STR(""));

	CHECK(q.evaluate_boolean(doc) == false);
	CHECK_DOUBLE_NAN(q.evaluate_number(doc));

	CHECK(q.evaluate_string(0, 0, doc) == 1); // null terminator

#ifndef PUGIXML_NO_STL
	CHECK(q.evaluate_string(doc).empty());
#endif

	CHECK(q.evaluate_node_set(doc).empty());

	CHECK(!q.evaluate_node(doc));
}
#endif

TEST(xpath_api_evaluate_node_set_fail)
{
	xpath_query q(STR("1"));

#ifdef PUGIXML_NO_EXCEPTIONS
	CHECK(q.evaluate_node_set(xml_node()).empty());
#else
	try
	{
		q.evaluate_node_set(xml_node());

		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (const xpath_exception&)
	{
	}
#endif
}

TEST(xpath_api_evaluate_node_fail)
{
	xpath_query q(STR("1"));

#ifdef PUGIXML_NO_EXCEPTIONS
	CHECK(!q.evaluate_node(xml_node()));
#else
	try
	{
		q.evaluate_node(xml_node());

		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (const xpath_exception&)
	{
	}
#endif
}

TEST(xpath_api_evaluate_string)
{
	xpath_query q(STR("\"0123456789\""));

	std::basic_string<char_t> base = STR("xxxxxxxxxxxxxxxx");

	// test for enough space
	std::basic_string<char_t> s0 = base;
	CHECK(q.evaluate_string(&s0[0], 16, xml_node()) == 11 && memcmp(&s0[0], STR("0123456789\0xxxxx"), 16 * sizeof(char_t)) == 0);

	// test for just enough space
	std::basic_string<char_t> s1 = base;
	CHECK(q.evaluate_string(&s1[0], 11, xml_node()) == 11 && memcmp(&s1[0], STR("0123456789\0xxxxx"), 16 * sizeof(char_t)) == 0);
	
	// test for just not enough space
	std::basic_string<char_t> s2 = base;
	CHECK(q.evaluate_string(&s2[0], 10, xml_node()) == 11 && memcmp(&s2[0], STR("012345678\0xxxxxx"), 16 * sizeof(char_t)) == 0);

	// test for not enough space
	std::basic_string<char_t> s3 = base;
	CHECK(q.evaluate_string(&s3[0], 5, xml_node()) == 11 && memcmp(&s3[0], STR("0123\0xxxxxxxxxxx"), 16 * sizeof(char_t)) == 0);

	// test for single character buffer
	std::basic_string<char_t> s4 = base;
	CHECK(q.evaluate_string(&s4[0], 1, xml_node()) == 11 && memcmp(&s4[0], STR("\0xxxxxxxxxxxxxxx"), 16 * sizeof(char_t)) == 0);

	// test for empty buffer
	std::basic_string<char_t> s5 = base;
	CHECK(q.evaluate_string(&s5[0], 0, xml_node()) == 11 && memcmp(&s5[0], STR("xxxxxxxxxxxxxxxx"), 16 * sizeof(char_t)) == 0);
	CHECK(q.evaluate_string(0, 0, xml_node()) == 11);
}

TEST(xpath_api_return_type)
{
#ifdef PUGIXML_NO_EXCEPTIONS
	CHECK(xpath_query(STR("")).return_type() == xpath_type_none);
#endif

	CHECK(xpath_query(STR("node")).return_type() == xpath_type_node_set);
	CHECK(xpath_query(STR("1")).return_type() == xpath_type_number);
	CHECK(xpath_query(STR("'s'")).return_type() == xpath_type_string);
	CHECK(xpath_query(STR("true()")).return_type() == xpath_type_boolean);
}

TEST(xpath_api_query_bool)
{
	xpath_query q(STR("node"));
	
	CHECK(q);
	CHECK((!q) == false);
}

#ifdef PUGIXML_NO_EXCEPTIONS
TEST(xpath_api_query_bool_fail)
{
	xpath_query q(STR(""));
	
	CHECK((q ? true : false) == false);
	CHECK((!q) == true);
}
#endif

TEST(xpath_api_query_result)
{
	xpath_query q(STR("node"));

	CHECK(q.result());
	CHECK(q.result().error == 0);
	CHECK(q.result().offset == 0);
	CHECK(strcmp(q.result().description(), "No error") == 0);
}

TEST(xpath_api_query_result_fail)
{
#ifndef PUGIXML_NO_EXCEPTIONS
	try
	{
#endif
		xpath_query q(STR("//foo/child::/bar"));

#ifndef PUGIXML_NO_EXCEPTIONS
		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (const xpath_exception& q)
	{
#endif
		xpath_parse_result result = q.result();

		CHECK(!result);
		CHECK(result.error != 0 && result.error[0] != 0);
		CHECK(result.description() == result.error);
		CHECK(result.offset == 13);

#ifndef PUGIXML_NO_EXCEPTIONS
	}
#endif
}

#ifndef PUGIXML_NO_EXCEPTIONS
TEST(xpath_api_exception_what)
{
	try
	{
		xpath_query q(STR(""));

		CHECK_FORCE_FAIL("Expected exception");
	}
	catch (const xpath_exception& e)
	{
		CHECK(e.what()[0] != 0);
	}
}
#endif

TEST(xpath_api_node_set_ctor_out_of_memory)
{
	test_runner::_memory_fail_threshold = 1;

	xpath_node data[2];

	CHECK_ALLOC_FAIL(xpath_node_set ns(data, data + 2));
}

TEST(xpath_api_node_set_copy_ctor_out_of_memory)
{
	xpath_node data[2];
	xpath_node_set ns(data, data + 2);

	test_runner::_memory_fail_threshold = 1;

	CHECK_ALLOC_FAIL(xpath_node_set copy = ns);
}

TEST_XML(xpath_api_node_set_assign_out_of_memory_preserve, "<node><a/><b/></node>")
{
	xpath_node_set ns = doc.select_nodes(STR("node/*"));
	CHECK(ns.size() == 2);
	CHECK(ns.type() == xpath_node_set::type_sorted);

	xpath_node_set nsall = doc.select_nodes(STR("//*"));
	nsall.sort(true);
	CHECK(nsall.size() == 3);
	CHECK(nsall.type() == xpath_node_set::type_sorted_reverse);

	test_runner::_memory_fail_threshold = 1;

	CHECK_ALLOC_FAIL(ns = nsall);

	CHECK(ns.size() == 2);
	CHECK(ns.type() == xpath_node_set::type_sorted);
	CHECK(ns[0] == doc.child(STR("node")).child(STR("a")) && ns[1] == doc.child(STR("node")).child(STR("b")));
}

TEST_XML(xpath_api_deprecated_select_single_node, "<node><head/><foo id='1'/><foo/><tail/></node>")
{
	xpath_node n1 = doc.select_single_node(STR("node/foo"));

	xpath_query q(STR("node/foo"));
	xpath_node n2 = doc.select_single_node(q);

	CHECK(n1.node().attribute(STR("id")).as_int() == 1);
	CHECK(n2.node().attribute(STR("id")).as_int() == 1);
}

TEST(xpath_api_empty)
{
	xml_node c;

	xpath_query q;
	CHECK(!q);
	CHECK(!q.evaluate_boolean(c));
}

#if __cplusplus >= 201103
TEST_XML(xpath_api_nodeset_move_ctor, "<node><foo/><foo/><bar/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/bar/preceding::*"));

	CHECK(set.size() == 2);
	CHECK(set.type() == xpath_node_set::type_sorted_reverse);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 2);
	CHECK(move.type() == xpath_node_set::type_sorted_reverse);
	CHECK(move[1] == doc.first_child().first_child());
}


TEST_XML(xpath_api_nodeset_move_ctor_single, "<node><foo/><foo/><bar/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/bar"));

	CHECK(set.size() == 1);
	CHECK(set.type() == xpath_node_set::type_sorted);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 1);
	CHECK(move.type() == xpath_node_set::type_sorted);
	CHECK(move[0] == doc.first_child().last_child());
}

TEST(xpath_api_nodeset_move_ctor_empty)
{
	xpath_node_set set;
	set.sort();

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_sorted);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 0);
	CHECK(move.type() == xpath_node_set::type_sorted);
}

TEST_XML(xpath_api_nodeset_move_assign, "<node><foo/><foo/><bar/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/bar/preceding::*"));

	CHECK(set.size() == 2);
	CHECK(set.type() == xpath_node_set::type_sorted_reverse);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move;

	CHECK(move.size() == 0);
	CHECK(move.type() == xpath_node_set::type_unsorted);

	move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 2);
	CHECK(move.type() == xpath_node_set::type_sorted_reverse);
	CHECK(move[1] == doc.first_child().first_child());
}

TEST_XML(xpath_api_nodeset_move_assign_destroy, "<node><foo/><foo/><bar/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/bar/preceding::*"));

	CHECK(set.size() == 2);
	CHECK(set.type() == xpath_node_set::type_sorted_reverse);

	xpath_node_set all = doc.select_nodes(STR("//*"));

	CHECK(all.size() == 4);

	test_runner::_memory_fail_threshold = 1;

	all = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(all.size() == 2);
	CHECK(all.type() == xpath_node_set::type_sorted_reverse);
	CHECK(all[1] == doc.first_child().first_child());
}

TEST_XML(xpath_api_nodeset_move_assign_single, "<node><foo/><foo/><bar/></node>")
{
	xpath_node_set set = doc.select_nodes(STR("node/bar"));

	CHECK(set.size() == 1);
	CHECK(set.type() == xpath_node_set::type_sorted);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move;

	CHECK(move.size() == 0);
	CHECK(move.type() == xpath_node_set::type_unsorted);

	move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 1);
	CHECK(move.type() == xpath_node_set::type_sorted);
	CHECK(move[0] == doc.first_child().last_child());
}

TEST(xpath_api_nodeset_move_assign_empty)
{
	xpath_node_set set;
	set.sort();

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_sorted);

	test_runner::_memory_fail_threshold = 1;

	xpath_node_set move;

	CHECK(move.size() == 0);
	CHECK(move.type() == xpath_node_set::type_unsorted);

	move = std::move(set);

	CHECK(set.size() == 0);
	CHECK(set.type() == xpath_node_set::type_unsorted);

	CHECK(move.size() == 0);
	CHECK(move.type() == xpath_node_set::type_sorted);
}

TEST(xpath_api_query_move)
{
	xml_node c;

	xpath_query q1(STR("true()"));
	xpath_query q4(STR("true() and false()"));

	test_runner::_memory_fail_threshold = 1;

	CHECK(q1);
	CHECK(q1.evaluate_boolean(c));

	xpath_query q2 = std::move(q1);
	CHECK(!q1);
	CHECK(!q1.evaluate_boolean(c));
	CHECK(q2);
	CHECK(q2.evaluate_boolean(c));

	xpath_query q3;
	CHECK(!q3);
	CHECK(!q3.evaluate_boolean(c));

	q3 = std::move(q2);
	CHECK(!q2);
	CHECK(!q2.evaluate_boolean(c));
	CHECK(q3);
	CHECK(q3.evaluate_boolean(c));

	CHECK(q4);
	CHECK(!q4.evaluate_boolean(c));

	q4 = std::move(q3);

	CHECK(!q3);
	CHECK(!q3.evaluate_boolean(c));
	CHECK(q4);
	CHECK(q4.evaluate_boolean(c));

	q4 = std::move(*&q4);

	CHECK(q4);
	CHECK(q4.evaluate_boolean(c));
}

TEST(xpath_api_query_vector)
{
	std::vector<xpath_query> qv;

	for (int i = 0; i < 10; ++i)
	{
		char_t expr[2];
		expr[0] = char_t('0' + i);
		expr[1] = 0;

		qv.push_back(xpath_query(expr));
	}

	double result = 0;

	for (auto& q: qv)
		result += q.evaluate_number(xml_node());

	CHECK(result == 45);
}
#endif
#endif
