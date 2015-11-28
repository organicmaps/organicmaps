#include "common.hpp"

#include "writer_string.hpp"
#include "allocator.hpp"

#include <string>
#include <vector>

namespace
{
	int page_allocs = 0;
	int page_deallocs = 0;

	bool is_page(size_t size)
	{
		return size >= 16384;
	}

	void* allocate(size_t size)
	{
		void* ptr = memory_allocate(size);
		page_allocs += is_page(memory_size(ptr));
		return ptr;
	}

	void deallocate(void* ptr)
	{
		page_deallocs += is_page(memory_size(ptr));
		memory_deallocate(ptr);
	}
}

TEST(memory_custom_memory_management)
{
	page_allocs = page_deallocs = 0;

	// remember old functions
	allocation_function old_allocate = get_memory_allocation_function();
	deallocation_function old_deallocate = get_memory_deallocation_function();

	// replace functions
	set_memory_management_functions(allocate, deallocate);

	{
		// parse document
		xml_document doc;

		CHECK(page_allocs == 0 && page_deallocs == 0);

		CHECK(doc.load_string(STR("<node />")));
	
		CHECK(page_allocs == 1 && page_deallocs == 0);

		// modify document (no new page)
		CHECK(doc.first_child().set_name(STR("foobars")));
		CHECK(page_allocs == 1 && page_deallocs == 0);

		// modify document (new page)
		std::basic_string<pugi::char_t> s(65536, 'x');

		CHECK(doc.first_child().set_name(s.c_str()));
		CHECK(page_allocs == 2 && page_deallocs == 0);

		// modify document (new page, old one should die)
		s += s;

		CHECK(doc.first_child().set_name(s.c_str()));
		CHECK(page_allocs == 3 && page_deallocs == 1);
	}

	CHECK(page_allocs == 3 && page_deallocs == 3);

	// restore old functions
	set_memory_management_functions(old_allocate, old_deallocate);
}

TEST(memory_large_allocations)
{
	page_allocs = page_deallocs = 0;

	// remember old functions
	allocation_function old_allocate = get_memory_allocation_function();
	deallocation_function old_deallocate = get_memory_deallocation_function();

	// replace functions
	set_memory_management_functions(allocate, deallocate);

	{
		xml_document doc;

		CHECK(page_allocs == 0 && page_deallocs == 0);

		// initial fill
		for (size_t i = 0; i < 128; ++i)
		{
			std::basic_string<pugi::char_t> s(i * 128, 'x');

			CHECK(doc.append_child(node_pcdata).set_value(s.c_str()));
		}

		CHECK(page_allocs > 0 && page_deallocs == 0);

		// grow-prune loop
		while (doc.first_child())
		{
			pugi::xml_node node;

			// grow
			for (node = doc.first_child(); node; node = node.next_sibling())
			{
				std::basic_string<pugi::char_t> s = node.value();

				CHECK(node.set_value((s + s).c_str()));
			}

			// prune
			for (node = doc.first_child(); node; )
			{
				pugi::xml_node next = node.next_sibling().next_sibling();

				node.parent().remove_child(node);

				node = next;
			}
		}

		CHECK(page_allocs == page_deallocs + 1); // only one live page left (it waits for new allocations)

		char buffer;
		CHECK(doc.load_buffer_inplace(&buffer, 0, parse_fragment, get_native_encoding()));

		CHECK(page_allocs == page_deallocs); // no live pages left
	}

	CHECK(page_allocs == page_deallocs); // everything is freed

	// restore old functions
	set_memory_management_functions(old_allocate, old_deallocate);
}

TEST(memory_page_management)
{
	page_allocs = page_deallocs = 0;

	// remember old functions
	allocation_function old_allocate = get_memory_allocation_function();
	deallocation_function old_deallocate = get_memory_deallocation_function();

	// replace functions
	set_memory_management_functions(allocate, deallocate);

	{
		xml_document doc;

		CHECK(page_allocs == 0 && page_deallocs == 0);

		// initial fill
		std::vector<xml_node> nodes;

		for (size_t i = 0; i < 4000; ++i)
		{
			xml_node node = doc.append_child(STR("node"));
			CHECK(node);

			nodes.push_back(node);
		}

		CHECK(page_allocs > 0 && page_deallocs == 0);

		// grow-prune loop
		size_t offset = 0;
		size_t prime = 15485863;

		while (nodes.size() > 0)
		{
			offset = (offset + prime) % nodes.size();

			doc.remove_child(nodes[offset]);

			nodes[offset] = nodes.back();
			nodes.pop_back();
		}

		CHECK(page_allocs == page_deallocs + 1); // only one live page left (it waits for new allocations)

		char buffer;
		CHECK(doc.load_buffer_inplace(&buffer, 0, parse_fragment, get_native_encoding()));

		CHECK(page_allocs == page_deallocs); // no live pages left
	}

	CHECK(page_allocs == page_deallocs); // everything is freed

	// restore old functions
	set_memory_management_functions(old_allocate, old_deallocate);
}

TEST(memory_string_allocate_increasing)
{
	xml_document doc;

	doc.append_child(node_pcdata).set_value(STR("x"));

	std::basic_string<char_t> s = STR("ab");

	for (int i = 0; i < 17; ++i)
	{
		doc.append_child(node_pcdata).set_value(s.c_str());

		s += s;
	}

	std::string result = save_narrow(doc, format_no_declaration | format_raw, encoding_utf8);

	CHECK(result.size() == 262143);
	CHECK(result[0] == 'x');

	for (size_t j = 1; j < result.size(); ++j)
	{
		CHECK(result[j] == (j % 2 ? 'a' : 'b'));
	}
}

TEST(memory_string_allocate_decreasing)
{
	xml_document doc;

	std::basic_string<char_t> s = STR("ab");

	for (int i = 0; i < 17; ++i) s += s;

	for (int j = 0; j < 17; ++j)
	{
		s.resize(s.size() / 2);

		doc.append_child(node_pcdata).set_value(s.c_str());
	}

	doc.append_child(node_pcdata).set_value(STR("x"));

	std::string result = save_narrow(doc, format_no_declaration | format_raw, encoding_utf8);

	CHECK(result.size() == 262143);
	CHECK(result[result.size() - 1] == 'x');

	for (size_t k = 0; k + 1 < result.size(); ++k)
	{
		CHECK(result[k] == (k % 2 ? 'b' : 'a'));
	}
}

TEST(memory_string_allocate_increasing_inplace)
{
	xml_document doc;

	xml_node node = doc.append_child(node_pcdata);

	node.set_value(STR("x"));

	std::basic_string<char_t> s = STR("ab");

	for (int i = 0; i < 17; ++i)
	{
		node.set_value(s.c_str());

		s += s;
	}

	std::string result = save_narrow(doc, format_no_declaration | format_raw, encoding_utf8);

	CHECK(result.size() == 131072);

	for (size_t j = 0; j < result.size(); ++j)
	{
		CHECK(result[j] == (j % 2 ? 'b' : 'a'));
	}
}

TEST(memory_string_allocate_decreasing_inplace)
{
	xml_document doc;

	xml_node node = doc.append_child(node_pcdata);

	std::basic_string<char_t> s = STR("ab");

	for (int i = 0; i < 17; ++i) s += s;

	for (int j = 0; j < 17; ++j)
	{
		s.resize(s.size() / 2);

		node.set_value(s.c_str());
	}

	node.set_value(STR("x"));

	std::string result = save_narrow(doc, format_no_declaration | format_raw, encoding_utf8);

	CHECK(result == "x");
}
