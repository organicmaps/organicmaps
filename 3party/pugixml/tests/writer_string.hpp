#ifndef HEADER_TEST_WRITER_STRING_HPP
#define HEADER_TEST_WRITER_STRING_HPP

#include "../src/pugixml.hpp"

#include <string>

struct xml_writer_string: public pugi::xml_writer
{
	std::string contents;

	virtual void write(const void* data, size_t size);

	std::string as_narrow() const;
	std::basic_string<wchar_t> as_wide() const;
	std::basic_string<pugi::char_t> as_string() const;
};

std::string save_narrow(const pugi::xml_document& doc, unsigned int flags, pugi::xml_encoding encoding);
bool test_save_narrow(const pugi::xml_document& doc, unsigned int flags, pugi::xml_encoding encoding, const char* expected, size_t length);

std::string write_narrow(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding);
bool test_write_narrow(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding, const char* expected, size_t length);

std::basic_string<wchar_t> write_wide(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding);

#endif
