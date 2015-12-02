#include "pugixml.hpp"

#include <iostream>

int main()
{
    pugi::xml_document doc;

// tag::code[]
    const char* source = "<!--comment--><node>&lt;</node>";

    // Parsing with default options; note that comment node is not added to the tree, and entity reference &lt; is expanded
    doc.load_string(source);
    std::cout << "First node value: [" << doc.first_child().value() << "], node child value: [" << doc.child_value("node") << "]\n";

    // Parsing with additional parse_comments option; comment node is now added to the tree
    doc.load_string(source, pugi::parse_default | pugi::parse_comments);
    std::cout << "First node value: [" << doc.first_child().value() << "], node child value: [" << doc.child_value("node") << "]\n";

    // Parsing with additional parse_comments option and without the (default) parse_escapes option; &lt; is not expanded
    doc.load_string(source, (pugi::parse_default | pugi::parse_comments) & ~pugi::parse_escapes);
    std::cout << "First node value: [" << doc.first_child().value() << "], node child value: [" << doc.child_value("node") << "]\n";

    // Parsing with minimal option mask; comment node is not added to the tree, and &lt; is not expanded
    doc.load_string(source, pugi::parse_minimal);
    std::cout << "First node value: [" << doc.first_child().value() << "], node child value: [" << doc.child_value("node") << "]\n";
// end::code[]
}

// vim:et
