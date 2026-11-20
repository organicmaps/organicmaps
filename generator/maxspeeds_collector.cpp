#include "generator/maxspeeds_collector.hpp"

#include "generator/final_processor_utils.hpp"
#include "generator/maxspeeds_parser.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace generator
{
namespace
{
std::string ToStream(routing::SpeedInUnits const & speed)
{
  if (speed.IsValid())
    return std::to_string(speed.GetSpeed());
  return {};
}
}  // namespace

MaxspeedsCollector::MaxspeedsCollector(std::string const & filename) : CollectorInterface(filename)
{
  m_stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  m_stream.open(GetTmpFilename());
}

std::shared_ptr<CollectorInterface> MaxspeedsCollector::Clone(IDRInterfacePtr const &) const
{
  return std::make_shared<MaxspeedsCollector>(GetFilename());
}

void MaxspeedsCollector::CollectFeature(feature::FeatureBuilder const & ft, OsmElement const & p)
{
  if (!p.IsWay())
    return;

  routing::SpeedInUnits maxspeed, maxspeedForward, maxspeedBackward, maxspeedAdvisory;
  std::string maxspeedConditionalStr;
  bool isReverse = false;

  for (auto const & t : p.Tags())
    if (t.m_key == "maxspeed")
      ParseMaxspeedTag(t.m_value, maxspeed);
    else if (t.m_key == "maxspeed:conditional")
      maxspeedConditionalStr = t.m_value;
    else if (t.m_key == "maxspeed:advisory")
      ParseMaxspeedTag(t.m_value, maxspeedAdvisory);
    else if (t.m_key == "maxspeed:forward")
      ParseMaxspeedTag(t.m_value, maxspeedForward);
    else if (t.m_key == "maxspeed:backward")
      ParseMaxspeedTag(t.m_value, maxspeedBackward);
    else if (t.m_key == "oneway")
      isReverse = (t.m_value == "-1");

  // Note 1. isReverse == true means feature |p| has tag "oneway" with value "-1". Now (10.2018)
  // no feature with a tag oneway==-1 and a tag maxspeed:forward/backward is found. But to
  // be on the safe side this case is handled.
  // Note 2. If oneway==-1 the order of points is changed while conversion to mwm. So it's
  // necessary to swap forward and backward as well.
  if (isReverse)
    std::swap(maxspeedForward, maxspeedBackward);

  // Plain maxspeed applies to each direction that has no own maxspeed:forward/backward value.
  // With maxspeed:forward only, backward stays empty and the forward speed is used in both directions
  // (behavior since commit 1b00488c3, 2018-10-18, "[routing] Creating maxspeed.csv for generation maxspeed section.").
  if (!maxspeedForward.IsValid())
    maxspeedForward = maxspeed;
  if (!maxspeedBackward.IsValid())
    maxspeedBackward = maxspeed;

  bool recalcIndicator = false;
  if (!maxspeedForward.IsValid() && ftypes::IsLinkChecker::Instance()(ft.GetTypes()))
  {
    if (maxspeedAdvisory.IsValid())
    {
      // Assign maxspeed:advisory for highway:xxx_link types to avoid situation when
      // not defined link speed is predicted bigger than defined parent ingoing highway speed.
      // https://www.openstreetmap.org/way/5511439#map=18/45.55465/-122.67787
      maxspeedForward = maxspeedAdvisory;
    }
    else
    {
      // Write indicator that it's a link and the speed should be recalculated.
      // Idea is to set the link speed equal to the ingoing highway speed (now it's "default" and can be bigger).
      // https://www.openstreetmap.org/way/842536050#map=19/45.43449/-122.56678
      recalcIndicator = true;
    }
  }

  // Note. Keeping only forward and backward speeds if they have the same units.
  // The exception is forward or backward speed have a special values
  // like "none" or "walk". In that case units mean nothing and the values should
  // be processed in a special way.
  if (maxspeedBackward.IsValid())
  {
    if (!maxspeedForward.IsValid() || !HaveSameUnits(maxspeedForward, maxspeedBackward))
      maxspeedBackward = {};
    else if (!maxspeedForward.IsNumeric())
      maxspeedForward.SetUnits(maxspeedBackward.GetUnits());  // Dummy units of "none"/"walk" would mislabel backward.
  }

  routing::SpeedInUnits maxspeedCond;
  std::string condTime;
  if (ParseMaxspeedConditionalTag(maxspeedConditionalStr, maxspeedCond, condTime))
  {
    if ((maxspeedForward.IsValid() && !HaveSameUnits(maxspeedForward, maxspeedCond)) ||
        !HaveSameUnits(maxspeedBackward, maxspeedCond))
    {
      maxspeedCond = {};
      condTime.clear();
    }
  }

  if (!maxspeedForward.IsValid() && !maxspeedCond.IsValid())
  {
    if (recalcIndicator)
      maxspeedForward.SetSpeed(routing::kCommonMaxSpeedValue);
    else
      return;
  }

  m_stream << p.m_id << ","
           << UnitsToString(maxspeedForward.IsValid() ? maxspeedForward.GetUnits() : maxspeedCond.GetUnits()) << ","
           << ToStream(maxspeedForward) << "," << ToStream(maxspeedBackward) << "," << ToStream(maxspeedCond) << ","
           << condTime << "\n";
}

void MaxspeedsCollector::Finish()
{
  if (m_stream.is_open())
    m_stream.close();
}

void MaxspeedsCollector::Save()
{
  /// @todo Can keep calculated speeds in memory to avoid tmp files dumping and merging.
  CHECK(!m_stream.is_open(), ("Finish() has not been called."));
  LOG(LINFO, ("Saving maxspeed tag values to", GetFilename()));

  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void MaxspeedsCollector::OrderCollectedData()
{
  OrderTextFileByLine(GetFilename());
}

void MaxspeedsCollector::MergeInto(MaxspeedsCollector & collector) const
{
  CHECK(!m_stream.is_open() && !collector.m_stream.is_open(), ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
