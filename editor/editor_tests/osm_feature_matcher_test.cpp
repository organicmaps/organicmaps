#include "testing/testing.hpp"

#include "editor/osm_feature_matcher.hpp"
#include "editor/xml_feature.hpp"

#include "3party/pugixml/src/pugixml.hpp"

namespace
{
char const * const osmRawResponseWay = R"SEP(
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

char const * const osmRawResponseNode = R"SEP(
<osm version="0.6" generator="CGImap 0.4.0 (5501 thorn-02.openstreetmap.org)" copyright="OpenStreetMap and contributors">
  <bounds minlat="53.8977000" minlon="27.5578900" maxlat="53.8977700" maxlon="27.5579800"/>
  <node id="2673014342" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8976095" lon="27.5579360"/>
  <node id="2673014343" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977023" lon="27.5582512"/>
  <node id="2673014344" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977366" lon="27.5579628"/>
  <node id="2673014345" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977652" lon="27.5578039"/>
  <node id="2673014347" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977970" lon="27.5579116"/>
  <node id="2673014349" visible="true" version="1" changeset="20577443" timestamp="2014-02-15T14:37:11Z" user="iglezz" uid="450366" lat="53.8977977" lon="27.5581702"/>
  <node id="277172019" visible="true" version="2" changeset="20577443" timestamp="2014-02-15T14:37:39Z" user="iglezz" uid="450366" lat="53.8977254" lon="27.5578377"/>
  <node id="3900254358" visible="true" version="1" changeset="36051033" timestamp="2015-12-19T17:36:14Z" user="i+f" uid="3070773" lat="53.8977398" lon="27.5579251">
    <tag k="name" v="Главное управление капитального строительства"/>
    <tag k="office" v="company"/>
    <tag k="website" v="guks.by"/>
  </node>
  <way id="261703253" visible="true" version="2" changeset="35318978" timestamp="2015-11-14T23:30:51Z" user="osm-belarus" uid="86479">
    <nd ref="2673014345"/>
    <nd ref="2673014347"/>
    <nd ref="2673014344"/>
    <nd ref="2673014349"/>
    <nd ref="2673014343"/>
    <nd ref="2673014342"/>
    <nd ref="277172019"/>
    <nd ref="2673014345"/>
    <tag k="addr:housenumber" v="16"/>
    <tag k="addr:postcode" v="220030"/>
    <tag k="addr:street" v="улица Карла Маркса"/>
    <tag k="building" v="residential"/>
  </way>
</osm>
)SEP";

char const * const osmRawResponseRelation = R"SEP(
<?xml version="1.0" encoding="UTF-8"?>
<osm version="0.6" generator="CGImap 0.4.0 (22560 thorn-01.openstreetmap.org)" copyright="OpenStreetMap and contributors">
 <bounds minlat="55.7509200" minlon="37.6397200" maxlat="55.7515400" maxlon="37.6411300"/>
 <node id="271892032" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:35Z" user="Scondo" uid="421524" lat="55.7524913" lon="37.6397264"/>
 <node id="271892033" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:17Z" user="Scondo" uid="421524" lat="55.7522475" lon="37.6391447"/>
 <node id="583193392" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:07Z" user="Vovanium" uid="87682" lat="55.7507909" lon="37.6404902"/>
 <node id="583193432" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7510964" lon="37.6397197"/>
 <node id="583193426" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7510560" lon="37.6394035"/>
 <node id="583193429" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7512865" lon="37.6396919"/>
 <node id="583193395" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7509787" lon="37.6401799"/>
 <node id="583193415" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7510898" lon="37.6403571"/>
 <node id="583193424" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7508581" lon="37.6399029"/>
 <node id="583193422" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7509689" lon="37.6400415"/>
 <node id="583193398" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7514775" lon="37.6401937"/>
 <node id="583193416" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7513532" lon="37.6405069"/>
 <node id="583193431" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7512162" lon="37.6398695"/>
 <node id="583193390" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7507783" lon="37.6410989"/>
 <node id="583193388" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7509982" lon="37.6416194"/>
 <node id="583193405" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7514149" lon="37.6406910"/>
 <node id="583193408" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7509930" lon="37.6412441"/>
 <node id="583193410" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7509124" lon="37.6406648"/>
 <node id="583193401" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:09Z" user="Vovanium" uid="87682" lat="55.7516648" lon="37.6407506"/>
 <node id="666179513" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7510740" lon="37.6401059"/>
 <node id="666179517" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7511507" lon="37.6403206"/>
 <node id="666179519" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7512863" lon="37.6403782"/>
 <node id="666179521" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7512185" lon="37.6403206"/>
 <node id="666179522" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7512214" lon="37.6400483"/>
 <node id="666179524" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7513393" lon="37.6401111"/>
 <node id="666179526" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7514337" lon="37.6402525"/>
 <node id="666179528" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7507439" lon="37.6406349"/>
 <node id="666179530" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7507291" lon="37.6407868"/>
 <node id="666179531" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7507380" lon="37.6409544"/>
 <node id="666179540" visible="true" version="1" changeset="4128036" timestamp="2010-03-14T18:30:48Z" user="Vovanium" uid="87682" lat="55.7508736" lon="37.6408287"/>
 <node id="595699492" visible="true" version="2" changeset="4128036" timestamp="2010-03-14T18:31:08Z" user="Vovanium" uid="87682" lat="55.7508900" lon="37.6410015"/>
 <node id="271892037" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:03Z" user="Scondo" uid="421524" lat="55.7519689" lon="37.6393462"/>
 <node id="666179544" visible="true" version="2" changeset="8261156" timestamp="2011-05-27T10:18:03Z" user="Scondo" uid="421524" lat="55.7523858" lon="37.6394615"/>
 <node id="271892040" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:09Z" user="Scondo" uid="421524" lat="55.7518044" lon="37.6401900"/>
 <node id="271892039" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:11Z" user="Scondo" uid="421524" lat="55.7518997" lon="37.6400631"/>
 <node id="271892031" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:23Z" user="Scondo" uid="421524" lat="55.7517772" lon="37.6406618"/>
 <node id="271892036" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:23Z" user="Scondo" uid="421524" lat="55.7521424" lon="37.6397730"/>
 <node id="271892035" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:25Z" user="Scondo" uid="421524" lat="55.7522520" lon="37.6396264"/>
 <node id="666179542" visible="true" version="2" changeset="8261156" timestamp="2011-05-27T10:18:26Z" user="Scondo" uid="421524" lat="55.7523415" lon="37.6393631"/>
 <node id="271892038" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:30Z" user="Scondo" uid="421524" lat="55.7517353" lon="37.6396389"/>
 <node id="666179545" visible="true" version="2" changeset="8261156" timestamp="2011-05-27T10:18:30Z" user="Scondo" uid="421524" lat="55.7523947" lon="37.6392844"/>
 <node id="271892041" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:34Z" user="Scondo" uid="421524" lat="55.7516804" lon="37.6398672"/>
 <node id="666179548" visible="true" version="2" changeset="8261156" timestamp="2011-05-27T10:18:35Z" user="Scondo" uid="421524" lat="55.7524390" lon="37.6393828"/>
 <node id="271892030" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:38Z" user="Scondo" uid="421524" lat="55.7515240" lon="37.6400640"/>
 <node id="271892034" visible="true" version="4" changeset="8261156" timestamp="2011-05-27T10:18:40Z" user="Scondo" uid="421524" lat="55.7521203" lon="37.6393028"/>
 <node id="2849850611" visible="true" version="2" changeset="33550372" timestamp="2015-08-24T15:55:36Z" user="vadp" uid="326091" lat="55.7507261" lon="37.6405934"/>
 <node id="2849850614" visible="true" version="1" changeset="22264538" timestamp="2014-05-11T07:26:43Z" user="Vadim Zudkin" uid="177747" lat="55.7509233" lon="37.6401297"/>
 <node id="3712207029" visible="true" version="1" changeset="33550372" timestamp="2015-08-24T15:55:35Z" user="vadp" uid="326091" lat="55.7510865" lon="37.6400013"/>
 <node id="3712207030" visible="true" version="1" changeset="33550372" timestamp="2015-08-24T15:55:35Z" user="vadp" uid="326091" lat="55.7512462" lon="37.6399456"/>
 <node id="3712207031" visible="true" version="2" changeset="33550412" timestamp="2015-08-24T15:57:10Z" user="vadp" uid="326091" lat="55.7514944" lon="37.6401534"/>
 <node id="3712207032" visible="true" version="2" changeset="33550412" timestamp="2015-08-24T15:57:10Z" user="vadp" uid="326091" lat="55.7516969" lon="37.6407362">
  <tag k="access" v="private"/>
  <tag k="barrier" v="gate"/>
 </node>
 <node id="3712207033" visible="true" version="2" changeset="33550412" timestamp="2015-08-24T15:57:10Z" user="vadp" uid="326091" lat="55.7517316" lon="37.6408217"/>
 <node id="3712207034" visible="true" version="2" changeset="33550412" timestamp="2015-08-24T15:57:10Z" user="vadp" uid="326091" lat="55.7517602" lon="37.6409066"/>
 <node id="2849850613" visible="true" version="3" changeset="33551686" timestamp="2015-08-24T16:50:21Z" user="vadp" uid="326091" lat="55.7507965" lon="37.6399611"/>
 <node id="338464706" visible="true" version="3" changeset="33551686" timestamp="2015-08-24T16:50:21Z" user="vadp" uid="326091" lat="55.7510322" lon="37.6393637"/>
 <node id="338464708" visible="true" version="7" changeset="33551686" timestamp="2015-08-24T16:50:21Z" user="vadp" uid="326091" lat="55.7515407" lon="37.6383137"/>
 <node id="3755931947" visible="true" version="1" changeset="34206452" timestamp="2015-09-23T13:58:11Z" user="trolleway" uid="397326" lat="55.7517090" lon="37.6407565"/>
 <way id="25009838" visible="true" version="14" changeset="28090002" timestamp="2015-01-12T16:15:21Z" user="midrug" uid="2417727">
  <nd ref="271892030"/>
  <nd ref="271892031"/>
  <nd ref="271892032"/>
  <nd ref="666179544"/>
  <nd ref="666179548"/>
  <nd ref="666179545"/>
  <nd ref="666179542"/>
  <nd ref="271892033"/>
  <nd ref="271892034"/>
  <nd ref="271892035"/>
  <nd ref="271892036"/>
  <nd ref="271892037"/>
  <nd ref="271892038"/>
  <nd ref="271892039"/>
  <nd ref="271892040"/>
  <nd ref="271892041"/>
  <nd ref="271892030"/>
  <tag k="addr:housenumber" v="12-14"/>
  <tag k="addr:street" v="улица Солянка"/>
  <tag k="building" v="yes"/>
  <tag k="building:colour" v="lightpink"/>
  <tag k="building:levels" v="3"/>
  <tag k="description:en" v="Housed the Board of Trustees, a public institution of the Russian Empire until 1917"/>
  <tag k="end_date" v="1826"/>
  <tag k="name" v="Опекунский совет"/>
  <tag k="name:de" v="Kuratorium"/>
  <tag k="name:en" v="Board of Trustees Building"/>
  <tag k="ref" v="7710784000"/>
  <tag k="roof:material" v="metal"/>
  <tag k="source:description:en" v="wikipedia:ru"/>
  <tag k="start_date" v="1823"/>
  <tag k="tourism" v="attraction"/>
  <tag k="wikipedia" v="ru:Опекунский совет (Москва)"/>
 </way>
 <way id="45814282" visible="true" version="3" changeset="4128036" timestamp="2010-03-14T18:31:24Z" user="Vovanium" uid="87682">
  <nd ref="583193405"/>
  <nd ref="583193408"/>
  <nd ref="595699492"/>
  <nd ref="666179540"/>
  <nd ref="583193410"/>
  <nd ref="583193415"/>
  <nd ref="666179517"/>
  <nd ref="666179521"/>
  <nd ref="666179519"/>
  <nd ref="583193416"/>
  <nd ref="583193405"/>
 </way>
 <way id="109538181" visible="true" version="7" changeset="34206452" timestamp="2015-09-23T13:58:11Z" user="trolleway" uid="397326">
  <nd ref="338464708"/>
  <nd ref="338464706"/>
  <nd ref="2849850613"/>
  <nd ref="2849850614"/>
  <nd ref="3712207029"/>
  <nd ref="3712207030"/>
  <nd ref="3712207031"/>
  <nd ref="3712207032"/>
  <nd ref="3755931947"/>
  <nd ref="3712207033"/>
  <nd ref="3712207034"/>
  <tag k="highway" v="service"/>
 </way>
 <way id="45814281" visible="true" version="6" changeset="9583527" timestamp="2011-10-17T16:38:55Z" user="luch86" uid="266092">
  <nd ref="583193388"/>
  <nd ref="583193390"/>
  <nd ref="666179531"/>
  <nd ref="666179530"/>
  <nd ref="666179528"/>
  <nd ref="583193392"/>
  <nd ref="583193395"/>
  <nd ref="666179513"/>
  <nd ref="666179522"/>
  <nd ref="666179524"/>
  <nd ref="666179526"/>
  <nd ref="583193398"/>
  <nd ref="583193401"/>
  <nd ref="583193388"/>
 </way>
 <way id="45814283" visible="true" version="3" changeset="28090002" timestamp="2015-01-12T16:15:23Z" user="midrug" uid="2417727">
  <nd ref="583193422"/>
  <nd ref="583193424"/>
  <nd ref="583193426"/>
  <nd ref="583193429"/>
  <nd ref="583193431"/>
  <nd ref="583193432"/>
  <nd ref="583193422"/>
  <tag k="addr:street" v="улица Солянка"/>
  <tag k="building" v="yes"/>
  <tag k="building:colour" v="goldenrod"/>
  <tag k="building:levels" v="2"/>
  <tag k="roof:colour" v="black"/>
  <tag k="roof:material" v="tar_paper"/>
 </way>
 <way id="367274913" visible="true" version="2" changeset="33550484" timestamp="2015-08-24T16:00:25Z" user="vadp" uid="326091">
  <nd ref="2849850614"/>
  <nd ref="2849850611"/>
  <tag k="highway" v="service"/>
 </way>
 <relation id="365808" visible="true" version="6" changeset="28090002" timestamp="2015-01-12T16:15:14Z" user="midrug" uid="2417727">
  <member type="way" ref="45814281" role="outer"/>
  <member type="way" ref="45814282" role="inner"/>
  <tag k="addr:housenumber" v="14/2"/>
  <tag k="addr:street" v="улица Солянка"/>
  <tag k="building" v="yes"/>
  <tag k="building:colour" v="gold"/>
  <tag k="building:levels" v="2"/>
  <tag k="roof:material" v="metal"/>
  <tag k="type" v="multipolygon"/>
 </relation>
</osm>
)SEP";
}  // namespace

// Note: Geometry should not contain duplicates.

UNIT_TEST(GetBestOsmNode_Test)
{
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseNode, ::strlen(osmRawResponseNode)), ());

    auto const bestNode = osm::GetBestOsmNode(osmResponse, ms::LatLon(53.8977398, 27.5579251));
    TEST_EQUAL(editor::XMLFeature(bestNode).GetName(),
               "Главное управление капитального строительства", ());
  }
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseNode, ::strlen(osmRawResponseNode)), ());

    auto const bestNode = osm::GetBestOsmNode(osmResponse, ms::LatLon(53.8977254, 27.5578377));
    TEST_EQUAL(bestNode.attribute("id").value(), string("277172019"), ());
  }
}

UNIT_TEST(GetBestOsmWay_Test)
{
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseWay, ::strlen(osmRawResponseWay)), ());
    vector<m2::PointD> const geometry = {
        MercatorBounds::FromLatLon(53.8977484, 27.557359),
        MercatorBounds::FromLatLon(53.8978710, 27.557681),
        MercatorBounds::FromLatLon(53.8978034, 27.557764),
        MercatorBounds::FromLatLon(53.8977652, 27.557803),
        MercatorBounds::FromLatLon(53.8977254, 27.557837),
        MercatorBounds::FromLatLon(53.8976570, 27.557661),
        MercatorBounds::FromLatLon(53.8976041, 27.557518),
    };

    auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
    TEST_EQUAL(editor::XMLFeature(bestWay).GetName(), "Беллесбумпром", ());
  }
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseWay, ::strlen(osmRawResponseWay)), ());
    vector<m2::PointD> const geometry = {
        MercatorBounds::FromLatLon(53.8975484, 27.557359),  // diff
        MercatorBounds::FromLatLon(53.8978710, 27.557681),
        MercatorBounds::FromLatLon(53.8975034, 27.557764),  // diff
        MercatorBounds::FromLatLon(53.8977652, 27.557803),
        MercatorBounds::FromLatLon(53.8975254, 27.557837),  // diff
        MercatorBounds::FromLatLon(53.8976570, 27.557661),
        MercatorBounds::FromLatLon(53.8976041, 27.557318),  // diff
    };

    auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
    TEST(!bestWay, ());
  }
}

UNIT_TEST(GetBestOsmRealtion_Test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmRawResponseRelation, ::strlen(osmRawResponseRelation)), ());
  vector<m2::PointD> const geometry = {
    {37.6400469, 67.4549381},
    {37.6401059, 67.4546779},
    {37.6401113, 67.4551473},
    {37.640181, 67.4545089},
    {37.6401944, 67.455394},
    {37.6402534, 67.4553162},
    {37.6403205, 67.454812},
    {37.6403205, 67.4549327},
    {37.640358, 67.4547047},
    {37.6403768, 67.4550534},
    {37.6404895, 67.4541736},
    {37.6405056, 67.4551741},
    {37.6406343, 67.4540905},
    {37.6406638, 67.4543909},
    {37.6406906, 67.4552814},
    {37.6407496, 67.4557266},
    {37.6407496, 67.45572},
    {37.6407872, 67.4540636},
    {37.6408274, 67.4543211},
    {37.6409535, 67.4540797},
    {37.6410018, 67.4543506},
    {37.6410983, 67.4541522},
    {37.6412432, 67.454533},
    {37.6416187, 67.4545411}
  };

  auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), string("365808"), ());
}

namespace
{
char const * const osmResponseBuildingMiss = R"SEP(
<osm version="0.6" generator="CGImap 0.4.0 (8662 thorn-01.openstreetmap.org)">
 <bounds minlat="51.5342700" minlon="-0.2047000" maxlat="51.5343200" maxlon="-0.2046300"/>
 <node id="861357349" visible="true" version="3" changeset="31214483" timestamp="2015-05-16T23:10:03Z" user="Derick Rethans" uid="37137" lat="51.5342451" lon="-0.2046356"/>
 <node id="3522706827" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:47Z" user="Derick Rethans" uid="37137" lat="51.5342834" lon="-0.2046544">
  <tag k="addr:housenumber" v="26a"/>
  <tag k="addr:street" v="Salusbury Road"/>
 </node>
 <node id="3522707171" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:50Z" user="Derick Rethans" uid="37137" lat="51.5342161" lon="-0.2047884"/>
 <node id="3522707175" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:50Z" user="Derick Rethans" uid="37137" lat="51.5342627" lon="-0.2048113"/>
 <node id="3522707179" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:50Z" user="Derick Rethans" uid="37137" lat="51.5342918" lon="-0.2046585"/>
 <node id="3522707180" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:50Z" user="Derick Rethans" uid="37137" lat="51.5343060" lon="-0.2048326"/>
 <node id="3522707185" visible="true" version="1" changeset="31214483" timestamp="2015-05-16T23:09:50Z" user="Derick Rethans" uid="37137" lat="51.5343350" lon="-0.2046798"/>
 <way id="345630057" visible="true" version="3" changeset="38374962" timestamp="2016-04-07T09:19:02Z" user="Derick Rethans" uid="37137">
  <nd ref="3522707179"/>
  <nd ref="3522707185"/>
  <nd ref="3522707180"/>
  <nd ref="3522707175"/>
  <nd ref="3522707179"/>
  <tag k="addr:housenumber" v="26"/>
  <tag k="addr:street" v="Salusbury Road"/>
  <tag k="building" v="yes"/>
  <tag k="building:levels" v="1"/>
  <tag k="name" v="Londis"/>
  <tag k="shop" v="convenience"/>
 </way>
 <way id="345630019" visible="true" version="2" changeset="38374962" timestamp="2016-04-07T09:19:02Z" user="Derick Rethans" uid="37137">
  <nd ref="861357349"/>
  <nd ref="3522706827"/>
  <nd ref="3522707179"/>
  <nd ref="3522707175"/>
  <nd ref="3522707171"/>
  <nd ref="861357349"/>
  <tag k="addr:housenumber" v="26"/>
  <tag k="addr:street" v="Salusbury Road"/>
  <tag k="building" v="yes"/>
  <tag k="building:levels" v="2"/>
  <tag k="name" v="Shampoo Hair Salon"/>
  <tag k="shop" v="hairdresser"/>
 </way>
</osm>
)SEP";
}  // namespace

UNIT_TEST(HouseBuildingMiss_test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmResponseBuildingMiss, ::strlen(osmResponseBuildingMiss)), ());
  vector<m2::PointD> const geometry = {
    {-0.2048121407986514, 60.333984198674443},
    {-0.20478800091734684,  60.333909096821458},
    {-0.20465925488366565,  60.334029796228037},
    {-0.20463511500236109,  60.333954694375052},
  };

  auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), string("345630019"), ());
}
