#include "coding/mvt_reader.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/bits.hpp"

#include <algorithm>

namespace mvt
{
namespace
{
using Source = ReaderSource<MemReaderWithExceptions>;

uint8_t constexpr kWireVarint = 0;
uint8_t constexpr kWireFixed64 = 1;
uint8_t constexpr kWireLen = 2;
uint8_t constexpr kWireFixed32 = 5;

// Geometry command ids.
uint8_t constexpr kMoveTo = 1;
uint8_t constexpr kLineTo = 2;
uint8_t constexpr kClosePath = 7;

bool ReadTag(Source & src, uint64_t & fieldNumber, uint8_t & wireType)
{
  if (src.Size() == 0)
    return false;
  uint64_t const tag = ReadVarUint<uint64_t>(src);
  fieldNumber = tag >> 3;
  wireType = static_cast<uint8_t>(tag & 7);
  return true;
}

bool SkipFieldContent(Source & src, uint8_t wireType)
{
  switch (wireType)
  {
  case kWireVarint: UNUSED_VALUE(ReadVarUint<uint64_t>(src)); return true;
  // ReaderSource::Skip() is not bounds-checked, so verify the size explicitly.
  case kWireFixed64:
    if (src.Size() < 8)
      return false;
    src.Skip(8);
    return true;
  case kWireFixed32:
    if (src.Size() < 4)
      return false;
    src.Skip(4);
    return true;
  case kWireLen:
  {
    uint64_t const len = ReadVarUint<uint64_t>(src);
    if (len > src.Size())
      return false;
    src.Skip(len);
    return true;
  }
  default: return false;
  }
}

std::string ReadString(Source & src)
{
  uint64_t const len = ReadVarUint<uint64_t>(src);
  if (len > src.Size())
    return {};
  std::string s(len, '\0');
  if (len > 0)
    src.Read(&s[0], static_cast<size_t>(len));
  return s;
}

Value ParseValue(Source & src)
{
  Value v;
  while (src.Size() > 0)
  {
    uint64_t field;
    uint8_t wire;
    if (!ReadTag(src, field, wire))
      break;
    switch (field)
    {
    case 1:
      if (wire != kWireLen)
        return {};
      v.m_type = Value::Type::String;
      v.m_string = ReadString(src);
      return v;
    case 2:
    {
      if (wire != kWireFixed32)
        return {};
      float f;
      src.Read(&f, sizeof(f));  // Fixed32 wire type holds a little-endian float.
      v.m_double = f;
      v.m_type = Value::Type::Double;
      return v;
    }
    case 3:
      if (wire != kWireFixed64)
        return {};
      src.Read(&v.m_double, sizeof(v.m_double));
      v.m_type = Value::Type::Double;
      return v;
    case 4:
    case 6:
    case 7:
    {
      if (wire != kWireVarint)
        return {};
      uint64_t const raw = ReadVarUint<uint64_t>(src);
      if (field == 7)
      {
        v.m_bool = raw != 0;
        v.m_type = Value::Type::Bool;
      }
      else
      {
        v.m_double = field == 6 ? static_cast<double>(bits::ZigZagDecode(raw)) : static_cast<double>(raw);
        v.m_type = Value::Type::Double;
      }
      return v;
    }
    case 5:
      if (wire != kWireVarint)
        return {};
      v.m_double = static_cast<double>(ReadVarUint<uint64_t>(src));
      v.m_type = Value::Type::Double;
      return v;
    default:
      if (!SkipFieldContent(src, wire))
        return {};
      break;
    }
  }
  return v;
}

bool ParseGeometry(Source & src, Feature & f)
{
  // Unsigned so that wrapping on absurd deltas from malformed tiles is well-defined.
  uint64_t x = 0;
  uint64_t y = 0;
  std::vector<m2::PointD> current;
  while (src.Size() > 0)
  {
    uint64_t const header = ReadVarUint<uint64_t>(src);
    uint64_t const cmd = header & 7u;
    uint64_t const count = header >> 3;
    for (uint64_t i = 0; i < count; ++i)
    {
      switch (cmd)
      {
      case kMoveTo:
      {
        if (!current.empty())
        {
          f.m_lines.push_back(std::move(current));
          current.clear();
        }
        x += bits::ZigZagDecode(ReadVarUint<uint64_t>(src));
        y += bits::ZigZagDecode(ReadVarUint<uint64_t>(src));
        current.emplace_back(static_cast<double>(x), static_cast<double>(y));
        break;
      }
      case kLineTo:
      {
        if (current.empty())
          return false;
        x += bits::ZigZagDecode(ReadVarUint<uint64_t>(src));
        y += bits::ZigZagDecode(ReadVarUint<uint64_t>(src));
        current.emplace_back(static_cast<double>(x), static_cast<double>(y));
        break;
      }
      case kClosePath:
        // Polygon ring boundary: linestring consumers treat the part as ended.
        if (!current.empty())
        {
          f.m_lines.push_back(std::move(current));
          current.clear();
        }
        break;
      default: return false;
      }
    }
  }
  if (!current.empty())
    f.m_lines.push_back(std::move(current));
  return !f.m_lines.empty();
}

bool ParseFeature(Source & src, Layer & layer, Feature & f)
{
  while (src.Size() > 0)
  {
    uint64_t field;
    uint8_t wire;
    if (!ReadTag(src, field, wire))
      return false;
    switch (field)
    {
    case 1:
      if (wire != kWireVarint || !SkipFieldContent(src, wire))
        return false;
      break;
    case 2:
    {
      if (wire != kWireLen)
        return false;
      uint64_t const len = ReadVarUint<uint64_t>(src);
      if (len > src.Size())
        return false;
      Source tagsSrc(src.SubReader(len));
      while (tagsSrc.Size() > 0)
      {
        auto const keyIdx = ReadVarUint<uint32_t>(tagsSrc);
        auto const valIdx = ReadVarUint<uint32_t>(tagsSrc);
        f.m_tags.emplace_back(keyIdx, valIdx);
      }
      break;
    }
    case 3:
    {
      if (wire != kWireVarint)
        return false;
      auto const t = ReadVarUint<uint64_t>(src);
      if (t > static_cast<uint64_t>(GeomType::Polygon))
        return false;
      f.m_geomType = static_cast<GeomType>(t);
      break;
    }
    case 4:
    {
      if (wire != kWireLen)
        return false;
      uint64_t const len = ReadVarUint<uint64_t>(src);
      if (len > src.Size())
        return false;
      Source geomSrc(src.SubReader(len));
      return ParseGeometry(geomSrc, f);
    }
    default:
      if (!SkipFieldContent(src, wire))
        return false;
      break;
    }
  }
  return false;
}

bool ParseLayer(Source & src, Layer & layer)
{
  bool hasName = false;
  while (src.Size() > 0)
  {
    uint64_t field;
    uint8_t wire;
    if (!ReadTag(src, field, wire))
      return false;
    switch (field)
    {
    case 1:
      if (wire != kWireLen)
        return false;
      layer.m_name = ReadString(src);
      hasName = true;
      break;
    case 2:
    {
      if (wire != kWireLen)
        return false;
      uint64_t const len = ReadVarUint<uint64_t>(src);
      if (len > src.Size())
        return false;
      Feature f;
      Source featureSrc(src.SubReader(len));
      if (!ParseFeature(featureSrc, layer, f))
        return false;
      layer.m_features.push_back(std::move(f));
      break;
    }
    case 3:
      if (wire != kWireLen)
        return false;
      layer.m_keys.push_back(ReadString(src));
      break;
    case 4:
    {
      if (wire != kWireLen)
        return false;
      uint64_t const len = ReadVarUint<uint64_t>(src);
      if (len > src.Size())
        return false;
      Source valueSrc(src.SubReader(len));
      layer.m_values.push_back(ParseValue(valueSrc));
      break;
    }
    case 5:
      if (wire != kWireVarint)
        return false;
      layer.m_extent = static_cast<uint32_t>(ReadVarUint<uint64_t>(src));
      break;
    case 15:
      if (wire != kWireVarint || !SkipFieldContent(src, wire))
        return false;
      break;
    default:
      if (!SkipFieldContent(src, wire))
        return false;
      break;
    }
  }
  return hasName && layer.m_extent > 0;
}
}  // namespace

Value const * Layer::GetTag(Feature const & feature, std::string const & key) const
{
  auto const keyIt = std::find(m_keys.begin(), m_keys.end(), key);
  if (keyIt == m_keys.end())
    return nullptr;
  uint32_t const keyIdx = static_cast<uint32_t>(std::distance(m_keys.begin(), keyIt));
  for (auto const & tag : feature.m_tags)
    if (tag.first == keyIdx && tag.second < m_values.size())
      return &m_values[tag.second];
  return nullptr;
}

bool Decode(std::string const & data, std::vector<Layer> & layers)
{
  layers.clear();
  MemReaderWithExceptions reader(data.data(), data.size());
  Source src(reader);
  try
  {
    while (src.Size() > 0)
    {
      uint64_t field;
      uint8_t wire;
      if (!ReadTag(src, field, wire))
        return false;
      if (field != 3 || wire != kWireLen)
        return false;
      uint64_t const len = ReadVarUint<uint64_t>(src);
      if (len > src.Size())
        return false;
      Layer layer;
      Source layerSrc(src.SubReader(len));
      if (!ParseLayer(layerSrc, layer))
        return false;
      layers.push_back(std::move(layer));
    }
  }
  catch (Reader::Exception const &)
  {
    // Truncated or malformed tile.
    layers.clear();
    return false;
  }
  return true;
}
}  // namespace mvt
