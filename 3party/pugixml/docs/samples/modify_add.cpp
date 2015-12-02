#include "pugixml.hpp"

#include <iostream>

int main()
{
    pugi::xml_document doc;

    // tag::code[]
    // add node with some name
    pugi::xml_node node = doc.append_child("node");

    // add description node with text child
    pugi::xml_node descr = node.append_child("description");
    descr.append_child(pugi::node_pcdata).set_value("Simple node");

    // add param node before the description
    pugi::xml_node param = node.insert_child_before("param", descr);

    // add attributes to param node
    param.append_attribute("name") = "version";
    param.append_attribute("value") = 1.1;
    param.insert_attribute_after("type", param.attribute("name")) = "float";
    // end::code[]

    doc.print(std::cout);
}

// vim:et
