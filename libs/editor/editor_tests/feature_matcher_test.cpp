#include "testing/testing.hpp"

#include "editor/feature_matcher.hpp"
#include "editor/xml_feature.hpp"

#include <string>
#include <vector>

#include <pugixml.hpp>

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

    auto const bestNode = matcher::GetBestOsmNode(osmResponse, ms::LatLon(53.8977398, 27.5579251));
    TEST_EQUAL(editor::XMLFeature(bestNode).GetName(), "Главное управление капитального строительства", ());
  }
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseNode, ::strlen(osmRawResponseNode)), ());

    auto const bestNode = matcher::GetBestOsmNode(osmResponse, ms::LatLon(53.8977254, 27.5578377));
    TEST_EQUAL(bestNode.attribute("id").value(), std::string("277172019"), ());
  }
}

UNIT_TEST(GetBestOsmWay_Test)
{
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseWay, ::strlen(osmRawResponseWay)), ());
    std::vector<m2::PointD> const geometry = {
        {27.557515856307106, 64.236609073256034}, {27.55784576801841, 64.236820967769773},
        {27.557352241556003, 64.236863883114324}, {27.55784576801841, 64.236820967769773},
        {27.557352241556003, 64.236863883114324}, {27.557765301747366, 64.236963124848614},
        {27.557352241556003, 64.236863883114324}, {27.557765301747366, 64.236963124848614},
        {27.55768215326728, 64.237078459837136}};

    auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
    TEST(bestWay, ());
    TEST_EQUAL(editor::XMLFeature(bestWay).GetName(), "Беллесбумпром", ());
  }
  {
    pugi::xml_document osmResponse;
    TEST(osmResponse.load_buffer(osmRawResponseWay, ::strlen(osmRawResponseWay)), ());
    // Each point is moved for 0.0001 on x and y. It is aboout a half of side length.
    std::vector<m2::PointD> const geometry = {
        {27.557615856307106, 64.236709073256034}, {27.55794576801841, 64.236920967769773},
        {27.557452241556003, 64.236963883114324}, {27.55794576801841, 64.236920967769773},
        {27.557452241556003, 64.236963883114324}, {27.557865301747366, 64.237063124848614},
        {27.557452241556003, 64.236963883114324}, {27.557865301747366, 64.237063124848614},
        {27.55778215326728, 64.237178459837136}};

    auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
    TEST(!bestWay, ());
  }
}

UNIT_TEST(GetBestOsmRealtion_Test)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(osmRawResponseRelation, ::strlen(osmRawResponseRelation)), ());
  std::vector<m2::PointD> const geometry = {
      {37.640253436865322, 67.455316241497655}, {37.64019442826654, 67.455394025559684},
      {37.640749645536772, 67.455726619480004}, {37.640253436865322, 67.455316241497655},
      {37.640749645536772, 67.455726619480004}, {37.64069063693799, 67.455281372780206},
      {37.640253436865322, 67.455316241497655}, {37.64069063693799, 67.455281372780206},
      {37.640505564514598, 67.455174084418815}, {37.640253436865322, 67.455316241497655},
      {37.640505564514598, 67.455174084418815}, {37.640376818480917, 67.455053385012263},
      {37.640253436865322, 67.455316241497655}, {37.640376818480917, 67.455053385012263},
      {37.640320492091178, 67.454932685605684}, {37.640253436865322, 67.455316241497655},
      {37.640320492091178, 67.454932685605684}, {37.640111279786453, 67.455147262328467},
      {37.640111279786453, 67.455147262328467}, {37.640320492091178, 67.454932685605684},
      {37.640046906769612, 67.454938050023742}, {37.640046906769612, 67.454938050023742},
      {37.640320492091178, 67.454932685605684}, {37.640320492091178, 67.454811986199104},
      {37.640046906769612, 67.454938050023742}, {37.640320492091178, 67.454811986199104},
      {37.640105915368395, 67.454677875747365}, {37.640105915368395, 67.454677875747365},
      {37.640320492091178, 67.454811986199104}, {37.64035804301767, 67.454704697837713},
      {37.640105915368395, 67.454677875747365}, {37.64035804301767, 67.454704697837713},
      {37.64018101722138, 67.454508896578176},  {37.64018101722138, 67.454508896578176},
      {37.64035804301767, 67.454704697837713},  {37.640663814847642, 67.454390879380639},
      {37.64018101722138, 67.454508896578176},  {37.640663814847642, 67.454390879380639},
      {37.640489471260366, 67.454173620448813}, {37.640489471260366, 67.454173620448813},
      {37.640663814847642, 67.454390879380639}, {37.640634310548251, 67.454090471968726},
      {37.640634310548251, 67.454090471968726}, {37.640663814847642, 67.454390879380639},
      {37.640827429598772, 67.454321141945741}, {37.640634310548251, 67.454090471968726},
      {37.640827429598772, 67.454321141945741}, {37.640787196463265, 67.454063649878378},
      {37.640787196463265, 67.454063649878378}, {37.640827429598772, 67.454321141945741},
      {37.64095349342341, 67.454079743132581},  {37.64095349342341, 67.454079743132581},
      {37.640827429598772, 67.454321141945741}, {37.641001773186048, 67.454350646245103},
      {37.64095349342341, 67.454079743132581},  {37.641001773186048, 67.454350646245103},
      {37.641098332711294, 67.454152162776523}, {37.641098332711294, 67.454152162776523},
      {37.641001773186048, 67.454350646245103}, {37.641243171999179, 67.45453303645948},
      {37.641098332711294, 67.454152162776523}, {37.641243171999179, 67.45453303645948},
      {37.641618681264049, 67.454541083086582}, {37.641618681264049, 67.454541083086582},
      {37.641243171999179, 67.45453303645948},  {37.64069063693799, 67.455281372780206},
      {37.641618681264049, 67.454541083086582}, {37.64069063693799, 67.455281372780206},
      {37.640749645536772, 67.455726619480004}};

  auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), std::string("365808"), ());
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
  std::vector<m2::PointD> const geometry = {
      {-0.2048121407986514, 60.333984198674443},  {-0.20478800091734684, 60.333909096821458},
      {-0.20465925488366565, 60.334029796228037}, {-0.2048121407986514, 60.333984198674443},
      {-0.20478800091734684, 60.333909096821458}, {-0.20463511500236109, 60.333954694375052}};

  auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), std::string("345630019"), ());
}

std::string const kHouseWithSeveralEntrances = R"xxx("
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
  TEST(osmResponse.load_buffer(kHouseWithSeveralEntrances.c_str(), kHouseWithSeveralEntrances.size()), ());

  std::vector<m2::PointD> geometry = {
      {37.570076119676798, 67.574481424499169}, {37.570258509891175, 67.574527022052763},
      {37.569802534355233, 67.575570401367315}, {37.570258509891175, 67.574527022052763},
      {37.569802534355233, 67.575570401367315}, {37.56998492456961, 67.57561599892091}};

  auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), std::string("30680719"), ());
}

std::string const kRelationWithSingleWay = R"XXX(
<osm version="0.6" generator="CGImap 0.6.0 (2191 thorn-01.openstreetmap.org)" copyright="OpenStreetMap and contributors" attribution="http://www.openstreetmap.org/copyright" license="http://opendatacommons.org/licenses/odbl/1-0/">
<node id="287253844" visible="true" version="6" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7955735" lon="37.5399204"/>
<node id="287253846" visible="true" version="6" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7952597" lon="37.5394774"/>
<node id="287253848" visible="true" version="6" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7947419" lon="37.5406379"/>
<node id="287253850" visible="true" version="7" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7950556" lon="37.5410809"/>
<node id="3481651579" visible="true" version="2" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7946855" lon="37.5407643"/>
<node id="3481651621" visible="true" version="2" changeset="41744272" timestamp="2016-08-27T20:57:16Z" user="Vadiм" uid="326091" lat="55.7949992" lon="37.5412073"/>
<node id="4509122916" visible="true" version="1" changeset="43776534" timestamp="2016-11-18T19:35:01Z" user="aq123" uid="4289510" lat="55.7954158" lon="37.5396978">
<tag k="entrance" v="main"/>
</node>
<way id="26232961" visible="true" version="10" changeset="43776534" timestamp="2016-11-18T19:35:01Z" user="aq123" uid="4289510">
<nd ref="287253844"/>
<nd ref="4509122916"/>
<nd ref="287253846"/>
<nd ref="287253848"/>
<nd ref="3481651579"/>
<nd ref="3481651621"/>
<nd ref="287253850"/>
<nd ref="287253844"/>
<tag k="building" v="yes"/>
<tag k="leisure" v="sports_centre"/>
</way>
<relation id="111" visible="true" version="6" changeset="222" timestamp="2015-01-12T16:15:14Z" user="test" uid="333">
<member type="way" ref="26232961"/>
<tag k="building" v="yes"/>
<tag k="type" v="multipolygon"/>
</relation>
</osm>
)XXX";

UNIT_TEST(RelationWithSingleWay)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(kRelationWithSingleWay.c_str(), kRelationWithSingleWay.size()), ());

  std::vector<m2::PointD> geometry = {
      {37.539920043497688, 67.533792313440074}, {37.539477479006933, 67.533234413960827},
      {37.541207503834414, 67.532770391797811}, {37.539477479006933, 67.533234413960827},
      {37.541207503834414, 67.532770391797811}, {37.54076493934366, 67.532212492318536}};

  auto const bestWay = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(bestWay.attribute("id").value(), std::string("26232961"), ());
}

UNIT_TEST(ScoreTriangulatedGeometries)
{
  std::vector<m2::PointD> lhs = {{0, 0}, {10, 10}, {10, 0}, {0, 0}, {10, 10}, {0, 10}};

  std::vector<m2::PointD> rhs = {{-1, -1}, {9, 9}, {9, -1}, {-1, -1}, {9, 9}, {-1, 9}};

  auto const score = matcher::ScoreTriangulatedGeometries(lhs, rhs);
  TEST_GREATER(score, 0.6, ());
}
}  // namespace
