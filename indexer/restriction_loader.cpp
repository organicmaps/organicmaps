#include "indexer/restriction_loader.hpp"

#include "coding/reader.hpp"

using namespace routing;

namespace
{
template <class TSource>
void DeserializeRestrictions(Restriction::Type type, uint32_t count, TSource & src,
                             RestrictionVec & restrictions)
{
  feature::RestrictionSerializer serializer(Restriction(type, 0 /* linkNumber */));
  Restriction prevRestriction(type, 0 /* linkNumber */);
  prevRestriction.m_featureIds.resize(feature::RestrictionSerializer::kSupportedLinkNumber, 0);
  for (size_t i = 0; i < count; ++i)
  {
    serializer.Deserialize(prevRestriction, src);
    restrictions.push_back(serializer.GetRestriction());
    prevRestriction = serializer.GetRestriction();
  }
}
}  // namespace

namespace feature
{
RestrictionLoader::RestrictionLoader(MwmValue const & mwmValue)
  : m_countryFileName(mwmValue.GetCountryFileName())
{
  if (!mwmValue.m_cont.IsExist(ROUTING_FILE_TAG))
    return;

  try
  {
    m_reader = make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(ROUTING_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    DeserializeRestrictions(Restriction::Type::No, m_header.m_noRestrictionCount, src,
                            m_restrictions);
    DeserializeRestrictions(Restriction::Type::Only, m_header.m_onlyRestrictionCount, src,
                            m_restrictions);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR,
        ("File", m_countryFileName, "Error while reading", ROUTING_FILE_TAG, "section.", e.Msg()));
  }
}
}  // namespace feature
