#include "pugixml.hpp"

#include <iostream>

int main()
{
    // tag::code[]
    // get a test document
    pugi::xml_document doc;
    doc.load_string("<foo bar='baz'><call>hey</call></foo>");

    // default options; prints
    // <?xml version="1.0"?>
    // <foo bar="baz">
    //         <call>hey</call>
    // </foo>
    doc.save(std::cout);
    std::cout << std::endl;

    // default options with custom indentation string; prints
    // <?xml version="1.0"?>
    // <foo bar="baz">
    // --<call>hey</call>
    // </foo>
    doc.save(std::cout, "--");
    std::cout << std::endl;

    // default options without indentation; prints
    // <?xml version="1.0"?>
    // <foo bar="baz">
    // <call>hey</call>
    // </foo>
    doc.save(std::cout, "\t", pugi::format_default & ~pugi::format_indent); // can also pass "" instead of indentation string for the same effect
    std::cout << std::endl;

    // raw output; prints
    // <?xml version="1.0"?><foo bar="baz"><call>hey</call></foo>
    doc.save(std::cout, "\t", pugi::format_raw);
    std::cout << std::endl << std::endl;

    // raw output without declaration; prints
    // <foo bar="baz"><call>hey</call></foo>
    doc.save(std::cout, "\t", pugi::format_raw | pugi::format_no_declaration);
    std::cout << std::endl;
    // end::code[]
}

// vim:et
