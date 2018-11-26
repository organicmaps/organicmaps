#include "descriptions/serdes.hpp"

#include <utility>

namespace descriptions
{
Serializer::Serializer(DescriptionsCollection && descriptions)
  : m_descriptions(std::move(descriptions))
{
  CHECK(!m_descriptions.empty(), ());

  std::sort(m_descriptions.begin(), m_descriptions.end(), base::LessBy(&FeatureDescription::m_featureIndex));

  m_langMetaCollection.reserve(m_descriptions.size());

  size_t stringsCount = 0;

  for (size_t i = 0; i < m_descriptions.size(); ++i)
  {
    auto & index = m_descriptions[i];
    CHECK(!index.m_description.IsEmpty(), ());

    LangMeta langMeta;
    index.m_description.ForEach([this, &stringsCount, &langMeta, i](LangCode lang, std::string const & str)
                                {
                                  CHECK_GREATER_OR_EQUAL(lang, 0, ());
                                  CHECK(lang < StringUtf8Multilang::kMaxSupportedLanguages, ());
                                  CHECK(!str.empty(), ());
                                  ++stringsCount;
                                  auto & group = m_groupedByLang[lang];
                                  langMeta.insert(std::make_pair(lang, static_cast<StringIndex>(group.size())));
                                  group.push_back(i);
                                });
    m_langMetaCollection.push_back(langMeta);
  }

  std::map<LangCode, uint32_t> indicesOffsets;
  uint32_t currentOffset = 0;
  for (auto & langIndices : m_groupedByLang)
  {
    indicesOffsets.insert(std::make_pair(langIndices.first, currentOffset));
    currentOffset += langIndices.second.size();
  }

  for (auto & langMeta : m_langMetaCollection)
  {
    for (auto & translation : langMeta)
      translation.second += indicesOffsets[translation.first];
  }
}
}  // namespace descriptions
