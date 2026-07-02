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
  return {"I am here on Organic Maps", "Open in Organic Maps or in a browser", "Open in another maps app",
          "Get Organic Maps"};
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
                    {Metadata::FMD_PHONE_NUMBER, "+3311; +3322"},
                    {Metadata::FMD_WEBSITE, "https://toureiffel.paris"}};

  Result const r = Build(place, TestStrings());

  TEST_EQUAL(r.m_url, "https://omaps.app/48.858093,2.294694/Eiffel_Tower?z=16", ());
  TEST_EQUAL(r.m_subjectBasis, "Eiffel Tower", ());

  // Plain body: name, address, link only (no metadata, no geo:).
  TEST_EQUAL(r.m_text,
             "Eiffel Tower\nChamp de Mars, 5 Av. Anatole France\n"
             "https://omaps.app/48.858093,2.294694/Eiffel_Tower?z=16",
             ());

  // HTML body: heading, type, address, metadata (multiple phones), action links, coordinates.
  TEST(r.m_html.find("<b>Eiffel Tower</b>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("Tourist attraction") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("Mo-Su 09:00-00:00") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"tel:+3311\">+3311</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"tel:+3322\">+3322</a>") != std::string::npos, (r.m_html));
  TEST(r.m_html.find("<a href=\"https://toureiffel.paris\">toureiffel.paris</a>") != std::string::npos, (r.m_html));
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
  TEST_EQUAL(r.m_url, "https://omaps.app/48.858093,2.294694?z=17", ());
  // No name, so the subject basis falls back to the address.
  TEST_EQUAL(r.m_subjectBasis, "5 Av. Anatole France", ());
  TEST(r.m_text.starts_with("I am here on Organic Maps\n"), (r.m_text));
  TEST(r.m_html.find("<b>I am here on Organic Maps</b>") != std::string::npos, (r.m_html));
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
