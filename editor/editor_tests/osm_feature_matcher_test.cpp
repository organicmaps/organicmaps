#include "testing/testing.hpp"

#include "editor/osm_feature_matcher.hpp"
#include "editor/xml_feature.hpp"

#include "3party/pugixml/src/pugixml.hpp"


static char const * const osmRawResponse = R"SEP(
<osm version="0.6" generator="CGImap 0.4.0 (22123 thorn-03.openstreetmap.org)" copyright="OpenStreetMap and contributors">
 <bounds minlat="53.8976570" minlon="27.5576615" maxlat="53.8976570" maxlon="27.5576615"/>
 <node id="277171984" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8978034" lon="27.5577642"/>
 <node id="277171986" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8978710" lon="27.5576815"/>
 <node id="2673014345" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977652" lon="27.5578039"/>
 <node id="277171999" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8977484" lon="27.5573596"/>
 <node id="277172019" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8977254" lon="27.5578377"/>
 <node id="277172022" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8976041" lon="27.5575181"/>
 <node id="277172096" visible="true" version="5" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8972383" lon="27.5581521"/>
 <node id="277172108" visible="true" version="5" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8971731" lon="27.5579751"/>
 <node id="420748954" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:42Z" user="iglezz" uid="450366" lat="53.8973934" lon="27.5579876"/>
 <node id="420748956" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:42Z" user="iglezz" uid="450366" lat="53.8976570" lon="27.5576615"/>
 <node id="420748957" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:42Z" user="iglezz" uid="450366" lat="53.8973811" lon="27.5579541"/>
 <way id="25432677" visible="true" version="16" changeset="36051033" timestamp="2015-12-19T17:36:15Z" user="i+f" uid="3070773">
  <nd ref="277171999"/>
  <nd ref="277171986"/>
  <nd ref="277171984"/>
  <nd ref="2673014345"/>
  <nd ref="277172019"/>
  <nd ref="420748956"/>
  <nd ref="277172022"/>
  <nd ref="277171999"/>
  <tag k="addr:housenumber" v="16"/>
  <tag k="addr:postcode" v="220030"/>
  <tag k="addr:street" v="улица Карла Маркса"/>
  <tag k="building" v="yes"/>
  <tag k="name" v="Беллесбумпром"/>
  <tag k="office" v="company"/>
  <tag k="website" v="bellesbumprom.by"/>
 </way>
 <way id="35995664" visible="true" version="7" changeset="35318978" timestamp="2015-11-14T23:39:54Z" user="osm-belarus" uid="86479">
  <nd ref="420748956"/>
  <nd ref="420748957"/>
  <nd ref="420748954"/>
  <nd ref="277172096"/>
  <nd ref="277172108"/>
  <nd ref="277172022"/>
  <nd ref="420748956"/>
  <tag k="addr:housenumber" v="31"/>
  <tag k="addr:postcode" v="220030"/>
  <tag k="addr:street" v="Комсомольская улица"/>
  <tag k="building" v="residential"/>
 </way>
</osm>
)SEP";


UNIT_TEST(GetBestOsmWay_test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmRawResponse, ::strlen(osmRawResponse)), ());
  set<m2::PointD> geometry = {
    {53.8977484, 27.557359},
    {53.8978710, 27.557681},
    {53.8978034, 27.557764},
    {53.8977652, 27.557803},
    {53.8977254, 27.557837},
    {53.8976570, 27.557661},
    {53.8976041, 27.557518},
    {53.8977484, 27.557359}
  };

  auto const bestWay = osm::GetBestOsmWay(osmResponse, geometry);
  TEST_EQUAL(editor::XMLFeature(bestWay).GetName(), "Беллесбумпром", ());
}


UNIT_TEST(GetBestOsmNode_test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmRawResponse, ::strlen(osmRawResponse)), ());

  auto const bestWay = osm::GetBestOsmWay(osmResponse, geometry);
  TEST_EQUAL(editor::XMLFeature(bestWay).GetCenter(), "Беллесбумпром", ());

}
