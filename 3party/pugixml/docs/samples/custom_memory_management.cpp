#include "pugixml.hpp"

#include <new>

// tag::decl[]
void* custom_allocate(size_t size)
{
    return new (std::nothrow) char[size];
}

void custom_deallocate(void* ptr)
{
    delete[] static_cast<char*>(ptr);
}
// end::decl[]

int main()
{
// tag::call[]
    pugi::set_memory_management_functions(custom_allocate, custom_deallocate);
// end::call[]

    pugi::xml_document doc;
    doc.load_string("<node/>");
}

// vim:et
