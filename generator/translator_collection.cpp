#include "generator/translator_collection.hpp"

#include "generator/osm_element.hpp"

#include <algorithm>
#include <iterator>

namespace generator
{
void TranslatorCollection::Emit(OsmElement /* const */ & element)
{
  for (auto & t : m_collection)
  {
    OsmElement copy = element;
    t->Emit(copy);
  }
}

bool TranslatorCollection::Finish()
{
  return std::all_of(std::begin(m_collection), std::end(m_collection), [](auto & t) {
    return t->Finish();
  });
}

void TranslatorCollection::GetNames(std::vector<std::string> & names) const
{
  for (auto & t : m_collection)
  {
    std::vector<std::string> temp;
    t->GetNames(temp);
    std::move(std::begin(temp), std::end(temp), std::back_inserter(names));
  }
}
}  // namespace generator
