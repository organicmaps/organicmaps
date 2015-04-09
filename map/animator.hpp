#pragma once

#include "std/shared_ptr.hpp"

#include "geometry/point2d.hpp"
#include "geometry/any_rect2d.hpp"
#include "anim/task.hpp"

class Framework;
class RotateScreenTask;
class ChangeViewportTask;
class MoveScreenTask;
/// Class, which is responsible for
/// tracking all map animations.
class Animator
{
private:

  Framework * m_framework;

  shared_ptr<RotateScreenTask> m_rotateScreenTask;
  shared_ptr<ChangeViewportTask> m_changeViewportTask;
  shared_ptr<MoveScreenTask> m_moveScreenTask;

  void StopAnimation(shared_ptr<anim::Task> const & task);

public:

  /// constructor by Framework
  Animator(Framework * framework);
  /// rotate screen by shortest path.
  void RotateScreen(double startAngle,
                    double endAngle);
  /// stopping screen rotation
  void StopRotation();
  /// move screen from one point to another
  shared_ptr<ChangeViewportTask> const & ChangeViewport(m2::AnyRectD const & start,
                                         m2::AnyRectD const & endPt,
                                         double rotationSpeed);
  /// stop screen moving
  void StopChangeViewport();
  /// get screen rotation speed
  double GetRotationSpeed() const;
  /// move screen from one point to another
  shared_ptr<MoveScreenTask> const & MoveScreen(m2::PointD const & startPt,
                                                m2::PointD const & endPt,
                                                double speed);
  /// stopping screen movement
  void StopMoveScreen();
};
