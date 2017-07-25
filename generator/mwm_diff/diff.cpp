#include "generator/mwm_diff/diff.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"

using namespace std;

namespace
{
// We could use my::CopyFileX but it fails if the file at |srcFile| is empty.
bool CopyFile(string const & srcFile, string const & dstFile)
{
  size_t constexpr kBufferSize = 1024 * 1024;

  try
  {
    FileReader reader(srcFile, true /* withExceptions */);
    ReaderSource<FileReader> src(reader);
    FileWriter writer(dstFile);

    rw::ReadAndWrite(src, writer, kBufferSize);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Could not read from", srcFile, ":", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Could not write to", dstFile, ":", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace

namespace generator
{
namespace mwm_diff
{
bool MakeDiff(string const & /* oldMwmPath */, string const & newMwmPath, string const & diffPath)
{
  return CopyFile(newMwmPath, diffPath);
}

bool ApplyDiff(string const & /* oldMwmPath */, string const & newMwmPath, string const & diffPath)
{
  return CopyFile(diffPath, newMwmPath);
}
}  // namespace mwm_diff
}  // namespace generator
