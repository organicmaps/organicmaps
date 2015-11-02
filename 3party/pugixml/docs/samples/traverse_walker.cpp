#include "pugixml.hpp"

#include <iostream>

const char* node_types[] =
{
    "null", "document", "element", "pcdata", "cdata", "comment", "pi", "declaration"
};

// tag::impl[]
struct simple_walker: pugi::xml_tree_walker
{
    virtual bool for_each(pugi::xml_node& node)
    {
        for (int i = 0; i < depth(); ++i) std::cout << "  "; // indentation

        std::cout << node_types[node.type()] << ": name='" << node.name() << "', value='" << node.value() << "'\n";

        return true; // continue traversal
    }
};
// end::impl[]

int main()
{
    pugi::xml_document doc;
    if (!doc.load_file("tree.xml")) return -1;

    // tag::traverse[]
    simple_walker walker;
    doc.traverse(walker);
    // end::traverse[]
}

// vim:et
