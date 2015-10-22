#include "pugixml.hpp"

#include <iostream>

int main()
{
    // get a test document
    pugi::xml_document doc;
    doc.load_string("<foo bar='baz'>hey</foo>");

    // tag::code[]
    // save document to file
    std::cout << "Saving result: " << doc.save_file("save_file_output.xml") << std::endl;
    // end::code[]
}

// vim:et
