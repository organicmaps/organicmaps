#pragma once

#include "metrics/eye_info.hpp"

#include <array>
#include <chrono>
#include <functional>
#include <memory>

#include <boost/optional.hpp>

class TipsApi
{
public:
  using Tip = boost::optional<eye::Tips::Type>;
  using Duration = std::chrono::duration<uint64_t>;
  using Condition = std::function<bool()>;
  using Conditions = std::array<Condition, static_cast<size_t>(eye::Tips::Type::Count)>;

  class Delegate
  {
  public:
    virtual boost::optional<m2::PointD> GetCurrentPosition() const = 0;
    virtual bool IsCountryLoaded(m2::PointD const & pt) const = 0;
    virtual bool HaveTransit(m2::PointD const & pt) const = 0;
  };

  static Duration GetShowAnyTipPeriod();
  static Duration GetShowSameTipPeriod();
  static size_t GetActionClicksCountToDisable();
  static size_t GetGotitClicksCountToDisable();

  TipsApi();

  void SetDelegate(std::unique_ptr<Delegate> delegate);
  Tip GetTip() const;

  static Tip GetTipForTesting(Duration showAnyTipPeriod, Duration showSameTipPeriod,
                              Conditions const & triggers);

private:
  std::unique_ptr<Delegate> m_delegate;
  Conditions m_conditions;
};
