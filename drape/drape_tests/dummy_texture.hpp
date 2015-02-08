#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"

class DummyTexture : public dp::Texture
{
public:
  virtual dp::RefPointer<ResourceInfo> FindResource(Key const & /*key*/, bool & /*newResource*/)
  {
    return dp::RefPointer<ResourceInfo>();
  }
};
