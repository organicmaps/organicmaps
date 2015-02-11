#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"

class DummyTexture : public dp::Texture
{
public:
  dp::RefPointer<ResourceInfo> FindResource(Key const & key)
  {
    bool dummy = false;
    return FindResource(key, dummy);
  }

  virtual dp::RefPointer<ResourceInfo> FindResource(Key const & /*key*/, bool & /*newResource*/)
  {
    return dp::RefPointer<ResourceInfo>();
  }
};
