#pragma once

#include <QtCore/QObject>

#include "../std/function.hpp"

class QPaintDevice;
class TestMainLoop : public QObject
{
  Q_OBJECT

public:

  typedef function<void (QPaintDevice *)> TRednerFn;
  TestMainLoop(TRednerFn const & fn);
  virtual ~TestMainLoop() {}

  void exec(char const * testName);

protected:
  bool eventFilter(QObject * obj, QEvent * event);

private:
  TRednerFn m_renderFn;
};
