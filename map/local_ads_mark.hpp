#pragma once

#include "map/user_mark_container.hpp"

#include <cstdint>
#include <limits>
#include <string>

struct LocalAdsMarkData
{
  m2::PointD m_position = m2::PointD::Zero();
  std::string m_symbolName;
  uint8_t m_minZoomLevel = 1;
  std::string m_mainText;
  std::string m_auxText;
  uint16_t m_priority = std::numeric_limits<uint16_t>::max();
};

class LocalAdsMark : public UserMark
{
public:
  LocalAdsMark(m2::PointD const & ptOrg, UserMarkContainer * container);
  virtual ~LocalAdsMark() {}

  df::RenderState::DepthLayer GetDepthLayer() const override;

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  UserMark::Type GetMarkType() const override { return Type::LOCAL_ADS; }

  drape_ptr<TitlesInfo> GetTitleDecl() const override;
  uint16_t GetPriority() const override { return m_data.m_priority; }
  bool HasSymbolPriority() const override { return true; }
  bool HasTitlePriority() const override { return true; }
  int GetMinZoom() const override { return static_cast<int>(m_data.m_minZoomLevel); }
  FeatureID GetFeatureID() const override { return m_featureId; }

  void SetData(LocalAdsMarkData && data);
  void SetFeatureId(FeatureID const & id);

private:
  LocalAdsMarkData m_data;
  FeatureID m_featureId;
  dp::TitleDecl m_titleDecl;
};
