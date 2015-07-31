#pragma once

#include "std/string.hpp"

namespace graphics
{
  static const int maxDepth = 20000;
  static const int minDepth = -20000;

  enum EDensity
  {
    EDensityLDPI = 0,
    EDensityMDPI,
    EDensityHDPI,
    EDensityXHDPI,
    EDensityXXHDPI,
    EDensityIPhone6Plus
  };

  /// get density name
  char const * convert(EDensity density);

  /// get density from name
  void convert(char const * name, EDensity & density);

  /// get scaling koefficient for specified density
  double visualScale(EDensity density);

  /// When adding values here,
  /// please check constructor of ResourceManager,
  /// and defines.cpp
  enum ETextureType
  {
    ESmallTexture = 0,
    EMediumTexture,
    ELargeTexture,
    ERenderTargetTexture,
    EStaticTexture,
    EInvalidTexture //< Should be last
  };

  char const * convert(ETextureType type);

  /// When adding values here,
  /// please check constructor of ResourceManager.
  enum EStorageType
  {
    ETinyStorage = 0,
    ESmallStorage,
    EMediumStorage,
    ELargeStorage,
    EInvalidStorage //< Should be last
  };

  char const * convert(EStorageType type);

  enum EShaderType
  {
    EVertexShader,
    EFragmentShader
  };

  enum ESemantic
  {
    ESemPosition,
    ESemNormal,
    ESemTexCoord0,
    ESemSampler0,
    ESemModelView,
    ESemProjection,
    ETransparency,
    ESemLength,
    ERouteHalfWidth,
    ERouteColor,
    ERouteClipLength,
    ERouteTextureRect
  };

  void convert(char const * name, ESemantic & sem);

  enum EDataType
  {
    EInteger,
    EIntegerVec2,
    EIntegerVec3,
    EIntegerVec4,
    EFloat,
    EFloatVec2,
    EFloatVec3,
    EFloatVec4,
    EFloatMat2,
    EFloatMat3,
    EFloatMat4,
    ESampler2D
  };

  unsigned elemSize(EDataType dt);

  enum EPrimitives
  {
    ETriangles,
    ETrianglesFan,
    ETrianglesStrip
  };

  enum EMatrix
  {
    EModelView,
    EProjection
  };

  enum EDepthFunc
  {
    ELessEqual
  };

  enum EPosition
  {
    EPosCenter = 0x00,
    EPosAbove = 0x01,
    EPosUnder = 0x02,
    EPosLeft = 0x04,
    EPosRight = 0x10,
    EPosAboveLeft = EPosAbove | EPosLeft,
    EPosAboveRight = EPosAbove | EPosRight,
    EPosUnderLeft = EPosUnder | EPosLeft,
    EPosUnderRight = EPosUnder | EPosRight
  };
}
