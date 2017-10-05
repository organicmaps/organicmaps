#include "generator/ugc_translator.hpp"

#include "generator/ugc_db.hpp"

#include "ugc/serdes_json.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

namespace generator
{
UGCTranslator::UGCTranslator() : m_db(":memory:") {}

UGCTranslator::UGCTranslator(std::string const & dbFilename) : m_db(dbFilename) {}

bool UGCTranslator::TranslateUGC(osm::Id const & id, ugc::UGC & ugc)
{
  std::vector<uint8_t> blob;

  if (!m_db.Get(id, blob))
    return false;

  std::string src(blob.cbegin(), blob.cend());

  ugc::DeserializerJsonV0 des(src);

  des(ugc);

  return true;
}

void UGCTranslator::CreateDb(std::string const & data)
{
  CHECK(m_db.Exec(data), ());
}
}  // namespace generator
