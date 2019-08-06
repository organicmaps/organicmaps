#include "generator/collector_tag.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

namespace generator
{
CollectorTag::CollectorTag(std::string const & filename, std::string const & tagKey,
                           Validator const & validator)
  : CollectorInterface(filename)
  , m_tagKey(tagKey)
  , m_validator(validator)
{
  m_writer.exceptions(std::fstream::failbit | std::fstream::badbit);
  m_writer.open(GetTmpFilename());
}

std::shared_ptr<CollectorInterface>
CollectorTag::Clone(std::shared_ptr<cache::IntermediateDataReader> const &) const
{
  return std::make_shared<CollectorTag>(GetFilename(), m_tagKey, m_validator);
}

void CollectorTag::Collect(OsmElement const & el)
{
  auto const tag = el.GetTag(m_tagKey);
  if (!tag.empty() && m_validator(tag))
    m_writer << GetGeoObjectId(el).GetEncodedId() << "\t" << tag << "\n";
}

void CollectorTag::Finish()
{
  if (m_writer.is_open())
    m_writer.close();
}

void CollectorTag::Save()
{
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void CollectorTag::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CollectorTag::MergeInto(CollectorTag & collector) const
{
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
