#include "pugixml.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>

void print_doc(const char* message, const pugi::xml_document& doc, const pugi::xml_parse_result& result)
{
    std::cout
        << message
        << "\t: load result '" << result.description() << "'"
        << ", first character of root name: U+" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << pugi::as_wide(doc.first_child().name())[0]
        << ", year: " << doc.first_child().first_child().first_child().child_value()
        << std::endl;
}

bool try_imbue(std::wistream& stream, const char* name)
{
    try
    {
        stream.imbue(std::locale(name));

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

int main()
{
    pugi::xml_document doc;

    {
    // tag::code[]
        std::ifstream stream("weekly-utf-8.xml");
        pugi::xml_parse_result result = doc.load(stream);
    // end::code[]

        // first character of root name: U+9031, year: 1997
        print_doc("UTF8 file from narrow stream", doc, result);
    }

    {
        std::ifstream stream("weekly-utf-16.xml");
        pugi::xml_parse_result result = doc.load(stream);

        // first character of root name: U+9031, year: 1997
        print_doc("UTF16 file from narrow stream", doc, result);
    }

    {
        // Since wide streams are treated as UTF-16/32 ones, you can't load the UTF-8 file from a wide stream
        // directly if you have localized characters; you'll have to provide a UTF8 locale (there is no
        // standard one; you can use utf8_codecvt_facet from Boost or codecvt_utf8 from C++0x)
        std::wifstream stream("weekly-utf-8.xml");

        if (try_imbue(stream, "en_US.UTF-8")) // try Linux encoding
        {
            pugi::xml_parse_result result = doc.load(stream);

            // first character of root name: U+00E9, year: 1997
            print_doc("UTF8 file from wide stream", doc, result);
        }
        else
        {
            std::cout << "UTF-8 locale is not available\n";
        }
    }

    {
        // Since wide streams are treated as UTF-16/32 ones, you can't load the UTF-16 file from a wide stream without
        // using custom codecvt; you can use codecvt_utf16 from C++0x
    }

    {
        // Since encoding names are non-standard, you can't load the Shift-JIS (or any other non-ASCII) file
        // from a wide stream portably
        std::wifstream stream("weekly-shift_jis.xml");

        if (try_imbue(stream, ".932") || // try Microsoft encoding
            try_imbue(stream, "ja_JP.SJIS")) // try Linux encoding; run "localedef -i ja_JP -c -f SHIFT_JIS /usr/lib/locale/ja_JP.SJIS" to get it
        {
            pugi::xml_parse_result result = doc.load(stream);

            // first character of root name: U+9031, year: 1997
            print_doc("Shift-JIS file from wide stream", doc, result);
        }
        else
        {
            std::cout << "Shift-JIS locale is not available\n";
        }
    }
}

// vim:et
