#pragma once

#include "storage/diff_scheme/diffs_data_source.hpp"
#include "storage/storage_defines.hpp"

#include "platform/country_defines.hpp"
#include "platform/country_file.hpp"
#include "platform/downloader_defines.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <string>

namespace storage
{
class CountryInfoGetter;
class Storage;

/// \returns true if |position| is covered by a downloaded mwms and false otherwise.
/// \note |position| has coordinates in mercator.
/// \note This method takes into acount only maps enumerated in countries.json.
bool IsPointCoveredByDownloadedMaps(m2::PointD const & position, Storage const & storage,
                                    CountryInfoGetter const & countryInfoGetter);

bool IsDownloadFailed(Status status);

bool IsEnoughSpaceForDownload(MwmSize mwmSize);
bool IsEnoughSpaceForDownload(CountryId const & countryId, Storage const & storage);
bool IsEnoughSpaceForUpdate(CountryId const & countryId, Storage const & storage);

// Performs blocking I/O. Callers are responsible for running it off the GUI thread.
// A file that does not match |expectedHash| is deleted before failure is returned.
downloader::DownloadStatus ValidateDownloadedFile(std::string const & path, std::string const & expectedHash);

/// \brief Calculates limit rect for |countryId| (expandable or not).
/// \returns bounding box in mercator coordinates.
m2::RectD CalcLimitRect(CountryId const & countryId, Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter);

MwmSize GetRemoteSize(diffs::DiffsDataSource const & diffsDataSource, platform::CountryFile const & file);
}  // namespace storage
