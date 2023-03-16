#include "generator/generator_integration_tests/helpers.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"
#include "coding/zip_reader.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

namespace generator_integration_tests
{
void DecompressZipArchive(std::string const & src, std::string const & dst)
{
  auto & plaftorm = GetPlatform();
  if (!plaftorm.MkDirRecursively(dst))
    MYTHROW(MkDirFailure, (dst));

  ZipFileReader::FileList files;
  ZipFileReader::FilesList(src, files);
  for (auto const & p : files)
  {
    auto const output = base::JoinPath(dst, p.first);
    if (strings::EndsWith(output, base::GetNativeSeparator()))
    {
      if (!plaftorm.MkDirRecursively(output))
        MYTHROW(MkDirFailure, (output));
    }
    else
    {
      ZipFileReader::UnzipFile(src, p.first, output);
    }
  }
}
}  // namespace generator_integration_tests
