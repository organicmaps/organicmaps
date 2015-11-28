#include "pugixml.hpp"

#include <iostream>

int main()
{
    // tag::code[]
    // get a test document
    pugi::xml_document doc;
    doc.load_string("<foo bar='baz'><call>hey</call></foo>");

    // add a custom declaration node
    pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    decl.append_attribute("standalone") = "no";

    // <?xml version="1.0" encoding="UTF-8" standalone="no"?> 
    // <foo bar="baz">
    //         <call>hey</call>
    // </foo>
    doc.save(std::cout);
    std::cout << std::endl;
    // end::code[]
}

// vim:et
