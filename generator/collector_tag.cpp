#include "generator/collector_tag.hpp"

#include "generator/final_processor_utils.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"

namespace generator
{
CollectorTag::CollectorTag(std::string const & filename, std::string const & tagKey, Validator const & validator)
  : CollectorInterface(filename)
  , m_tagKey(tagKey)
  , m_validator(validator)
{
  m_stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  m_stream.open(GetTmpFilename());
}

std::shared_ptr<CollectorInterface> CollectorTag::Clone(IDRInterfacePtr const &) const
{
  return std::make_shared<CollectorTag>(GetFilename(), m_tagKey, m_validator);
}

void CollectorTag::Collect(OsmElement const & el)
{
  auto const tag = el.GetTag(m_tagKey);
  if (!tag.empty() && m_validator(tag))
    m_stream << GetGeoObjectId(el).GetEncodedId() << "\t" << tag << "\n";
}

void CollectorTag::Finish()
{
  if (m_stream.is_open())
    m_stream.close();
}

void CollectorTag::Save()
{
  CHECK(!m_stream.is_open(), ("Finish() has not been called."));
  if (Platform::IsFileExistsByFullPath(GetTmpFilename()))
    CHECK(base::CopyFileX(GetTmpFilename(), GetFilename()), ());
}

void CollectorTag::OrderCollectedData()
{
  OrderTextFileByLine(GetFilename());
}

void CollectorTag::MergeInto(CollectorTag & collector) const
{
  CHECK(!m_stream.is_open() || !collector.m_stream.is_open(), ("Finish() has not been called."));
  base::AppendFileToFile(GetTmpFilename(), collector.GetTmpFilename());
}
}  // namespace generator
