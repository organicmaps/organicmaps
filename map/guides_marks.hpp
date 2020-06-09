#pragma once

#include "map/user_mark.hpp"

class GuideMark : public UserMark
{
public:
  enum class Type
  {
    City,
    Outdoor
  };

  explicit GuideMark(m2::PointD const & ptOrg);

  void SetGuideType(Type type);
  Type GetType() const { return m_type; }

  void SetIsDownloaded(bool isDownloaded);
  bool GetIsDownloaded() const { return m_isDownloaded; }

  void SetGuideId(std::string guideId);
  std::string GetGuideId() const { return m_guideId; }

  void SetDepth(float depth);

  // df::UserPointMark overrides.
  float GetDepth() const override { return m_depth; }
  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::GuidesMarkLayer; }
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  m2::PointD GetPixelOffset() const override;

private:
  void Update();

  float m_depth = 0.0f;

  std::string m_guideId;
  Type m_type = Type::City;
  bool m_isDownloaded = false;

  SymbolNameZoomInfo m_symbolInfo;
};

class GuidesClusterMark : public UserMark
{
public:
  explicit GuidesClusterMark(m2::PointD const & ptOrg);

  void SetGuidesCount(uint32_t cityGuidesCount, uint32_t outdoorGuidesCount);

  void SetDepth(float depth);

  // df::UserPointMark overrides.
  float GetDepth() const override { return m_depth; }
  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::GuidesMarkLayer; }
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  drape_ptr<TitlesInfo> GetTitleDecl() const override;

private:
  void Update();

  float m_depth = 0.0f;

  uint32_t m_cityGuidesCount = 0;
  uint32_t m_outdoorGuidesCount = 0;

  SymbolNameZoomInfo m_symbolInfo;
  dp::TitleDecl m_titleDecl;
};

class GuideSelectionMark : public UserMark
{
public:
  explicit GuideSelectionMark(m2::PointD const & ptOrg);

  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::GuidesBottomMarkLayer; }
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override { return nullptr; }
  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override;

private:
  ColoredSymbolZoomInfo m_coloredInfo;
};
