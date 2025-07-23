#include "scoped_mwm.hpp"

#include "defines.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_impl.hpp"

#include "platform/mwm_version.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/timer.hpp"

namespace platform
{
namespace tests_support
{
ScopedMwm::ScopedMwm(std::string const & relativePath) : m_file(relativePath, ScopedFile::Mode::Create)
{
  FilesContainerW container(m_file.GetFullPath());

  // Each writer must be in it's own scope to avoid conflicts on the final write.
  {
    auto w = container.GetWriter(VERSION_FILE_TAG);
    version::WriteVersion(*w, base::SecondsSinceEpoch());
  }

  using namespace feature;
  DataHeader header;
  header.SetScales(feature::g_arrCountryScales);
  header.SetType(DataHeader::MapType::Country);

  {
    auto w = container.GetWriter(HEADER_FILE_TAG);
    header.Save(*w);
  }
}
}  // namespace tests_support
}  // namespace platform
