#include "feature_merger.hpp"

#include "../indexer/point_to_int64.hpp"


MergedFeatureBuilder1::MergedFeatureBuilder1(FeatureBuilder1 const & fb)
  : FeatureBuilder1(fb)
{
  m_Params.SortTypes();
}

void MergedFeatureBuilder1::AppendFeature(MergedFeatureBuilder1 const & fb)
{
  bool reverse = false;
  if (LastPoint().SquareLength(fb.FirstPoint()) > LastPoint().SquareLength(fb.LastPoint()))
    reverse = true;

  int const size = fb.m_Geometry.size();
  if (reverse)
  {
    for (int i = size-2; i >=0; --i)
      AddPoint(fb.m_Geometry[i]);
  }
  else
  {
    for (int i = 1; i < size; ++i)
      AddPoint(fb.m_Geometry[i]);
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
  MergedFeatureBuilder1 * p = new MergedFeatureBuilder1(fb);

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
      map_t::iterator it = m_map.find(get_key(curr.LastPoint()));
      bool isMerged = false;
      if (it != m_map.end())
      {
        // Find best feature to continue.
        for (size_t i = 0; i < it->second.size(); ++i)
        {
          MergedFeatureBuilder1 * pp = it->second[i];

          if (pp->HasType(type))
          {
            isMerged = true;
            curr.AppendFeature(*(pp));

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
