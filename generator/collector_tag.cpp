#include "generator/collector_tag.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

namespace generator
{
CollectorTag::CollectorTag(std::string const & filename, std::string const & tagKey,
                           Validator const & validator, bool ignoreIfNotOpen)
  : CollectorInterface(filename)
  , m_tagKey(tagKey)
  , m_validator(validator)
  , m_ignoreIfNotOpen(ignoreIfNotOpen) {}

std::shared_ptr<CollectorInterface>
CollectorTag::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CollectorTag>(GetFilename(), m_tagKey, m_validator, m_ignoreIfNotOpen);
}

void CollectorTag::Collect(OsmElement const & el)
{
  auto const tag = el.GetTag(m_tagKey);
  if (!tag.empty() && m_validator(tag))
    m_stream << GetGeoObjectId(el).GetEncodedId() << "\t" << tag << "\n";
}

void CollectorTag::Save()
{
  std::ofstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  try
  {
    stream.open(GetFilename());
    stream << m_stream.str();
  }
  catch (std::ios::failure const & e)
  {
    if (m_ignoreIfNotOpen)
    {
      LOG(LINFO, ("Сould not open file", GetFilename(), ". This was ignored."));
    }
    else
    {
      throw e;
    }
  }
}

void CollectorTag::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CollectorTag::MergeInto(CollectorTag & collector) const
{
  collector.m_stream << m_stream.str();
}
}  // namespace generator
