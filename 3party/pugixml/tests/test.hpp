#ifndef HEADER_TEST_TEST_HPP
#define HEADER_TEST_TEST_HPP

#include "../src/pugixml.hpp"

#include <setjmp.h>

struct test_runner
{
	test_runner(const char* name)
	{
		_name = name;
		_next = _tests;
		_tests = this;
	}

	virtual ~test_runner() {}

	virtual void run() = 0;

	const char* _name;
	test_runner* _next;

	static test_runner* _tests;
	static size_t _memory_fail_threshold;
	static bool _memory_fail_triggered;
	static jmp_buf _failure_buffer;
	static const char* _failure_message;

	static const char* _temp_path;
};

bool test_string_equal(const pugi::char_t* lhs, const pugi::char_t* rhs);

template <typename Node> inline bool test_node_name_value(const Node& node, const pugi::char_t* name, const pugi::char_t* value)
{
	return test_string_equal(node.name(), name) && test_string_equal(node.value(), value);
}

bool test_node(const pugi::xml_node& node, const pugi::char_t* contents, const pugi::char_t* indent, unsigned int flags);
bool test_double_nan(double value);

#ifndef PUGIXML_NO_XPATH
bool test_xpath_string(const pugi::xpath_node& node, const pugi::char_t* query, pugi::xpath_variable_set* variables, const pugi::char_t* expected);
bool test_xpath_boolean(const pugi::xpath_node& node, const pugi::char_t* query, pugi::xpath_variable_set* variables, bool expected);
bool test_xpath_number(const pugi::xpath_node& node, const pugi::char_t* query, pugi::xpath_variable_set* variables, double expected);
bool test_xpath_number_nan(const pugi::xpath_node& node, const pugi::char_t* query, pugi::xpath_variable_set* variables);

bool test_xpath_fail_compile(const pugi::char_t* query, pugi::xpath_variable_set* variables);

struct xpath_node_set_tester
{
	pugi::xpath_node* document_order;
	size_t document_size;

	pugi::xpath_node_set result;
	unsigned int last;
	const char* message;

	void check(bool condition);

	xpath_node_set_tester(const pugi::xpath_node_set& set, const char* message);
	~xpath_node_set_tester();

	xpath_node_set_tester& operator%(unsigned int expected);
};

#endif

struct dummy_fixture {};

#define TEST_FIXTURE(name, fixture) \
	struct test_runner_helper_##name: fixture \
	{ \
		void run(); \
	}; \
	static struct test_runner_##name: test_runner \
	{ \
		test_runner_##name(): test_runner(#name) {} \
		\
		virtual void run() \
		{ \
			test_runner_helper_##name helper; \
			helper.run(); \
		} \
	} test_runner_instance_##name; \
	void test_runner_helper_##name::run()

#define TEST(name) TEST_FIXTURE(name, dummy_fixture)

#define TEST_XML_FLAGS(name, xml, flags) \
	struct test_fixture_##name \
	{ \
		pugi::xml_document doc; \
		\
		test_fixture_##name() \
		{ \
			CHECK(doc.load_string(PUGIXML_TEXT(xml), flags)); \
		} \
		\
	private: \
		test_fixture_##name(const test_fixture_##name&); \
		test_fixture_##name& operator=(const test_fixture_##name&); \
	}; \
	\
	TEST_FIXTURE(name, test_fixture_##name)

#define TEST_XML(name, xml) TEST_XML_FLAGS(name, xml, pugi::parse_default)

#define CHECK_JOIN(text, file, line) text " at " file ":" #line
#define CHECK_JOIN2(text, file, line) CHECK_JOIN(text, file, line)
#define CHECK_TEXT(condition, text) if (condition) ; else test_runner::_failure_message = CHECK_JOIN2(text, __FILE__, __LINE__), longjmp(test_runner::_failure_buffer, 1)
#define CHECK_FORCE_FAIL(text) test_runner::_failure_message = CHECK_JOIN2(text, __FILE__, __LINE__), longjmp(test_runner::_failure_buffer, 1)

#if (defined(_MSC_VER) && _MSC_VER == 1200) || defined(__MWERKS__) || (defined(__BORLANDC__) && __BORLANDC__ <= 0x540)
#	define STRINGIZE(value) "??" // Some compilers have issues with stringizing expressions that contain strings w/escaping inside
#else
#	define STRINGIZE(value) #value
#endif

#define CHECK(condition) CHECK_TEXT(condition, STRINGIZE(condition) " is false")
#define CHECK_STRING(value, expected) CHECK_TEXT(test_string_equal(value, expected), STRINGIZE(value) " is not equal to " STRINGIZE(expected))
#define CHECK_DOUBLE(value, expected) CHECK_TEXT((value > expected ? value - expected : expected - value) < 1e-6, STRINGIZE(value) " is not equal to " STRINGIZE(expected))
#define CHECK_DOUBLE_NAN(value) CHECK_TEXT(test_double_nan(value), STRINGIZE(value) " is not equal to NaN")
#define CHECK_NAME_VALUE(node, name, value) CHECK_TEXT(test_node_name_value(node, name, value), STRINGIZE(node) " name/value do not match " STRINGIZE(name) " and " STRINGIZE(value))
#define CHECK_NODE_EX(node, expected, indent, flags) CHECK_TEXT(test_node(node, expected, indent, flags), STRINGIZE(node) " contents does not match " STRINGIZE(expected))
#define CHECK_NODE(node, expected) CHECK_NODE_EX(node, expected, PUGIXML_TEXT(""), pugi::format_raw)

#ifndef PUGIXML_NO_XPATH
#define CHECK_XPATH_STRING_VAR(node, query, variables, expected) CHECK_TEXT(test_xpath_string(node, query, variables, expected), STRINGIZE(query) " does not evaluate to " STRINGIZE(expected) " in context " STRINGIZE(node))
#define CHECK_XPATH_BOOLEAN_VAR(node, query, variables, expected) CHECK_TEXT(test_xpath_boolean(node, query, variables, expected), STRINGIZE(query) " does not evaluate to " STRINGIZE(expected) " in context " STRINGIZE(node))
#define CHECK_XPATH_NUMBER_VAR(node, query, variables, expected) CHECK_TEXT(test_xpath_number(node, query, variables, expected), STRINGIZE(query) " does not evaluate to " STRINGIZE(expected) " in context " STRINGIZE(node))
#define CHECK_XPATH_NUMBER_NAN_VAR(node, query, variables) CHECK_TEXT(test_xpath_number_nan(node, query, variables), STRINGIZE(query) " does not evaluate to NaN in context " STRINGIZE(node))
#define CHECK_XPATH_NODESET_VAR(node, query, variables) xpath_node_set_tester(pugi::xpath_query(query, variables).evaluate_node_set(node), CHECK_JOIN2(STRINGIZE(query) " does not evaluate to expected set in context " STRINGIZE(node), __FILE__, __LINE__))
#define CHECK_XPATH_FAIL_VAR(query, variables) CHECK_TEXT(test_xpath_fail_compile(query, variables), STRINGIZE(query) " should not compile")

#define CHECK_XPATH_STRING(node, query, expected) CHECK_XPATH_STRING_VAR(node, query, 0, expected)
#define CHECK_XPATH_BOOLEAN(node, query, expected) CHECK_XPATH_BOOLEAN_VAR(node, query, 0, expected)
#define CHECK_XPATH_NUMBER(node, query, expected) CHECK_XPATH_NUMBER_VAR(node, query, 0, expected)
#define CHECK_XPATH_NUMBER_NAN(node, query) CHECK_XPATH_NUMBER_NAN_VAR(node, query, 0)
#define CHECK_XPATH_NODESET(node, query) CHECK_XPATH_NODESET_VAR(node, query, 0)
#define CHECK_XPATH_FAIL(query) CHECK_XPATH_FAIL_VAR(query, 0)
#endif

#ifdef PUGIXML_NO_EXCEPTIONS
#define CHECK_ALLOC_FAIL(code) do { CHECK(!test_runner::_memory_fail_triggered); code; CHECK(test_runner::_memory_fail_triggered); test_runner::_memory_fail_triggered = false; } while (test_runner::_memory_fail_triggered)
#else
#define CHECK_ALLOC_FAIL(code) do { CHECK(!test_runner::_memory_fail_triggered); try { code; } catch (std::bad_alloc&) {} CHECK(test_runner::_memory_fail_triggered); test_runner::_memory_fail_triggered = false; } while (test_runner::_memory_fail_triggered)
#endif

#define STR(text) PUGIXML_TEXT(text)

#if defined(__DMC__) || defined(__BORLANDC__)
#define U_LITERALS // DMC does not understand \x01234 (it parses first three digits), but understands \u01234
#endif

#if (defined(_MSC_VER) && _MSC_VER == 1200) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER == 800) || defined(__BORLANDC__)
// NaN comparison on MSVC6 is incorrect, see http://www.nabble.com/assertDoubleEquals,-NaN---Microsoft-Visual-Studio-6-td9137859.html
// IC8 and BCC are also affected by the same bug
#	define MSVC6_NAN_BUG 
#endif

inline wchar_t wchar_cast(unsigned int value)
{
	return static_cast<wchar_t>(value); // to avoid C4310 on MSVC
}

bool is_little_endian();
pugi::xml_encoding get_native_encoding();

#endif
