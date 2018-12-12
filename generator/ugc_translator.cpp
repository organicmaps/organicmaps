#include "generator/ugc_translator.hpp"

#include "generator/ugc_db.hpp"

#include "ugc/serdes_json.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/string_utils.hpp"

#include <vector>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
UGCTranslator::UGCTranslator() : m_db(":memory:") {}

UGCTranslator::UGCTranslator(std::string const & dbFilename) : m_db(dbFilename) {}

bool UGCTranslator::TranslateUGC(base::GeoObjectId const & id, ugc::UGC & ugc)
{
  std::vector<uint8_t> src;

  if (!m_db.Get(id, src))
    return false;

  std::string str(src.cbegin(), src.cend());

  base::Json json(str);

  auto const size = json_array_size(json.get());

  if (size == 0)
    return false;

  if (size > 1)
  {
    LOG(LWARNING, ("Osm id duplication in UGC database", id.GetEncodedId()));
    return false;
  }

  ugc::DeserializerJsonV0 des(json_array_get(json.get(), 0));

  des(ugc);

  return true;
}

void UGCTranslator::CreateDb(std::string const & data)
{
  CHECK(m_db.Exec(data), ());
}
}  // namespace generator
