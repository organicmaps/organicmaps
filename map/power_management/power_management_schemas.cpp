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
       /* UgcUploading */ true, /* BookmarkCloudUploading */ true, /* LocalAdsDataDownloading */ true,
       /* MapDownloader */ true, /* StatisticsUploading */ true, /* AdsDownloading */ true
     }}
  },
  {
    Scheme::EconomyMedium,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ false, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ true, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ false, /* LocalAdsDataDownloading */ true,
       /* MapDownloader */ true, /* StatisticsUploading */ true, /* AdsDownloading */ true
     }}
  },
  {
    Scheme::EconomyMaximum,
    {{
       /* Buildings3d */ false, /* PerspectiveView */ false, /* TrackRecording */ false,
       /* TrafficJams */ false, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ false,
       /* UgcUploading */ false, /* BookmarkCloudUploading */ false, /* LocalAdsDataDownloading */ true,
       /* MapDownloader */ true, /* StatisticsUploading */ true, /* AdsDownloading */ true
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
       /* UgcUploading */ true, /* BookmarkCloudUploading */ true, /* LocalAdsDataDownloading */ true,
       /* MapDownloader */ true, /* StatisticsUploading */ true, /* AdsDownloading */ true
     }}
  },
  {
    AutoScheme::EconomyMedium,
    {{
       /* Buildings3d */ true, /* PerspectiveView */ false, /* TrackRecording */ true,
       /* TrafficJams */ true, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ true,
       /* UgcUploading */ true, /* BookmarkCloudUploading */ false, /* LocalAdsDataDownloading */ true,
       /* MapDownloader */ false, /* StatisticsUploading */ false, /* AdsDownloading */ true
     }}
  },
  {
    AutoScheme::EconomyMaximum,
    {{
       /* Buildings3d */ false, /* PerspectiveView */ false, /* TrackRecording */ false,
       /* TrafficJams */ false, /* GpsTrackingForTraffic */ false, /* OsmEditsUploading */ false,
       /* UgcUploading */ false, /* BookmarkCloudUploading */ false, /* LocalAdsDataDownloading */ false,
       /* MapDownloader */ false, /* StatisticsUploading */ false, /* AdsDownloading */ false
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

std::string DebugPrint(Facility const facility)
{
  switch (facility)
  {
  case Facility::Buildings3d: return "Buildings3d";
  case Facility::PerspectiveView: return "PerspectiveView";
  case Facility::TrackRecording: return "TrackRecording";
  case Facility::TrafficJams: return "TrafficJams";
  case Facility::GpsTrackingForTraffic: return "GpsTrackingForTraffic";
  case Facility::OsmEditsUploading: return "OsmEditsUploading";
  case Facility::UgcUploading: return "UgcUploading";
  case Facility::BookmarkCloudUploading: return "BookmarkCloudUploading";
  case Facility::LocalAdsDataDownloading: return "LocalAdsDataDownloading";
  case Facility::MapDownloader: return "MapDownloader";
  case Facility::StatisticsUploading: return "StatisticsUploading";
  case Facility::AdsDownloading: return "AdsDownloading";
  case Facility::Count: return "Count";
  }
  UNREACHABLE();
}

std::string DebugPrint(Scheme const scheme)
{
  switch (scheme)
  {
  case Scheme::None: return "None";
  case Scheme::Normal: return "Normal";
  case Scheme::EconomyMedium: return "EconomyMedium";
  case Scheme::EconomyMaximum: return "EconomyMaximum";
  case Scheme::Auto: return "Auto";
  }
  UNREACHABLE();
}
}  // namespace power_management
