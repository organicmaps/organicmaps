#include "map/local_ads_mark.hpp"

#include "drape_frontend/color_constants.hpp"

#include <utility>

namespace
{
static std::string const kLocalAdsPrimaryText = "LocalAdsPrimaryText";
static std::string const kLocalAdsPrimaryTextOutline = "LocalAdsPrimaryTextOutline";
static std::string const kLocalAdsSecondaryText = "LocalAdsSecondaryText";
static std::string const kLocalAdsSecondaryTextOutline = "LocalAdsSecondaryTextOutline";

float const kLocalAdsPrimaryTextSize = 11.0f;
float const kLocalAdsSecondaryTextSize = 10.0f;
float const kSecondaryOffsetY = 2.0;
}  // namespace

LocalAdsMark::LocalAdsMark(m2::PointD const & ptOrg,
                           UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
  m_titleDecl.m_anchor = dp::Top;
  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant(kLocalAdsPrimaryText);
  m_titleDecl.m_primaryTextFont.m_outlineColor = df::GetColorConstant(kLocalAdsPrimaryTextOutline);
  m_titleDecl.m_primaryTextFont.m_size = kLocalAdsPrimaryTextSize;
  m_titleDecl.m_secondaryTextFont.m_color = df::GetColorConstant(kLocalAdsSecondaryText);
  m_titleDecl.m_secondaryTextFont.m_outlineColor = df::GetColorConstant(kLocalAdsSecondaryTextOutline);
  m_titleDecl.m_secondaryTextFont.m_size = kLocalAdsSecondaryTextSize;
  m_titleDecl.m_secondaryOffset = m2::PointF(0, kSecondaryOffsetY);
}

df::RenderState::DepthLayer LocalAdsMark::GetDepthLayer() const
{
  return df::RenderState::LocalAdsMarkLayer;
}

drape_ptr<dp::TitleDecl> LocalAdsMark::GetTitleDecl() const
{
  drape_ptr<dp::TitleDecl> titleDecl = make_unique_dp<dp::TitleDecl>(m_titleDecl);
  return titleDecl;
}

void LocalAdsMark::SetData(LocalAdsMarkData && data)
{
  SetDirty();
  m_data = std::move(data);

  m_titleDecl.m_primaryText = m_data.m_mainText;
  if (!m_titleDecl.m_primaryText.empty())
    m_titleDecl.m_secondaryText = m_data.m_auxText;
  else
    m_titleDecl.m_secondaryText.clear();
}

void LocalAdsMark::SetFeatureId(FeatureID const & id)
{
  SetDirty();
  m_featureId = id;
}
