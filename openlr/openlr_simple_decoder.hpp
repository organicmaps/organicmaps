#pragma once

#include "base/exception.hpp"

#include "3party/pugixml/src/pugixml.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

class Index;

namespace openlr
{
struct LinearSegment;

DECLARE_EXCEPTION(DecoderError, RootException);

class OpenLRSimpleDecoder
{
public:
  class SegmentsFilter
  {
  public:
    SegmentsFilter(std::string const & idsPath, bool const multipointsOnly);

    bool Matches(LinearSegment const & segment) const;

  private:
    std::unordered_set<uint32_t> m_ids;
    bool m_idsSet;
    bool const m_multipointsOnly;
  };

  static int const kHandleAllSegments;

  OpenLRSimpleDecoder(std::string const & dataFilename, std::vector<Index> const & indexes);

  void Decode(std::string const & outputFilename, int segmentsToHandle,
              SegmentsFilter const & filter, int numThreads);

private:
  std::vector<Index> const & m_indexes;
  pugi::xml_document m_document;
};
}  // namespace openlr
