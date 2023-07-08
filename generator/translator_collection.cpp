#include "generator/translator_collection.hpp"

#include "generator/osm_element.hpp"

#include "base/stl_helpers.hpp"

namespace generator
{
std::shared_ptr<TranslatorInterface> TranslatorCollection::Clone() const
{
  auto p = std::make_shared<TranslatorCollection>();
  for (auto const & c : m_collection)
    p->Append(c->Clone());
  return p;
}

void TranslatorCollection::Emit(OsmElement /* const */ & element)
{
  for (auto & t : m_collection)
  {
    OsmElement copy = element;
    t->Emit(copy);
  }
}

void TranslatorCollection::Finish()
{
  for (auto & t : m_collection)
    t->Finish();
}

bool TranslatorCollection::Save()
{
  return base::AllOf(m_collection, [](auto & t) { return t->Save(); });
}

void TranslatorCollection::Merge(TranslatorInterface const & other) { other.MergeInto(*this); }

void TranslatorCollection::MergeInto(TranslatorCollection & other) const
{
  auto & otherCollection = other.m_collection;
  CHECK_EQUAL(m_collection.size(), otherCollection.size(), ());
  for (size_t i = 0; i < m_collection.size(); ++i)
    otherCollection[i]->Merge(*m_collection[i]);
}
}  // namespace generator
