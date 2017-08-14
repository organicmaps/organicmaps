#include "generator/mwm_diff/diff.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"
#include "coding/zlib.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <iterator>
#include <vector>

#include "3party/bsdiff-courgette/bsdiff/bsdiff.h"

using namespace std;

namespace
{
// Format Version 0: bsdiff+gzip.
uint32_t const kLatestVersion = 0;
}  // namespace

namespace generator
{
namespace mwm_diff
{
bool MakeDiff(string const & oldMwmPath, string const & newMwmPath, string const & diffPath)
{
  try
  {
    FileReader oldReader(oldMwmPath);
    FileReader newReader(newMwmPath);
    FileWriter diffFileWriter(diffPath);

    vector<uint8_t> diffBuf;
    MemWriter<vector<uint8_t>> diffMemWriter(diffBuf);

    auto const status = bsdiff::CreateBinaryPatch(oldReader, newReader, diffMemWriter);

    if (status != bsdiff::BSDiffStatus::OK)
    {
      LOG(LERROR, ("Could not create patch with bsdiff:", status));
      return false;
    }

    using Deflate = coding::ZLib::Deflate;
    Deflate deflate(Deflate::Format::ZLib, Deflate::Level::BestCompression);

    vector<uint8_t> deflatedDiffBuf;
    deflate(diffBuf.data(), diffBuf.size(), back_inserter(deflatedDiffBuf));

    uint32_t const header = kLatestVersion;
    WriteToSink(diffFileWriter, header);
    diffFileWriter.Write(deflatedDiffBuf.data(), deflatedDiffBuf.size());
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Could not open file when creating a patch:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Could not open file when creating a patch:", e.Msg()));
    return false;
  }

  return true;
}

bool ApplyDiff(string const & oldMwmPath, string const & newMwmPath, string const & diffPath)
{
  try
  {
    FileReader oldReader(oldMwmPath);
    FileWriter newWriter(newMwmPath);
    FileReader diffFileReader(diffPath);

    ReaderSource<FileReader> diffFileSource(diffFileReader);
    auto const header = ReadPrimitiveFromSource<uint32_t>(diffFileSource);
    if (header != kLatestVersion)
    {
      LOG(LERROR, ("Unknown version format of mwm diff:", header));
      return false;
    }

    vector<uint8_t> deflatedDiff(diffFileSource.Size());
    diffFileSource.Read(deflatedDiff.data(), deflatedDiff.size());

    using Inflate = coding::ZLib::Inflate;
    vector<uint8_t> decompressedData;
    Inflate inflate(Inflate::Format::ZLib);
    vector<uint8_t> diffBuf;
    inflate(deflatedDiff.data(), deflatedDiff.size(), back_inserter(diffBuf));

    MemReader diffMemReader(diffBuf.data(), diffBuf.size());

    auto status = bsdiff::ApplyBinaryPatch(oldReader, newWriter, diffMemReader);

    if (status != bsdiff::BSDiffStatus::OK)
    {
      LOG(LERROR, ("Could not apply patch with bsdiff:", status));
      return false;
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Could not open file when applying a patch:", e.Msg()));
    return false;
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Could not open file when applying a patch:", e.Msg()));
    return false;
  }

  return true;
}
}  // namespace mwm_diff
}  // namespace generator
