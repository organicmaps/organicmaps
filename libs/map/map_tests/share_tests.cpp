#include "testing/testing.hpp"

#include "map/share.hpp"

#include "indexer/feature_meta.hpp"

#include <string>

namespace share_tests
{
using namespace share;
using feature::Metadata;

Strings TestStrings()
{
  return {"I am here on Organic Maps", "Open in Organic Maps or in a browser", "Open in Another App",
          "Get Organic Maps"};
}

// Counts non-overlapping occurrences of |what| in |where|.
size_t Count(std::string const & where, std::string const & what)
{
  size_t count = 0;
  for (size_t p = where.find(what); p != std::string::npos; p = where.find(what, p + what.size()))
    ++count;
  return count;
}

UNIT_TEST(Share_Build_Poi)
{
  Place place;
  place.m_name = "Eiffel Tower";
  place.m_typeLabel = "Tourist attraction";
  place.m_address = "Champ de Mars, 5 Av. Anatole France";
  place.m_ll = {48.858093, 2.294694};
  place.m_zoom = 16;
  place.m_fields = {{Metadata::FMD_OPEN_HOURS, "Mo-Su 09:00-00:00"},
                    {Metadata::FMD_PHONE_NUMBER, "+33 1 11; +3322"},
                    {Metadata::FMD_WEBSITE, "https://toureiffel.paris; menu.toureiffel.paris"}};

  Result const r = Build(place, TestStrings());

  // The ge0 short link, shared until ParseClearCoordinates is out in the wild (see share.cpp).
  TEST_EQUAL(r.m_url, "https://omaps.app/w4CNuoc9QN/Eiffel_Tower", ());
  TEST_EQUAL(r.m_subjectBasis, "Eiffel Tower", ());
  TEST(!r.m_isMyPosition, ());

  // Plain body: name, address, coordinates, link (no metadata, no geo:).
  TEST_EQUAL(r.m_text,
             "Eiffel Tower\nChamp de Mars, 5 Av. Anatole France\n"
             "48.858093, 2.294694\nhttps://omaps.app/w4CNuoc9QN/Eiffel_Tower",
             ());

  // HTML body: heading, type, address, metadata (multiple phones), action links, coordinates.
  TEST(r.m_html.find("<b>Eiffel Tower</b>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("Tourist attraction") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("Mo-Su 09:00-00:00") != std::string::npos, (r.m_html));
  // A tel: uri must not contain spaces, while the displayed number keeps them.
  TEST(r.m_html.find("<a href=\"tel:+33111\">+33 1 11</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"tel:+3322\">+3322</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"https://toureiffel.paris\">toureiffel.paris</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"https://menu.toureiffel.paris\">menu.toureiffel.paris</a>") != std::string::npos,
       (r.m_html));
  TEST(r.m_html.find("Open in Organic Maps or in a browser") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"geo:") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("https://omaps.app/get") != std::string::npos, (r.m_html));
}

UNIT_TEST(Share_Build_MyPosition)
{
  Place place;
  place.m_isMyPosition = true;
  place.m_address = "5 Av. Anatole France";
  place.m_ll = {48.858093, 2.294694};
  place.m_zoom = 17;

  Result const r = Build(place, TestStrings());

  // No name in the link; the my-position heading is used instead.
  TEST_EQUAL(r.m_url, "https://omaps.app/04CNuoc9QN", ());
  // No name, so the subject basis falls back to the address.
  TEST_EQUAL(r.m_subjectBasis, "5 Av. Anatole France", ());
  TEST_EQUAL(r.m_text,
             "I am here on Organic Maps\n5 Av. Anatole France\n"
             "48.858093, 2.294694\nhttps://omaps.app/04CNuoc9QN",
             ());
  TEST(r.m_html.find("<b>I am here on Organic Maps</b>") != std::string::npos, (r.m_html));
  TEST(r.m_isMyPosition, ());
}

// place_page::Info shows the address as the title of an unnamed building (SetTitlesAndSubtitle),
// keeping the address itself set - it must not be shared twice.
UNIT_TEST(Share_Build_UnnamedBuilding)
{
  Place place;
  place.m_name = "5 Avenue Anatole France";
  place.m_address = place.m_name;
  place.m_ll = {48.858093, 2.294694};
  place.m_zoom = 17;

  Result const r = Build(place, TestStrings());
  TEST_EQUAL(r.m_text,
             "5 Avenue Anatole France\n48.858093, 2.294694\nhttps://omaps.app/04CNuoc9QN/5_Avenue_Anatole_France", ());
  TEST_EQUAL(Count(r.m_html, "5 Avenue Anatole France"), 1, (r.m_html));
}

// place_page::Info shows the coordinates as the subtitle of an unmatched map point
// (SetCustomNameWithCoordinates) - the body prints them on its own line instead.
UNIT_TEST(Share_Build_UnknownPlace)
{
  Place place;
  place.m_typeLabel = "48.858093, 2.294694";
  place.m_address = "Champ de Mars";
  place.m_ll = {48.858093, 2.294694};
  place.m_zoom = 17;

  Result const r = Build(place, TestStrings());
  TEST_EQUAL(r.m_text, "Champ de Mars\n48.858093, 2.294694\nhttps://omaps.app/04CNuoc9QN", ());
  TEST_EQUAL(Count(r.m_html, "48.858093, 2.294694"), 1, (r.m_html));
}

// Tapping water or an area with no downloaded map gives a place with nothing but coordinates:
// no name, no type, no address and no metadata - the mail must not open with an empty line.
UNIT_TEST(Share_Build_BareMapPoint)
{
  Place place;
  place.m_typeLabel = "48.858093, 2.294694";
  place.m_ll = {48.858093, 2.294694};
  place.m_zoom = 17;

  Result const r = Build(place, TestStrings());
  TEST_EQUAL(r.m_text, "48.858093, 2.294694\nhttps://omaps.app/04CNuoc9QN", ());
  TEST(r.m_subjectBasis.empty(), (r.m_subjectBasis));
  TEST(!r.m_html.starts_with("<br>"), (r.m_html));
  TEST(r.m_html.starts_with("<a href="), (r.m_html));
}

// URL schemes are case-insensitive, and OSM website tags are not normalized on import.
UNIT_TEST(Share_Build_UpperCaseWebsiteScheme)
{
  Place place;
  place.m_name = "Cafe";
  place.m_ll = {1.0, 2.0};
  place.m_zoom = 15;
  place.m_fields = {{Metadata::FMD_WEBSITE, "HTTP://example.com"}};

  Result const r = Build(place, TestStrings());
  TEST(r.m_html.find("<a href=\"HTTP://example.com\">example.com</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("https://HTTP://") == std::string::npos, (r.m_html));
}

UNIT_TEST(Share_Build_HtmlEscaping)
{
  Place place;
  place.m_name = "Ben & Jerry's <ice>";
  place.m_ll = {1.0, 2.0};
  place.m_zoom = 15;

  Result const r = Build(place, TestStrings());
  TEST(r.m_html.find("<b>Ben &amp; Jerry's &lt;ice&gt;</b>") != std::string::npos, (r.m_html));
}
}  // namespace share_tests
