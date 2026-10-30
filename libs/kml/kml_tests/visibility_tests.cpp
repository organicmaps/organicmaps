#include "testing/testing.hpp"

#include "kml/serdes.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace visibility_tests
{
UNIT_TEST(Kml_XmlBooleanVisibilitySurvivesSerialization)
{
  std::pair<std::string, std::string> const cases[] = {{"false", "true"}, {"true", "false"}, {"0", "1"}, {"1", "0"}};
  for (auto const & [categoryValue, placemarkValue] : cases)
  {
    auto const placemark = [&](std::string const & geometry)
    { return "<Placemark><visibility>" + placemarkValue + "</visibility>" + geometry + "</Placemark>"; };
    std::string const input = "<kml><Document><visibility>" + categoryValue + "</visibility>" +
                              placemark("<Point><coordinates>1,1</coordinates></Point>") +
                              placemark(
                                  "<Style><LineStyle><color>ff0000ff</color></LineStyle></Style>"
                                  "<LineString><coordinates>1,1 2,2</coordinates></LineString>") +
                              placemark(
                                  "<Polygon><outerBoundaryIs><LinearRing><coordinates>1,1 3,1 1,3 1,1</coordinates>"
                                  "</LinearRing></outerBoundaryIs></Polygon>") +
                              "</Document></kml>";
    bool const categoryVisible = categoryValue == "1" || categoryValue == "true";
    bool const placemarkVisible = placemarkValue == "1" || placemarkValue == "true";
    auto const check = [&](kml::FileData const & data)
    {
      TEST_EQUAL(data.m_bookmarksData.size(), 1, ());
      TEST_EQUAL(data.m_tracksData.size(), 1, ());
      TEST_EQUAL(data.m_areasData.size(), 1, ());
      TEST_EQUAL(data.m_categoryData.m_visible, categoryVisible, (categoryValue));
      TEST_EQUAL(data.m_bookmarksData.front().m_visible, placemarkVisible, (placemarkValue));
      TEST_EQUAL(data.m_tracksData.front().m_visible, placemarkVisible, (placemarkValue));
      TEST_EQUAL(data.m_areasData.front().m_visible, placemarkVisible, (placemarkValue));
    };
    kml::FileData data;
    kml::DeserializerKml(data).Deserialize(MemReader(input.data(), input.size()));
    check(data);
    std::string saved;
    MemWriter writer(saved);
    kml::KmlWriter(writer).Write(data);
    kml::FileData loaded;
    kml::DeserializerKml(loaded).Deserialize(MemReader(saved.data(), saved.size()));
    check(loaded);
  }
}
}  // namespace visibility_tests
