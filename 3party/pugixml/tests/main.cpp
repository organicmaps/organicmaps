#include "test.hpp"
#include "allocator.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <assert.h>

#include <string>

#ifndef PUGIXML_NO_EXCEPTIONS
#   include <exception>
#endif

#ifdef _WIN32_WCE
#   undef DebugBreak
#   pragma warning(disable: 4201) // nonstandard extension used: nameless struct/union
#   include <windows.h>
#endif

test_runner* test_runner::_tests = 0;
size_t test_runner::_memory_fail_threshold = 0;
bool test_runner::_memory_fail_triggered = false;
jmp_buf test_runner::_failure_buffer;
const char* test_runner::_failure_message;
const char* test_runner::_temp_path;

static size_t g_memory_total_size = 0;
static size_t g_memory_total_count = 0;
static size_t g_memory_fail_triggered = false;

static void* custom_allocate(size_t size)
{
	if (test_runner::_memory_fail_threshold > 0 && test_runner::_memory_fail_threshold < g_memory_total_size + size)
	{
		g_memory_fail_triggered = true;
		test_runner::_memory_fail_triggered = true;

		return 0;
	}
	else
	{
		void* ptr = memory_allocate(size);
		assert(ptr);

		g_memory_total_size += memory_size(ptr);
		g_memory_total_count++;
		
		return ptr;
	}
}

#ifndef PUGIXML_NO_EXCEPTIONS
static void* custom_allocate_throw(size_t size)
{
	void* result = custom_allocate(size);

	if (!result)
		throw std::bad_alloc();

	return result;
}
#endif

static void custom_deallocate(void* ptr)
{
	assert(ptr);

	g_memory_total_size -= memory_size(ptr);
	g_memory_total_count--;
	
	memory_deallocate(ptr);
}

static void replace_memory_management()
{
	// create some document to touch original functions
	{
		pugi::xml_document doc;
		doc.append_child().set_name(STR("node"));
	}

	// replace functions
	pugi::set_memory_management_functions(custom_allocate, custom_deallocate);
}

#if defined(_MSC_VER) && _MSC_VER > 1200 && _MSC_VER < 1400 && !defined(__INTEL_COMPILER) && !defined(__DMC__)
#include <exception>

namespace std
{
	_CRTIMP2 _Prhand _Raise_handler;
	_CRTIMP2 void __cdecl _Throw(const exception&) {}
}
#endif

static bool run_test(test_runner* test, const char* test_name, pugi::allocation_function allocate)
{
#ifndef PUGIXML_NO_EXCEPTIONS
	try
	{
#endif
		g_memory_total_size = 0;
		g_memory_total_count = 0;
		g_memory_fail_triggered = false;
		test_runner::_memory_fail_threshold = 0;
		test_runner::_memory_fail_triggered = false;
	
		pugi::set_memory_management_functions(allocate, custom_deallocate);
		
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4611) // interaction between _setjmp and C++ object destruction is non-portable
#   pragma warning(disable: 4793) // function compiled as native: presence of '_setjmp' makes a function unmanaged
#endif

		volatile int result = setjmp(test_runner::_failure_buffer);
	
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

		if (result)
		{
			printf("Test %s failed: %s\n", test_name, test_runner::_failure_message);
			return false;
		}

		test->run();

		if (test_runner::_memory_fail_triggered)
		{
			printf("Test %s failed: unguarded memory fail triggered\n", test_name);
			return false;
		}

		if (g_memory_total_size != 0 || g_memory_total_count != 0)
		{
			printf("Test %s failed: memory leaks found (%u bytes in %u allocations)\n", test_name, static_cast<unsigned int>(g_memory_total_size), static_cast<unsigned int>(g_memory_total_count));
			return false;
		}

		return true;
#ifndef PUGIXML_NO_EXCEPTIONS
	}
	catch (const std::exception& e)
	{
		printf("Test %s failed: exception %s\n", test_name, e.what());
		return false;
	}
	catch (...)
	{
		printf("Test %s failed for unknown reason\n", test_name);
		return false;
	}
#endif
}

#if defined(__CELLOS_LV2__) && defined(PUGIXML_NO_EXCEPTIONS) && !defined(__SNC__)
#include <exception>

void std::exception::_Raise() const
{
	abort();
}
#endif

int main(int, char** argv)
{
#ifdef __BORLANDC__
	_control87(MCW_EM | PC_53, MCW_EM | MCW_PC);
#endif

	// setup temp path as the executable folder
	std::string temp = argv[0];
	std::string::size_type slash = temp.find_last_of("\\/");
	temp.erase((slash != std::string::npos) ? slash + 1 : 0);

	test_runner::_temp_path = temp.c_str();
	
	replace_memory_management();

	unsigned int total = 0;
	unsigned int passed = 0;

	test_runner* test = 0; // gcc3 "variable might be used uninitialized in this function" bug workaround

	for (test = test_runner::_tests; test; test = test->_next)
	{
		total++;
		passed += run_test(test, test->_name, custom_allocate);

	#ifndef PUGIXML_NO_EXCEPTIONS
		if (g_memory_fail_triggered)
		{
			total++;
			passed += run_test(test, (test->_name + std::string(" (throw)")).c_str(), custom_allocate_throw);
		}
	#endif
	}

	unsigned int failed = total - passed;

	if (failed != 0)
		printf("FAILURE: %u out of %u tests failed.\n", failed, total);
	else
		printf("Success: %u tests passed.\n", total);

	return failed;
}

#ifdef _WIN32_WCE
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return main(0, NULL);
}
#endif
