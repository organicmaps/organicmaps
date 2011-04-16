#pragma once

#include "../std/list.hpp"
#include "../std/function.hpp"
#include "../geometry/point2d.hpp"

/// Base class for OS-dependent implementation of locator
class Locator
{
public:

  enum EMode
  {
    ERoughMode,  //< monitor only significant place changes, saves battery.
    EPreciseMode //< monitor place changes with the maximum available accuracy
  };

  typedef function<void (m2::PointD const & /*latLongPos*/, double /*errorRadius*/, double /*locationTimeStamp*/, double /*currentTimeStamp*/)> onUpdateLocationFn;
  typedef function<void (double /*trueHeading*/, double /*magneticHeading*/, double /*accuracy*/)> onUpdateHeadingFn;
  typedef function<void (EMode /*oldMode*/, EMode /*newMode*/)> onChangeModeFn;

private:

  list<onUpdateLocationFn> m_onUpdateLocationFns;
  list<onUpdateHeadingFn> m_onUpdateHeadingFns;
  list<onChangeModeFn> m_onChangeModeFns;

  EMode  m_mode;

  bool m_isRunning;

public:

  Locator();
  virtual ~Locator();

  bool isRunning() const;
  virtual void start(EMode mode) = 0;
  virtual void stop() = 0;
  virtual void setMode(EMode mode) = 0;
  EMode mode() const;

  template <typename Fn>
  void addOnUpdateLocationFn(Fn fn)
  {
    m_onUpdateLocationFns.push_back(fn);
  }

  template <typename Fn>
  void addOnUpdateHeadingFn(Fn fn)
  {
    m_onUpdateHeadingFns.push_back(fn);
  }

  template <typename Fn>
  void addOnChangeModeFn(Fn fn)
  {
    m_onChangeModeFns.push_back(fn);
  }

  void callOnUpdateLocationFns(m2::PointD const & pt, double errorRadius, double locTimeStamp, double curTimeStamp);
  void callOnUpdateHeadingFns(double trueHeading, double magneticHeading, double accuracy);
  void callOnChangeModeFns(EMode oldMode, EMode newMode);
};
