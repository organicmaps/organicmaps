#include "testing/testing.hpp"

#include "editor/feature_matcher.hpp"
#include "editor/xml_feature.hpp"

#include <string>
#include <vector>

#include <pugixml.hpp>

namespace
{
// This place on OSM map https://www.openstreetmap.org/relation/1359233.
std::string const kSenatskiyDvorets = R"XXX(
<?xml version="1.0" encoding="UTF-8"?>
<osm version="0.6" generator="CGImap 0.6.0 (31854 thorn-01.openstreetmap.org)" copyright="OpenStreetMap and contributors" attribution="http://www.openstreetmap.org/copyright" license="http://opendatacommons.org/licenses/odbl/1-0/">
  <node id="271895281" visible="true" version="9" changeset="39264478" timestamp="2016-05-12T13:15:43Z" user="Felis Pimeja" uid="260756" lat="55.7578113" lon="37.6220187" />
  <node id="271895282" visible="true" version="6" changeset="6331881" timestamp="2010-11-09T21:58:31Z" user="Felis Pimeja" uid="260756" lat="55.7587478" lon="37.6223438" />
  <node id="271895283" visible="true" version="9" changeset="39264478" timestamp="2016-05-12T14:00:50Z" user="Felis Pimeja" uid="260756" lat="55.7590475" lon="37.6221887" />
  <node id="271895284" visible="true" version="9" changeset="39264478" timestamp="2016-05-12T14:00:50Z" user="Felis Pimeja" uid="260756" lat="55.7587811" lon="37.6206936" />
  <node id="271895285" visible="true" version="9" changeset="39264478" timestamp="2016-05-12T14:00:50Z" user="Felis Pimeja" uid="260756" lat="55.7585581" lon="37.6208892" />
  <node id="271895287" visible="true" version="8" changeset="39264478" timestamp="2016-05-12T13:15:44Z" user="Felis Pimeja" uid="260756" lat="55.7579401" lon="37.6212370" />
  <node id="271895288" visible="true" version="8" changeset="39264478" timestamp="2016-05-12T13:15:44Z" user="Felis Pimeja" uid="260756" lat="55.7580549" lon="37.6218815" />
  <node id="353142031" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T13:11:39Z" user="Felis Pimeja" uid="260756" lat="55.7582873" lon="37.6213727" />
  <node id="353142032" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T13:11:40Z" user="Felis Pimeja" uid="260756" lat="55.7581121" lon="37.6214459" />
  <node id="353142033" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T13:11:40Z" user="Felis Pimeja" uid="260756" lat="55.7583325" lon="37.6216261" />
  <node id="353142034" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T13:11:40Z" user="Felis Pimeja" uid="260756" lat="55.7581614" lon="37.6217225" />
  <node id="353142039" visible="true" version="7" changeset="39264478" timestamp="2016-05-12T14:00:50Z" user="Felis Pimeja" uid="260756" lat="55.7585468" lon="37.6208256" />
  <node id="353142040" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T21:48:55Z" user="Felis Pimeja" uid="260756" lat="55.7587047" lon="37.6217831" />
  <node id="353142041" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T22:01:20Z" user="Felis Pimeja" uid="260756" lat="55.7585349" lon="37.6218703" />
  <node id="353142042" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T14:03:22Z" user="Felis Pimeja" uid="260756" lat="55.7585608" lon="37.6220405" />
  <node id="353142043" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T21:58:14Z" user="Felis Pimeja" uid="260756" lat="55.7587140" lon="37.6220934" />
  <node id="353142044" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T21:51:38Z" user="Felis Pimeja" uid="260756" lat="55.7587923" lon="37.6220481" />
  <node id="353142045" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T21:49:59Z" user="Felis Pimeja" uid="260756" lat="55.7587555" lon="37.6218406" />
  <node id="353142046" visible="true" version="4" changeset="39264478" timestamp="2016-05-12T14:03:22Z" user="Felis Pimeja" uid="260756" lat="55.7587009" lon="37.6218191" />
  <node id="353142049" visible="true" version="5" changeset="39264478" timestamp="2016-05-12T13:11:40Z" user="Felis Pimeja" uid="260756" lat="55.7582228" lon="37.6220672" />
  <node id="353142050" visible="true" version="3" changeset="6331881" timestamp="2010-11-09T21:48:58Z" user="Felis Pimeja" uid="260756" lat="55.7582714" lon="37.6220589" />
  <node id="353142051" visible="true" version="4" changeset="6331881" timestamp="2010-11-09T22:01:30Z" user="Felis Pimeja" uid="260756" lat="55.7584225" lon="37.6221015" />
  <node id="671182909" visible="true" version="4" changeset="39264478" timestamp="2016-05-12T13:15:44Z" user="Felis Pimeja" uid="260756" lat="55.7578298" lon="37.6221226" />
  <node id="983727885" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:11:46Z" user="Felis Pimeja" uid="260756" lat="55.7584302" lon="37.6220342" />
  <node id="983728327" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T14:03:22Z" user="Felis Pimeja" uid="260756" lat="55.7587356" lon="37.6218592" />
  <node id="983728709" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:11:46Z" user="Felis Pimeja" uid="260756" lat="55.7582566" lon="37.6213646" />
  <node id="983730214" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:11:46Z" user="Felis Pimeja" uid="260756" lat="55.7582607" lon="37.6213877" />
  <node id="983730647" visible="true" version="1" changeset="6331881" timestamp="2010-11-09T21:43:48Z" user="Felis Pimeja" uid="260756" lat="55.7582481" lon="37.6220778" />
  <node id="983731126" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T14:03:22Z" user="Felis Pimeja" uid="260756" lat="55.7587086" lon="37.6220650" />
  <node id="983731305" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:11:46Z" user="Felis Pimeja" uid="260756" lat="55.7583984" lon="37.6218559" />
  <node id="983732911" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:11:47Z" user="Felis Pimeja" uid="260756" lat="55.7582046" lon="37.6219650" />
  <node id="4181167714" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T13:15:44Z" user="Felis Pimeja" uid="260756" lat="55.7580204" lon="37.6216881">
    <tag k="entrance" v="yes" />
  </node>
  <node id="4181259382" visible="true" version="1" changeset="39264478" timestamp="2016-05-12T14:03:16Z" user="Felis Pimeja" uid="260756" lat="55.7588440" lon="37.6222940" />
  <node id="4420391892" visible="true" version="1" changeset="42470341" timestamp="2016-09-27T13:17:17Z" user="literan" uid="830106" lat="55.7588469" lon="37.6210627">
    <tag k="entrance" v="main" />
  </node>
  <way id="25010101" visible="true" version="12" changeset="42470341" timestamp="2016-09-27T13:17:18Z" user="literan" uid="830106">
    <nd ref="271895283" />
    <nd ref="4420391892" />
    <nd ref="271895284" />
    <nd ref="353142039" />
    <nd ref="271895285" />
    <nd ref="271895287" />
    <nd ref="4181167714" />
    <nd ref="271895288" />
    <nd ref="271895281" />
    <nd ref="671182909" />
  </way>
  <way id="31560451" visible="true" version="2" changeset="6331881" timestamp="2010-11-09T22:01:05Z" user="Felis Pimeja" uid="260756">
    <nd ref="353142031" />
    <nd ref="353142033" />
    <nd ref="353142034" />
    <nd ref="353142032" />
    <nd ref="983728709" />
    <nd ref="983730214" />
    <nd ref="353142031" />
  </way>
  <way id="31560453" visible="true" version="2" changeset="6331881" timestamp="2010-11-09T22:00:32Z" user="Felis Pimeja" uid="260756">
    <nd ref="353142040" />
    <nd ref="353142041" />
    <nd ref="353142042" />
    <nd ref="983731126" />
    <nd ref="353142043" />
    <nd ref="353142044" />
    <nd ref="353142045" />
    <nd ref="983728327" />
    <nd ref="353142046" />
    <nd ref="353142040" />
  </way>
  <way id="31560454" visible="true" version="3" changeset="39264478" timestamp="2016-05-12T13:11:32Z" user="Felis Pimeja" uid="260756">
    <nd ref="353142049" />
    <nd ref="983730647" />
    <nd ref="353142050" />
    <nd ref="353142051" />
    <nd ref="983727885" />
  </way>
  <way id="417596299" visible="true" version="2" changeset="39264478" timestamp="2016-05-12T14:03:19Z" user="Felis Pimeja" uid="260756">
    <nd ref="671182909" />
    <nd ref="271895282" />
    <nd ref="4181259382" />
    <nd ref="271895283" />
  </way>
  <way id="417596307" visible="true" version="1" changeset="39264478" timestamp="2016-05-12T13:11:18Z" user="Felis Pimeja" uid="260756">
    <nd ref="983727885" />
    <nd ref="983731305" />
    <nd ref="983732911" />
    <nd ref="353142049" />
  </way>
  <relation id="85761" visible="true" version="15" changeset="44993517" timestamp="2017-01-08T04:43:37Z" user="Alexander-II" uid="3580412">
    <member type="way" ref="25010101" role="outer" />
    <member type="way" ref="417596299" role="outer" />
    <member type="way" ref="31560451" role="inner" />
    <member type="way" ref="31560453" role="inner" />
    <member type="way" ref="417596307" role="inner" />
    <member type="way" ref="31560454" role="inner" />
    <tag k="addr:city" v="Москва" />
    <tag k="addr:country" v="RU" />
    <tag k="addr:housenumber" v="2" />
    <tag k="addr:street" v="Театральный проезд" />
    <tag k="building" v="yes" />
    <tag k="building:colour" v="tan" />
    <tag k="building:levels" v="5" />
    <tag k="contact:phone" v="+7 499 5017800" />
    <tag k="contact:website" v="http://metmos.ru" />
    <tag k="int_name" v="Hotel Metropol" />
    <tag k="name" v="Метрополь" />
    <tag k="name:de" v="Hotel Metropol" />
    <tag k="name:el" v="Ξενοδοχείο Μετροπόλ" />
    <tag k="name:en" v="Hotel Metropol" />
    <tag k="name:pl" v="Hotel Metropol" />
    <tag k="opening_hours" v="24/7" />
    <tag k="roof:material" v="metal" />
    <tag k="tourism" v="hotel" />
    <tag k="type" v="multipolygon" />
    <tag k="wikidata" v="Q2034313" />
    <tag k="wikipedia" v="ru:Метрополь (гостиница, Москва)" />
  </relation>
</osm>
)XXX";

UNIT_TEST(MatchByGeometry)
{
  pugi::xml_document osmResponse;
  TEST(osmResponse.load_buffer(kSenatskiyDvorets.c_str(), kSenatskiyDvorets.size()), ());

  // It is a triangulated polygon. Every triangle is presented as three points.
  // For simplification, you can visualize it as a single sequence of points
  // by using, for ex. Gnuplot.
  std::vector<m2::PointD> geometry = {
      {37.621818614168603, 67.468231078000599}, {37.621858847304139, 67.468292768808396},
      {37.621783745451154, 67.468236442418657}, {37.621783745451154, 67.468236442418657},
      {37.621858847304139, 67.468292768808396}, {37.621840071840893, 67.468327637525846},
      {37.621783745451154, 67.468236442418657}, {37.621840071840893, 67.468327637525846},
      {37.620694768582979, 67.46837323507944},  {37.621783745451154, 67.468236442418657},
      {37.620694768582979, 67.46837323507944},  {37.620887887633501, 67.46797626814228},
      {37.620887887633501, 67.46797626814228},  {37.620694768582979, 67.46837323507944},
      {37.620826196825703, 67.467957492679034}, {37.621783745451154, 67.468236442418657},
      {37.620887887633501, 67.46797626814228},  {37.621625495118082, 67.467576618996077},
      {37.621625495118082, 67.467576618996077}, {37.620887887633501, 67.46797626814228},
      {37.621373367468806, 67.467496152725033}, {37.621373367468806, 67.467496152725033},
      {37.620887887633501, 67.46797626814228},  {37.621365320841704, 67.467439826335323},
      {37.621373367468806, 67.467496152725033}, {37.621365320841704, 67.467439826335323},
      {37.621386778513994, 67.467447872962424}, {37.621783745451154, 67.468236442418657},
      {37.621625495118082, 67.467576618996077}, {37.621869576140256, 67.467936035006772},
      {37.621869576140256, 67.467936035006772}, {37.621625495118082, 67.467576618996077},
      {37.621856165095096, 67.467691953984598}, {37.621856165095096, 67.467691953984598},
      {37.621625495118082, 67.467576618996077}, {37.621966135665531, 67.467348631228134},
      {37.621966135665531, 67.467348631228134}, {37.621625495118082, 67.467576618996077},
      {37.621722054643357, 67.467270847166105}, {37.621966135665531, 67.467348631228134},
      {37.621722054643357, 67.467270847166105}, {37.621880304976401, 67.46708309253367},
      {37.621880304976401, 67.46708309253367},  {37.621722054643357, 67.467270847166105},
      {37.621445787112748, 67.467185016477004}, {37.621880304976401, 67.46708309253367},
      {37.621445787112748, 67.467185016477004}, {37.621236574808023, 67.466879244647032},
      {37.621236574808023, 67.466879244647032}, {37.621445787112748, 67.467185016477004},
      {37.621365320841704, 67.467439826335323}, {37.621236574808023, 67.466879244647032},
      {37.621365320841704, 67.467439826335323}, {37.620887887633501, 67.46797626814228},
      {37.621966135665531, 67.467348631228134}, {37.621880304976401, 67.46708309253367},
      {37.622068059608836, 67.46738081773654},  {37.622068059608836, 67.46738081773654},
      {37.621880304976401, 67.46708309253367},  {37.622121703789531, 67.466683443387467},
      {37.622121703789531, 67.466683443387467}, {37.621880304976401, 67.46708309253367},
      {37.622019779846227, 67.466648574670018}, {37.622068059608836, 67.46738081773654},
      {37.622121703789531, 67.466683443387467}, {37.622078788444981, 67.467426415290134},
      {37.621869576140256, 67.467936035006772}, {37.621856165095096, 67.467691953984598},
      {37.622033190891386, 67.467748280374309}, {37.621869576140256, 67.467936035006772},
      {37.622033190891386, 67.467748280374309}, {37.622041237518488, 67.467981632560367},
      {37.622041237518488, 67.467981632560367}, {37.622033190891386, 67.467748280374309},
      {37.62210024611727, 67.467734869329149},  {37.622041237518488, 67.467981632560367},
      {37.62210024611727, 67.467734869329149},  {37.622344327139444, 67.468314226480686},
      {37.622344327139444, 67.468314226480686}, {37.62210024611727, 67.467734869329149},
      {37.622078788444981, 67.467426415290134}, {37.622078788444981, 67.467426415290134},
      {37.62210024611727, 67.467734869329149},  {37.622060012981734, 67.46746664842567},
      {37.622344327139444, 67.468314226480686}, {37.622078788444981, 67.467426415290134},
      {37.622121703789531, 67.466683443387467}, {37.622041237518488, 67.467981632560367},
      {37.622344327139444, 67.468314226480686}, {37.622092199490169, 67.468252535672889},
      {37.622092199490169, 67.468252535672889}, {37.622344327139444, 67.468314226480686},
      {37.622049284145589, 67.468392010542686}, {37.622049284145589, 67.468392010542686},
      {37.622344327139444, 67.468314226480686}, {37.622188759015415, 67.468845303869585},
      {37.622049284145589, 67.468392010542686}, {37.622188759015415, 67.468845303869585},
      {37.621840071840893, 67.468327637525846}, {37.621840071840893, 67.468327637525846},
      {37.622188759015415, 67.468845303869585}, {37.620694768582979, 67.46837323507944},
      {37.622041237518488, 67.467981632560367}, {37.622092199490169, 67.468252535672889},
      {37.622065377399821, 67.468244489045759}};

  auto const matched = matcher::GetBestOsmWayOrRelation(osmResponse, geometry);
  TEST_EQUAL(matched.attribute("id").value(), std::string("85761"), ());
}
}  // namespace
