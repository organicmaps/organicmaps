#include "feature_merger.hpp"

#include "../indexer/point_to_int64.hpp"
#include "../indexer/classificator.hpp"


MergedFeatureBuilder1::MergedFeatureBuilder1(FeatureBuilder1 const & fb, bool isOK)
  : FeatureBuilder1(fb), m_isOK(isOK)
{
  m_Params.FinishAddingTypes();
}

void MergedFeatureBuilder1::AppendFeature(MergedFeatureBuilder1 const & fb, bool toBack)
{
  if (fb.m_isOK)
    m_isOK = true;

  m2::PointD const pt = toBack ? LastPoint() : FirstPoint();

  bool fromEnd = false;
  if ((pt.SquareLength(fb.FirstPoint()) > pt.SquareLength(fb.LastPoint())) == toBack)
    fromEnd = true;

  for (size_t i = 0; i < fb.m_Geometry.size(); ++i)
    m_LimitRect.Add(fb.m_Geometry[i]);

  if (fromEnd)
  {
    if (toBack)
      m_Geometry.insert(m_Geometry.end(), fb.m_Geometry.rbegin() + 1, fb.m_Geometry.rend());
    else
      m_Geometry.insert(m_Geometry.begin(), fb.m_Geometry.rbegin(), fb.m_Geometry.rend() - 1);
  }
  else
  {
    if (toBack)
      m_Geometry.insert(m_Geometry.end(), fb.m_Geometry.begin() + 1, fb.m_Geometry.end());
    else
      m_Geometry.insert(m_Geometry.begin(), fb.m_Geometry.begin(), fb.m_Geometry.end() - 1);
  }
}

bool MergedFeatureBuilder1::EqualGeometry(MergedFeatureBuilder1 const & fb) const
{
  return (m_Geometry == fb.m_Geometry);
}


FeatureMergeProcessor::key_t FeatureMergeProcessor::get_key(m2::PointD const & p)
{
  return PointToInt64(p, m_coordBits);
}

FeatureMergeProcessor::FeatureMergeProcessor(uint32_t coordBits)
: m_coordBits(coordBits)
{
}

void FeatureMergeProcessor::operator() (FeatureBuilder1 const & fb)
{
  this->operator() (new MergedFeatureBuilder1(fb, true));
}

void FeatureMergeProcessor::operator() (MergedFeatureBuilder1 * p)
{
  key_t const k1 = get_key(p->FirstPoint());
  key_t const k2 = get_key(p->LastPoint());

  m_map[k1].push_back(p);
  if (k1 != k2)
    m_map[k2].push_back(p);
}

void FeatureMergeProcessor::Remove(key_t key, MergedFeatureBuilder1 const * p)
{
  map_t::iterator i = m_map.find(key);
  CHECK(i != m_map.end(), ());

  vector_t & v = i->second;
  v.erase(remove(v.begin(), v.end(), p), v.end());
  if (v.empty()) m_map.erase(i);
}

void FeatureMergeProcessor::Remove(MergedFeatureBuilder1 const * p)
{
  key_t const k1 = get_key(p->FirstPoint());
  key_t const k2 = get_key(p->LastPoint());

  Remove(k1, p);
  if (k1 != k2)
    Remove(k2, p);
}

void FeatureMergeProcessor::DoMerge(FeatureEmitterIFace & emitter)
{
  while (!m_map.empty())
  {
    // Get any starting feature.
    vector_t & vS = m_map.begin()->second;
    CHECK(!vS.empty(), ());
    MergedFeatureBuilder1 * p = vS.front();  // may be 'back' is better

    // Remove next processing type. If it's a last type - remove from map.
    uint32_t type;
    bool isRemoved = false;
    if (p->PopAnyType(type))
    {
      isRemoved = true;
      Remove(p);
    }

    // We will merge to the copy of p.
    MergedFeatureBuilder1 curr(*p);
    curr.SetType(type);

    // Cycle while merging.
    while (true)
    {
      bool isMerged = false;

      bool toBack = true;
      map_t::iterator it = m_map.find(get_key(curr.LastPoint()));
      if (it == m_map.end())
      {
        it = m_map.find(get_key(curr.FirstPoint()));
        toBack = false;
      }

      if (it != m_map.end())
      {
        // Find best feature to continue.
        for (size_t i = 0; i < it->second.size(); ++i)
        {
          MergedFeatureBuilder1 * pp = it->second[i];

          if (pp->HasType(type))
          {
            isMerged = true;
            curr.AppendFeature(*pp, toBack);

            if (pp->PopExactType(type))
            {
              Remove(pp);
              delete pp;
            }
            break;
          }
        }
      }

      if (!isMerged)
        break;
    }

    // Push feature p to output.
    if (m_last.NotEmpty() && m_last.EqualGeometry(curr))
      m_last.AddType(type);
    else
    {
      if (m_last.NotEmpty())
        emitter(m_last);

      m_last = curr;
    }

    // Delete if the feature was removed from map.
    if (isRemoved) delete p;
  }

  if (m_last.NotEmpty())
    emitter(m_last);
}


uint32_t FeatureTypesProcessor::GetType(char const * arr[2])
{
  uint32_t const type = classif().GetTypeByPath(vector<string>(arr, arr + 2));
  CHECK_NOT_EQUAL(type, ftype::GetEmptyValue(), ());
  return type;
}

void FeatureTypesProcessor::CorrectType(uint32_t & t) const
{
  // 1. get normalized type:
  // highway-motorway-bridge => highway-motorway
  uint32_t normal = ftype::GetEmptyValue();
  uint8_t v;
  if (!ftype::GetValue(t, 0, v)) return;
  ftype::PushValue(normal, v);
  if (!ftype::GetValue(t, 1, v)) return;
  ftype::PushValue(normal, v);

  t = normal;

  // 2. get mapping type:
  map<uint32_t, uint32_t>::const_iterator i = m_mapping.find(t);
  if (i != m_mapping.end()) t = i->second;
}

void FeatureTypesProcessor::SetMappingTypes(char const * arr1[2], char const * arr2[2])
{
  m_mapping[GetType(arr1)] = GetType(arr2);
}

MergedFeatureBuilder1 * FeatureTypesProcessor::operator() (FeatureBuilder1 const & fb)
{
  MergedFeatureBuilder1 * p = new MergedFeatureBuilder1(fb, true);

  p->ForEachChangeTypes(do_change_types(*this));

  return p;
}
