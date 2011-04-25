#include "location.hpp"

using namespace location;

class QtLocationService : public LocationService
{
public:
  virtual void StartUpdate(bool /*useAccurateMode*/)
  {
    // @TODO
  }

  virtual void StopUpdate()
  {
    // @TODO
  }
};

extern "C" location::LocationService & GetLocationService()
{
  static QtLocationService ls;
  return ls;
}
