#pragma once

#include "drape/pointers.hpp"

#include "std/array.hpp"

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

  ShowHideAnimation(bool isInitialiVisible, double fullDuraction);
  ~ShowHideAnimation();

  void Show();
  void ShowAnim();
  void Hide();
  void HideAnim();
  EState GetState() const;
  double GetT() const;
  bool IsFinished() const;

private:
  void RefreshInterpolator(array<EState, 2> validStates, double endValue);
  class ShowHideInterpolator;

  drape_ptr<ShowHideInterpolator> m_interpolator;
  ShowHideAnimation::EState m_state;
  double m_fullDuration;
};

} // namespace df
