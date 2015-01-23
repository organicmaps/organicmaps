#include "map/anim_phase_chain.hpp"

#include "map/framework.hpp"

AnimPhase::AnimPhase(double endScale, double timeInterval)
  : m_endScale(endScale)
  , m_timeInterval(timeInterval)
{
}

AnimPhaseChain::AnimPhaseChain(Framework & f, double & scale)
  : m_f(f)
  , m_scale(scale)
{
}

void AnimPhaseChain::AddAnimPhase(AnimPhase const & phase)
{
  m_animPhases.push_back(phase);
}

void AnimPhaseChain::OnStart(double ts)
{
  m_startTime = ts;
  m_startScale = m_scale;
  m_phaseIndex = 0;
}

void AnimPhaseChain::OnStep(double ts)
{
  ASSERT(m_phaseIndex < m_animPhases.size(), ());

  AnimPhase const * phase = &m_animPhases[m_phaseIndex];
  double elapsedTime = ts - m_startTime;
  if (elapsedTime > phase->m_timeInterval)
  {
    m_startTime = ts;
    m_scale = phase->m_endScale;
    m_startScale = m_scale;
    m_phaseIndex++;
    if (m_phaseIndex >= m_animPhases.size())
    {
      End();
      return;
    }
  }

  elapsedTime = ts - m_startTime;
  double t = elapsedTime / phase->m_timeInterval;
  m_scale = m_startScale + t * (phase->m_endScale - m_startScale);

  ///@TODO UVR
  //m_f.Invalidate();
}

void InitDefaultPinAnim(AnimPhaseChain * chain)
{
  chain->AddAnimPhase(AnimPhase(1.2, 0.15));
  chain->AddAnimPhase(AnimPhase(0.8, 0.08));
  chain->AddAnimPhase(AnimPhase(1, 0.05));
}

shared_ptr<anim::Task> CreateDefaultPinAnim(Framework & f, double & scale)
{
  shared_ptr<AnimPhaseChain> anim = make_shared<AnimPhaseChain>(f, scale);
  InitDefaultPinAnim(anim.get());
  return anim;
}

