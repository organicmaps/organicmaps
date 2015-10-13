#include "pugixml.hpp"

#include <iostream>
#include <string>

int main()
{
    pugi::xml_document doc;
    if (!doc.load_file("xgconsole.xml")) return -1;

// tag::code[]
    // Select nodes via compiled query
    pugi::xpath_query query_remote_tools("/Profile/Tools/Tool[@AllowRemote='true']");

    pugi::xpath_node_set tools = query_remote_tools.evaluate_node_set(doc);
    std::cout << "Remote tool: ";
    tools[2].node().print(std::cout);

    // Evaluate numbers via compiled query
    pugi::xpath_query query_timeouts("sum(//Tool/@Timeout)");
    std::cout << query_timeouts.evaluate_number(doc) << std::endl;

    // Evaluate strings via compiled query for different context nodes
    pugi::xpath_query query_name_valid("string-length(substring-before(@Filename, '_')) > 0 and @OutputFileMasks");
    pugi::xpath_query query_name("concat(substring-before(@Filename, '_'), ' produces ', @OutputFileMasks)");

    for (pugi::xml_node tool = doc.first_element_by_path("Profile/Tools/Tool"); tool; tool = tool.next_sibling())
    {
        std::string s = query_name.evaluate_string(tool);

        if (query_name_valid.evaluate_boolean(tool)) std::cout << s << std::endl;
    }
// end::code[]
}

// vim:et
