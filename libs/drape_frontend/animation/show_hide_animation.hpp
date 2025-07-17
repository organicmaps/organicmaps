#pragma once

#include "drape/pointers.hpp"

#include <array>

namespace df
{
class ShowHideAnimation
{
public:
  enum EState
  {
    STATE_INVISIBLE,
    STATE_VISIBLE,
    STATE_SHOW_DIRECTION,
    STATE_HIDE_DIRECTION
  };

  ShowHideAnimation(bool isInitialVisible, double fullDuration);
  ~ShowHideAnimation();

  void Show();
  void ShowAnimated();
  void Hide();
  void HideAnimated();
  EState GetState() const;
  double GetT() const;
  bool IsFinished() const;

private:
  void RefreshInterpolator(std::array<EState, 2> validStates, double endValue);
  class ShowHideInterpolator;

  drape_ptr<ShowHideInterpolator> m_interpolator;
  ShowHideAnimation::EState m_state;
  double m_fullDuration;
};
}  // namespace df
