#pragma once

#include "drape/drape_global.hpp"
#include "drape/color.hpp"
#include "drape/stipple_pen_resource.hpp"

#include "indexer/feature_decl.hpp"
#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace df
{

struct CommonViewParams
{
  float m_depth = 0.0f;
  int m_minVisibleScale = 0;
  uint8_t m_rank = 0;
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

struct CircleViewParams : CommonViewParams
{
  CircleViewParams(FeatureID const & id) : m_id(id) {}

  FeatureID m_id;
  dp::Color m_color;
  float m_radius = 0.0f;
  bool m_hasArea = false;
  bool m_createdByEditor = false;
};

struct AreaViewParams : CommonViewParams
{
  dp::Color m_color;
  float m_minPosZ = 0.0f;
  float m_posZ = 0.0f;
};

struct LineViewParams : CommonViewParams
{
  dp::Color m_color;
  float m_width = 0.0f;
  dp::LineCap m_cap;
  dp::LineJoin m_join;
  buffer_vector<uint8_t, 8> m_pattern;
  float m_baseGtoPScale = 1.0f;
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
  m2::PointF m_primaryOffset;
  bool m_primaryOptional = false;
  bool m_secondaryOptional = false;
  bool m_hasArea = false;
  bool m_createdByEditor = false;
  uint32_t m_extendingSize = 0;
  float m_posZ = 0.0f;
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

} // namespace df
