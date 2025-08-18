#pragma once

#include "drape/pointers.hpp"
#include "drape/texture.hpp"

class DummyTexture : public dp::Texture
{
public:
  ref_ptr<ResourceInfo> FindResource(Key const & key)
  {
    bool dummy = false;
    return FindResource(key, dummy);
  }

  virtual ref_ptr<ResourceInfo> FindResource(Key const & /*key*/, bool & /*newResource*/) { return nullptr; }
};
