#pragma once

#include <memory>

namespace storage
{
class MapFilesDownloader;

std::unique_ptr<MapFilesDownloader> GetDownloader();
}  // namespace storage
