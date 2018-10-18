#include "generator/maxspeed_builder.hpp"

#include "coding/file_writer.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

#include <fstream>
#include <sstream>

namespace feature
{
using namespace base;
using namespace std;

void MaxspeedBuilder::operator()(OsmElement const & p)
{
  ostringstream ss;
  ss << p.id << ",";

  auto const & tags = p.Tags();
  string maxspeedForward;
  string maxspeedBackward;

  for (auto const & t : tags)
  {
    if (t.key == string("maxspeed"))
    {
      ss << t.value;
      m_data.push_back(ss.str());
      return;
    }

    if (t.key == string("maxspeed:forward"))
      maxspeedForward = t.value;

    if (t.key == string("maxspeed:backward"))
      maxspeedBackward = t.value;
  }

  if (maxspeedForward.empty())
    return;

  ss << maxspeedForward;
  if (!maxspeedBackward.empty())
    ss << "," << maxspeedBackward;

  m_data.push_back(ss.str());
}

void MaxspeedBuilder::Flush()
{
  try
  {
    LOG(LINFO, ("Saving maxspeed tag values to", m_filePath));
    ofstream stream(m_filePath);

    if (!stream.is_open())
      LOG(LERROR, ("Cannot open file", m_filePath));

    for (auto const & s : m_data)
      stream << s << '\n';

    LOG(LINFO, ("Wrote", m_data.size(), "maxspeed tags to", m_filePath));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while saving tags to", m_filePath, ":", e.what()));
  }
}
}  // namespace feature
