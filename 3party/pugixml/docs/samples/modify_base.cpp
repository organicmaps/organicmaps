#include "pugixml.hpp"

#include <string.h>
#include <iostream>

int main()
{
    pugi::xml_document doc;
    if (!doc.load_string("<node id='123'>text</node><!-- comment -->", pugi::parse_default | pugi::parse_comments)) return -1;

    // tag::node[]
    pugi::xml_node node = doc.child("node");

    // change node name
    std::cout << node.set_name("notnode");
    std::cout << ", new node name: " << node.name() << std::endl;

    // change comment text
    std::cout << doc.last_child().set_value("useless comment");
    std::cout << ", new comment text: " << doc.last_child().value() << std::endl;

    // we can't change value of the element or name of the comment
    std::cout << node.set_value("1") << ", " << doc.last_child().set_name("2") << std::endl;
    // end::node[]

    // tag::attr[]
    pugi::xml_attribute attr = node.attribute("id");

    // change attribute name/value
    std::cout << attr.set_name("key") << ", " << attr.set_value("345");
    std::cout << ", new attribute: " << attr.name() << "=" << attr.value() << std::endl;

    // we can use numbers or booleans
    attr.set_value(1.234);
    std::cout << "new attribute value: " << attr.value() << std::endl;

    // we can also use assignment operators for more concise code
    attr = true;
    std::cout << "final attribute value: " << attr.value() << std::endl;
    // end::attr[]
}

// vim:et
