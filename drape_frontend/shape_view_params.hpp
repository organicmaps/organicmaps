#pragma once

#include "drape/drape_global.hpp"
#include "drape/color.hpp"
#include "drape/stipple_pen_resource.hpp"

#include "indexer/feature_decl.hpp"
#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace df
{

double const kShapeCoordScalar = 1000;
int constexpr kBuildingOutlineSize = 16;

struct CommonViewParams
{
  float m_depth = 0.0f;
  int m_minVisibleScale = 0;
  uint8_t m_rank = 0;
  m2::PointD m_tileCenter;
};

struct PoiSymbolViewParams : CommonViewParams
{
  PoiSymbolViewParams(FeatureID const & id) : m_id(id) {}

  FeatureID m_id;
  string m_symbolName;
  uint32_t m_extendingSize;
  float m_posZ = 0.0f;
  bool m_hasArea = false;
  bool m_createdByEditor = false;
  bool m_obsoleteInEditor = false;
};

struct AreaViewParams : CommonViewParams
{
  dp::Color m_color;
  dp::Color m_outlineColor = dp::Color::Transparent();
  float m_minPosZ = 0.0f;
  float m_posZ = 0.0f;
  bool m_is3D = false;
};

struct LineViewParams : CommonViewParams
{
  dp::Color m_color;
  float m_width = 0.0f;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
  buffer_vector<uint8_t, 8> m_pattern;
  float m_baseGtoPScale = 1.0f;
  int m_zoomLevel = -1;
};

struct TextViewParams : CommonViewParams
{
  TextViewParams() {}

  FeatureID m_featureID;
  dp::FontDecl m_primaryTextFont;
  string m_primaryText;
  dp::FontDecl m_secondaryTextFont;
  string m_secondaryText;
  dp::Anchor m_anchor;
  m2::PointF m_primaryOffset = m2::PointF(0.0f, 0.0f);
  m2::PointF m_secondaryOffset = m2::PointF(0.0f, 0.0f);
  bool m_primaryOptional = false;
  bool m_secondaryOptional = false;
  bool m_hasArea = false;
  bool m_createdByEditor = false;
  uint32_t m_extendingSize = 0;
  float m_posZ = 0.0f;
  bool m_limitedText = false;
  m2::PointF m_limits = m2::PointF(0.0f, 0.0f);
};

struct PathTextViewParams : CommonViewParams
{
  FeatureID m_featureID;
  dp::FontDecl m_textFont;
  string m_text;
  float m_baseGtoPScale = 1.0f;
};

struct PathSymbolViewParams : CommonViewParams
{
  FeatureID m_featureID;
  string m_symbolName;
  float m_offset = 0.0f;
  float m_step = 0.0f;
  float m_baseGtoPScale = 1.0f;
};

struct ColoredSymbolViewParams : CommonViewParams
{
  enum class Shape
  {
    Rectangle, Circle, RoundedRectangle
  };

  FeatureID m_featureID;
  Shape m_shape = Shape::Circle;
  dp::Color m_color;
  dp::Color m_outlineColor;
  float m_radiusInPixels = 0.0f;
  m2::PointF m_sizeInPixels = m2::PointF(0.0f, 0.0f);
  float m_outlineWidth = 0.0f;
};

} // namespace df
