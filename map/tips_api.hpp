#pragma once

#include "storage/storage_defines.hpp"

#include "indexer/isolines_info.hpp"

#include "metrics/eye_info.hpp"

#include "geometry/point2d.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

class TipsApi
{
public:
  using Duration = std::chrono::hours;
  using Condition = std::function<bool(eye::Info const & info)>;
  using Conditions = std::array<Condition, static_cast<size_t>(eye::Tip::Type::Count)>;

  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual std::optional<m2::PointD> GetCurrentPosition() const = 0;
    virtual bool IsCountryLoaded(m2::PointD const & pt) const = 0;
    virtual bool HaveTransit(m2::PointD const & pt) const = 0;
    virtual double GetLastBackgroundTime() const = 0;
    virtual m2::PointD const & GetViewportCenter() const = 0;
    virtual storage::CountryId GetCountryId(m2::PointD const & pt) const = 0;
    virtual isolines::Quality GetIsolinesQuality(storage::CountryId const & countryId) const = 0;
  };

  static Duration GetShowAnyTipPeriod();
  static Duration GetShowSameTipPeriod();
  static Duration ShowTipAfterCollapsingPeriod();
  static size_t GetActionClicksCountToDisable();
  static size_t GetGotitClicksCountToDisable();

  explicit TipsApi(std::unique_ptr<Delegate> delegate);

  std::optional<eye::Tip::Type> GetTip() const;

  void SetEnabled(bool isEnabled);

  static std::optional<eye::Tip::Type> GetTipForTesting(Duration showAnyTipPeriod,
                                                        Duration showSameTipPeriod,
                                                        TipsApi::Delegate const & delegate,
                                                        Conditions const & triggers);

private:
  std::unique_ptr<Delegate> m_delegate;
  Conditions m_conditions;
  bool m_isEnabled = true;
};
