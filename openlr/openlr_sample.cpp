#include "openlr/openlr_sample.hpp"

#include "indexer/index.hpp"

#include "base/string_utils.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>

namespace
{
void ParseMWMSegments(std::string const & line, uint32_t const lineNumber,
                      std::vector<openlr::SampleItem::MWMSegment> & segments, Index const & index)
{
  std::vector<string> parts;
  strings::ParseCSVRow(line, '=', parts);

  for (auto const & seg : parts)
  {
    std::vector<string> segParts;
    strings::ParseCSVRow(seg, '-', segParts);
    CHECK_EQUAL(segParts.size(), 5, ());

    auto const mwmId = index.GetMwmIdByCountryFile(platform::CountryFile(segParts[0]));

    uint32_t featureIndex;
    if (!strings::to_uint(segParts[1], featureIndex))
      MYTHROW(openlr::SamplePoolLoadError, ("Can't parse MWMSegment", seg, "line:", lineNumber));

    uint32_t segId;
    if (!strings::to_uint(segParts[2], segId))
      MYTHROW(openlr::SamplePoolLoadError, ("Can't parse MWMSegment", seg, "line:", lineNumber));

    bool const isForward = (segParts[3] == "fwd");
    double length = 0;
    if (!strings::to_double(segParts[4], length))
      MYTHROW(openlr::SamplePoolLoadError, ("Can't parse MWMSegment", seg, "line:", lineNumber));

    segments.push_back({FeatureID(mwmId, featureIndex), segId, isForward, length});
  }
}

void ParseSampleItem(std::string const & line, uint32_t const lineNumber, openlr::SampleItem & item,
                     Index const & index)
{
  std::vector<string> parts;
  strings::ParseCSVRow(line, '\t', parts);
  CHECK_GREATER_OR_EQUAL(parts.size(), 2, ());
  CHECK_LESS_OR_EQUAL(parts.size(), 3, ());

  auto nextFieldIndex = 0;
  if (parts.size() == 3)
  {
    item.m_evaluation = openlr::ParseItemEvaluation(parts[nextFieldIndex]);
    ++nextFieldIndex;
  }
  else
  {
    item.m_evaluation = openlr::ItemEvaluation::Unevaluated;
  }

  if (!strings::to_uint(parts[nextFieldIndex], item.m_partnerSegmentId.Get()))
  {
    MYTHROW(openlr::SamplePoolLoadError, ("Error: can't parse field", nextFieldIndex,
                                          "(number expected) in line:", lineNumber));
  }
  ++nextFieldIndex;

  ParseMWMSegments(parts[nextFieldIndex], lineNumber, item.m_segments, index);
}
}  // namepsace

namespace openlr
{
ItemEvaluation ParseItemEvaluation(std::string const & s)
{
  if (s == "Unevaluated")
    return openlr::ItemEvaluation::Unevaluated;

  if (s == "Positive")
    return openlr::ItemEvaluation::Positive;

  if (s == "Negative")
    return openlr::ItemEvaluation::Negative;

  if (s == "RelPositive")
    return openlr::ItemEvaluation::RelPositive;

  if (s == "RelNegative")
    return openlr::ItemEvaluation::RelNegative;

  if (s == "Ignore")
    return openlr::ItemEvaluation::Ignore;

  return openlr::ItemEvaluation::NotAValue;
}

std::string ToString(ItemEvaluation const e)
{
  switch (e)
  {
  case openlr::ItemEvaluation::Unevaluated: return "Unevaluated";
  case openlr::ItemEvaluation::Positive: return "Positive";
  case openlr::ItemEvaluation::Negative: return "Negative";
  case openlr::ItemEvaluation::RelPositive: return "RelPositive";
  case openlr::ItemEvaluation::RelNegative: return "RelNegative";
  case openlr::ItemEvaluation::Ignore: return "Ignore";
  default: return "NotAValue";
  }
}

SamplePool LoadSamplePool(std::string const & fileName, Index const & index)
{
  std::ifstream sample(fileName);
  if (!sample.is_open())
    MYTHROW(SamplePoolLoadError, ("Can't read from file", fileName, strerror(errno)));

  SamplePool pool;
  for (struct {uint32_t lineNumber = 0; string line; } st; getline(sample, st.line); ++st.lineNumber)
  {
    SampleItem item = SampleItem::Uninitialized();
    ParseSampleItem(st.line, st.lineNumber, item, index);
    pool.push_back(item);
  }

  return pool;
}

void SaveSamplePool(std::string const & fileName, SamplePool const & sample,
                    bool const saveEvaluation)
{
  LOG(LDEBUG, ("Saving sample to file:", fileName));
  std::ofstream out(fileName);
  if (!out.is_open())
    MYTHROW(SamplePoolSaveError, ("Can't write to file", fileName, strerror(errno)));
  out << std::fixed;  // Avoid scientific notation cause '-' is used as fields separator.
  for (auto const & item : sample)
  {
    if (saveEvaluation)
      out << ToString(item.m_evaluation) << '\t';

    out << item.m_partnerSegmentId.Get() << '\t';

    for (auto it = begin(item.m_segments); it != end(item.m_segments); ++it)
    {
      auto const & fid = it->m_fid;
      out << fid.m_mwmId.GetInfo()->GetCountryName() << '-'
          << fid.m_index << '-' << it->m_segId
          << '-' << (it->m_isForward ? "fwd" : "bwd")
          << '-' << it->m_length;

      if (next(it) != end(item.m_segments))
        out << '=';
    }
    out << endl;
  }
  if (out.fail())
    MYTHROW(SamplePoolSaveError, ("An error occured while writing file", fileName, strerror(errno)));
}
}  // namespace openlr
