#pragma once

#include "drape_frontend/render_state_extension.hpp"

#include "drape/color.hpp"
#include "drape/drape_global.hpp"
#include "drape/stipple_pen_resource.hpp"

#include "indexer/feature_decl.hpp"
#include "geometry/point2d.hpp"

#include <cstdint>
#include <limits>
#include <string>

namespace df
{
double const kShapeCoordScalar = 1000;
int constexpr kBuildingOutlineSize = 16;
uint32_t constexpr kStartUserMarkOverlayIndex = 1000;

struct CommonViewParams
{
  DepthLayer m_depthLayer = DepthLayer::GeometryLayer;
  float m_depth = 0.0f;
  bool m_depthTestEnabled = true;
  int m_minVisibleScale = 0;
  uint8_t m_rank = 0;
  m2::PointD m_tileCenter;
};

enum class SpecialDisplacement : uint8_t
{
  None,
  SpecialMode,
  UserMark,
  SpecialModeUserMark,
  HouseNumber,
};

struct CommonOverlayViewParams : public CommonViewParams
{
  SpecialDisplacement m_specialDisplacement = SpecialDisplacement::None;
  uint16_t m_specialPriority = std::numeric_limits<uint16_t>::max();
  int m_startOverlayRank = 0;
};

struct PoiSymbolViewParams : CommonOverlayViewParams
{
  PoiSymbolViewParams(FeatureID const & id) : m_id(id) {}

  FeatureID m_id;
  std::string m_symbolName;
  uint32_t m_extendingSize = 0;
  float m_posZ = 0.0f;
  bool m_hasArea = false;
  bool m_prioritized = false;
  bool m_obsoleteInEditor = false;
  dp::Anchor m_anchor = dp::Center;
  m2::PointF m_offset = m2::PointF(0.0f, 0.0f);
};

struct AreaViewParams : CommonViewParams
{
  dp::Color m_color;
  dp::Color m_outlineColor = dp::Color::Transparent();
  float m_minPosZ = 0.0f;
  float m_posZ = 0.0f;
  bool m_is3D = false;
  bool m_hatching = false;
  float m_baseGtoPScale = 1.0f;
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

struct TextViewParams : CommonOverlayViewParams
{
  TextViewParams() {}

  FeatureID m_featureID;
  dp::TitleDecl m_titleDecl;
  bool m_hasArea = false;
  bool m_createdByEditor = false;
  uint32_t m_extendingSize = 0;
  float m_posZ = 0.0f;
  bool m_limitedText = false;
  m2::PointF m_limits = m2::PointF(0.0f, 0.0f);
};

struct PathTextViewParams : CommonOverlayViewParams
{
  FeatureID m_featureID;
  dp::FontDecl m_textFont;
  std::string m_mainText;
  std::string m_auxText;
  float m_baseGtoPScale = 1.0f;
};

struct PathSymbolViewParams : CommonViewParams
{
  FeatureID m_featureID;
  std::string m_symbolName;
  float m_offset = 0.0f;
  float m_step = 0.0f;
  float m_baseGtoPScale = 1.0f;
};

struct ColoredSymbolViewParams : CommonOverlayViewParams
{
  enum class Shape
  {
    Rectangle, Circle, RoundedRectangle
  };

  FeatureID m_featureID;
  Shape m_shape = Shape::Circle;
  dp::Anchor m_anchor = dp::Center;
  dp::Color m_color;
  dp::Color m_outlineColor;
  float m_radiusInPixels = 0.0f;
  m2::PointF m_sizeInPixels = m2::PointF(0.0f, 0.0f);
  float m_outlineWidth = 0.0f;
  m2::PointF m_offset = m2::PointF(0.0f, 0.0f);
};
}  // namespace df
