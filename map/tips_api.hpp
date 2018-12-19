#pragma once

#include "metrics/eye_info.hpp"

#include "geometry/point2d.hpp"

#include <array>
#include <cstdint>
#include <chrono>
#include <functional>
#include <memory>

#include <boost/optional.hpp>

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

    virtual boost::optional<m2::PointD> GetCurrentPosition() const = 0;
    virtual bool IsCountryLoaded(m2::PointD const & pt) const = 0;
    virtual bool HaveTransit(m2::PointD const & pt) const = 0;
    virtual double GetLastBackgroundTime() const = 0;
  };

  static Duration GetShowAnyTipPeriod();
  static Duration GetShowSameTipPeriod();
  static Duration ShowTipAfterCollapsingPeriod();
  static size_t GetActionClicksCountToDisable();
  static size_t GetGotitClicksCountToDisable();

  explicit TipsApi(Delegate const & delegate);

  boost::optional<eye::Tip::Type> GetTip() const;

  static boost::optional<eye::Tip::Type> GetTipForTesting(Duration showAnyTipPeriod,
                                                          Duration showSameTipPeriod,
                                                          TipsApi::Delegate const & delegate,
                                                          Conditions const & triggers);

private:
  Delegate const & m_delegate;
  Conditions m_conditions;
};
