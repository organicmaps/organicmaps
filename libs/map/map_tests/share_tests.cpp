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
                    {Metadata::FMD_PHONE_NUMBER, "+33 1 11; +3322"},
                    {Metadata::FMD_WEBSITE, "https://toureiffel.paris; menu.toureiffel.paris"}};

  Result const r = Build(place, TestStrings());

  // The ge0 short link, shared until ParseClearCoordinates is out in the wild (see share.cpp).
  TEST_EQUAL(r.m_url, "https://omaps.app/w4CNuoc9QN/Eiffel_Tower", ());
  TEST_EQUAL(r.m_subjectBasis, "Eiffel Tower", ());

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
