#include "mwm_diff/diff.hpp"

#include "coding/buffered_file_writer.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"
#include "coding/zlib.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include <cstdint>
#include <iterator>
#include <vector>

#include "3party/bsdiff-courgette/bsdiff/bsdiff.h"

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
  std::vector<uint8_t> diffBuf;
  MemWriter<std::vector<uint8_t>> diffMemWriter(diffBuf);

  auto const status = bsdiff::CreateBinaryPatch(oldReader, newReader, diffMemWriter);

  if (status != bsdiff::BSDiffStatus::OK)
  {
    LOG(LERROR, ("Could not create patch with bsdiff:", status));
    return false;
  }

  using Deflate = coding::ZLib::Deflate;
  Deflate deflate(Deflate::Format::ZLib, Deflate::Level::BestCompression);

  std::vector<uint8_t> deflatedDiffBuf;
  deflate(diffBuf.data(), diffBuf.size(), back_inserter(deflatedDiffBuf));

  // A basic header that holds only version.
  WriteToSink(diffFileWriter, static_cast<uint32_t>(VERSION_V0));
  diffFileWriter.Write(deflatedDiffBuf.data(), deflatedDiffBuf.size());

  return true;
}

generator::mwm_diff::DiffApplicationResult ApplyDiffVersion0(FileReader & oldReader, FileWriter & newWriter,
                                                             ReaderSource<FileReader> & diffFileSource,
                                                             base::Cancellable const & cancellable)
{
  using generator::mwm_diff::DiffApplicationResult;

  std::vector<uint8_t> deflatedDiff(base::checked_cast<size_t>(diffFileSource.Size()));
  diffFileSource.Read(deflatedDiff.data(), deflatedDiff.size());

  using Inflate = coding::ZLib::Inflate;
  Inflate inflate(Inflate::Format::ZLib);
  std::vector<uint8_t> diffBuf;
  inflate(deflatedDiff.data(), deflatedDiff.size(), back_inserter(diffBuf));

  // Our bsdiff assumes that both the old mwm and the diff files are correct and
  // does no checks when using its readers.
  // Yet sometimes we observe corrupted files in the logs, and to avoid
  // crashes from such files the exception-throwing version of MemReader is used here.
  // |oldReader| is a FileReader so it throws exceptions too but we
  // are more confident in the uncorrupted status of the old file because
  // its checksum is compared to the one stored in the diff file.
  MemReaderWithExceptions diffMemReader(diffBuf.data(), diffBuf.size());

  auto const status = bsdiff::ApplyBinaryPatch(oldReader, newWriter, diffMemReader, cancellable);

  if (status == bsdiff::BSDiffStatus::CANCELLED)
  {
    LOG(LDEBUG, ("Diff application has been cancelled"));
    return DiffApplicationResult::Cancelled;
  }

  if (status == bsdiff::BSDiffStatus::OK)
    return DiffApplicationResult::Ok;

  LOG(LERROR, ("Could not apply patch with bsdiff:", status));
  return DiffApplicationResult::Failed;
}
}  // namespace

namespace generator
{
namespace mwm_diff
{
bool MakeDiff(std::string const & oldMwmPath, std::string const & newMwmPath, std::string const & diffPath)
{
  try
  {
    FileReader oldReader(oldMwmPath);
    FileReader newReader(newMwmPath);
    FileWriter diffFileWriter(diffPath);

    switch (VERSION_LATEST)
    {
    case VERSION_V0: return MakeDiffVersion0(oldReader, newReader, diffFileWriter);
    default: LOG(LERROR, ("Making mwm diffs with diff format version", VERSION_LATEST, "is not implemented"));
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

DiffApplicationResult ApplyDiff(std::string const & oldMwmPath, std::string const & newMwmPath,
                                std::string const & diffPath, base::Cancellable const & cancellable)
{
  try
  {
    FileReader oldReader(oldMwmPath);
    BufferedFileWriter newWriter(newMwmPath);
    FileReader diffFileReader(diffPath);

    ReaderSource<FileReader> diffFileSource(diffFileReader);
    auto const version = ReadPrimitiveFromSource<uint32_t>(diffFileSource);

    switch (version)
    {
    case VERSION_V0: return ApplyDiffVersion0(oldReader, newWriter, diffFileSource, cancellable);
    default: LOG(LERROR, ("Unknown version format of mwm diff:", version)); return DiffApplicationResult::Failed;
    }
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Could not open file for reading when applying a patch:", e.Msg()));
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Could not open file for writing when applying a patch:", e.Msg()));
  }

  return cancellable.IsCancelled() ? DiffApplicationResult::Cancelled : DiffApplicationResult::Failed;
}

std::string DebugPrint(DiffApplicationResult const & result)
{
  switch (result)
  {
  case DiffApplicationResult::Ok: return "Ok";
  case DiffApplicationResult::Failed: return "Failed";
  case DiffApplicationResult::Cancelled: return "Cancelled";
  }
  UNREACHABLE();
}
}  // namespace mwm_diff
}  // namespace generator
