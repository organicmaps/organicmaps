#include "../src/pugixml.hpp"
#include "allocator.hpp"

int main(int argc, const char** argv)
{
    pugi::set_memory_management_functions(memory_allocate, memory_deallocate);

    pugi::xml_document doc;

    for (int i = 1; i < argc; ++i)
    {
	    doc.load_file(argv[i]);
	    doc.load_file(argv[i], pugi::parse_minimal);
	    doc.load_file(argv[i], pugi::parse_full);
	}
}
