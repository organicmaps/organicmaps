#include "pugixml.hpp"

#include <iostream>

void check_xml(const char* source)
{
// tag::code[]
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_string(source);

    if (result)
    {
        std::cout << "XML [" << source << "] parsed without errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n\n";
    }
    else
    {
        std::cout << "XML [" << source << "] parsed with errors, attr value: [" << doc.child("node").attribute("attr").value() << "]\n";
        std::cout << "Error description: " << result.description() << "\n";
        std::cout << "Error offset: " << result.offset << " (error at [..." << (source + result.offset) << "]\n\n";
    }
// end::code[]
}

int main()
{
    check_xml("<node attr='value'><child>text</child></node>");
    check_xml("<node attr='value'><child>text</chil></node>");
    check_xml("<node attr='value'><child>text</child>");
    check_xml("<node attr='value\"><child>text</child></node>");
    check_xml("<node attr='value'><#tag /></node>");
}

// vim:et
