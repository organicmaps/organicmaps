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
enum Version
{
  // Format Version 0: bsdiff+gzip.
  VERSION_V0 = 0,
  VERSION_LATEST = VERSION_V0
};

bool MakeDiffVersion0(FileReader & oldReader, FileReader & newReader, FileWriter & diffFileWriter)
{
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

  // A basic header that holds only version.
  WriteToSink(diffFileWriter, static_cast<uint32_t>(VERSION_V0));
  diffFileWriter.Write(deflatedDiffBuf.data(), deflatedDiffBuf.size());

  return true;
}

bool ApplyDiffVersion0(FileReader & oldReader, FileWriter & newWriter,
                       ReaderSource<FileReader> & diffFileSource)
{
  vector<uint8_t> deflatedDiff(diffFileSource.Size());
  diffFileSource.Read(deflatedDiff.data(), deflatedDiff.size());

  using Inflate = coding::ZLib::Inflate;
  vector<uint8_t> decompressedData;
  Inflate inflate(Inflate::Format::ZLib);
  vector<uint8_t> diffBuf;
  inflate(deflatedDiff.data(), deflatedDiff.size(), back_inserter(diffBuf));

  // Our bsdiff assumes that both the old mwm and the diff files are correct and
  // does no checks when using its readers.
  // Yet sometimes we observe corrupted files in the logs, and to avoid
  // crashes from such files the exception-throwing version of MemReader is used here.
  // |oldReader| is a FileReader so it throws exceptions too but we
  // are more confident in the uncorrupted status of the old file because
  // its checksum is compared to the one stored in the diff file.
  MemReaderWithExceptions diffMemReader(diffBuf.data(), diffBuf.size());

  auto status = bsdiff::ApplyBinaryPatch(oldReader, newWriter, diffMemReader);

  if (status != bsdiff::BSDiffStatus::OK)
  {
    LOG(LERROR, ("Could not apply patch with bsdiff:", status));
    return false;
  }

  return true;
}
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

    switch (VERSION_LATEST)
    {
    case VERSION_V0: return MakeDiffVersion0(oldReader, newReader, diffFileWriter);
    default:
      LOG(LERROR,
          ("Making mwm diffs with diff format version", VERSION_LATEST, "is not implemented"));
    }
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

  return false;
}

bool ApplyDiff(string const & oldMwmPath, string const & newMwmPath, string const & diffPath)
{
  try
  {
    FileReader oldReader(oldMwmPath);
    FileWriter newWriter(newMwmPath);
    FileReader diffFileReader(diffPath);

    ReaderSource<FileReader> diffFileSource(diffFileReader);
    auto const version = ReadPrimitiveFromSource<uint32_t>(diffFileSource);

    switch (version)
    {
    case VERSION_V0: return ApplyDiffVersion0(oldReader, newWriter, diffFileSource);
    default: LOG(LERROR, ("Unknown version format of mwm diff:", version));
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

  return false;
}
}  // namespace mwm_diff
}  // namespace generator
