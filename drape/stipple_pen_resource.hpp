#pragma once

#include "drape_global.hpp"

#include "../base/buffer_vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

namespace dp
{

class StipplePenPacker
{
public:
  StipplePenPacker(m2::PointU const & canvasSize);

  m2::RectU PackResource(uint32_t width);

private:
  m2::PointU m_canvasSize;
  buffer_vector<uint32_t, 4> m_columns;
  uint32_t m_currentColumn;
};

struct StipplePenInfo
{
  buffer_vector<uint8_t, 8> m_pattern;
};

struct StipplePenKey
{
  enum { Tag = StipplePenTag };

  StipplePenKey(uint64_t value) : m_keyValue(value) {} // don't use this ctor. Only for tests
  StipplePenKey(StipplePenInfo const & info);

  bool operator == (StipplePenKey const & other) const { return m_keyValue == other.m_keyValue; }
  bool operator < (StipplePenKey const & other) const { return m_keyValue < other.m_keyValue; }

private:
  friend string DebugPrint(StipplePenKey const & );
  uint64_t m_keyValue;
};

class StipplePenResource
{
public:
  StipplePenResource(StipplePenInfo const & key);

  uint32_t GetSize() const;
  uint32_t GetBufferSize() const;
  TextureFormat GetExpectedFormat() const;

  void Rasterize(void * buffer);

private:
  StipplePenInfo m_key;
  uint32_t m_pixelLength;
};

string DebugPrint(StipplePenKey const & key);

}
