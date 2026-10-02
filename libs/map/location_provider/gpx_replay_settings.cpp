#include "map/location_provider/gpx_replay_settings.hpp"

#ifdef DEBUG

#include "map/location_provider/gpx_replay_provider.hpp"

#include "base/logging.hpp"

#include <string>

namespace location_provider
{
namespace
{
std::string FileNameFromPath(std::string const & path)
{
  auto const slash = path.find_last_of("/\\");
  if (slash == std::string::npos)
    return path;
  return path.substr(slash + 1);
}
}  // namespace

GpxReplayFileContribution & GpxReplayFileContribution::Instance()
{
  static GpxReplayFileContribution instance;
  return instance;
}

std::string GpxReplayFileContribution::GetDetail() const
{
  auto const & path = GpxReplayProvider::Instance().GetGpxPath();
  if (path.empty())
    return "Not selected";
  return FileNameFromPath(path);
}

void GpxReplayFileContribution::OnFilePicked(std::string const & path)
{
  GpxReplayProvider::Instance().SetGpxPath(path);
  LOG(LINFO, ("Mock GPX file selected:", path));
}

GpxReplayArmContribution & GpxReplayArmContribution::Instance()
{
  static GpxReplayArmContribution instance;
  return instance;
}

std::string GpxReplayArmContribution::GetTitle() const
{
  return GpxReplayProvider::Instance().IsArmed() ? "Stop mock GPX" : "Arm mock GPX";
}

std::string GpxReplayArmContribution::GetDetail() const
{
  auto const & path = GpxReplayProvider::Instance().GetGpxPath();
  if (GpxReplayProvider::Instance().IsArmed())
    return path.empty() ? "Armed" : ("Armed: " + FileNameFromPath(path));
  if (path.empty())
    return "Select a GPX file first";
  return "Ready: " + FileNameFromPath(path);
}

void GpxReplayArmContribution::OnSelected()
{
  auto & provider = GpxReplayProvider::Instance();
  if (provider.IsArmed())
  {
    provider.Disarm();
    LOG(LINFO, ("Mock GPX disarmed via Settings"));
    return;
  }

  std::string error;
  if (!provider.TryArmFromStoredPath(error))
  {
    LOG(LWARNING, (error));
    return;
  }
  LOG(LINFO, ("Mock GPX armed via Settings:", provider.GetGpxPath()));
}
}  // namespace location_provider

#endif  // DEBUG
