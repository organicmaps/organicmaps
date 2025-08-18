#include "mwm_version.hpp"

#include "coding/files_container.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/gmtime.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <sstream>

namespace version
{
namespace
{
char const MWM_PROLOG[] = "MWM";
}

MwmVersion MwmVersion::Read(FilesContainerR const & container)
{
  ModelReaderPtr versionReader = container.GetReader(VERSION_FILE_TAG);
  ReaderSource<ModelReaderPtr> src(versionReader);

  size_t const prologSize = ARRAY_SIZE(MWM_PROLOG);
  char prolog[prologSize];
  src.Read(prolog, prologSize);

  if (strcmp(prolog, MWM_PROLOG) != 0)
    MYTHROW(CorruptedMwmFile, ());

  MwmVersion version;
  version.m_format = static_cast<Format>(ReadVarUint<uint32_t>(src));
  version.m_secondsSinceEpoch = ReadVarUint<uint32_t>(src);
  return version;
}

uint32_t MwmVersion::GetVersion() const
{
  auto const tm = base::GmTime(base::SecondsSinceEpochToTimeT(m_secondsSinceEpoch));
  return base::GenerateYYMMDD(tm.tm_year, tm.tm_mon, tm.tm_mday);
}

std::string DebugPrint(Format f)
{
  return "v" + strings::to_string(static_cast<uint32_t>(f) + 1);
}

std::string DebugPrint(MwmVersion const & mwmVersion)
{
  std::stringstream s;
  s << "MwmVersion "
    << "{ m_format: " << DebugPrint(mwmVersion.GetFormat())
    << ", m_secondsSinceEpoch: " << mwmVersion.GetSecondsSinceEpoch() << " }";
  return s.str();
}

void WriteVersion(Writer & w, uint64_t secondsSinceEpoch)
{
  w.Write(MWM_PROLOG, ARRAY_SIZE(MWM_PROLOG));

  // write inner data version
  WriteVarUint(w, static_cast<uint32_t>(Format::lastFormat));
  WriteVarUint(w, secondsSinceEpoch);
}

uint32_t ReadVersionDate(ModelReaderPtr const & reader)
{
  return MwmVersion::Read(FilesContainerR(reader)).GetVersion();
}

}  // namespace version
