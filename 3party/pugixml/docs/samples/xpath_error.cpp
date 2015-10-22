#include "pugixml.hpp"

#include <iostream>

int main()
{
    pugi::xml_document doc;
    if (!doc.load_file("xgconsole.xml")) return -1;

// tag::code[]
    // Exception is thrown for incorrect query syntax
    try
    {
        doc.select_nodes("//nodes[#true()]");
    }
    catch (const pugi::xpath_exception& e)
    {
        std::cout << "Select failed: " << e.what() << std::endl;
    }

    // Exception is thrown for incorrect query semantics
    try
    {
        doc.select_nodes("(123)/next");
    }
    catch (const pugi::xpath_exception& e)
    {
        std::cout << "Select failed: " << e.what() << std::endl;
    }

    // Exception is thrown for query with incorrect return type
    try
    {
        doc.select_nodes("123");
    }
    catch (const pugi::xpath_exception& e)
    {
        std::cout << "Select failed: " << e.what() << std::endl;
    }
// end::code[]
}

// vim:et
