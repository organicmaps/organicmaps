#pragma once

#include "std/cstdint.hpp"

class FilesContainerR;
class ReaderSrc;
class Writer;
class ModelReaderPtr;

namespace version
{
enum Format
{
  unknownFormat = -1,
  v1 = 0,  // April 2011
  v2,      // November 2011 (store type index, instead of raw type in mwm)
  v3,      // March 2013 (store type index, instead of raw type in search data)
  v4,      // April 2015 (distinguish и and й in search index)
  v5,      // July 2015 (feature id is the index in vector now).
  v6,      // October 2015 (offsets vector is in mwm now).
  lastFormat = v6
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
}  // namespace version
