// Run by the Sailfish OS CI job: the app must get the Qt provider from the shared factory,
// not the silent UnavailableLocationService fallback.
#include "platform/location_service/location_service.hpp"
#include "platform/location_service/qt_location_service.hpp"

#include <QCoreApplication>

#include <iostream>

namespace
{
struct Observer final : location::LocationObserver
{
  void OnLocationError(location::TLocationError) override {}
  void OnLocationUpdated(location::GpsInfo const &) override {}
};
}  // namespace

int main(int argc, char * argv[])
{
  QCoreApplication app(argc, argv);
  Observer observer;
  auto const service = CreateDesktopLocationService(observer);
  if (!dynamic_cast<QtLocationService *>(service.get()))
  {
    std::cerr << "CreateDesktopLocationService() didn't select QtLocationService\n";
    return 1;
  }
  std::cout << "CreateDesktopLocationService() selects QtLocationService\n";
  return 0;
}
