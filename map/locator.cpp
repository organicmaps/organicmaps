#include "../base/SRC_FIRST.hpp"
#include "locator.hpp"

Locator::Locator() : m_isRunning(false)
{}

Locator::~Locator()
{}

void Locator::setMode(EMode mode)
{
  m_mode = mode;
}

Locator::EMode Locator::mode() const
{
  return m_mode;
}

void Locator::start(Locator::EMode /*mode*/)
{
  m_isRunning = true;
}

void Locator::stop()
{
  m_isRunning = false;
}

bool Locator::isRunning() const
{
  return m_isRunning;
}

void Locator::callOnChangeModeFns(EMode oldMode, EMode newMode)
{
  list<onChangeModeFn> handlers = m_onChangeModeFns;
  for (list<onChangeModeFn>::iterator it = handlers.begin(); it != handlers.end(); ++it)
    (*it)(oldMode, newMode);
}

void Locator::callOnUpdateLocationFns(m2::PointD const & pt, double errorRadius, double locTimeStamp, double curTimeStamp)
{
  list<onUpdateLocationFn> handlers = m_onUpdateLocationFns;
  for (list<onUpdateLocationFn>::iterator it = handlers.begin(); it != handlers.end(); ++it)
    (*it)(pt, errorRadius, locTimeStamp, curTimeStamp);
}

void Locator::callOnUpdateHeadingFns(double trueHeading, double magneticHeading, double accuracy)
{
  list<onUpdateHeadingFn> handlers = m_onUpdateHeadingFns;
  for (list<onUpdateHeadingFn>::iterator it = handlers.begin(); it != handlers.end(); ++it)
    (*it)(trueHeading, magneticHeading, accuracy);
}
