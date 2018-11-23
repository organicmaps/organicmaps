#include "generator/maxspeeds_collector.hpp"

#include "generator/maxspeeds_parser.hpp"

#include "routing_common/maxspeed_conversion.hpp"

#include "coding/file_writer.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <sstream>

using namespace base;
using namespace generator;
using namespace routing;
using namespace std;

namespace
{
bool ParseMaxspeedAndWriteToStream(string const & maxspeed, SpeedInUnits & speed, ostringstream & ss)
{
  if (!ParseMaxspeedTag(maxspeed, speed))
    return false;

  ss << UnitsToString(speed.GetUnits()) << "," << strings::to_string(speed.GetSpeed());
  return true;
}
}  // namespace

namespace generator
{
void MaxspeedsCollector::Process(OsmElement const & p)
{
  ostringstream ss;
  ss << p.id << ",";

  auto const & tags = p.Tags();
  string maxspeedForwardStr;
  string maxspeedBackwardStr;
  bool isReverse = false;

  for (auto const & t : tags)
  {
    if (t.key == "maxspeed")
    {
      SpeedInUnits dummySpeed;
      if (!ParseMaxspeedAndWriteToStream(t.value, dummySpeed, ss))
        return;
      m_data.push_back(ss.str());
      return;
    }

    if (t.key == "maxspeed:forward")
      maxspeedForwardStr = t.value;
    else if (t.key == "maxspeed:backward")
      maxspeedBackwardStr = t.value;
    else if (t.key == "oneway")
      isReverse = (t.value == "-1");
  }

  // Note 1. isReverse == true means feature |p| has tag "oneway" with value "-1". Now (10.2018)
  // no feature with a tag oneway==-1 and a tag maxspeed:forward/backward is found. But to
  // be on the safe side this case is handled.
  // Note 2. If oneway==-1 the order of points is changed while conversion to mwm. So it's
  // necessary to swap forward and backward as well.
  if (isReverse)
    maxspeedForwardStr.swap(maxspeedBackwardStr);

  if (maxspeedForwardStr.empty())
    return;

  SpeedInUnits maxspeedForward;
  if (!ParseMaxspeedAndWriteToStream(maxspeedForwardStr, maxspeedForward, ss))
    return;

  if (!maxspeedBackwardStr.empty())
  {
    SpeedInUnits maxspeedBackward;
    if (!ParseMaxspeedTag(maxspeedBackwardStr, maxspeedBackward))
      return;

    // Note. Keeping only maxspeed:forward and maxspeed:backward if they have the same units.
    // The exception is maxspeed:forward or maxspeed:backward have a special values
    // like "none" or "walk". In that case units mean nothing an the values should
    // be processed in a special way.
    if (!HaveSameUnits(maxspeedForward, maxspeedBackward))
      return;

    ss << "," << strings::to_string(maxspeedBackward.GetSpeed());
  }

  m_data.push_back(ss.str());
}

void MaxspeedsCollector::Flush()
{
  LOG(LINFO, ("Saving maxspeed tag values to", m_filePath));
  ofstream stream(m_filePath);

  if (!stream)
  {
    LOG(LERROR, ("Cannot open file", m_filePath));
    return;
  }

  for (auto const & s : m_data)
    stream << s << '\n';

  if (stream.fail())
    LOG(LERROR, ("Cannot write to file", m_filePath));
  else
    LOG(LINFO, ("Wrote", m_data.size(), "maxspeed tags to", m_filePath));
}
}  // namespace generator
