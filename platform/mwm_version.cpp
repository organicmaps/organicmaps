#include "mwm_version.hpp"

#include "coding/files_container.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/gmtime.hpp"
#include "base/string_utils.hpp"
#include "base/timegm.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <sstream>

namespace version
{
namespace
{
// Editing maps older than approximately two months old is disabled, since the data
// is most likely already fixed on OSM. Not limited to the latest one or two versions,
// because a user can forget to update maps after a new app version has been installed
// automatically in the background.
uint64_t constexpr kMaxSecondsTillNoEdits = 3600 * 24 * 31 * 2;
char const MWM_PROLOG[] = "MWM";

template <class TSource>
void ReadVersionT(TSource & src, MwmVersion & version)
{
  size_t const prologSize = ARRAY_SIZE(MWM_PROLOG);
  char prolog[prologSize];
  src.Read(prolog, prologSize);

  if (strcmp(prolog, MWM_PROLOG) != 0)
  {
    version.SetFormat(Format::v2);
    version.SetSecondsSinceEpoch(base::YYMMDDToSecondsSinceEpoch(111101));
    return;
  }

  // Read format value "as-is". It's correctness will be checked later
  // with the correspondent return value.
  version.SetFormat(static_cast<Format>(ReadVarUint<uint32_t>(src)));
  if (version.GetFormat() < Format::v8)
  {
    version.SetSecondsSinceEpoch(
        base::YYMMDDToSecondsSinceEpoch(static_cast<uint32_t>(ReadVarUint<uint64_t>(src))));
  }
  else
  {
    version.SetSecondsSinceEpoch(ReadVarUint<uint32_t>(src));
  }
}
}  // namespace

uint32_t MwmVersion::GetVersion() const
{
  auto const tm = base::GmTime(base::SecondsSinceEpochToTimeT(m_secondsSinceEpoch));
  return base::GenerateYYMMDD(tm.tm_year, tm.tm_mon, tm.tm_mday);
}

bool MwmVersion::IsEditableMap() const
{
  return m_secondsSinceEpoch + kMaxSecondsTillNoEdits > base::SecondsSinceEpoch();
}

std::string DebugPrint(Format f)
{
  return "v" + strings::to_string(static_cast<uint32_t>(f) + 1);
}

std::string DebugPrint(MwmVersion const & mwmVersion)
{
  std::stringstream s;
  s << "MwmVersion [format:" << DebugPrint(mwmVersion.GetFormat())
    << ", seconds:" << mwmVersion.GetSecondsSinceEpoch()  << "]";

  return s.str();
}

void WriteVersion(Writer & w, uint64_t secondsSinceEpoch)
{
  w.Write(MWM_PROLOG, ARRAY_SIZE(MWM_PROLOG));

  // write inner data version
  WriteVarUint(w, static_cast<uint32_t>(Format::lastFormat));
  WriteVarUint(w, secondsSinceEpoch);
}

void ReadVersion(ReaderSrc & src, MwmVersion & version) { ReadVersionT(src, version); }

bool ReadVersion(FilesContainerR const & container, MwmVersion & version)
{
  if (!container.IsExist(VERSION_FILE_TAG))
    return false;

  ModelReaderPtr versionReader = container.GetReader(VERSION_FILE_TAG);
  ReaderSource<ModelReaderPtr> src(versionReader);
  ReadVersionT(src, version);
  return true;
}

uint32_t ReadVersionDate(ModelReaderPtr const & reader)
{
  MwmVersion version;
  if (!ReadVersion(FilesContainerR(reader), version))
    return 0;

  return version.GetVersion();
}

bool IsSingleMwm(int64_t version)
{
  int64_t constexpr kMinSingleMwmVersion = 160302;
  return version >= kMinSingleMwmVersion || version == 0 /* Version of mwm in the root directory. */;
}

MwmType GetMwmType(MwmVersion const & version)
{
  if (!IsSingleMwm(version.GetVersion()))
    return MwmType::SeparateMwms;
  if (version.GetFormat() < Format::v8)
    return MwmType::SeparateMwms;
  if (version.GetFormat() > Format::v8)
    return MwmType::SingleMwm;
  return MwmType::Unknown;
}
}  // namespace version
