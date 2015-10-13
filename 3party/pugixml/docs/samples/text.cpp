#include "pugixml.hpp"

#include <iostream>

int main()
{
    pugi::xml_document doc;

    // get a test document
    doc.load_string("<project><name>test</name><version>1.1</version><public>yes</public></project>");

    pugi::xml_node project = doc.child("project");

    // tag::access[]
    std::cout << "Project name: " << project.child("name").text().get() << std::endl;
    std::cout << "Project version: " << project.child("version").text().as_double() << std::endl;
    std::cout << "Project visibility: " << (project.child("public").text().as_bool(/* def= */ true) ? "public" : "private") << std::endl;
    std::cout << "Project description: " << project.child("description").text().get() << std::endl;
    // end::access[]

    std::cout << std::endl;

    // tag::modify[]
    // change project version
    project.child("version").text() = 1.2;

    // add description element and set the contents
    // note that we do not have to explicitly add the node_pcdata child
    project.append_child("description").text().set("a test project");
    // end::modify[]

    doc.save(std::cout);
}

// vim:et
