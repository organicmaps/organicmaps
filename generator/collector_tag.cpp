#include "generator/collector_tag.hpp"

#include "generator/osm_element.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

namespace generator
{
CollectorTag::CollectorTag(std::string const & filename, std::string const & tagKey,
                           Validator const & validator, bool ignoreIfNotOpen)
  : m_tagKey(tagKey), m_validator(validator), m_needCollect(true)
{
  m_stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  try
  {
    m_stream.open(filename);
  }
  catch (std::ios::failure const & e)
  {
    if (ignoreIfNotOpen)
    {
      m_needCollect = false;
      LOG(LINFO, ("Сould not open file", filename, ". This was ignored."));
    }
    else
    {
      throw e;
    }
  }
}

void CollectorTag::Collect(OsmElement const & el)
{
  if (!m_needCollect)
    return;

  auto const tag = el.GetTag(m_tagKey);
  if (!tag.empty() && m_validator(tag))
    m_stream << GetGeoObjectId(el).GetEncodedId() << "\t" << tag << "\n";
}
}  // namespace generator
