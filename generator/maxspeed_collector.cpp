#include "generator/maxspeed_collector.hpp"

#include "coding/file_writer.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <fstream>
#include <sstream>

namespace feature
{
using namespace base;
using namespace std;

void MaxspeedCollector::Process(OsmElement const & p)
{
  ostringstream ss;
  ss << p.id << ",";

  auto const & tags = p.Tags();
  string maxspeedForward;
  string maxspeedBackward;
  bool isReverse = false;

  for (auto const & t : tags)
  {
    if (t.key == "maxspeed")
    {
      ss << t.value;
      m_data.push_back(ss.str());
      return;
    }

    if (t.key == "maxspeed:forward")
      maxspeedForward = t.value;
    else if (t.key == "maxspeed:backward")
      maxspeedBackward = t.value;
    else if (t.key == "oneway")
      isReverse = (t.value == "-1");
  }

  // Note 1. isReverse == true means feature |p| has tag "oneway" with value "-1". Now (10.2018)
  // no feature with a tag oneway==-1 and a tag maxspeed:forward/backward is found. But to
  // be on the safe side the case is processed.
  // Note 2. If oneway==-1 the order of points is changed while conversion to mwm. So it's
  // necessary to swap forward and backward as well.
  if (isReverse)
    maxspeedForward.swap(maxspeedBackward);

  if (maxspeedForward.empty())
    return;

  ss << maxspeedForward;
  if (!maxspeedBackward.empty())
    ss << "," << maxspeedBackward;

  m_data.push_back(ss.str());
}

void MaxspeedCollector::Flush()
{
  LOG(LINFO, ("Saving maxspeed tag values to", m_filePath));
  ofstream stream(m_filePath);

  if (!stream.is_open())
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
}  // namespace feature
