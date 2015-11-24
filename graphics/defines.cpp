#include "graphics/defines.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/platform.hpp"

#include "base/macros.hpp"
#include "base/logging.hpp"

#include "std/string.hpp"
#include "std/cstring.hpp"


namespace graphics
{
  namespace
  {
    // Use custom struct to leave {} initialization notation (pair doesn't support it).
    template <class FirstT, class SecondT> struct DataT
    {
      FirstT first;
      SecondT second;
    };

    typedef DataT<int, char const *> DataIS;

    template <class SecondT, size_t N>
    SecondT FindSecondByFirst(DataT<int, SecondT> (&arr)[N], int t)
    {
      for (size_t i = 0; i < N; ++i)
      {
        if (t == arr[i].first)
          return arr[i].second;
      }

      LOG(LERROR, (t));
      return SecondT();
    }

    template <class FirstT, class SecondT, size_t N, class CompareT>
    FirstT FindFirstBySecond(DataT<FirstT, SecondT> (&arr)[N],
                             SecondT t, CompareT comp)
    {
      for (size_t i = 0; i < N; ++i)
      {
        if (comp(t, arr[i].second))
          return arr[i].first;
      }

      LOG(LERROR, (t));
      return FirstT();
    }

    struct EqualStrings
    {
      bool operator() (char const * s1, char const * s2) const
      {
        return (strcmp(s1, s2) == 0);
      }
    };
  }

  DataIS s_density[] = {
    {EDensityLDPI, "ldpi"},
    {EDensityMDPI, "mdpi"},
    {EDensityHDPI, "hdpi"},
    {EDensityXHDPI, "xhdpi"},
    {EDensityXXHDPI, "xxhdpi"},
    {EDensityIPhone6Plus, "6plus"}
  };

  char const * convert(EDensity density)
  {
    return FindSecondByFirst(s_density, density);
  }

  void convert(char const * name, EDensity & density)
  {
    density = static_cast<EDensity>(FindFirstBySecond(s_density, name, EqualStrings()));
  }

  double visualScaleExact(int exactDensityDPI)
  {
    double const mdpiDensityDPI = 160.;
    double const tabletFactor = 1.28;
    // In case of tablets and iPads increased DPI is used to make visual scale bigger.
    if (GetPlatform().IsTablet())
      exactDensityDPI *= tabletFactor;

    // For some old devices (for example iPad 2) the density could be less than 160 DPI.
    // Returns one in that case to keep readable text on the map.
    if (exactDensityDPI <= mdpiDensityDPI)
      return 1.;
    return exactDensityDPI / mdpiDensityDPI;
  }

  DataIS s_semantics[] = {
    {ESemPosition, "Position"},
    {ESemNormal, "Normal"},
    {ESemTexCoord0, "TexCoord0"},
    {ESemSampler0, "Sampler0"},
    {ESemModelView, "ModelView"},
    {ESemProjection, "Projection"},
    {ETransparency, "Transparency"},
    {ESemLength, "Length"},
    {ERouteHalfWidth, "u_halfWidth"},
    {ERouteColor, "u_color"},
    {ERouteClipLength, "u_clipLength"},
    {ERouteTextureRect, "u_textureRect"}
  };

  void convert(char const * name, ESemantic & sem)
  {
    sem = static_cast<ESemantic>(FindFirstBySecond(s_semantics, name, EqualStrings()));
  }

  DataIS s_storages[] = {
    {ETinyStorage, "TinyStorage"},
    {ESmallStorage, "SmallStorage"},
    {EMediumStorage, "MediumStorage"},
    {ELargeStorage, "LargeStorage"},
    {EInvalidStorage, "InvalidStorage"}
  };

  char const * convert(EStorageType type)
  {
    return FindSecondByFirst(s_storages, type);
  }

  DataIS s_textures[] = {
    {ESmallTexture, "SmallTexture"},
    {EMediumTexture, "MediumTexture"},
    {ELargeTexture, "LargeTexture"},
    {ERenderTargetTexture, "RenderTargetTexture"},
    {EStaticTexture, "StaticTexture"},
    {EInvalidTexture, "InvalidTexture"}
  };

  char const * convert(ETextureType type)
  {
    return FindSecondByFirst(s_textures, type);
  }

  DataT<int, unsigned> s_dataTypes[] = {
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

  unsigned elemSize(EDataType type)
  {
    return FindSecondByFirst(s_dataTypes, type);
  }
}
