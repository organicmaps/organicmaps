#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace power_management
{
// Note: the order is important.
// Note: new facilities must be added before Facility::Count.
// Note: do not use Facility::Count in external code, this value for internal use only.
enum class Facility : uint8_t
{
  Buildings3d,
  PerspectiveView,
  TrackRecording,
  TrafficJams,
  GpsTrackingForTraffic,
  OsmEditsUploading,
  UgcUploading,
  BookmarkCloudUploading,
  MapDownloader,

  Count
};

// Note: the order is important.
enum class Scheme : uint8_t
{
  None,
  Normal,
  EconomyMedium,
  EconomyMaximum,
  Auto,
};

enum class AutoScheme : uint8_t
{
  Normal,
  EconomyMedium,
  EconomyMaximum,
};

using FacilitiesState = std::array<bool, static_cast<size_t>(Facility::Count)>;

FacilitiesState const & GetFacilitiesState(Scheme const scheme);
FacilitiesState const & GetFacilitiesState(AutoScheme const autoScheme);

std::string DebugPrint(Facility const facility);
std::string DebugPrint(Scheme const scheme);
}  // namespace power_management
