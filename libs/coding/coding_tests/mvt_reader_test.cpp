#include "coding/mvt_reader.hpp"
#include "base/bits.hpp"

#include "testing/testing.hpp"

#include <string>
#include <vector>

namespace mvt_reader_test
{
using namespace mvt;

namespace
{
// Geometry command ids from the MVT spec.
uint64_t constexpr kMoveTo = 1;
uint64_t constexpr kLineTo = 2;

void WriteVarint(std::string & out, uint64_t v)
{
  while (v > 0x7F)
  {
    out.push_back(static_cast<char>((v & 0x7F) | 0x80));
    v >>= 7;
  }
  out.push_back(static_cast<char>(v));
}

// Appends a protobuf field with a length-delimited payload.
template <class Fn>
void WriteField(std::string & out, uint64_t fieldNumber, Fn const & payloadWriter)
{
  std::string payload;
  payloadWriter(payload);
  WriteVarint(out, (fieldNumber << 3) | 2 /* wire type */);
  WriteVarint(out, payload.size());
  out += payload;
}

std::string MakeTestTile()
{
  // One layer "roads", extent 4096, one two-part linestring feature with tags.
  std::string tile;
  WriteField(tile, 3 /* Layer */, [](std::string & layer)
  {
    WriteVarint(layer, (15 << 3) | 0);  // version
    WriteVarint(layer, 2);
    WriteField(layer, 1 /* name */, [](std::string & s) { s = "Traffic flow"; });
    WriteField(layer, 3 /* key */, [](std::string & s) { s = "traffic_level"; });
    WriteField(layer, 3 /* key */, [](std::string & s) { s = "road_closure"; });
    WriteField(layer, 4 /* value */, [](std::string & s)
    {
      WriteVarint(s, (3 << 3) | 1 /* double_value, fixed64 */);
      double const d = 0.25;
      for (size_t i = 0; i < sizeof(d); ++i)
        s.push_back(static_cast<char>((reinterpret_cast<uint8_t const *>(&d))[i]));
    });
    WriteField(layer, 4 /* value */, [](std::string & s)
    {
      WriteVarint(s, (7 << 3) | 0 /* bool_value */);
      WriteVarint(s, 1);
    });
    WriteVarint(layer, (5 << 3) | 0);  // extent, varint field
    WriteVarint(layer, 4096);

    WriteField(layer, 2 /* feature */, [&](std::string & f)
    {
      WriteVarint(f, (3 << 3) | 0);  // type
      WriteVarint(f, static_cast<uint64_t>(GeomType::LineString));
      WriteField(f, 2 /* tags */, [](std::string & t)
      {
        WriteVarint(t, 0);  // key: traffic_level
        WriteVarint(t, 0);  // value: 0.25
        WriteVarint(t, 1);  // key: road_closure
        WriteVarint(t, 1);  // value: true
      });
      WriteField(f, 4 /* geometry */, [](std::string & g)
      {
        // Part 1: MoveTo(100, 200), LineTo(300, 400), LineTo(500, 600).
        // Part 2: MoveTo(10, 20), LineTo(30, 40).
        WriteVarint(g, (1 << 3) | kMoveTo);
        WriteVarint(g, bits::ZigZagEncode(100));
        WriteVarint(g, bits::ZigZagEncode(200));
        WriteVarint(g, (2 << 3) | kLineTo);
        WriteVarint(g, bits::ZigZagEncode(200));
        WriteVarint(g, bits::ZigZagEncode(200));
        WriteVarint(g, bits::ZigZagEncode(200));
        WriteVarint(g, bits::ZigZagEncode(200));
        WriteVarint(g, (1 << 3) | kMoveTo);
        WriteVarint(g, bits::ZigZagEncode(-490));  // 500 -> 10
        WriteVarint(g, bits::ZigZagEncode(-580));  // 600 -> 20
        WriteVarint(g, (1 << 3) | kLineTo);
        WriteVarint(g, bits::ZigZagEncode(20));
        WriteVarint(g, bits::ZigZagEncode(20));
      });
    });
  });
  return tile;
}
}  // namespace

UNIT_TEST(MvtReader_Decode)
{
  std::vector<Layer> layers;
  TEST(Decode(MakeTestTile(), layers), ());
  TEST_EQUAL(layers.size(), 1, ());
  auto const & layer = layers[0];
  TEST_EQUAL(layer.m_name, "Traffic flow", ());
  TEST_EQUAL(layer.m_extent, 4096, ());
  TEST_EQUAL(layer.m_keys, std::vector<std::string>({"traffic_level", "road_closure"}), ());
  TEST_EQUAL(layer.m_features.size(), 1, ());

  auto const & f = layer.m_features[0];
  TEST_EQUAL(f.m_geomType, GeomType::LineString, ());
  TEST_EQUAL(f.m_lines.size(), 2, ());
  TEST_EQUAL(f.m_lines[0].size(), 3, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[0][0].x, 100.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[0][0].y, 200.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[0][2].x, 500.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[0][2].y, 600.0, 1e-9, ());
  TEST_EQUAL(f.m_lines[1].size(), 2, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[1][0].x, 10.0, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(f.m_lines[1][1].x, 30.0, 1e-9, ());

  Value const * level = layer.GetTag(f, "traffic_level");
  TEST(level != nullptr, ());
  TEST_EQUAL(level->m_type, Value::Type::Double, ());
  TEST_ALMOST_EQUAL_ABS(level->m_double, 0.25, 1e-9, ());

  Value const * closure = layer.GetTag(f, "road_closure");
  TEST(closure != nullptr && closure->m_type == Value::Type::Bool && closure->m_bool, ());

  TEST(layer.GetTag(f, "nonexistent") == nullptr, ());
}

UNIT_TEST(MvtReader_Malformed)
{
  std::vector<Layer> layers;
  TEST(!Decode("\x0a", layers), ());  // Truncated length-delimited header.
  TEST(layers.empty(), ());

  std::string truncated = MakeTestTile();
  truncated.resize(truncated.size() / 2);
  TEST(!Decode(truncated, layers), ());
}

// Every prefix of a valid tile must be handled gracefully: no crashes, no out-of-bounds
// reads on malformed network payloads. Prefixes ending at a field boundary may decode
// successfully, so only crash- and exception-freedom is asserted.
UNIT_TEST(MvtReader_TruncatedPrefixes)
{
  std::string const tile = MakeTestTile();
  std::vector<Layer> layers;
  for (size_t size = 0; size < tile.size(); ++size)
  {
    layers.clear();
    bool const ok = Decode(tile.substr(0, size), layers);
    if (!ok)
      TEST(layers.empty(), (size));
  }
}

UNIT_TEST(MvtReader_EmptyLayerList)
{
  // A message with no fields decodes to zero layers (an empty but valid tile).
  std::vector<Layer> layers;
  TEST(Decode(std::string{}, layers), ());
  TEST(layers.empty(), ());
}
}  // namespace mvt_reader_test
