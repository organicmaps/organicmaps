#include "feature_data.hpp"

#include "../std/algorithm.hpp"


using namespace feature;

bool FeatureParamsBase::operator == (FeatureParamsBase const & rhs) const
{
  return (name == rhs.name && house == rhs.house && ref == rhs.ref &&
          layer == rhs.layer && rank == rhs.rank);
}

bool FeatureParamsBase::CheckValid() const
{
   CHECK(layer >= -10 && layer <= 10, ());
   return true;
}

string FeatureParamsBase::DebugString() const
{
  string utf8name;
  name.GetString(0, utf8name);

  return ("'" + utf8name + "' Layer:" + debug_print(layer) +
          (rank != 0 ? " Rank:" + debug_print(rank) : "") +
          (!house.IsEmpty() ? " House:" + house.Get() : "") +
          (!ref.empty() ? " Ref:" + ref : "") + " ");
}

void FeatureParams::AddTypes(FeatureParams const & rhs)
{
  m_Types.insert(m_Types.end(), rhs.m_Types.begin(), rhs.m_Types.end());
}

namespace
{
  size_t GetMaximunTypesCount() { return HEADER_TYPE_MASK + 1; }
}

void FeatureParams::SortTypes()
{
  sort(m_Types.begin(), m_Types.end());
}

void FeatureParams::FinishAddingTypes()
{
  sort(m_Types.begin(), m_Types.end());
  m_Types.erase(unique(m_Types.begin(), m_Types.end()), m_Types.end());
  if (m_Types.size() > GetMaximunTypesCount())
    m_Types.resize(GetMaximunTypesCount());
}

void FeatureParams::SetType(uint32_t t)
{
  m_Types.clear();
  m_Types.push_back(t);
}

bool FeatureParams::PopAnyType(uint32_t & t)
{
  t = m_Types.back();
  m_Types.pop_back();
  return m_Types.empty();
}

bool FeatureParams::PopExactType(uint32_t t)
{
  m_Types.erase(remove(m_Types.begin(), m_Types.end(), t), m_Types.end());
  return m_Types.empty();
}

bool FeatureParams::IsTypeExist(uint32_t t) const
{
  return (find(m_Types.begin(), m_Types.end(), t) != m_Types.end());
}

bool FeatureParams::operator == (FeatureParams const & rhs) const
{
  return (FeatureParamsBase::operator ==(rhs) &&
          m_Types == rhs.m_Types &&
          m_Geom == rhs.m_Geom);
}

bool FeatureParams::CheckValid() const
{
  CHECK(!m_Types.empty() && m_Types.size() <= GetMaximunTypesCount(), ());
  CHECK(m_Geom != GEOM_UNDEFINED, ());

  return FeatureParamsBase::CheckValid();
}

uint8_t FeatureParams::GetHeader() const
{
  uint8_t header = static_cast<uint8_t>(m_Types.size() - 1);

  if (!name.IsEmpty())
    header |= HEADER_HAS_NAME;

  if (layer != 0)
    header |= HEADER_HAS_LAYER;

  switch (m_Geom)
  {
  case GEOM_POINT:
    header |= HEADER_GEOM_POINT;
    if (rank != 0) header |= HEADER_HAS_ADDINFO;
    break;
  case GEOM_LINE:
    header |= HEADER_GEOM_LINE;
    if (!ref.empty()) header |= HEADER_HAS_ADDINFO;
    break;
  case GEOM_AREA:
    header |= HEADER_GEOM_AREA;
    if (!house.IsEmpty()) header |= HEADER_HAS_ADDINFO;
    break;
  }

  return header;
}
