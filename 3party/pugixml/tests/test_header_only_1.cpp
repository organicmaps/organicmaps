#define PUGIXML_HEADER_ONLY
#define pugi pugih

#include "common.hpp"

// Check header guards
#include "../src/pugixml.hpp"
#include "../src/pugixml.hpp"

TEST(header_only_1)
{
	xml_document doc;
	CHECK(doc.load_string(STR("<node/>")));
	CHECK_STRING(doc.first_child().name(), STR("node"));

#ifndef PUGIXML_NO_XPATH
	CHECK(doc.first_child() == doc.select_node(STR("//*")).node());
#endif
}
