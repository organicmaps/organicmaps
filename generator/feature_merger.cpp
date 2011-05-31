#include "feature_merger.hpp"

#include "../indexer/point_to_int64.hpp"
#include "../indexer/classificator.hpp"


MergedFeatureBuilder1::MergedFeatureBuilder1(FeatureBuilder1 const & fb)
  : FeatureBuilder1(fb), m_isRound(false)
{
  m_Params.FinishAddingTypes();
}

void MergedFeatureBuilder1::SetRound()
{
  m_isRound = true;
  m_roundBounds[0] = m_roundBounds[1] = m_Geometry;
}

void MergedFeatureBuilder1::AppendFeature(MergedFeatureBuilder1 const & fb, bool fromBegin, bool toBack)
{
  if (fb.m_isRound)
  {
    if (toBack)
      m_roundBounds[1] = fb.m_Geometry;
    else
      m_roundBounds[0] = fb.m_Geometry;
    return;
  }

  if (toBack)
    m_roundBounds[1].clear();
  else
    m_roundBounds[0].clear();

  m_isRound = false;

  for (size_t i = 0; i < fb.m_Geometry.size(); ++i)
    m_LimitRect.Add(fb.m_Geometry[i]);

  if (fromBegin)
  {
    if (toBack)
      m_Geometry.insert(m_Geometry.end(), fb.m_Geometry.begin() + 1, fb.m_Geometry.end());
    else
      m_Geometry.insert(m_Geometry.begin(), fb.m_Geometry.begin(), fb.m_Geometry.end() - 1);
  }
  else
  {
    if (toBack)
      m_Geometry.insert(m_Geometry.end(), fb.m_Geometry.rbegin() + 1, fb.m_Geometry.rend());
    else
      m_Geometry.insert(m_Geometry.begin(), fb.m_Geometry.rbegin(), fb.m_Geometry.rend() - 1);
  }
}

bool MergedFeatureBuilder1::EqualGeometry(MergedFeatureBuilder1 const & fb) const
{
  return (m_Geometry == fb.m_Geometry);
}

pair<m2::PointD, bool> MergedFeatureBuilder1::GetKeyPoint(size_t i) const
{
  size_t sz = m_roundBounds[0].size();
  if (i < sz) return make_pair(m_roundBounds[0][i], false);
  i -= sz;

  if (i == 0) return make_pair(FirstPoint(), false);

  sz = m_roundBounds[1].size();
  if (i < sz) return make_pair(m_roundBounds[1][i], true);

  i -= sz;

  ASSERT_EQUAL ( i, 1, () );
  return make_pair(LastPoint(), true);
}

size_t MergedFeatureBuilder1::GetKeyPointsCount() const
{
  return m_roundBounds[0].size() + m_roundBounds[1].size() + 2;
}

double MergedFeatureBuilder1::GetPriority() const
{
  double pr = 0.0;
  for (size_t i = 1; i < m_Geometry.size(); ++i)
    pr += m_Geometry[i-1].SquareLength(m_Geometry[i]);
  return pr;
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
  this->operator() (new MergedFeatureBuilder1(fb));
}

void FeatureMergeProcessor::operator() (MergedFeatureBuilder1 * p)
{
  key_t const k1 = get_key(p->FirstPoint());
  key_t const k2 = get_key(p->LastPoint());

  m_map[k1].push_back(p);
  if (k1 != k2)
    m_map[k2].push_back(p);
  else
  {
    ///@ todo Do it only for small round features!
    p->SetRound();

    p->ForEachMiddlePoints(bind(&FeatureMergeProcessor::Insert, this, _1, p));
  }
}

void FeatureMergeProcessor::Insert(m2::PointD const & pt, MergedFeatureBuilder1 * p)
{
  m_map[get_key(pt)].push_back(p);
}

void FeatureMergeProcessor::Remove(key_t key, MergedFeatureBuilder1 const * p)
{
  map_t::iterator i = m_map.find(key);
  if (i != m_map.end())
  {
    vector_t & v = i->second;
    v.erase(remove(v.begin(), v.end(), p), v.end());
    if (v.empty()) m_map.erase(i);
  }
}

void FeatureMergeProcessor::Remove(MergedFeatureBuilder1 const * p)
{
  key_t const k1 = get_key(p->FirstPoint());
  key_t const k2 = get_key(p->LastPoint());

  Remove(k1, p);
  if (k1 != k2)
    Remove(k2, p);
  else
  {
    ASSERT ( p->IsRound(), () );

    p->ForEachMiddlePoints(bind(&FeatureMergeProcessor::Remove1, this, _1, p));
  }
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

    // Iterate through key points while merging.
    size_t ind = 0;
    while (ind < curr.GetKeyPointsCount())  // GetKeyPointsCount() can be different on each iteration
    {
      pair<m2::PointD, bool> const pt = curr.GetKeyPoint(ind++);
      map_t::const_iterator it = m_map.find(get_key(pt.first));

      MergedFeatureBuilder1 * pp = 0;
      if (it != m_map.end())
      {
        // Find best feature to continue.
        double bestPr = -1.0;
        for (size_t i = 0; i < it->second.size(); ++i)
        {
          MergedFeatureBuilder1 * pTest = it->second[i];
          if (pTest->HasType(type))
          {
            double const pr = pTest->GetPriority();
            ASSERT_GREATER ( pr, 0.0, () );
            if (pr > bestPr)
            {
              pp = pTest;
              bestPr = pr;
            }
          }
        }

        // Merge current feature with best feature.
        if (pp)
        {
          bool const toBack = pt.second;
          bool fromBegin = true;
          if ((pt.first.SquareLength(pp->FirstPoint()) > pt.first.SquareLength(pp->LastPoint())) == toBack)
            fromBegin = false;

          curr.AppendFeature(*pp, fromBegin, toBack);

          if (pp->PopExactType(type))
          {
            Remove(pp);
            delete pp;
          }

          // start from the beginning if we have a successful merge
          ind = 0;
        }
      }
    }

    if (m_last.NotEmpty() && m_last.EqualGeometry(curr))
    {
      // curr is equal with m_last by geometry - just add new type to m_last
      m_last.AddType(type);
    }
    else
    {
      // emit m_last and set curr as last processed feature (m_last)
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

uint32_t FeatureTypesProcessor::GetType(char const * arr[], size_t n)
{
  uint32_t const type = classif().GetTypeByPath(vector<string>(arr, arr + n));
  CHECK_NOT_EQUAL(type, ftype::GetEmptyValue(), ());
  return type;
}

void FeatureTypesProcessor::CorrectType(uint32_t & t) const
{
  if (m_dontNormilize.count(t) > 0) return;

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
  m_mapping[GetType(arr1, 2)] = GetType(arr2, 2);
}

void FeatureTypesProcessor::SetDontNormilizeType(char const * arr[3])
{
  m_dontNormilize.insert(GetType(arr, 3));
}

MergedFeatureBuilder1 * FeatureTypesProcessor::operator() (FeatureBuilder1 const & fb)
{
  MergedFeatureBuilder1 * p = new MergedFeatureBuilder1(fb);

  p->ForEachChangeTypes(do_change_types(*this));

  return p;
}
