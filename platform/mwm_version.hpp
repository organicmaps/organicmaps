#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"

class FilesContainerR;
class ReaderSrc;
class Writer;
class ModelReaderPtr;

namespace version
{
enum class Format
{
  unknownFormat = -1,
  v1 = 0,  // April 2011
  v2,      // November 2011 (store type index, instead of raw type in mwm)
  v3,      // March 2013 (store type index, instead of raw type in search data)
  v4,      // April 2015 (distinguish и and й in search index)
  v5,      // July 2015 (feature id is the index in vector now).
  v6,      // October 2015 (offsets vector is in mwm now).
  v7,      // November 2015 (supply different search index formats).
  v8,      // February 2016 (long strings in metadata; store seconds since epoch in MwmVersion).
           // December 2016 (index graph section was added in version 161206, between v8 and v9).
  v9,      // April 2017 (OSRM sections are deleted and replaced by cross mwm section).
  lastFormat = v9
};

string DebugPrint(Format f);

class MwmVersion
{
public:
  Format GetFormat() const { return m_format; }
  uint64_t GetSecondsSinceEpoch() const { return m_secondsSinceEpoch; }
  /// \return version as YYMMDD.
  uint32_t GetVersion() const;

  void SetFormat(Format format) { m_format = format; }
  void SetSecondsSinceEpoch(uint64_t secondsSinceEpoch) { m_secondsSinceEpoch = secondsSinceEpoch; }
  bool IsEditableMap() const;

private:
  /// Data layout format in mwm file.
  Format m_format{Format::unknownFormat};
  uint64_t m_secondsSinceEpoch{0};
};

/// Writes latest format and current timestamp to the writer.
void WriteVersion(Writer & w, uint64_t secondsSinceEpoch);

/// Reads mwm version from src.
void ReadVersion(ReaderSrc & src, MwmVersion & version);

/// \return True when version was successfully parsed from container,
///         otherwise returns false. In the latter case version is
///         unchanged.
bool ReadVersion(FilesContainerR const & container, MwmVersion & version);

/// Helper function that is used in FindAllLocalMaps.
uint32_t ReadVersionDate(ModelReaderPtr const & reader);

/// \returns true if version is version of an mwm which was generated after small mwm update.
/// This means it contains routing file as well.
bool IsSingleMwm(int64_t version);

/// \brief This enum sets constants which are used for writing test to set a version of mwm
/// which should be processed as either single or two components (mwm and routing) mwms.
enum ForTesting
{
  FOR_TESTING_TWO_COMPONENT_MWM1 = 10,
  FOR_TESTING_TWO_COMPONENT_MWM2,
  FOR_TESTING_SINGLE_MWM1 = 991215,
  FOR_TESTING_SINGLE_MWM2,
  FOR_TESTING_SINGLE_MWM_LATEST,
};
}  // namespace version
