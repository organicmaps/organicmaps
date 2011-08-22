#include "unpack_mwm.hpp"
#include "../coding/file_container.hpp"
#include "../coding/file_writer.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

void UnpackMwm(string const & filePath)
{
  LOG(LINFO, ("Unpacking mwm sections..."));
  FilesContainerR container(filePath);
  vector<string> tags;
  container.ForEachTag(MakeBackInsertFunctor<vector<string> >(tags));
  for (size_t i = 0; i < tags.size(); ++i)
  {
    LOG(LINFO, ("Unpacking", tags[i]));
    FilesContainerR::ReaderT reader = container.GetReader(tags[i]);
    FileWriter writer(filePath + "." + tags[i]);

    uint64_t const size = reader.Size();
    uint64_t pos = 0;
    while (pos < size)
    {
      vector<char> buffer(min(static_cast<size_t>(size - pos), static_cast<size_t>(1024 * 1024)));
      reader.Read(pos, &buffer[0], buffer.size());
      writer.Write(&buffer[0], buffer.size());
      pos += buffer.size();
    }
  }
  LOG(LINFO, ("Unpacking done."));
}
