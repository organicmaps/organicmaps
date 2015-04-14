//
//  osm_o5m_source_test.cpp
//  generator_tool
//
//  Created by Sergey Yershov on 14.04.15.
//

#include "testing/testing.hpp"
#include "generator/osm_o5m_source.hpp"

#include "std/set.hpp"


/*

 <?xml version='1.0' encoding='UTF-8'?>
 <osm version='0.6' upload='true' generator='JOSM'>
 <node id='-273105' action='modify' visible='true' lat='62.18269750679' lon='-134.28965517091'>
 <tag k='name' v='Продуктовый' />
 <tag k='opening_hours' v='24/7' />
 <tag k='shop' v='convenience' />
 </node>
 </osm>

 */

/* begin binary data: node.o5m */
uint8_t node_o5m_data[] = /* 92 */
{0xFF,0xE0,0x04,0x6F,0x35,0x6D,0x32,0xFF,0x10,0x51,0xA1,0xAB,0x21,0x00,0xCD
  ,0xE6,0xD7,0x80,0x0A,0xBE,0xCE,0x82,0xD1,0x04,0x00,0x6E,0x61,0x6D,0x65,0x00
  ,0xD0,0x9F,0xD1,0x80,0xD0,0xBE,0xD0,0xB4,0xD1,0x83,0xD0,0xBA,0xD1,0x82,0xD0
  ,0xBE,0xD0,0xB2,0xD1,0x8B,0xD0,0xB9,0x00,0x00,0x6F,0x70,0x65,0x6E,0x69,0x6E
  ,0x67,0x5F,0x68,0x6F,0x75,0x72,0x73,0x00,0x32,0x34,0x2F,0x37,0x00,0x00,0x73
  ,0x68,0x6F,0x70,0x00,0x63,0x6F,0x6E,0x76,0x65,0x6E,0x69,0x65,0x6E,0x63,0x65
  ,0x00,0xFE};
/* end binary data. size = 92 bytes */


/*
 
 <osm version="0.6" generator="CGImap 0.3.3 (31790 thorn-03.openstreetmap.org)" copyright="OpenStreetMap and contributors" attribution="http://www.openstreetmap.org/copyright" license="http://opendatacommons.org/licenses/odbl/1-0/">
 <node id="513709898" visible="true" version="8" changeset="12059128" timestamp="2012-06-29T18:09:47Z" user="Xmypblu" uid="395071" lat="55.0927062" lon="38.7666704">
 <tag k="amenity" v="cinema"/>
 <tag k="name" v="КТ Горизонт"/>
 </node>
 </osm>

 */

/* begin binary data: node2.o5m */
uint8_t node2_o5m_data[] = /* 93 */
{0xFF,0xE0,0x04,0x6F,0x35,0x6D,0x32,0xFF,0x10,0x52,0x94,0xDD,0xF4,0xE9,0x03
  ,0x08,0xD6,0xBD,0xEF,0xFE,0x09,0xF0,0x87,0xC0,0x0B,0x00,0xBF,0x8E,0x18,0x00
  ,0x58,0x6D,0x79,0x70,0x62,0x6C,0x75,0x00,0xA0,0xCC,0xDA,0xF1,0x02,0xAC,0xEB
  ,0xB3,0x8D,0x04,0x00,0x61,0x6D,0x65,0x6E,0x69,0x74,0x79,0x00,0x63,0x69,0x6E
  ,0x65,0x6D,0x61,0x00,0x00,0x6E,0x61,0x6D,0x65,0x00,0xD0,0x9A,0xD0,0xA2,0x20
  ,0xD0,0x93,0xD0,0xBE,0xD1,0x80,0xD0,0xB8,0xD0,0xB7,0xD0,0xBE,0xD0,0xBD,0xD1
  ,0x82,0x00,0xFE};
/* end binary data. size = 93 bytes */


/*

 <?xml version='1.0' encoding='UTF-8'?>
 <osm version='0.6' upload='true' generator='JOSM'>
 <node id='-273141' action='modify' visible='true' lat='60.73662144558' lon='-135.06327391353' />
 <node id='-273139' action='modify' visible='true' lat='60.73186582163' lon='-135.05498459176' />
 <node id='-273137' action='modify' visible='true' lat='60.72928677326' lon='-135.05450504422' />
 <node id='-273135' action='modify' visible='true' lat='60.72600404848' lon='-135.05101119785' />
 <node id='-273133' action='modify' visible='true' lat='60.7207107856' lon='-135.0490245009' />
 <node id='-273131' action='modify' visible='true' lat='60.71776226098' lon='-135.04587318849' />
 <node id='-273129' action='modify' visible='true' lat='60.71528261036' lon='-135.04464006624' />
 <node id='-273126' action='modify' visible='true' lat='60.71283628103' lon='-135.0465582564' />
 <node id='-273125' action='modify' visible='true' lat='60.70931726156' lon='-135.05094269106' />
 <way id='-273127' action='modify' visible='true'>
 <nd ref='-273125' />
 <nd ref='-273126' />
 <nd ref='-273129' />
 <nd ref='-273131' />
 <nd ref='-273133' />
 <nd ref='-273135' />
 <nd ref='-273137' />
 <nd ref='-273139' />
 <nd ref='-273141' />
 <tag k='name' v='Yukon River' />
 <tag k='name:ru' v='Юкон' />
 <tag k='waterway' v='river' />
 </way>
 </osm>

 */

/* begin binary data: way.o5m */
uint8_t way_o5m_data[] = /* 175 */
{0xFF,0xE0,0x04,0x6F,0x35,0x6D,0x32,0xFF,0x10,0x0E,0xE9,0xAB,0x21,0x00,0xC5
  ,0x94,0x88,0x88,0x0A,0x8C,0xB1,0x9D,0xC3,0x04,0x10,0x08,0x04,0x00,0x9C,0x8F
  ,0x0A,0x87,0xE7,0x05,0x10,0x07,0x04,0x00,0xF6,0x4A,0xFD,0x92,0x03,0x10,0x08
  ,0x04,0x00,0xF6,0xA1,0x04,0xF5,0x80,0x04,0x10,0x08,0x04,0x00,0xB4,0xB6,0x02
  ,0x89,0xBB,0x06,0x10,0x08,0x04,0x00,0xB4,0xEC,0x03,0xD9,0xCC,0x03,0x10,0x08
  ,0x04,0x00,0xD6,0xC0,0x01,0xB7,0x83,0x03,0x10,0x08,0x06,0x00,0xDB,0xAB,0x02
  ,0x9F,0xFE,0x02,0x10,0x08,0x02,0x00,0x87,0xAD,0x05,0xEB,0xA5,0x04,0xFF,0x11
  ,0x44,0xCD,0xAB,0x21,0x00,0x0B,0xC9,0xAB,0x21,0x01,0x05,0x03,0x03,0x03,0x03
  ,0x03,0x03,0x00,0x6E,0x61,0x6D,0x65,0x00,0x59,0x75,0x6B,0x6F,0x6E,0x20,0x52
  ,0x69,0x76,0x65,0x72,0x00,0x00,0x6E,0x61,0x6D,0x65,0x3A,0x72,0x75,0x00,0xD0
  ,0xAE,0xD0,0xBA,0xD0,0xBE,0xD0,0xBD,0x00,0x00,0x77,0x61,0x74,0x65,0x72,0x77
  ,0x61,0x79,0x00,0x72,0x69,0x76,0x65,0x72,0x00,0xFE};
/* end binary data. size = 175 bytes */


/*
 
 <?xml version='1.0' encoding='UTF-8'?>
 <osm version='0.6' upload='true' generator='JOSM'>
 <node id='-273196' action='modify' visible='true' lat='60.71960512058' lon='-135.0538199763'>
 <tag k='name' v='Whitehorse' />
 <tag k='place' v='town' />
 </node>
 <node id='-273174' action='modify' visible='true' lat='60.71893500205' lon='-135.02422504237' />
 <node id='-273172' action='modify' visible='true' lat='60.70097061781' lon='-134.99915155667' />
 <node id='-273170' action='modify' visible='true' lat='60.68655163161' lon='-135.03409002034' />
 <node id='-273168' action='modify' visible='true' lat='60.68836274296' lon='-135.1128728306' />
 <node id='-273166' action='modify' visible='true' lat='60.70090356772' lon='-135.1290404334' />
 <node id='-273164' action='modify' visible='true' lat='60.72181641257' lon='-135.11246178985' />
 <node id='-273162' action='modify' visible='true' lat='60.74043883115' lon='-135.08451101891' />
 <node id='-273161' action='modify' visible='true' lat='60.73997005387' lon='-135.06327391353' />
 <way id='-273163' action='modify' visible='true'>
 <nd ref='-273161' />
 <nd ref='-273162' />
 <nd ref='-273164' />
 <nd ref='-273166' />
 <nd ref='-273168' />
 <nd ref='-273170' />
 <nd ref='-273172' />
 <nd ref='-273174' />
 <nd ref='-273161' />
 </way>
 <relation id='-273177' action='modify' visible='true'>
 <member type='way' ref='-273163' role='outer' />
 <member type='node' ref='-273196' role='' />
 <tag k='name' v='Whitehorse' />
 <tag k='place' v='town' />
 <tag k='type' v='multipolygon' />
 </relation>
 </osm>

 */

/* begin binary data: relation.o5m*/
uint8_t relation_o5m_data[] = /* 224 */
{0xFF,0xE0,0x04,0x6F,0x35,0x6D,0x32,0xFF,0x10,0x2B,0xD7,0xAC,0x21,0x00,0xAD
  ,0xCF,0xFC,0x87,0x0A,0xA6,0xCE,0x88,0xC3,0x04,0x00,0x6E,0x61,0x6D,0x65,0x00
  ,0x57,0x68,0x69,0x74,0x65,0x68,0x6F,0x72,0x73,0x65,0x00,0x00,0x70,0x6C,0x61
  ,0x63,0x65,0x00,0x74,0x6F,0x77,0x6E,0x00,0x10,0x07,0x2C,0x00,0x9A,0x90,0x24
  ,0xD9,0x68,0x10,0x08,0x04,0x00,0xDE,0xCD,0x1E,0xF7,0xF6,0x15,0x10,0x08,0x04
  ,0x00,0x91,0xD3,0x2A,0xFB,0xCC,0x11,0x10,0x08,0x04,0x00,0xE7,0x95,0x60,0xFE
  ,0x9A,0x02,0x10,0x08,0x04,0x00,0x97,0xDE,0x13,0xC0,0xA7,0x0F,0x10,0x08,0x04
  ,0x00,0xB6,0x9E,0x14,0xD2,0xC3,0x19,0x10,0x08,0x04,0x00,0xA6,0x8F,0x22,0xE0
  ,0xDD,0x16,0x10,0x07,0x02,0x00,0xA6,0xF6,0x19,0x9F,0x49,0xFF,0x11,0x10,0x95
  ,0xAC,0x21,0x00,0x0B,0x91,0xAC,0x21,0x01,0x03,0x03,0x03,0x03,0x03,0x03,0x1A
  ,0xFF,0x12,0x46,0xB1,0xAC,0x21,0x00,0x11,0x95,0xAC,0x21,0x00,0x31,0x6F,0x75
  ,0x74,0x65,0x72,0x00,0xD7,0xAC,0x21,0x00,0x30,0x00,0x00,0x6E,0x61,0x6D,0x65
  ,0x00,0x57,0x68,0x69,0x74,0x65,0x68,0x6F,0x72,0x73,0x65,0x00,0x00,0x70,0x6C
  ,0x61,0x63,0x65,0x00,0x74,0x6F,0x77,0x6E,0x00,0x00,0x74,0x79,0x70,0x65,0x00
  ,0x6D,0x75,0x6C,0x74,0x69,0x70,0x6F,0x6C,0x79,0x67,0x6F,0x6E,0x00,0xFE};
/* end binary data. size = 224 bytes */


UNIT_TEST(OSM_O5M_Source_Node_read_test)
{
  std::string data(std::begin(node2_o5m_data), std::end(node2_o5m_data));
  std::stringstream ss(data);

  osm::O5MSourceReader dataset([&ss](uint8_t * buffer, size_t size)
  {
    return ss.read((char *)buffer, size).gcount();
  });

  osm::O5MSourceReader::iterator it = dataset.begin();
  osm::O5MSourceReader::Entity const & em = *it;

  CHECK_EQUAL(em.id, 513709898, ());
  CHECK_EQUAL(em.user, string("Xmypblu"), ());
  CHECK_EQUAL(em.uid, 395071, ());
  CHECK_EQUAL(em.version, 8, ());
  CHECK_EQUAL(em.changeset, 12059128, ());
  CHECK_EQUAL(em.lon, 387666704, ());
  CHECK_EQUAL(em.lat, 550927062, ());

  auto const & tags = em.tags();
  auto tag_iterator = tags.begin();
  auto const & tag = *tag_iterator;
  CHECK_EQUAL(tag.key, string("amenity"), ());
  CHECK_EQUAL(tag.value, string("cinema"), ());
  tag_iterator++;
  CHECK_EQUAL(tag.key, string("name"), ());
  CHECK_EQUAL(tag.value, string("КТ Горизонт"), ());
  tag_iterator++;
  CHECK(!(tag_iterator != tags.end()), ());
}

UNIT_TEST(OSM_O5M_Source_Way_read_test)
{
  std::string data(std::begin(way_o5m_data), std::end(way_o5m_data));
  std::stringstream ss(data);

  osm::O5MSourceReader dataset([&ss](uint8_t * buffer, size_t size)
  {
   return ss.read((char *)buffer, size).gcount();
  });

  set<int64_t> nodes;

  vector<pair<string, string>> valid_tags = {
    {"name", "Yukon River"},
    {"name:ru", "Юкон"},
    {"waterway", "river"}
  };

  for (auto const & em : dataset)
  {
    switch (em.type) {
      case osm::O5MSourceReader::O5M_CMD_NODE:
      {
        nodes.insert(em.id);
        for (auto const & tag : em.tags())
        {
          CHECK(false, ("Unexpected tag:", tag.key , tag.value));
        }
        break;
      }
      case osm::O5MSourceReader::O5M_CMD_WAY:
      {
        size_t nd_count = 0;
        size_t tag_count = 0;
        for (auto const &nd : em.nodes())
        {
          nd_count++;
          CHECK(nodes.count(nd), ());
        }
        CHECK_EQUAL(nodes.size(), nd_count, ());
        for (auto const & tag : em.tags())
        {
          CHECK_EQUAL(tag.key, valid_tags[tag_count].first, ());
          CHECK_EQUAL(tag.value, valid_tags[tag_count].second, ());
          tag_count++;
        }
        CHECK_EQUAL(valid_tags.size(), tag_count, ());
        break;
      }
      default:
        break;
    }
  }
}

UNIT_TEST(OSM_O5M_Source_Relation_read_test)
{
  std::string data(std::begin(relation_o5m_data), std::end(relation_o5m_data));
  std::stringstream ss(data);

  osm::O5MSourceReader dataset([&ss](uint8_t * buffer, size_t size)
  {
   return ss.read((char *)buffer, size).gcount();
  });

  set<int64_t> nodes;
  set<int64_t> entities;

  vector<pair<string, string>> node_valid_tags = {
    {"name", "Whitehorse"},
    {"place", "town"}
  };

  vector<pair<string, string>> relation_valid_tags = {
    {"name", "Whitehorse"},
    {"place", "town"},
    {"type", "multipolygon"}
  };

  vector<pair<osm::O5MSourceReader::ECommand, string>> relation_members = {
    {osm::O5MSourceReader::O5M_CMD_WAY, "outer"},
    {osm::O5MSourceReader::O5M_CMD_NODE, ""}
  };

  for (auto const & em : dataset)
  {
    entities.insert(em.id);

    switch (em.type) {
      case osm::O5MSourceReader::O5M_CMD_NODE:
      {
        nodes.insert(em.id);
        size_t tag_count = 0;
        for (auto const & tag : em.tags())
        {
          CHECK_EQUAL(tag.key, node_valid_tags[tag_count].first, ());
          CHECK_EQUAL(tag.value, node_valid_tags[tag_count].second, ());
          tag_count++;
        }
        break;
      }
      case osm::O5MSourceReader::O5M_CMD_WAY:
      {
        size_t nd_count = 0;
        for (auto const &nd : em.nodes())
        {
          nd_count++;
          CHECK(nodes.count(nd), ());
        }
        CHECK_EQUAL(nodes.size(), nd_count, ());
        for (auto const & tag : em.tags())
        {
          CHECK(false, ("Unexpected tag:", tag.key , tag.value));
        }
        break;
      }
      case osm::O5MSourceReader::O5M_CMD_REL:
      {
        size_t member_count = 0;
        size_t tag_count = 0;
        for (auto const &member : em.members())
        {
          CHECK(entities.count(member.ref), ());
          CHECK_EQUAL(relation_members[member_count].first, member.type, ("Current member:", member_count));
          CHECK_EQUAL(relation_members[member_count].second, member.role, ("Current member:", member_count));
          member_count++;
        }
        CHECK_EQUAL(member_count, 2, ());
        for (auto const & tag : em.tags())
        {
          CHECK_EQUAL(tag.key, relation_valid_tags[tag_count].first, ());
          CHECK_EQUAL(tag.value, relation_valid_tags[tag_count].second, ());
          tag_count++;
        }
        CHECK_EQUAL(relation_valid_tags.size(), tag_count, ());
        break;
      }
      default:
        break;
    }
  }
}
