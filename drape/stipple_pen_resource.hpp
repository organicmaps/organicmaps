#pragma once

#include "drape_global.hpp"
#include "pointers.hpp"

#include "../base/buffer_vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/map.hpp"

namespace dp
{

class StipplePenPacker
{
public:
  StipplePenPacker(m2::PointU const & canvasSize);

  m2::RectU PackResource(uint32_t width);
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;

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
  StipplePenKey(buffer_vector<uint8_t, 8> const & pattern);
  StipplePenKey(StipplePenInfo const & info);

  bool operator == (StipplePenKey const & other) const { return m_keyValue == other.m_keyValue; }
  bool operator < (StipplePenKey const & other) const { return m_keyValue < other.m_keyValue; }

private:
  void Init(buffer_vector<uint8_t, 8> const & pattern);

private:
  friend string DebugPrint(StipplePenKey const & );
  uint64_t m_keyValue;
};

class StipplePenResource
{
public:
  StipplePenResource() : m_pixelLength(0) {}
  StipplePenResource(StipplePenInfo const & key);

  uint32_t GetSize() const;
  uint32_t GetBufferSize() const;

  void Rasterize(void * buffer);

private:
  StipplePenInfo m_key;
  uint32_t m_pixelLength;
};

class Texture;
class StipplePenIndex
{
public:
  StipplePenIndex(m2::PointU const & canvasSize) : m_packer(canvasSize) {}
  m2::RectF const & MapResource(StipplePenInfo const &info);
  void UploadResources(RefPointer<Texture> texture);

private:
  typedef map<StipplePenKey, m2::RectF> TResourceMapping;
  typedef pair<m2::RectU, StipplePenResource> TPendingNode;
  typedef buffer_vector<TPendingNode, 32> TPendingNodes;

  TResourceMapping m_resourceMapping;
  TPendingNodes m_pendingNodes;
  StipplePenPacker m_packer;
};

string DebugPrint(StipplePenKey const & key);

}
