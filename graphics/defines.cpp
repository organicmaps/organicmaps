#include "defines.hpp"
#include "../coding/file_name_utils.hpp"
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

  Data s_density[] = {
    {EDensityLDPI, "ldpi"},
    {EDensityMDPI, "mdpi"},
    {EDensityHDPI, "hdpi"},
    {EDensityXHDPI, "xhdpi"},
    {EDensityXXHDPI, "xxhdpi"} // using xhdpi resources for xxdpi screens
  };

  char const * convert(EDensity density)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_density); ++i)
    {
      if (density == s_density[i].m_val)
        return s_density[i].m_name;
    }
    return 0;
  }

  string const resourcePath(string const & name, EDensity d)
  {
      return my::JoinFoldersToPath(string("resources-") + convert(d), name);
  }

  double visualScale(EDensity density)
  {
    static double const vs [5] = {0.75, 1, 1.5, 2, 3};
    return vs[density];
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

  namespace {
    struct DataType
    {
      EDataType m_dt;
      unsigned m_elemSize;
    };
  }

  DataType s_dataTypes [] = {
    {EInteger, sizeof(int)},
    {EIntegerVec2, sizeof(int) * 2},
    {EIntegerVec3, sizeof(int) * 3},
    {EIntegerVec4, sizeof(int) * 4},
    {EFloat, sizeof(float)},
    {EFloatVec2, sizeof(float) * 2},
    {EFloatVec3, sizeof(float) * 3},
    {EFloatVec4, sizeof(float) * 4},
    {EFloatMat2, sizeof(float) * 4},
    {EFloatMat3, sizeof(float) * 9},
    {EFloatMat4, sizeof(float) * 16},
    {ESampler2D, sizeof(int)}
  };

  unsigned elemSize(EDataType dt)
  {
    for (unsigned i = 0; i < ARRAY_SIZE(s_dataTypes); ++i)
      if (s_dataTypes[i].m_dt == dt)
        return s_dataTypes[i].m_elemSize;

    return 0;
  }

}
