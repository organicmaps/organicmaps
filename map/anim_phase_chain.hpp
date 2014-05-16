#pragma once

#include "../anim/task.hpp"

#include "../std/shared_ptr.hpp"

class Framework;

struct AnimPhase
{
  AnimPhase(double endScale, double timeInterval);

  double m_endScale;
  double m_timeInterval;
};

class AnimPhaseChain : public anim::Task
{
public:
  AnimPhaseChain(Framework & f, double & scale);

  void AddAnimPhase(AnimPhase const & phase);

  virtual void OnStart(double ts);
  virtual void OnStep(double ts);

private:
  Framework & m_f;
  vector<AnimPhase> m_animPhases;
  size_t m_phaseIndex;
  double & m_scale;
  double m_startTime;
  double m_startScale;
};

void InitDefaultPinAnim(AnimPhaseChain * chain);
shared_ptr<anim::Task> CreateDefaultPinAnim(Framework & f, double & scale);
