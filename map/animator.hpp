#pragma once

#include "../std/shared_ptr.hpp"

class Framework;
class RotateScreenTask;

/// Class, which is responsible for
/// tracking all map animations.
class Animator
{
private:

  Framework * m_framework;

  shared_ptr<RotateScreenTask> m_rotateScreenTask;

public:

  /// constructor by Framework
  Animator(Framework * framework);
  /// rotate screen by shortest path.
  void RotateScreen(double startAngle,
                    double endAngle,
                    double duration);
  /// stopping screen rotation
  void StopRotation();

};
