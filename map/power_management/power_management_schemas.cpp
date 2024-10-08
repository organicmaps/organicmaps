#include "map/power_management/power_management_schemas.hpp"

#include "base/assert.hpp"

#include <unordered_map>

using namespace power_management;

namespace
{
std::unordered_map<Scheme, FacilitiesState> const kSchemeToState =
{
  {
    Scheme::Normal,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ true, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ true, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ true, /* MapDownloader */ true,
     }}
  },
  {
    Scheme::EconomyMedium,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ false, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ true, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ false, /* MapDownloader */ true,
     }}
  },
  {
    Scheme::EconomyMaximum,
    {{
       /* Buildings3d */ false, /* PerspectiveView */ false, /* TrackRecording */ false,
       /* TrafficJams */ false, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ false,
       /* UgcUploading */ false, /* BookmarkCloudUploading */ false, /* MapDownloader */ true,
     }}
  },
};

std::unordered_map<AutoScheme, FacilitiesState> const kAutoSchemeToState =
{
  {
    AutoScheme::Normal,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ true, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ true, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ true, /* MapDownloader */ true,
     }}
  },
  {
    AutoScheme::EconomyMedium,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ false, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ false, /* MapDownloader */ false,
     }}
  },
  {
    AutoScheme::EconomyMaximum,
    {{
       /* Buildings3d */ false, /* PerspectiveView */ false, /* TrackRecording */ false,
       /* TrafficJams */ false, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ false,
       /* UgcUploading */ false, /* BookmarkCloudUploading */ false, /* MapDownloader */ false,
     }}
  },
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
}  // namespace power_management
