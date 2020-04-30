#include "map/guides_marks.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "base/string_utils.hpp"

namespace
{
std::string const kCityMarkOutlineColor = "GuideCityMarkOutline";
std::string const kCityMarkBgColor = "GuideCityMarkBg";
std::string const kOutdoorMarkOutlineColor = "GuideOutdoorMarkOutline";
std::string const kOutdoorMarkBgColor = "GuideOutdoorMarkBg";
std::string const kSelectionMarkColor = "Selection";

float constexpr kGuideMarkSize = 26.0f;
float constexpr kGuideMarkExSize = 34.0f;
float constexpr kGuideMarkTextSize = 14.0f;
float constexpr kGuideMarkTextMargin = 1.5f;
float constexpr kGuideMarkOutlineWidth = 1.0f;
float constexpr kGuideMarkRadius = 4.0f;
float constexpr kGuideSelectionWidth = 10.0f;
m2::PointF const kCityMarkOffset = {4.0, -5.0};
m2::PointF const kOutdoorMarkOffset = {5.0, -4.0};

int constexpr kMinGuideMarkerZoom = 1;
}  // namespace

GuideMark::GuideMark(m2::PointD const & ptOrg)
    : UserMark(ptOrg, UserMark::GUIDE)
{
  Update();
}

void GuideMark::Update()
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  df::ColoredSymbolViewParams params;
  params.m_color = df::GetColorConstant(m_type == Type::City ? kCityMarkBgColor
                                                             : kOutdoorMarkBgColor);
  params.m_shape = df::ColoredSymbolViewParams::Shape::RoundedRectangle;
  params.m_radiusInPixels = kGuideMarkRadius * vs;
  params.m_sizeInPixels = m2::PointF(kGuideMarkSize, kGuideMarkSize) * vs;
  params.m_outlineColor = df::GetColorConstant(m_type == Type::City ? kCityMarkOutlineColor
                                                                    : kOutdoorMarkOutlineColor);
  params.m_outlineWidth = kGuideMarkOutlineWidth * vs;
  m_coloredBgInfo.m_zoomInfo[kMinGuideMarkerZoom] = params;

  if (m_type == Type::City)
  {
    m_symbolInfo[kMinGuideMarkerZoom] = m_isDownloaded ? "guide_city_downloaded"
                                                       : "guide_city";
    m_symbolOffsets = SymbolOffsets(scales::UPPER_STYLE_SCALE, kCityMarkOffset * vs);
  }
  else
  {
    m_symbolInfo[kMinGuideMarkerZoom] = m_isDownloaded ? "guide_outdoor_downloaded"
                                                       : "guide_outdoor";
    m_symbolOffsets = SymbolOffsets(scales::UPPER_STYLE_SCALE, kOutdoorMarkOffset * vs);
  }
}

void GuideMark::SetGuideType(Type type)
{
  SetDirty();
  m_type = type;
  Update();
}

void GuideMark::SetIsDownloaded(bool isDownloaded)
{
  SetDirty();
  m_isDownloaded = isDownloaded;
  Update();
}

void GuideMark::SetGuideId(std::string guideId)
{
  SetDirty();
  m_guideId = guideId;
}

void GuideMark::SetIndex(uint32_t index)
{
  SetDirty();
  m_index = index;
}

drape_ptr<df::UserPointMark::ColoredSymbolZoomInfo> GuideMark::GetColoredSymbols() const
{
  return make_unique_dp<ColoredSymbolZoomInfo>(m_coloredBgInfo);
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> GuideMark::GetSymbolNames() const
{
  return make_unique_dp<SymbolNameZoomInfo>(m_symbolInfo);
}

drape_ptr<df::UserPointMark::SymbolOffsets> GuideMark::GetSymbolOffsets() const
{
  if (m_isDownloaded)
    return make_unique_dp<SymbolOffsets>(m_symbolOffsets);
  return nullptr;
}

GuidesClusterMark::GuidesClusterMark(m2::PointD const & ptOrg)
    : UserMark(ptOrg, UserMark::GUIDE_CLUSTER)
{
  Update();
}

void GuidesClusterMark::SetIndex(uint32_t index)
{
  SetDirty();
  m_index = index;
}

void GuidesClusterMark::Update()
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  auto const isCity = m_outdoorGuidesCount < m_cityGuidesCount;
  auto const totalCount = m_outdoorGuidesCount + m_cityGuidesCount;

  df::ColoredSymbolViewParams params;
  params.m_color = df::GetColorConstant(isCity ? kCityMarkBgColor : kOutdoorMarkBgColor);
  params.m_shape = df::ColoredSymbolViewParams::Shape::RoundedRectangle;
  params.m_radiusInPixels = kGuideMarkRadius * vs;
  params.m_sizeInPixels = m2::PointF(totalCount > 99 ? kGuideMarkExSize : kGuideMarkSize,
                                     kGuideMarkSize) * vs;
  params.m_outlineColor = df::GetColorConstant(isCity ? kCityMarkOutlineColor
                                                      : kOutdoorMarkOutlineColor);
  params.m_outlineWidth = kGuideMarkOutlineWidth * vs;
  m_coloredBgInfo.m_zoomInfo[kMinGuideMarkerZoom] = params;

  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(isCity ? kCityMarkOutlineColor
                                                                      : kOutdoorMarkOutlineColor);
  m_titleDecl.m_primaryTextFont.m_size = kGuideMarkTextSize;
  m_titleDecl.m_anchor = dp::Center;
  m_titleDecl.m_primaryText = totalCount > 99 ? "99+" : strings::to_string(totalCount);
}

void GuidesClusterMark::SetGuidesCount(uint32_t cityGuidesCount, uint32_t outdoorGuidesCount)
{
  SetDirty();
  m_cityGuidesCount = cityGuidesCount;
  m_outdoorGuidesCount = outdoorGuidesCount;
  Update();
}

drape_ptr<df::UserPointMark::ColoredSymbolZoomInfo> GuidesClusterMark::GetColoredSymbols() const
{
  return make_unique_dp<ColoredSymbolZoomInfo>(m_coloredBgInfo);
}

drape_ptr<df::UserPointMark::TitlesInfo> GuidesClusterMark::GetTitleDecl() const
{
  auto titleInfo = make_unique_dp<TitlesInfo>();
  titleInfo->push_back(m_titleDecl);
  return titleInfo;
}

GuideSelectionMark::GuideSelectionMark(m2::PointD const & ptOrg)
    : UserMark(ptOrg, UserMark::GUIDE_SELECTION)
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());

  df::ColoredSymbolViewParams params;
  params.m_color = df::GetColorConstant(kSelectionMarkColor);
  params.m_shape = df::ColoredSymbolViewParams::Shape::RoundedRectangle;
  params.m_radiusInPixels = kGuideMarkRadius * vs;
  params.m_sizeInPixels = m2::PointF(kGuideMarkSize + kGuideSelectionWidth,
                                     kGuideMarkSize + kGuideSelectionWidth) * vs;
  m_coloredInfo.m_zoomInfo[kMinGuideMarkerZoom] = params;
  m_coloredInfo.m_needOverlay = false;
}

drape_ptr<df::UserPointMark::ColoredSymbolZoomInfo> GuideSelectionMark::GetColoredSymbols() const
{
  return make_unique_dp<ColoredSymbolZoomInfo>(m_coloredInfo);
}
