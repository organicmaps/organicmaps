#include "storage/storage_tests/test_map_files_downloader.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

namespace storage
{
void TestMapFilesDownloader::GetServersList(TServersListCallback const & callback)
{
  vector<string> urls = {"http://localhost:34568/unit_tests/"};
  callback(urls);
}
}  // namespace storage
