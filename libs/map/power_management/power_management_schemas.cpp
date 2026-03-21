#include "map/power_management/power_management_schemas.hpp"

#include "base/assert.hpp"

#include <unordered_map>

namespace
{
std::unordered_map<power_management::Scheme, power_management::FacilitiesState> const kSchemeToState = {
    {power_management::Scheme::Normal,
     {{
         /* Buildings3d */ true,
         /* PerspectiveView */ true,
         /* TrackRecording */ true,
         /* TrafficJams */ true,
         /* GpsTrackingForTraffic */ true,
         /* OsmEditsUploading */ true,
         /* UgcUploading */ true,
         /* BookmarkCloudUploading */ true,
         /* MapDownloader */ true,
     }}},
    {power_management::Scheme::EconomyMedium,
     {{
         /* Buildings3d */ true,
         /* PerspectiveView */ false,
         /* TrackRecording */ true,
         /* TrafficJams */ true,
         /* GpsTrackingForTraffic */ true,
         /* OsmEditsUploading */ true,
         /* UgcUploading */ true,
         /* BookmarkCloudUploading */ false,
         /* MapDownloader */ true,
     }}},
    {power_management::Scheme::EconomyMaximum,
     {{
         /* Buildings3d */ false,
         /* PerspectiveView */ false,
         /* TrackRecording */ false,
         /* TrafficJams */ false,
         /* GpsTrackingForTraffic */ false,
         /* OsmEditsUploading */ false,
         /* UgcUploading */ false,
         /* BookmarkCloudUploading */ false,
         /* MapDownloader */ true,
     }}},
};

std::unordered_map<power_management::AutoScheme, power_management::FacilitiesState> const kAutoSchemeToState = {
    {power_management::AutoScheme::Normal,
     {{
         /* Buildings3d */ true,
         /* PerspectiveView */ true,
         /* TrackRecording */ true,
         /* TrafficJams */ true,
         /* GpsTrackingForTraffic */ true,
         /* OsmEditsUploading */ true,
         /* UgcUploading */ true,
         /* BookmarkCloudUploading */ true,
         /* MapDownloader */ true,
     }}},
    {power_management::AutoScheme::EconomyMedium,
     {{
         /* Buildings3d */ true,
         /* PerspectiveView */ false,
         /* TrackRecording */ true,
         /* TrafficJams */ true,
         /* GpsTrackingForTraffic */ false,
         /* OsmEditsUploading */ true,
         /* UgcUploading */ true,
         /* BookmarkCloudUploading */ false,
         /* MapDownloader */ false,
     }}},
    {power_management::AutoScheme::EconomyMaximum,
     {{
         /* Buildings3d */ false,
         /* PerspectiveView */ false,
         /* TrackRecording */ false,
         /* TrafficJams */ false,
         /* GpsTrackingForTraffic */ false,
         /* OsmEditsUploading */ false,
         /* UgcUploading */ false,
         /* BookmarkCloudUploading */ false,
         /* MapDownloader */ false,
     }}},
};
}  // namespace

namespace power_management
{
FacilitiesState const & GetFacilitiesState(Scheme const scheme)
{
  CHECK_NOT_EQUAL(scheme, Scheme::None, ());
  CHECK_NOT_EQUAL(scheme, Scheme::Auto, ());

  return kSchemeToState.at(scheme);
}

FacilitiesState const & GetFacilitiesState(AutoScheme const autoScheme)
{
  return kAutoSchemeToState.at(autoScheme);
}

std::string DebugPrint(Facility const facility)
{
  switch (facility)
  {
    using enum Facility;
  case Buildings3d: return "Buildings3d";
  case PerspectiveView: return "PerspectiveView";
  case TrackRecording: return "TrackRecording";
  case TrafficJams: return "TrafficJams";
  case GpsTrackingForTraffic: return "GpsTrackingForTraffic";
  case OsmEditsUploading: return "OsmEditsUploading";
  case UgcUploading: return "UgcUploading";
  case BookmarkCloudUploading: return "BookmarkCloudUploading";
  case MapDownloader: return "MapDownloader";
  case Count: return "Count";
  }
  UNREACHABLE();
}

std::string DebugPrint(Scheme const scheme)
{
  switch (scheme)
  {
    using enum Scheme;
  case None: return "None";
  case Normal: return "Normal";
  case EconomyMedium: return "EconomyMedium";
  case EconomyMaximum: return "EconomyMaximum";
  case Auto: return "Auto";
  }
  UNREACHABLE();
}
}  // namespace power_management
