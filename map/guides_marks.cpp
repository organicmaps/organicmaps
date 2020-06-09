#include "map/guides_marks.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "base/string_utils.hpp"

namespace
{
std::string const kCityMarkTextColor = "GuideCityMarkText";
std::string const kOutdoorMarkTextColor = "GuideOutdoorMarkText";
std::string const kSelectionMarkColor = "Selection";

float constexpr kGuideMarkSize = 26.0f;
float constexpr kGuideMarkTextSize = 14.0f;
float constexpr kGuideMarkRadius = 4.0f;
float constexpr kGuideSelectionWidth = 10.0f;
m2::PointD const kGuideMarkOffset = {0.0, 0.0};
m2::PointD const kGuideDownloadedMarkOffset = {1.5, -1.5};

int constexpr kMinGuideMarkerZoom = 1;
}  // namespace

GuideMark::GuideMark(m2::PointD const & ptOrg)
    : UserMark(ptOrg, UserMark::GUIDE)
{
  Update();
}

void GuideMark::Update()
{
  if (m_type == Type::City)
  {
    m_symbolInfo[kMinGuideMarkerZoom] = m_isDownloaded ? "guide_city_downloaded"
                                                       : "guide_city";
  }
  else
  {
    m_symbolInfo[kMinGuideMarkerZoom] = m_isDownloaded ? "guide_outdoor_downloaded"
                                                       : "guide_outdoor";
  }
}

m2::PointD GuideMark::GetPixelOffset() const
{
  auto const vs = static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  return m_isDownloaded ? kGuideDownloadedMarkOffset * vs : kGuideMarkOffset * vs;
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

void GuideMark::SetDepth(float depth)
{
  SetDirty();
  m_depth = depth;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> GuideMark::GetSymbolNames() const
{
  return make_unique_dp<SymbolNameZoomInfo>(m_symbolInfo);
}

GuidesClusterMark::GuidesClusterMark(m2::PointD const & ptOrg)
    : UserMark(ptOrg, UserMark::GUIDE_CLUSTER)
{
  Update();
}

void GuidesClusterMark::SetDepth(float depth)
{
  SetDirty();
  m_depth = depth;
}

void GuidesClusterMark::Update()
{
  auto const isCity = m_outdoorGuidesCount < m_cityGuidesCount;
  auto const totalCount = m_outdoorGuidesCount + m_cityGuidesCount;
  auto const bigCluster = totalCount > 99;
  if (isCity)
  {
    m_symbolInfo[kMinGuideMarkerZoom] = bigCluster ? "guide_city_cluster_plus"
                                                   : "guide_city_cluster";
  }
  else
  {
    m_symbolInfo[kMinGuideMarkerZoom] = bigCluster ? "guide_outdoor_cluster_plus"
                                                   : "guide_outdoor_cluster";
  }

  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(isCity ? kCityMarkTextColor
                                                                      : kOutdoorMarkTextColor);
  m_titleDecl.m_primaryTextFont.m_size = kGuideMarkTextSize;
  m_titleDecl.m_anchor = dp::Center;
  m_titleDecl.m_primaryText = bigCluster ? "99+" : strings::to_string(totalCount);
}

void GuidesClusterMark::SetGuidesCount(uint32_t cityGuidesCount, uint32_t outdoorGuidesCount)
{
  SetDirty();
  m_cityGuidesCount = cityGuidesCount;
  m_outdoorGuidesCount = outdoorGuidesCount;
  Update();
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> GuidesClusterMark::GetSymbolNames() const
{
  return make_unique_dp<SymbolNameZoomInfo>(m_symbolInfo);
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
