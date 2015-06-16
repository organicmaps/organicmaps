#include "show_hide_animation.hpp"

#include "base_interpolator.hpp"

#include "base/math.hpp"

namespace df
{

class ShowHideAnimation::ShowHideInterpolator : public BaseInterpolator
{
public:
  ShowHideInterpolator(ShowHideAnimation::EState & state, double startT, double endT, double duration)
    : BaseInterpolator(duration)
    , m_state(state)
    , m_startT(startT)
    , m_endT(endT)
  {
    m_state = m_endT > m_startT ? ShowHideAnimation::STATE_SHOW_DIRECTION : ShowHideAnimation::STATE_HIDE_DIRECTION;
  }

  void Advance(double elapsedSeconds) override
  {
    BaseInterpolator::Advance(elapsedSeconds);
    if (IsFinished())
      m_state = m_endT > m_startT ? ShowHideAnimation::STATE_VISIBLE : ShowHideAnimation::STATE_INVISIBLE;
  }

  double GetCurrentT() const
  {
    return m_startT + (m_endT - m_startT) * GetT();
  }

private:
  ShowHideAnimation::EState & m_state;
  double m_startT;
  double m_endT;
};

ShowHideAnimation::ShowHideAnimation(bool isInitialiVisible, double fullDuraction)
  : m_state(isInitialiVisible ? STATE_VISIBLE : STATE_INVISIBLE)
  , m_fullDuration(fullDuraction)
{
}

ShowHideAnimation::~ShowHideAnimation()
{
  m_interpolator.reset();
}

void ShowHideAnimation::Show()
{
  EState state = GetState();
  if (state == STATE_INVISIBLE || state == STATE_HIDE_DIRECTION)
  {
    m_state = STATE_VISIBLE;
    m_interpolator.reset();
  }
}

void ShowHideAnimation::ShowAnimated()
{
  RefreshInterpolator({ STATE_VISIBLE, STATE_SHOW_DIRECTION }, 1.0);
}

void ShowHideAnimation::Hide()
{
  EState state = GetState();
  if (state == STATE_VISIBLE || state == STATE_SHOW_DIRECTION)
  {
    m_state = STATE_INVISIBLE;
    m_interpolator.reset();
  }
}

void ShowHideAnimation::HideAnimated()
{
  RefreshInterpolator({ STATE_INVISIBLE, STATE_HIDE_DIRECTION }, 0.0);
}

ShowHideAnimation::EState ShowHideAnimation::GetState() const
{
  return m_state;
}

double ShowHideAnimation::GetT() const
{
  if (m_interpolator)
    return m_interpolator->GetCurrentT();

  ASSERT(m_state != STATE_SHOW_DIRECTION, ());
  ASSERT(m_state != STATE_HIDE_DIRECTION, ());

  return m_state == STATE_VISIBLE ? 1.0 : 0.0;
}

bool ShowHideAnimation::IsFinished() const
{
  return m_interpolator == nullptr || m_interpolator->IsFinished();
}

void ShowHideAnimation::RefreshInterpolator(array<EState, 2> validStates, double endValue)
{
  EState state = GetState();
  if (state == validStates[0] || state == validStates[1])
    return;

  double start = GetT();
  double end = endValue;
  double duration = fabs(end - start) * m_fullDuration;
  m_interpolator.reset(new ShowHideInterpolator(m_state, start, end, duration));
}

} // namespace df
