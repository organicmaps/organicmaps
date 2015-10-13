#include "writer_string.hpp"

#include "test.hpp"

static bool test_narrow(const std::string& result, const char* expected, size_t length)
{
	// check result
	if (result != std::string(expected, expected + length)) return false;

	// check comparison operator (incorrect implementation can theoretically early-out on zero terminators...)
	if (length > 0 && result == std::string(expected, expected + length - 1) + "?") return false;

	return true;
}

void xml_writer_string::write(const void* data, size_t size)
{
	contents.append(static_cast<const char*>(data), size);
}

std::string xml_writer_string::as_narrow() const
{
	return contents;
}

std::basic_string<wchar_t> xml_writer_string::as_wide() const
{
	CHECK(contents.size() % sizeof(wchar_t) == 0);

    // round-trip pointer through void* to avoid pointer alignment warnings; contents data should be heap allocated => safe to cast
	return std::basic_string<wchar_t>(static_cast<const wchar_t*>(static_cast<const void*>(contents.data())), contents.size() / sizeof(wchar_t));
}

std::basic_string<pugi::char_t> xml_writer_string::as_string() const
{
#ifdef PUGIXML_WCHAR_MODE // to avoid "condition is always true" warning in BCC
	CHECK(contents.size() % sizeof(pugi::char_t) == 0);
#endif

    // round-trip pointer through void* to avoid pointer alignment warnings; contents data should be heap allocated => safe to cast
	return std::basic_string<pugi::char_t>(static_cast<const pugi::char_t*>(static_cast<const void*>(contents.data())), contents.size() / sizeof(pugi::char_t));
}

std::string save_narrow(const pugi::xml_document& doc, unsigned int flags, pugi::xml_encoding encoding)
{
	xml_writer_string writer;

	doc.save(writer, STR("\t"), flags, encoding);

	return writer.as_narrow();
}

bool test_save_narrow(const pugi::xml_document& doc, unsigned int flags, pugi::xml_encoding encoding, const char* expected, size_t length)
{
	return test_narrow(save_narrow(doc, flags, encoding), expected, length);
}

std::string write_narrow(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding)
{
	xml_writer_string writer;

	node.print(writer, STR("\t"), flags, encoding);

	return writer.as_narrow();
}

bool test_write_narrow(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding, const char* expected, size_t length)
{
	return test_narrow(write_narrow(node, flags, encoding), expected, length);
}

std::basic_string<wchar_t> write_wide(pugi::xml_node node, unsigned int flags, pugi::xml_encoding encoding)
{
	xml_writer_string writer;

	node.print(writer, STR("\t"), flags, encoding);

	return writer.as_wide();
}
