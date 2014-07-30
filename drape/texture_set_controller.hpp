#pragma once

namespace dp
{

class TextureSetController
{
public:
  virtual ~TextureSetController() {}
  virtual void BindTextureSet(uint32_t textureSet) const = 0;
  virtual uint32_t GetTextureCount(uint32_t textureSet) const = 0;
};

} // namespace dp
