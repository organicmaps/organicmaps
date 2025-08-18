#include "generator/unpack_mwm.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/read_write_utils.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <vector>

namespace generator
{
void UnpackMwm(std::string const & filePath)
{
  LOG(LINFO, ("Unpacking mwm sections..."));

  FilesContainerR container(filePath);
  std::vector<std::string> tags;
  container.ForEachTag(base::MakeBackInsertFunctor<std::vector<std::string>>(tags));

  for (size_t i = 0; i < tags.size(); ++i)
  {
    LOG(LINFO, ("Unpacking", tags[i]));

    ReaderSource<FilesContainerR::TReader> reader(container.GetReader(tags[i]));
    FileWriter writer(filePath + "." + tags[i]);

    rw::ReadAndWrite(reader, writer, 1024 * 1024);
  }

  LOG(LINFO, ("Unpacking done."));
}

void DeleteSection(std::string const & filePath, std::string const & tag)
{
  FilesContainerW(filePath, FileWriter::OP_WRITE_EXISTING).DeleteSection(tag);
}
}  // namespace generator
