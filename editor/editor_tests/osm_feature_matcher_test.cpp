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
      {27.557515856307106, 64.236609073256034},
      {27.55784576801841,  64.236820967769773},
      {27.557352241556003, 64.236863883114324},
      {27.55784576801841,  64.236820967769773},
      {27.557352241556003, 64.236863883114324},
      {27.557765301747366, 64.236963124848614},
      {27.557352241556003, 64.236863883114324},
      {27.557765301747366, 64.236963124848614},
      {27.55768215326728,  64.237078459837136}
    };

    auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
    TEST(bestWay, ());
    TEST_EQUAL(editor::XMLFeature(bestWay).GetName(), "Беллесбумпром", ());
  }
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseWay, ::strlen(osmRawResponseWay)), ());
    // Each point is moved for 0.0001 on x and y. It is aboout a half of side length.
    vector<m2::PointD> const geometry = {
      {27.557615856307106, 64.236709073256034},
      {27.55794576801841,  64.236920967769773},
      {27.557452241556003, 64.236963883114324},
      {27.55794576801841,  64.236920967769773},
      {27.557452241556003, 64.236963883114324},
      {27.557865301747366, 64.237063124848614},
      {27.557452241556003, 64.236963883114324},
      {27.557865301747366, 64.237063124848614},
      {27.55778215326728,  64.237178459837136}
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
    {37.640253436865322, 67.455316241497655},
    {37.64019442826654, 67.455394025559684},
    {37.640749645536772, 67.455726619480004},
    {37.640253436865322, 67.455316241497655},
    {37.640749645536772, 67.455726619480004},
    {37.64069063693799, 67.455281372780206},
    {37.640253436865322, 67.455316241497655},
    {37.64069063693799, 67.455281372780206},
    {37.640505564514598, 67.455174084418815},
    {37.640253436865322, 67.455316241497655},
    {37.640505564514598, 67.455174084418815},
    {37.640376818480917, 67.455053385012263},
    {37.640253436865322, 67.455316241497655},
    {37.640376818480917, 67.455053385012263},
    {37.640320492091178, 67.454932685605684},
    {37.640253436865322, 67.455316241497655},
    {37.640320492091178, 67.454932685605684},
    {37.640111279786453, 67.455147262328467},
    {37.640111279786453, 67.455147262328467},
    {37.640320492091178, 67.454932685605684},
    {37.640046906769612, 67.454938050023742},
    {37.640046906769612, 67.454938050023742},
    {37.640320492091178, 67.454932685605684},
    {37.640320492091178, 67.454811986199104},
    {37.640046906769612, 67.454938050023742},
    {37.640320492091178, 67.454811986199104},
    {37.640105915368395, 67.454677875747365},
    {37.640105915368395, 67.454677875747365},
    {37.640320492091178, 67.454811986199104},
    {37.64035804301767, 67.454704697837713},
    {37.640105915368395, 67.454677875747365},
    {37.64035804301767, 67.454704697837713},
    {37.64018101722138, 67.454508896578176},
    {37.64018101722138, 67.454508896578176},
    {37.64035804301767, 67.454704697837713},
    {37.640663814847642, 67.454390879380639},
    {37.64018101722138, 67.454508896578176},
    {37.640663814847642, 67.454390879380639},
    {37.640489471260366, 67.454173620448813},
    {37.640489471260366, 67.454173620448813},
    {37.640663814847642, 67.454390879380639},
    {37.640634310548251, 67.454090471968726},
    {37.640634310548251, 67.454090471968726},
    {37.640663814847642, 67.454390879380639},
    {37.640827429598772, 67.454321141945741},
    {37.640634310548251, 67.454090471968726},
    {37.640827429598772, 67.454321141945741},
    {37.640787196463265, 67.454063649878378},
    {37.640787196463265, 67.454063649878378},
    {37.640827429598772, 67.454321141945741},
    {37.64095349342341, 67.454079743132581},
    {37.64095349342341, 67.454079743132581},
    {37.640827429598772, 67.454321141945741},
    {37.641001773186048, 67.454350646245103},
    {37.64095349342341, 67.454079743132581},
    {37.641001773186048, 67.454350646245103},
    {37.641098332711294, 67.454152162776523},
    {37.641098332711294, 67.454152162776523},
    {37.641001773186048, 67.454350646245103},
    {37.641243171999179, 67.45453303645948},
    {37.641098332711294, 67.454152162776523},
    {37.641243171999179, 67.45453303645948},
    {37.641618681264049, 67.454541083086582},
    {37.641618681264049, 67.454541083086582},
    {37.641243171999179, 67.45453303645948},
    {37.64069063693799, 67.455281372780206},
    {37.641618681264049, 67.454541083086582},
    {37.64069063693799, 67.455281372780206},
    {37.640749645536772, 67.455726619480004}
  };

  auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), string("365808"), ());
}

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

UNIT_TEST(HouseBuildingMiss_test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmResponseBuildingMiss, ::strlen(osmResponseBuildingMiss)), ());
  vector<m2::PointD> const geometry = {
    {-0.2048121407986514, 60.333984198674443},
    {-0.20478800091734684,  60.333909096821458},
    {-0.20465925488366565,  60.334029796228037},
    {-0.2048121407986514, 60.333984198674443},
    {-0.20478800091734684,  60.333909096821458},
    {-0.20463511500236109,  60.333954694375052}
  };

  auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), string("345630019"), ());
}

string const kHouseWithSeveralEntrances = R"xxx("
<osm version="0.6" generator="CGImap 0.6.0 (3589 thorn-03.openstreetmap.org)" copyright="OpenStreetMap and contributors" attribution="http://www.openstreetmap.org/copyright" license="http://opendatacommons.org/licenses/odbl/1-0/">
  <node id="339283610" visible="true" version="6" changeset="33699414" timestamp="2015-08-31T09:53:02Z" user="Lazy Ranma" uid="914471" lat="55.8184397" lon="37.5700770"/>
  <node id="339283612" visible="true" version="6" changeset="33699414" timestamp="2015-08-31T09:53:02Z" user="Lazy Ranma" uid="914471" lat="55.8184655" lon="37.5702599"/>
  <node id="339283614" visible="true" version="6" changeset="33699414" timestamp="2015-08-31T09:53:02Z" user="Lazy Ranma" uid="914471" lat="55.8190524" lon="37.5698027"/>
  <node id="339283615" visible="true" version="6" changeset="33699414" timestamp="2015-08-31T09:53:02Z" user="Lazy Ranma" uid="914471" lat="55.8190782" lon="37.5699856"/>
  <node id="1131238558" visible="true" version="7" changeset="33699414" timestamp="2015-08-31T09:52:50Z" user="Lazy Ranma" uid="914471" lat="55.8188226" lon="37.5699055">
    <tag k="entrance" v="yes"/>
    <tag k="ref" v="2"/>
  </node>
  <node id="1131238581" visible="true" version="7" changeset="33699414" timestamp="2015-08-31T09:52:51Z" user="Lazy Ranma" uid="914471" lat="55.8185163" lon="37.5700427">
    <tag k="entrance" v="yes"/>
    <tag k="ref" v="4"/>
  </node>
  <node id="1131238623" visible="true" version="7" changeset="33699414" timestamp="2015-08-31T09:52:51Z" user="Lazy Ranma" uid="914471" lat="55.8189758" lon="37.5698370">
    <tag k="entrance" v="yes"/>
    <tag k="ref" v="1"/>
  </node>
  <node id="1131238704" visible="true" version="7" changeset="33699414" timestamp="2015-08-31T09:52:52Z" user="Lazy Ranma" uid="914471" lat="55.8186694" lon="37.5699741">
    <tag k="entrance" v="yes"/>
    <tag k="ref" v="3"/>
  </node>
  <way id="30680719" visible="true" version="10" changeset="25301783" timestamp="2014-09-08T07:52:43Z" user="Felis Pimeja" uid="260756">
    <nd ref="339283614"/>
    <nd ref="339283615"/>
    <nd ref="339283612"/>
    <nd ref="339283610"/>
    <nd ref="1131238581"/>
    <nd ref="1131238704"/>
    <nd ref="1131238558"/>
    <nd ref="1131238623"/>
    <nd ref="339283614"/>
    <tag k="addr:city" v="Москва"/>
    <tag k="addr:country" v="RU"/>
    <tag k="addr:housenumber" v="14 к1"/>
    <tag k="addr:street" v="Ивановская улица"/>
    <tag k="building" v="yes"/>
  </way>
</osm>
)xxx";

UNIT_TEST(HouseWithSeveralEntrances)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(kHouseWithSeveralEntrances.c_str(),
                               kHouseWithSeveralEntrances.size()), ());

  vector<m2::PointD> geometry = {
    {37.570076119676798, 67.574481424499169},
    {37.570258509891175, 67.574527022052763},
    {37.569802534355233, 67.575570401367315},
    {37.570258509891175, 67.574527022052763},
    {37.569802534355233, 67.575570401367315},
    {37.56998492456961, 67.57561599892091}
  };

  auto const bestWay = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), string("30680719"), ());
}

string const kSenatskiyDvorets = "XXX("
  "<osm version=\"0.6\" generator=\"CGImap 0.6.0 (26980 thorn-02.openstreetmap.org)\" copyright=\"OpenStreetMap and contributors\" attribution=\"http://www.openstreetmap.org/copyright\" license=\"http://opendatacommons.org/licenses/odbl/1-0/\">\n"
  "<node id=\"253124975\" visible=\"true\" version=\"3\" changeset=\"6156507\" timestamp=\"2010-10-24T13:16:38Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525411\" lon=\"37.6178831\"/>\n"
  "<node id=\"253124976\" visible=\"true\" version=\"3\" changeset=\"6156507\" timestamp=\"2010-10-24T13:16:26Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7524561\" lon=\"37.6181939\"/>\n"
  "<node id=\"253124977\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:00Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531607\" lon=\"37.6196446\"/>\n"
  "<node id=\"253124978\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:00Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533535\" lon=\"37.6195596\"/>\n"
  "<node id=\"253124979\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:00Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7540120\" lon=\"37.6182974\"/>\n"
  "<node id=\"253124980\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:00Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539230\" lon=\"37.6179391\"/>\n"
  "<node id=\"311994670\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:01Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7526620\" lon=\"37.6180940\"/>\n"
  "<node id=\"311994722\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:01Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7526316\" lon=\"37.6182046\"/>\n"
  "<node id=\"311994790\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:01Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528074\" lon=\"37.6185733\"/>\n"
  "<node id=\"311994792\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:01Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7529378\" lon=\"37.6181056\"/>\n"
  "<node id=\"311994794\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:01Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531140\" lon=\"37.6181684\"/>\n"
  "<node id=\"311994796\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:02Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533688\" lon=\"37.6181759\"/>\n"
  "<node id=\"311994798\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:02Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7535483\" lon=\"37.6188395\"/>\n"
  "<node id=\"311994799\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:02Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7535279\" lon=\"37.6188750\"/>\n"
  "<node id=\"311994801\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:02Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7534727\" lon=\"37.6187893\"/>\n"
  "<node id=\"311994858\" visible=\"true\" version=\"3\" changeset=\"6877622\" timestamp=\"2011-01-05T23:26:07Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533363\" lon=\"37.6190615\"/>\n"
  "<node id=\"311994935\" visible=\"true\" version=\"3\" changeset=\"6877622\" timestamp=\"2011-01-05T23:26:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531336\" lon=\"37.6190462\"/>\n"
  "<node id=\"311994937\" visible=\"true\" version=\"2\" changeset=\"6156507\" timestamp=\"2010-10-24T13:16:41Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7529989\" lon=\"37.6187641\"/>\n"
  "<node id=\"311994938\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:03Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7529117\" lon=\"37.6188047\"/>\n"
  "<node id=\"311994940\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:03Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7529368\" lon=\"37.6188577\"/>\n"
  "<node id=\"311994942\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:03Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7538537\" lon=\"37.6182437\"/>\n"
  "<node id=\"311994946\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:03Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7535475\" lon=\"37.6183003\"/>\n"
  "<node id=\"311994947\" visible=\"true\" version=\"4\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:03Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536236\" lon=\"37.6185832\"/>\n"
  "<node id=\"311994988\" visible=\"true\" version=\"5\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:04Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536605\" lon=\"37.6186116\"/>\n"
  "<node id=\"963303643\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:04Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7537168\" lon=\"37.6188337\"/>\n"
  "<node id=\"963303645\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:04Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536206\" lon=\"37.6182132\"/>\n"
  "<node id=\"963303675\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:04Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532506\" lon=\"37.6196900\"/>\n"
  "<node id=\"963303715\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7524653\" lon=\"37.6181604\"/>\n"
  "<node id=\"963303765\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7534307\" lon=\"37.6193846\"/>\n"
  "<node id=\"963303806\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:18Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539204\" lon=\"37.6179199\"/>\n"
  "<node id=\"963303818\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536082\" lon=\"37.6190706\"/>\n"
  "<node id=\"963303845\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525457\" lon=\"37.6184104\"/>\n"
  "<node id=\"963303877\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7534380\" lon=\"37.6193966\"/>\n"
  "<node id=\"963303879\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:06Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539718\" lon=\"37.6179784\"/>\n"
  "<node id=\"963303904\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:06Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528479\" lon=\"37.6190450\"/>\n"
  "<node id=\"963303913\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:06Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527026\" lon=\"37.6178896\"/>\n"
  "<node id=\"963303927\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:06Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531785\" lon=\"37.6196760\"/>\n"
  "<node id=\"963303953\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:07Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528148\" lon=\"37.6189462\"/>\n"
  "<node id=\"963304000\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:07Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536824\" lon=\"37.6189285\"/>\n"
  "<node id=\"963304015\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:07Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7530937\" lon=\"37.6195613\"/>\n"
  "<node id=\"963304039\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:07Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527024\" lon=\"37.6178749\"/>\n"
  "<node id=\"963304074\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527858\" lon=\"37.6188854\"/>\n"
  "<node id=\"963304116\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536024\" lon=\"37.6181428\"/>\n"
  "<node id=\"963304133\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532860\" lon=\"37.6196355\"/>\n"
  "<node id=\"963304168\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525491\" lon=\"37.6184053\"/>\n"
  "<node id=\"963304248\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:32Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7540247\" lon=\"37.6182410\"/>\n"
  "<node id=\"963304259\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:08Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532958\" lon=\"37.6195922\"/>\n"
  "<node id=\"963304289\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:33Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7524644\" lon=\"37.6182397\"/>\n"
  "<node id=\"963304326\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:09Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539186\" lon=\"37.6184761\"/>\n"
  "<node id=\"963304350\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:09Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7530185\" lon=\"37.6193735\"/>\n"
  "<node id=\"963304366\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:09Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7537859\" lon=\"37.6179335\"/>\n"
  "<node id=\"963304387\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:09Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532263\" lon=\"37.6196960\"/>\n"
  "<node id=\"963304401\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:10Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528619\" lon=\"37.6185363\"/>\n"
  "<node id=\"963304425\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:10Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7524676\" lon=\"37.6182343\"/>\n"
  "<node id=\"963304480\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:10Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536742\" lon=\"37.6189158\"/>\n"
  "<node id=\"963304515\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:10Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531022\" lon=\"37.6195492\"/>\n"
  "<node id=\"963304530\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:11Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7540031\" lon=\"37.6182825\"/>\n"
  "<node id=\"963304560\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:11Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525270\" lon=\"37.6179346\"/>\n"
  "<node id=\"963304589\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:11Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528560\" lon=\"37.6190328\"/>\n"
  "<node id=\"963304647\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:11Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7537866\" lon=\"37.6179150\"/>\n"
  "<node id=\"963304654\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:42Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531277\" lon=\"37.6196026\"/>\n"
  "<node id=\"963304686\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:42Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7524567\" lon=\"37.6181524\"/>\n"
  "<node id=\"963304764\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:11Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539096\" lon=\"37.6184626\"/>\n"
  "<node id=\"963304768\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539553\" lon=\"37.6179696\"/>\n"
  "<node id=\"963304792\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7530104\" lon=\"37.6193863\"/>\n"
  "<node id=\"963304827\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525171\" lon=\"37.6179262\"/>\n"
  "<node id=\"963304864\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536001\" lon=\"37.6190584\"/>\n"
  "<node id=\"963304884\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527357\" lon=\"37.6188106\"/>\n"
  "<node id=\"963304917\" visible=\"true\" version=\"1\" changeset=\"6156507\" timestamp=\"2010-10-24T13:15:48Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539478\" lon=\"37.6179401\"/>\n"
  "<node id=\"963304944\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7537238\" lon=\"37.6188492\"/>\n"
  "<node id=\"963304955\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7529619\" lon=\"37.6182226\"/>\n"
  "<node id=\"963304983\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527438\" lon=\"37.6187975\"/>\n"
  "<node id=\"963305055\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536479\" lon=\"37.6189663\"/>\n"
  "<node id=\"963305060\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7540227\" lon=\"37.6181803\"/>\n"
  "<node id=\"963305086\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7527779\" lon=\"37.6188981\"/>\n"
  "<node id=\"963305100\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7540172\" lon=\"37.6182116\"/>\n"
  "<node id=\"963305123\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525699\" lon=\"37.6178843\"/>\n"
  "<node id=\"963305150\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7528071\" lon=\"37.6189595\"/>\n"
  "<node id=\"963305213\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7536570\" lon=\"37.6189771\"/>\n"
  "<node id=\"963305220\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:15Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7535946\" lon=\"37.6183023\"/>\n"
  "<node id=\"963305235\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:15Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531519\" lon=\"37.6196011\"/>\n"
  "<node id=\"963305270\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:06:15Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7525704\" lon=\"37.6178701\"/>\n"
  "<node id=\"1084517378\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:23:17Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532133\" lon=\"37.6189600\"/>\n"
  "<node id=\"1084517428\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:23:19Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532147\" lon=\"37.6181715\"/>\n"
  "<node id=\"1084518073\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:23:37Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532835\" lon=\"37.6189776\"/>\n"
  "<node id=\"1084518365\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:23:47Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531495\" lon=\"37.6190146\"/>\n"
  "<node id=\"1084518647\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:23:57Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532609\" lon=\"37.6189635\"/>\n"
  "<node id=\"1084518877\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533962\" lon=\"37.6189421\"/>\n"
  "<node id=\"1084519171\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:16Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533219\" lon=\"37.6190275\"/>\n"
  "<node id=\"1084519471\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:24Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531901\" lon=\"37.6189706\"/>\n"
  "<node id=\"1084519869\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:32Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533041\" lon=\"37.6189992\"/>\n"
  "<node id=\"1084520220\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:40Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531686\" lon=\"37.6189890\"/>\n"
  "<node id=\"1084520232\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:24:40Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7530751\" lon=\"37.6189237\"/>\n"
  "<node id=\"1084520811\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:25:00Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532372\" lon=\"37.6189576\"/>\n"
  "<node id=\"1084520938\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:25:05Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532666\" lon=\"37.6181737\"/>\n"
  "<node id=\"1087110479\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:55Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532648\" lon=\"37.6178957\"/>\n"
  "<node id=\"1087111924\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:55Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532147\" lon=\"37.6178938\"/>\n"
  "<node id=\"1087268866\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:55Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7539805\" lon=\"37.6183575\"/>\n"
  "<node id=\"2361011398\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:56Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531922\" lon=\"37.6179095\"/>\n"
  "<node id=\"2361011399\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:57Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7531924\" lon=\"37.6178930\"/>\n"
  "<node id=\"2361011435\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:58Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533111\" lon=\"37.6178974\"/>\n"
  "<node id=\"2361011436\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:59Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533113\" lon=\"37.6179143\"/>\n"
  "<node id=\"2497559173\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:02Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532011\" lon=\"37.6196916\"/>\n"
  "<node id=\"2497559222\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:12Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7532711\" lon=\"37.6196693\"/>\n"
  "<node id=\"2497559237\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533245\" lon=\"37.6195904\"/>\n"
  "<node id=\"2497559238\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:13Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7533465\" lon=\"37.6195484\"/>\n"
  "<node id=\"2497559245\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:14Z\" user=\"Felis Pimeja\" uid=\"260756\" lat=\"55.7538306\" lon=\"37.6181534\"/>\n"
  "<node id=\"5183046846\" visible=\"true\" version=\"1\" changeset=\"53169437\" timestamp=\"2017-10-23T06:57:53Z\" user=\"Павел Гетманцев\" uid=\"571410\" lat=\"55.7525108\" lon=\"37.6183372\"/>\n"
  "<way id=\"23378752\" visible=\"true\" version=\"10\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:53Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304248\"/>\n"
  "<nd ref=\"963305100\"/>\n"
  "</way>\n"
  "<way id=\"28403176\" visible=\"true\" version=\"4\" changeset=\"6877622\" timestamp=\"2011-01-05T23:26:02Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994935\"/>\n"
  "<nd ref=\"1084520232\"/>\n"
  "</way>\n"
  "<way id=\"28403177\" visible=\"true\" version=\"6\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:53Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994942\"/>\n"
  "<nd ref=\"2497559245\"/>\n"
  "</way>\n"
  "<way id=\"93540519\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:25:23Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1084517428\"/>\n"
  "<nd ref=\"1084520938\"/>\n"
  "</way>\n"
  "<way id=\"93540524\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:25:25Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994935\"/>\n"
  "<nd ref=\"1084518365\"/>\n"
  "<nd ref=\"1084520220\"/>\n"
  "<nd ref=\"1084519471\"/>\n"
  "<nd ref=\"1084517378\"/>\n"
  "<nd ref=\"1084520811\"/>\n"
  "<nd ref=\"1084518647\"/>\n"
  "<nd ref=\"1084518073\"/>\n"
  "<nd ref=\"1084519869\"/>\n"
  "<nd ref=\"1084519171\"/>\n"
  "<nd ref=\"311994858\"/>\n"
  "</way>\n"
  "<way id=\"93540575\" visible=\"true\" version=\"1\" changeset=\"6877622\" timestamp=\"2011-01-05T23:25:44Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1084518877\"/>\n"
  "<nd ref=\"311994858\"/>\n"
  "</way>\n"
  "<way id=\"93540579\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:54Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1084520938\"/>\n"
  "<nd ref=\"311994796\"/>\n"
  "</way>\n"
  "<way id=\"93671591\" visible=\"true\" version=\"1\" changeset=\"6887928\" timestamp=\"2011-01-06T23:55:45Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1087110479\"/>\n"
  "<nd ref=\"1087111924\"/>\n"
  "</way>\n"
  "<way id=\"93671598\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:54Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1087111924\"/>\n"
  "<nd ref=\"2361011399\"/>\n"
  "<nd ref=\"2361011398\"/>\n"
  "</way>\n"
  "<way id=\"93671614\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:54Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"253124975\"/>\n"
  "<nd ref=\"963304560\"/>\n"
  "</way>\n"
  "<way id=\"93671619\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:54Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"253124976\"/>\n"
  "<nd ref=\"963304425\"/>\n"
  "</way>\n"
  "<way id=\"93671623\" visible=\"true\" version=\"3\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:55Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303806\"/>\n"
  "<nd ref=\"963304647\"/>\n"
  "<nd ref=\"963304366\"/>\n"
  "</way>\n"
  "<way id=\"227433663\" visible=\"true\" version=\"4\" changeset=\"21532789\" timestamp=\"2014-04-06T13:44:23Z\" user=\"Павел Гетманцев\" uid=\"571410\">\n"
  "<nd ref=\"311994722\"/>\n"
  "<nd ref=\"311994790\"/>\n"
  "</way>\n"
  "<way id=\"227433738\" visible=\"true\" version=\"2\" changeset=\"18371454\" timestamp=\"2013-10-15T16:05:52Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"1084520232\"/>\n"
  "<nd ref=\"311994937\"/>\n"
  "</way>\n"
  "<way id=\"242244179\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:15Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994790\"/>\n"
  "<nd ref=\"963304401\"/>\n"
  "</way>\n"
  "<way id=\"242244184\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:16Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304589\"/>\n"
  "<nd ref=\"963304350\"/>\n"
  "</way>\n"
  "<way id=\"242244185\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:17Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2497559237\"/>\n"
  "<nd ref=\"2497559238\"/>\n"
  "</way>\n"
  "<way id=\"242244186\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:17Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994937\"/>\n"
  "<nd ref=\"311994940\"/>\n"
  "</way>\n"
  "<way id=\"242244187\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:17Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963305235\"/>\n"
  "<nd ref=\"253124977\"/>\n"
  "</way>\n"
  "<way id=\"242244189\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:18Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994794\"/>\n"
  "<nd ref=\"1084517428\"/>\n"
  "</way>\n"
  "<way id=\"242244191\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:18Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304387\"/>\n"
  "<nd ref=\"963303675\"/>\n"
  "</way>\n"
  "<way id=\"242244192\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:18Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304133\"/>\n"
  "<nd ref=\"963304259\"/>\n"
  "</way>\n"
  "<way id=\"242244198\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:20Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994947\"/>\n"
  "<nd ref=\"311994988\"/>\n"
  "</way>\n"
  "<way id=\"242244199\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:20Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"253124977\"/>\n"
  "<nd ref=\"963303927\"/>\n"
  "</way>\n"
  "<way id=\"242244206\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:21Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994792\"/>\n"
  "<nd ref=\"311994670\"/>\n"
  "</way>\n"
  "<way id=\"242244207\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:22Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303645\"/>\n"
  "<nd ref=\"963305220\"/>\n"
  "</way>\n"
  "<way id=\"242244208\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:22Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994799\"/>\n"
  "<nd ref=\"311994801\"/>\n"
  "</way>\n"
  "<way id=\"242244216\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:23Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304401\"/>\n"
  "<nd ref=\"963304955\"/>\n"
  "</way>\n"
  "<way id=\"242244219\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:24Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304074\"/>\n"
  "<nd ref=\"963303953\"/>\n"
  "</way>\n"
  "<way id=\"242244221\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:24Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303643\"/>\n"
  "<nd ref=\"963304764\"/>\n"
  "</way>\n"
  "<way id=\"242244223\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:25Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994940\"/>\n"
  "<nd ref=\"311994938\"/>\n"
  "</way>\n"
  "<way id=\"242244225\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:25Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304259\"/>\n"
  "<nd ref=\"2497559237\"/>\n"
  "</way>\n"
  "<way id=\"242244228\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:26Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304366\"/>\n"
  "<nd ref=\"2361011436\"/>\n"
  "</way>\n"
  "<way id=\"242244232\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:26Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303675\"/>\n"
  "<nd ref=\"2497559222\"/>\n"
  "</way>\n"
  "<way id=\"242244238\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:28Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304768\"/>\n"
  "<nd ref=\"963304917\"/>\n"
  "</way>\n"
  "<way id=\"242244242\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:29Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994988\"/>\n"
  "<nd ref=\"311994942\"/>\n"
  "</way>\n"
  "<way id=\"242244243\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:29Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303927\"/>\n"
  "<nd ref=\"2497559173\"/>\n"
  "</way>\n"
  "<way id=\"242244248\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:30Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994801\"/>\n"
  "<nd ref=\"1084518877\"/>\n"
  "</way>\n"
  "<way id=\"242244249\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:30Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303765\"/>\n"
  "<nd ref=\"963304864\"/>\n"
  "</way>\n"
  "<way id=\"242244252\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:31Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994670\"/>\n"
  "<nd ref=\"311994722\"/>\n"
  "</way>\n"
  "<way id=\"242244255\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:32Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303715\"/>\n"
  "<nd ref=\"253124976\"/>\n"
  "</way>\n"
  "<way id=\"242244257\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:32Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963305220\"/>\n"
  "<nd ref=\"311994946\"/>\n"
  "</way>\n"
  "<way id=\"242244261\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:33Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304955\"/>\n"
  "<nd ref=\"311994792\"/>\n"
  "</way>\n"
  "<way id=\"242244262\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:33Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304917\"/>\n"
  "<nd ref=\"253124980\"/>\n"
  "</way>\n"
  "<way id=\"242244265\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:34Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963305055\"/>\n"
  "<nd ref=\"963304480\"/>\n"
  "</way>\n"
  "<way id=\"242244266\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:34Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2497559245\"/>\n"
  "<nd ref=\"963304116\"/>\n"
  "</way>\n"
  "<way id=\"242244267\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:34Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994798\"/>\n"
  "<nd ref=\"311994799\"/>\n"
  "</way>\n"
  "<way id=\"242244268\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:34Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994938\"/>\n"
  "<nd ref=\"311994794\"/>\n"
  "</way>\n"
  "<way id=\"242244269\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:35Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2497559222\"/>\n"
  "<nd ref=\"963304133\"/>\n"
  "</way>\n"
  "<way id=\"242244270\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:35Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304168\"/>\n"
  "<nd ref=\"963304983\"/>\n"
  "</way>\n"
  "<way id=\"242244274\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:36Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994796\"/>\n"
  "<nd ref=\"311994798\"/>\n"
  "</way>\n"
  "<way id=\"242244276\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:36Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304515\"/>\n"
  "<nd ref=\"963304654\"/>\n"
  "</way>\n"
  "<way id=\"242244277\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:36Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304654\"/>\n"
  "<nd ref=\"963305235\"/>\n"
  "</way>\n"
  "<way id=\"242244279\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:37Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2497559173\"/>\n"
  "<nd ref=\"963304387\"/>\n"
  "</way>\n"
  "<way id=\"242244280\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:37Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304530\"/>\n"
  "<nd ref=\"963304248\"/>\n"
  "</way>\n"
  "<way id=\"242244289\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:39Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963305123\"/>\n"
  "<nd ref=\"253124975\"/>\n"
  "</way>\n"
  "<way id=\"242244291\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:39Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"253124980\"/>\n"
  "<nd ref=\"963303806\"/>\n"
  "</way>\n"
  "<way id=\"242244296\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:41Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"311994946\"/>\n"
  "<nd ref=\"311994947\"/>\n"
  "</way>\n"
  "<way id=\"242244304\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:42Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2361011398\"/>\n"
  "<nd ref=\"963303913\"/>\n"
  "</way>\n"
  "<way id=\"242244305\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:42Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304116\"/>\n"
  "<nd ref=\"963303645\"/>\n"
  "</way>\n"
  "<way id=\"242244307\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:43Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2361011436\"/>\n"
  "<nd ref=\"2361011435\"/>\n"
  "<nd ref=\"1087110479\"/>\n"
  "</way>\n"
  "<way id=\"242244309\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:43Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304983\"/>\n"
  "<nd ref=\"963304884\"/>\n"
  "<nd ref=\"963305086\"/>\n"
  "<nd ref=\"963304074\"/>\n"
  "</way>\n"
  "<way id=\"242244310\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:44Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"2497559238\"/>\n"
  "<nd ref=\"253124978\"/>\n"
  "<nd ref=\"963303877\"/>\n"
  "<nd ref=\"963303765\"/>\n"
  "</way>\n"
  "<way id=\"242244311\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:44Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304560\"/>\n"
  "<nd ref=\"963304827\"/>\n"
  "<nd ref=\"963304686\"/>\n"
  "<nd ref=\"963303715\"/>\n"
  "</way>\n"
  "<way id=\"242244312\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:44Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304864\"/>\n"
  "<nd ref=\"963303818\"/>\n"
  "<nd ref=\"963305213\"/>\n"
  "<nd ref=\"963305055\"/>\n"
  "</way>\n"
  "<way id=\"242244313\" visible=\"true\" version=\"2\" changeset=\"53169437\" timestamp=\"2017-10-23T06:57:59Z\" user=\"Павел Гетманцев\" uid=\"571410\">\n"
  "<nd ref=\"963304425\"/>\n"
  "<nd ref=\"963304289\"/>\n"
  "<nd ref=\"5183046846\"/>\n"
  "<nd ref=\"963303845\"/>\n"
  "<nd ref=\"963304168\"/>\n"
  "</way>\n"
  "<way id=\"242244314\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:45Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304350\"/>\n"
  "<nd ref=\"963304792\"/>\n"
  "<nd ref=\"963304015\"/>\n"
  "<nd ref=\"963304515\"/>\n"
  "</way>\n"
  "<way id=\"242244316\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:45Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963305100\"/>\n"
  "<nd ref=\"963305060\"/>\n"
  "<nd ref=\"963303879\"/>\n"
  "<nd ref=\"963304768\"/>\n"
  "</way>\n"
  "<way id=\"242244318\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:45Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303913\"/>\n"
  "<nd ref=\"963304039\"/>\n"
  "<nd ref=\"963305270\"/>\n"
  "<nd ref=\"963305123\"/>\n"
  "</way>\n"
  "<way id=\"242244319\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:46Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963303953\"/>\n"
  "<nd ref=\"963305150\"/>\n"
  "<nd ref=\"963303904\"/>\n"
  "<nd ref=\"963304589\"/>\n"
  "</way>\n"
  "<way id=\"242244320\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:46Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304480\"/>\n"
  "<nd ref=\"963304000\"/>\n"
  "<nd ref=\"963304944\"/>\n"
  "<nd ref=\"963303643\"/>\n"
  "</way>\n"
  "<way id=\"242244321\" visible=\"true\" version=\"1\" changeset=\"18371454\" timestamp=\"2013-10-15T15:55:46Z\" user=\"Felis Pimeja\" uid=\"260756\">\n"
  "<nd ref=\"963304764\"/>\n"
  "<nd ref=\"963304326\"/>\n"
  "<nd ref=\"1087268866\"/>\n"
  "<nd ref=\"253124979\"/>\n"
  "<nd ref=\"963304530\"/>\n"
  "</way>\n"
  "<relation id=\"1359233\" visible=\"true\" version=\"19\" changeset=\"53696353\" timestamp=\"2017-11-11T17:39:34Z\" user=\"Alexander-II\" uid=\"3580412\">\n"
  "<member type=\"way\" ref=\"242244255\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244311\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"93671614\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244289\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244318\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244304\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"93671598\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"93671591\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244307\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244228\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"93671623\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244291\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244262\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244238\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244316\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"23378752\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244280\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244321\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244221\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244320\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244265\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244312\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244249\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244310\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244185\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244225\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244192\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244269\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244232\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244191\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244279\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244243\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244199\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244187\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244277\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244276\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244314\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244184\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244319\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244219\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244309\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244270\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"242244313\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"93671619\" role=\"outer\"/>\n"
  "<member type=\"way\" ref=\"28403177\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244266\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244305\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244207\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244257\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244296\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244198\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244242\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"28403176\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"227433738\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244186\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244223\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244268\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244189\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"93540519\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"93540579\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244274\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244267\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244208\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244248\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"93540575\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"93540524\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"227433663\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244179\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244216\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244261\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244206\" role=\"inner\"/>\n"
  "<member type=\"way\" ref=\"242244252\" role=\"inner\"/>\n"
  "<tag k=\"addr:city\" v=\"Москва\"/>\n"
  "<tag k=\"addr:place\" v=\"Кремль\"/>\n"
  "<tag k=\"addr:postcode\" v=\"103073\"/>\n"
  "<tag k=\"building\" v=\"office\"/>\n"
  "<tag k=\"building:colour\" v=\"gold\"/>\n"
  "<tag k=\"building:levels\" v=\"4\"/>\n"
  "<tag k=\"building:part\" v=\"no\"/>\n"
  "<tag k=\"government\" v=\"president\"/>\n"
  "<tag k=\"name\" v=\"Сенатский дворец\"/>\n"
  "<tag k=\"name:de\" v=\"Senatspalast\"/>\n"
  "<tag k=\"name:en\" v=\"Kremlin Senate\"/>\n"
  "<tag k=\"name:es\" v=\"Palacio del Senado\"/>\n"
  "<tag k=\"name:fr\" v=\"Palais du Sénat\"/>\n"
  "<tag k=\"name:ru\" v=\"Сенатский дворец\"/>\n"
  "<tag k=\"note\" v=\"Крыша сложная. Пока разбираюсь как её правильно обозначить\"/>\n"
  "<tag k=\"office\" v=\"government\"/>\n"
  "<tag k=\"roof:colour\" v=\"darkslategrey\"/>\n"
  "<tag k=\"type\" v=\"multipolygon\"/>\n"
  "<tag k=\"wikidata\" v=\"Q930376\"/>\n"
  "<tag k=\"wikipedia\" v=\"ru:Сенатский дворец\"/>\n"
  "</relation>\n"
  "</osm>"
  ")XXX";

UNIT_TEST(MatchByGeometry)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(kSenatskiyDvorets.c_str(), kSenatskiyDvorets.size()), ());

  vector<m2::PointD> geometry = {
    {37.618409526485265815, 67.4572930295563821},
    {37.618404162067207608, 67.45729839397444038},
    {37.618240547316077027, 67.45714819026849795},
    {37.618240547316077027, 67.45714819026849795},
    {37.618404162067207608, 67.45729839397444038},
    {37.61823518289801882, 67.45715355468655616},
    {37.61823518289801882, 67.45715355468655616},
    {37.618404162067207608, 67.45729839397444038},
    {37.618205678598627628, 67.45744591547136792},
    {37.618205678598627628, 67.45744591547136792},
    {37.618404162067207608, 67.45729839397444038},
    {37.618573141236396395, 67.45775705171939762},
    {37.618573141236396395, 67.45775705171939762},
    {37.618404162067207608, 67.45729839397444038},
    {37.618798446795324253, 67.45764439893994790},
    {37.618573141236396395, 67.45775705171939762},
    {37.618798446795324253, 67.45764439893994790},
    {37.618803811213410881, 67.45794212414281787},
    {37.618803811213410881, 67.45794212414281787},
    {37.618798446795324253, 67.45764439893994790},
    {37.618884277484454515, 67.4577195007929049},
    {37.618884277484454515, 67.4577195007929049},
    {37.618798446795324253, 67.45764439893994790},
    {37.618809175631469088, 67.45762830568574486},
    {37.618884277484454515, 67.4577195007929049},
    {37.618809175631469088, 67.45762830568574486},
    {37.618897688529614243, 67.45770340753870186},
    {37.618803811213410881, 67.45794212414281787},
    {37.618884277484454515, 67.4577195007929049},
    {37.618945968292251791, 67.45777046276458577},
    {37.618803811213410881, 67.457942124142817875},
    {37.618945968292251791, 67.457770462764585773},
    {37.618857455394106637, 67.457987721696412109},
    {37.618857455394106637, 67.457987721696412109},
    {37.618945968292251791, 67.457770462764585773},
    {37.619031798981353631, 67.457842882408527885},
    {37.619031798981353631, 67.457842882408527885},
    {37.618945968292251791, 67.457770462764585773},
    {37.618959379337411519, 67.457757051719397623},
    {37.619031798981353631, 67.457842882408527885},
    {37.618959379337411519, 67.457757051719397623},
    {37.619045210026541781, 67.457829471363339735},
    {37.618857455394106637, 67.457987721696412109},
    {37.619031798981353631, 67.457842882408527885},
    {37.619045210026541781, 67.45833640887093452},
    {37.619045210026541781, 67.45833640887093452},
    {37.619031798981353631, 67.457842882408527885},
    {37.619372439528802943, 67.458132560984296333},
    {37.619045210026541781, 67.45833640887093452},
    {37.619372439528802943, 67.458132560984296333},
    {37.619549465325093252, 67.45828008248119545},
    {37.619549465325093252, 67.45828008248119545},
    {37.619372439528802943, 67.458132560984296333},
    {37.619385850573962671, 67.458116467730064869},
    {37.619549465325093252, 67.45828008248119545},
    {37.619385850573962671, 67.458116467730064869},
    {37.619560194161238087, 67.458266671436035722},
    {37.619045210026541781, 67.45833640887093452},
    {37.619549465325093252, 67.45828008248119545},
    {37.619600427296774114, 67.458368595379369026},
    {37.619600427296774114, 67.458368595379369026},
    {37.619549465325093252, 67.45828008248119545},
    {37.619603109505789007, 67.458325680034789684},
    {37.619045210026541781, 67.45833640887093452},
    {37.619600427296774114, 67.458368595379369026},
    {37.619592380669644172, 67.458626087446702968},
    {37.619592380669644172, 67.458626087446702968},
    {37.619600427296774114, 67.458368595379369026},
    {37.61969698682202079, 67.458502705831108415},
    {37.61969698682202079, 67.458502705831108415},
    {37.619600427296774114, 67.458368595379369026},
    {37.619691622403934161, 67.45845710827751418},
    {37.619691622403934161, 67.45845710827751418},
    {37.619600427296774114, 67.458368595379369026},
    {37.619675529149731119, 67.458416875141978153},
    {37.619675529149731119, 67.458416875141978153},
    {37.619600427296774114, 67.458368595379369026},
    {37.619643342641325034, 67.458384688633572068},
    {37.619592380669644172, 67.458626087446702968},
    {37.61969698682202079, 67.458502705831108415},
    {37.619635296014223513, 67.458607311983456611},
    {37.619635296014223513, 67.458607311983456611},
    {37.61969698682202079, 67.458502705831108415},
    {37.619688940194919269, 67.458545621175659335},
    {37.619635296014223513, 67.458607311983456611},
    {37.619688940194919269, 67.458545621175659335},
    {37.619670164731672912, 67.458580489893108734},
    {37.619045210026541781, 67.45833640887093452},
    {37.619592380669644172, 67.458626087446702968},
    {37.619061303280744823, 67.458695824881601766},
    {37.619061303280744823, 67.458695824881601766},
    {37.619592380669644172, 67.458626087446702968},
    {37.619549465325093252, 67.458714600344848122},
    {37.619549465325093252, 67.458714600344848122},
    {37.619592380669644172, 67.458626087446702968},
    {37.619589698460629279, 67.458677049418355409},
    {37.619061303280744823, 67.458695824881601766},
    {37.619549465325093252, 67.458714600344848122},
    {37.619383168364947778, 67.458864804050818975},
    {37.619383168364947778, 67.458864804050818975},
    {37.619549465325093252, 67.458714600344848122},
    {37.619560194161238087, 67.458728011390036272},
    {37.619383168364947778, 67.458864804050818975},
    {37.619560194161238087, 67.458728011390036272},
    {37.619396579410107506, 67.458878215095978703},
    {37.619061303280744823, 67.458695824881601766},
    {37.619383168364947778, 67.458864804050818975},
    {37.619058621071729931, 67.459165211462703837},
    {37.619061303280744823, 67.458695824881601766},
    {37.619058621071729931, 67.459165211462703837},
    {37.618876230857352994, 67.459036465429051077},
    {37.618876230857352994, 67.459036465429051077},
    {37.619058621071729931, 67.459165211462703837},
    {37.618967425964541462, 67.459251042151834099},
    {37.618967425964541462, 67.459251042151834099},
    {37.619058621071729931, 67.459165211462703837},
    {37.619069349907846345, 67.459181304716935301},
    {37.618967425964541462, 67.459251042151834099},
    {37.619069349907846345, 67.459181304716935301},
    {37.618978154800657876, 67.459267135406037141},
    {37.618876230857352994, 67.459036465429051077},
    {37.618967425964541462, 67.459251042151834099},
    {37.618916463992860599, 67.459296639705428333},
    {37.618876230857352994, 67.459036465429051077},
    {37.618916463992860599, 67.459296639705428333},
    {37.61883867993086028, 67.459074016355515369},
    {37.61883867993086028, 67.459074016355515369},
    {37.618916463992860599, 67.459296639705428333},
    {37.618833315512773652, 67.45937174155841376},
    {37.618833315512773652, 67.45937174155841376},
    {37.618916463992860599, 67.459296639705428333},
    {37.618849408767005116, 67.459385152603573488},
    {37.618849408767005116, 67.459385152603573488},
    {37.618916463992860599, 67.459296639705428333},
    {37.618927192829005435, 67.459312732959631376},
    {37.61883867993086028, 67.459074016355515369},
    {37.618833315512773652, 67.45937174155841376},
    {37.618610692162889109, 67.45927249982412377},
    {37.618610692162889109, 67.45927249982412377},
    {37.618833315512773652, 67.45937174155841376},
    {37.61846317066596157, 67.459715064314877964},
    {37.618610692162889109, 67.45927249982412377},
    {37.61846317066596157, 67.459715064314877964},
    {37.618243229525120341, 67.459615822580587974},
    {37.618243229525120341, 67.459615822580587974},
    {37.61846317066596157, 67.459715064314877964},
    {37.618283462660627947, 67.459881361275023437},
    {37.618283462660627947, 67.459881361275023437},
    {37.61846317066596157, 67.459715064314877964},
    {37.61847658171114972, 67.459731157569081006},
    {37.618283462660627947, 67.459881361275023437},
    {37.61847658171114972, 67.459731157569081006},
    {37.618296873705816097, 67.459897454529226479},
    {37.618243229525120341, 67.459615822580587974},
    {37.618283462660627947, 67.459881361275023437},
    {37.618211043016685835, 67.459905501156328},
    {37.618211043016685835, 67.459905501156328},
    {37.618283462660627947, 67.459881361275023437},
    {37.618240547316077027, 67.45991891220151615},
    {37.618243229525120341, 67.459615822580587974},
    {37.618211043016685835, 67.459905501156328},
    {37.618181538717323065, 67.459916229992472836},
    {37.618243229525120341, 67.459615822580587974},
    {37.618181538717323065, 67.459916229992472836},
    {37.617977690830656456, 67.459825034885284367},
    {37.618243229525120341, 67.459615822580587974},
    {37.617977690830656456, 67.459825034885284367},
    {37.618152034417931873, 67.459575589445051946},
    {37.618152034417931873, 67.459575589445051946},
    {37.617977690830656456, 67.459825034885284367},
    {37.617932093277062222, 67.459495123174008313},
    {37.617932093277062222, 67.459495123174008313},
    {37.617977690830656456, 67.459825034885284367},
    {37.617969644203554935, 67.459795530585921597},
    {37.617932093277062222, 67.459495123174008313},
    {37.617969644203554935, 67.459795530585921597},
    {37.617940139904163743, 67.459739204196182527},
    {37.617940139904163743, 67.459739204196182527},
    {37.617969644203554935, 67.459795530585921597},
    {37.617940139904163743, 67.459784801749776761},
    {37.617932093277062222, 67.459495123174008313},
    {37.617940139904163743, 67.459739204196182527},
    {37.617918682231902494, 67.459733839778095899},
    {37.617932093277062222, 67.459495123174008313},
    {37.617918682231902494, 67.459733839778095899},
    {37.61791600002285918, 67.459497805383051627},
    {37.618152034417931873, 67.459575589445051946},
    {37.617932093277062222, 67.459495123174008313},
    {37.618143987790830352, 67.459170575880790466},
    {37.618143987790830352, 67.459170575880790466},
    {37.617932093277062222, 67.459495123174008313},
    {37.617913317813815866, 67.458652909537050846},
    {37.618143987790830352, 67.459170575880790466},
    {37.617913317813815866, 67.458652909537050846},
    {37.618106436864337638, 67.457987721696412109},
    {37.618143987790830352, 67.459170575880790466},
    {37.618106436864337638, 67.457987721696412109},
    {37.618176174299236436, 67.45875483348038415},
    {37.61883867993086028, 67.459074016355515369},
    {37.618610692162889109, 67.45927249982412377},
    {37.618583870072541231, 67.459208126807283179},
    {37.61883867993086028, 67.459074016355515369},
    {37.618583870072541231, 67.459208126807283179},
    {37.618299555914859411, 67.459071334146500476},
    {37.61883867993086028, 67.459074016355515369},
    {37.618299555914859411, 67.459071334146500476},
    {37.618176174299236436, 67.45875483348038415},
    {37.618176174299236436, 67.45875483348038415},
    {37.618299555914859411, 67.459071334146500476},
    {37.618143987790830352, 67.459170575880790466},
    {37.618143987790830352, 67.459170575880790466},
    {37.618299555914859411, 67.459071334146500476},
    {37.618213725225729149, 67.45920276238919655},
    {37.618213725225729149, 67.45920276238919655},
    {37.618299555914859411, 67.459071334146500476},
    {37.618302238123874304, 67.459157164835602316},
    {37.619061303280744823, 67.458695824881601766},
    {37.618876230857352994, 67.459036465429051077},
    {37.618790400168222732, 67.45893990590377598},
    {37.619045210026541781, 67.45833640887093452},
    {37.619061303280744823, 67.458695824881601766},
    {37.618999612472947547, 67.458639498491891118},
    {37.619045210026541781, 67.45833640887093452},
    {37.618999612472947547, 67.458639498491891118},
    {37.618988883636802711, 67.458398099678731796},
    {37.618988883636802711, 67.458398099678731796},
    {37.618999612472947547, 67.458639498491891118},
    {37.618964743755498148, 67.458561714429862377},
    {37.618988883636802711, 67.458398099678731796},
    {37.618964743755498148, 67.458561714429862377},
    {37.618959379337411519, 67.45847856594977543},
    {37.618857455394106637, 67.457987721696412109},
    {37.619045210026541781, 67.45833640887093452},
    {37.618763578077874854, 67.458097692266846934},
    {37.618573141236396395, 67.457757051719397623},
    {37.618803811213410881, 67.457942124142817875},
    {37.618535590309903682, 67.45785361124467272},
    {37.618535590309903682, 67.45785361124467272},
    {37.618803811213410881, 67.457942124142817875},
    {37.618168127672134915, 67.458301540153485121},
    {37.618535590309903682, 67.45785361124467272},
    {37.618168127672134915, 67.458301540153485121},
    {37.61822177185283067, 67.458030637040963029},
    {37.61822177185283067, 67.458030637040963029},
    {37.618168127672134915, 67.458301540153485121},
    {37.618106436864337638, 67.457987721696412109},
    {37.618106436864337638, 67.457987721696412109},
    {37.618168127672134915, 67.458301540153485121},
    {37.618176174299236436, 67.45875483348038415},
    {37.61823518289801882, 67.457153554686556163},
    {37.618205678598627628, 67.457445915471367925},
    {37.618160081045033394, 67.457148190268497956},
    {37.618160081045033394, 67.457148190268497956},
    {37.618205678598627628, 67.457445915471367925},
    {37.617934775486105536, 67.457258160838904359},
    {37.617934775486105536, 67.457258160838904359},
    {37.618205678598627628, 67.457445915471367925},
    {37.618093025819149489, 67.457499559652063681},
    {37.617934775486105536, 67.457258160838904359},
    {37.618093025819149489, 67.457499559652063681},
    {37.617883813514453095, 67.4573359449009331},
    {37.617883813514453095, 67.4573359449009331},
    {37.618093025819149489, 67.457499559652063681},
    {37.617889177932511302, 67.457571979296005793},
    {37.617889177932511302, 67.457571979296005793},
    {37.618093025819149489, 67.457499559652063681},
    {37.618106436864337638, 67.457987721696412109},
    {37.617889177932511302, 67.457571979296005793},
    {37.618106436864337638, 67.457987721696412109},
    {37.617910635604800973, 67.458441015023311138},
    {37.617910635604800973, 67.458441015023311138},
    {37.618106436864337638, 67.457987721696412109},
    {37.617913317813815866, 67.458652909537050846},
    {37.617910635604800973, 67.458441015023311138},
    {37.617913317813815866, 67.458652909537050846},
    {37.617897224559612823, 67.458652909537050846},
    {37.617910635604800973, 67.458441015023311138},
    {37.617897224559612823, 67.458652909537050846},
    {37.617891860141554616, 67.458441015023311138},
    {37.617883813514453095, 67.4573359449009331},
    {37.617889177932511302, 67.457571979296005793},
    {37.617875766887323152, 67.457569297086962479},
    {37.617883813514453095, 67.4573359449009331},
    {37.617875766887323152, 67.457569297086962479},
    {37.617870402469264945, 67.4573359449009331},
    {37.617934775486105536, 67.457258160838904359},
    {37.617883813514453095, 67.4573359449009331},
    {37.617883813514453095, 67.457284982929280659},
    {37.618160081045033394, 67.457148190268497956},
    {37.617934775486105536, 67.457258160838904359},
    {37.618152034417931873, 67.457134779223309806},
    {37.618152034417931873, 67.457134779223309806},
    {37.617934775486105536, 67.457258160838904359},
    {37.617926728859004015, 67.457242067584701317},
    {37.61823518289801882, 67.457153554686556163},
    {37.618160081045033394, 67.457148190268497956},
    {37.618194949762482793, 67.457132097014266492}
  };

  auto const matched = osm::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(matched.attribute("id").value(), string("1359233"), ());
}
}  // namespace
