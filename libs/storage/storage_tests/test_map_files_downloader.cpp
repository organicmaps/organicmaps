#include "storage/storage_tests/test_map_files_downloader.hpp"

namespace storage
{
TestMapFilesDownloader::TestMapFilesDownloader() : HttpMapFilesDownloader()
{
  SetServersList({"http://localhost:24568/unit_tests/"});
}
}  // namespace storage
