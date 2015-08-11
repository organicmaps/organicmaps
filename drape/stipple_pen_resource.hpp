#pragma once

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/dynamic_texture.hpp"

#include "base/buffer_vector.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "std/map.hpp"
#include "std/mutex.hpp"

namespace dp
{

class StipplePenKey : public Texture::Key
{
public:
  StipplePenKey() = default;
  StipplePenKey(buffer_vector<uint8_t, 8> const & pattern) : m_pattern(pattern) {}
  virtual Texture::ResourceType GetType() const { return Texture::StipplePen; }

  buffer_vector<uint8_t, 8> m_pattern;
};

class StipplePenHandle
{
public:
  StipplePenHandle(uint64_t value) : m_keyValue(value) {} // don't use this ctor. Only for tests
  StipplePenHandle(buffer_vector<uint8_t, 8> const & pattern);
  StipplePenHandle(StipplePenKey const & info);

  bool operator == (StipplePenHandle const & other) const { return m_keyValue == other.m_keyValue; }
  bool operator < (StipplePenHandle const & other) const { return m_keyValue < other.m_keyValue; }

private:
  void Init(buffer_vector<uint8_t, 8> const & pattern);

private:
  friend string DebugPrint(StipplePenHandle const & );
  uint64_t m_keyValue;
};

class StipplePenRasterizator
{
public:
  StipplePenRasterizator() : m_pixelLength(0), m_patternLength(0) {}
  StipplePenRasterizator(StipplePenKey const & key);

  uint32_t GetSize() const;
  uint32_t GetPatternSize() const;
  uint32_t GetBufferSize() const;

  void Rasterize(void * buffer);

private:
  StipplePenKey m_key;
  uint32_t m_pixelLength;
  uint32_t m_patternLength;
};

class StipplePenResourceInfo : public Texture::ResourceInfo
{
public:
  StipplePenResourceInfo(m2::RectF const & texRect, uint32_t pixelLength, uint32_t patternLength)
    : Texture::ResourceInfo(texRect)
    , m_pixelLength(pixelLength)
    , m_patternLength(patternLength)
  {
  }

  virtual Texture::ResourceType GetType() const { return Texture::StipplePen; }
  uint32_t GetMaskPixelLength() const { return m_pixelLength; }
  uint32_t GetPatternPixelLength() const { return m_patternLength; }

private:
  uint32_t m_pixelLength;
  uint32_t m_patternLength;
};

class StipplePenPacker
{
public:
  StipplePenPacker(m2::PointU const & canvasSize);

  m2::RectU PackResource(uint32_t width);
  m2::RectF MapTextureCoords(m2::RectU const & pixelRect) const;

private:
  m2::PointU m_canvasSize;
  uint32_t m_currentRow;
};

class StipplePenIndex
{
public:
  StipplePenIndex(m2::PointU const & canvasSize) : m_packer(canvasSize) {}
  ref_ptr<Texture::ResourceInfo> ReserveResource(bool predefined, StipplePenKey const & key, bool & newResource);
  ref_ptr<Texture::ResourceInfo> MapResource(StipplePenKey const & key, bool & newResource);
  void UploadResources(ref_ptr<Texture> texture);
  glConst GetMinFilter() const;
  glConst GetMagFilter() const;

private:
  typedef map<StipplePenHandle, StipplePenResourceInfo> TResourceMapping;
  typedef pair<m2::RectU, StipplePenRasterizator> TPendingNode;
  typedef buffer_vector<TPendingNode, 32> TPendingNodes;

  TResourceMapping m_predefinedResourceMapping;
  TResourceMapping m_resourceMapping;
  TPendingNodes m_pendingNodes;
  StipplePenPacker m_packer;

  mutex m_lock;
  mutex m_mappingLock;
};

string DebugPrint(StipplePenHandle const & key);

class StipplePenTexture : public DynamicTexture<StipplePenIndex, StipplePenKey, Texture::StipplePen>
{
  typedef DynamicTexture<StipplePenIndex, StipplePenKey, Texture::StipplePen> TBase;
public:
  StipplePenTexture(m2::PointU const & size)
    : m_index(size)
  {
    TBase::TextureParams params{ size, TextureFormat::ALPHA, gl_const::GLNearest, gl_const::GLNearest };
    TBase::Init(make_ref(&m_index), params);
  }

  ~StipplePenTexture() { TBase::Reset(); }

  void ReservePattern(buffer_vector<uint8_t, 8> const & pattern);

private:
  StipplePenIndex m_index;
};

}
