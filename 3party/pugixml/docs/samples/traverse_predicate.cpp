#include "pugixml.hpp"

#include <string.h>
#include <iostream>

// tag::decl[]
bool small_timeout(pugi::xml_node node)
{
    return node.attribute("Timeout").as_int() < 20;
}

struct allow_remote_predicate
{
    bool operator()(pugi::xml_attribute attr) const
    {
        return strcmp(attr.name(), "AllowRemote") == 0;
    }

    bool operator()(pugi::xml_node node) const
    {
        return node.attribute("AllowRemote").as_bool();
    }
};
// end::decl[]

int main()
{
    pugi::xml_document doc;
    if (!doc.load_file("xgconsole.xml")) return -1;

    pugi::xml_node tools = doc.child("Profile").child("Tools");

    // tag::find[]
    // Find child via predicate (looks for direct children only)
    std::cout << tools.find_child(allow_remote_predicate()).attribute("Filename").value() << std::endl;

    // Find node via predicate (looks for all descendants in depth-first order)
    std::cout << doc.find_node(allow_remote_predicate()).attribute("Filename").value() << std::endl;

    // Find attribute via predicate
    std::cout << tools.last_child().find_attribute(allow_remote_predicate()).value() << std::endl;

    // We can use simple functions instead of function objects
    std::cout << tools.find_child(small_timeout).attribute("Filename").value() << std::endl;
    // end::find[]
}

// vim:et
