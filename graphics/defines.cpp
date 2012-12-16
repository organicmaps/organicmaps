#include "defines.hpp"
#include "../base/macros.hpp"
#include "../base/logging.hpp"
#include "../std/string.hpp"
// for strcmp
#include "../std/memcpy.hpp"

namespace graphics
{
  namespace {
   struct Data
   {
     int m_val;
     char const * m_name;
   };
  }

  Data s_semantics[] = {
    {ESemPosition, "Position"},
    {ESemNormal, "Normal"},
    {ESemTexCoord0, "TexCoord0"},
    {ESemSampler0, "Sampler0"},
    {ESemModelView, "ModelView"},
    {ESemProjection, "Projection"}
  };

  void convert(char const * name, ESemantic & sem)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_semantics); ++i)
    {
      if (strcmp(name, s_semantics[i].m_name) == 0)
      {
        sem = (ESemantic)s_semantics[i].m_val;
        return;
      }
    }

    sem = (ESemantic)0;
    LOG(LERROR, ("Unknown Semantics=", name, "specified!"));
  }

  Data s_storages [] = {
    {ETinyStorage, "TinyStorage"},
    {ESmallStorage, "SmallStorage"},
    {EMediumStorage, "MediumStorage"},
    {ELargeStorage, "LargeStorage"},
    {EInvalidStorage, "InvalidStorage"}
  };

  char const * convert(EStorageType type)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_storages); ++i)
    {
      if (s_storages[i].m_val == type)
        return s_storages[i].m_name;
    }

    LOG(LERROR, ("Unknown StorageType=", type, "specified!"));
    return "UnknownStorage";
  }

  Data s_textures [] = {
    {ESmallTexture, "SmallTexture"},
    {EMediumTexture, "MediumTexture"},
    {ELargeTexture, "LargeTexture"},
    {ERenderTargetTexture, "RenderTargetTexture"},
    {EStaticTexture, "StaticTexture"},
    {EInvalidTexture, "InvalidTexture"}
  };

  char const * convert(ETextureType type)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_textures); ++i)
    {
      if (s_textures[i].m_val == type)
        return s_textures[i].m_name;
    }

    LOG(LERROR, ("Unknown TextureType=", type, "specified!"));
    return "UnknownTexture";
  }
}
