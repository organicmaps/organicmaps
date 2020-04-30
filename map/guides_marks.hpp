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

  void SetIndex(uint32_t index);

  // df::UserPointMark overrides.
  uint32_t GetIndex() const override { return m_index; }
  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::GuidesMarkLayer; }
  df::SpecialDisplacement GetDisplacement() const override
  {
    return df::SpecialDisplacement::SpecialModeUserMark;
  }

  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override;
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  bool SymbolIsPOI() const override { return true; }

private:
  void Update();

  uint32_t m_index = 0;

  std::string m_guideId;
  Type m_type = Type::City;
  bool m_isDownloaded = false;

  ColoredSymbolZoomInfo m_coloredBgInfo;
  SymbolNameZoomInfo m_symbolInfo;
  SymbolOffsets m_symbolOffsets;
};

class GuidesClusterMark : public UserMark
{
public:
  explicit GuidesClusterMark(m2::PointD const & ptOrg);

  void SetGuidesCount(uint32_t cityGuidesCount, uint32_t outdoorGuidesCount);

  void SetIndex(uint32_t index);

  // df::UserPointMark overrides.
  uint32_t GetIndex() const override { return m_index; }
  df::DepthLayer GetDepthLayer() const override { return df::DepthLayer::GuidesMarkLayer; }
  df::SpecialDisplacement GetDisplacement() const override
  {
    return df::SpecialDisplacement::SpecialModeUserMark;
  }
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override { return nullptr; }

  drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const override;

  bool HasTitlePriority() const override { return true; }
  drape_ptr<TitlesInfo> GetTitleDecl() const override;

private:
  void Update();

  uint32_t m_index = 0;

  uint32_t m_cityGuidesCount = 0;
  uint32_t m_outdoorGuidesCount = 0;

  ColoredSymbolZoomInfo m_coloredBgInfo;
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
