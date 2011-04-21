#include "location.hpp"

using namespace location;

class WindowsLocationService : public LocationService
{
public:
  virtual void StartUpdate(bool useAccurateMode)
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
  static WindowsLocationService ls;
  return ls;
}
