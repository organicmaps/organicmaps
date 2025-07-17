#pragma once

#include "base/assert.hpp"

#include <cstdint>

namespace dp
{
enum class TextureFormat : uint8_t
{
  RGBA8,
  Red,
  RedGreen,
  DepthStencil,
  Depth,
  Unspecified
};

inline std::string DebugPrint(TextureFormat tf)
{
  switch (tf)
  {
  case TextureFormat::RGBA8: return "RGBA8";
  case TextureFormat::Red: return "Red";
  case TextureFormat::RedGreen: return "RedGreen";
  case TextureFormat::DepthStencil: return "DepthStencil";
  case TextureFormat::Depth: return "Depth";
  case TextureFormat::Unspecified: return "Unspecified";
  }

  UNREACHABLE();
  return {};
}

enum class TextureFilter : uint8_t
{
  Nearest,
  Linear
};

enum class TextureWrapping : uint8_t
{
  ClampToEdge,
  Repeat
};

inline uint8_t GetBytesPerPixel(TextureFormat format)
{
  uint8_t result = 0;
  switch (format)
  {
  case TextureFormat::RGBA8: result = 4; break;
  case TextureFormat::Red: result = 1; break;
  case TextureFormat::RedGreen: result = 2; break;
  case TextureFormat::DepthStencil: result = 4; break;
  case TextureFormat::Depth: result = 4; break;
  default: ASSERT(false, ()); break;
  }
  return result;
}
}  // namespace dp
