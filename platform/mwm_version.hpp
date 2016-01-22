#pragma once

#include "std/cstdint.hpp"

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
  v8,      // January 2016 (long strings in metadata).
  lastFormat = v8
};

struct MwmVersion
{
  MwmVersion();

  Format format;
  uint32_t timestamp;
};

/// Writes latest format and current timestamp to the writer.
void WriteVersion(Writer & w, uint32_t versionDate);

/// Reads mwm version from src.
void ReadVersion(ReaderSrc & src, MwmVersion & version);

/// \return True when version was successfully parsed from container,
///         otherwise returns false. In the latter case version is
///         unchanged.
bool ReadVersion(FilesContainerR const & container, MwmVersion & version);

/// Helper function that is used in FindAllLocalMaps.
uint32_t ReadVersionTimestamp(ModelReaderPtr const & reader);

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
