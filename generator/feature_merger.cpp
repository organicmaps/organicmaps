#include "generator/feature_merger.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"

#include "indexer/ftypes_sponsored.hpp"

#include "coding/point_coding.hpp"

using namespace feature;

MergedFeatureBuilder::MergedFeatureBuilder(FeatureBuilder const & fb)
  : FeatureBuilder(fb), m_isRound(false)
{
  m_params.FinishAddingTypes();
}

void MergedFeatureBuilder::SetRound()
{
  m_isRound = true;

  m_roundBounds[0] = m_roundBounds[1] = GetOuterGeometry();
}

void MergedFeatureBuilder::AppendFeature(MergedFeatureBuilder const & fb, bool fromBegin, bool toBack)
{
  // Also merge Osm IDs for debugging
  m_osmIds.insert(m_osmIds.end(), fb.m_osmIds.begin(), fb.m_osmIds.end());

  PointSeq & thisG = m_polygons.front();
  PointSeq const & fbG = fb.GetOuterGeometry();

  if (fb.m_isRound)
  {
    if (toBack)
      m_roundBounds[1] = fbG;
    else
      m_roundBounds[0] = fbG;
    return;
  }

  if (toBack)
    m_roundBounds[1].clear();
  else
    m_roundBounds[0].clear();

  m_isRound = false;

  using namespace feature;

  if (fromBegin)
  {
    if (toBack)
    {
      thisG.insert(thisG.end(), fbG.begin() + 1, fbG.end());
      CalcRect(fbG.begin() + 1, fbG.end(), m_limitRect);
    }
    else
    {
      thisG.insert(thisG.begin(), fbG.begin(), fbG.end() - 1);
      CalcRect(fbG.begin(), fbG.end() - 1, m_limitRect);
    }
  }
  else
  {
    if (toBack)
    {
      thisG.insert(thisG.end(), fbG.rbegin() + 1, fbG.rend());
      CalcRect(fbG.rbegin() + 1, fbG.rend(), m_limitRect);
    }
    else
    {
      thisG.insert(thisG.begin(), fbG.rbegin(), fbG.rend() - 1);
      CalcRect(fbG.rbegin(), fbG.rend() - 1, m_limitRect);
    }
  }
}

bool MergedFeatureBuilder::EqualGeometry(MergedFeatureBuilder const & fb) const
{
  return (GetOuterGeometry() == fb.GetOuterGeometry());
}

std::pair<m2::PointD, bool> MergedFeatureBuilder::GetKeyPoint(size_t i) const
{
  // 1. check first rounds
  size_t sz = m_roundBounds[0].size();
  if (i < sz)
    return std::make_pair(m_roundBounds[0][i], false);
  i -= sz;

  // 2. check first point
  if (i == 0)
    return std::make_pair(FirstPoint(), false);

  // 3. check last rounds
  --i;
  sz = m_roundBounds[1].size();
  if (i < sz)
    return std::make_pair(m_roundBounds[1][i], true);
  i -= sz;

  // 4. return last point
  ASSERT_EQUAL ( i, 0, () );
  return std::make_pair(LastPoint(), true);
}

size_t MergedFeatureBuilder::GetKeyPointsCount() const
{
  return m_roundBounds[0].size() + m_roundBounds[1].size() + 2;
}

double MergedFeatureBuilder::GetPriority() const
{
  PointSeq const & poly = GetOuterGeometry();

  double pr = 0.0;
  for (size_t i = 1; i < poly.size(); ++i)
    pr += poly[i-1].SquaredLength(poly[i]);
  return pr;
}


FeatureMergeProcessor::Key FeatureMergeProcessor::GetKey(m2::PointD const & p)
{
  return PointToInt64Obsolete(p, m_coordBits);
}

FeatureMergeProcessor::FeatureMergeProcessor(uint32_t coordBits)
: m_coordBits(coordBits)
{
}

void FeatureMergeProcessor::operator() (FeatureBuilder const & fb)
{
  this->operator() (new MergedFeatureBuilder(fb));
}

void FeatureMergeProcessor::operator() (MergedFeatureBuilder * p)
{
  Key const k1 = GetKey(p->FirstPoint());
  Key const k2 = GetKey(p->LastPoint());

  m_map[k1].push_back(p);
  if (k1 != k2)
    m_map[k2].push_back(p);
  else
  {
    ///@ todo Do it only for small round features!
    p->SetRound();

    p->ForEachMiddlePoints(std::bind(&FeatureMergeProcessor::Insert, this, std::placeholders::_1, p));
  }
}

void FeatureMergeProcessor::Insert(m2::PointD const & pt, MergedFeatureBuilder * p)
{
  m_map[GetKey(pt)].push_back(p);
}

void FeatureMergeProcessor::Remove(Key key, MergedFeatureBuilder const * p)
{
  auto i = m_map.find(key);
  if (i != m_map.end())
  {
    MergedFeatureBuilders & v = i->second;
    v.erase(remove(v.begin(), v.end(), p), v.end());
    if (v.empty()) m_map.erase(i);
  }
}

void FeatureMergeProcessor::Remove(MergedFeatureBuilder const * p)
{
  Key const k1 = GetKey(p->FirstPoint());
  Key const k2 = GetKey(p->LastPoint());

  Remove(k1, p);
  if (k1 != k2)
    Remove(k2, p);
  else
  {
    ASSERT ( p->IsRound(), () );

    p->ForEachMiddlePoints(std::bind(&FeatureMergeProcessor::Remove1, this, std::placeholders::_1, p));
  }
}

void FeatureMergeProcessor::DoMerge(FeatureEmitterIFace & emitter)
{
  while (!m_map.empty())
  {
    // Get any starting feature.
    MergedFeatureBuilders & vS = m_map.begin()->second;
    CHECK(!vS.empty(), ());
    MergedFeatureBuilder * p = vS.front();  // may be 'back' is better

    // Remove next processing type. If it's a last type - remove from map.
    uint32_t type;
    bool isRemoved = false;
    if (p->PopAnyType(type))
    {
      isRemoved = true;
      Remove(p);
    }

    // We will merge to the copy of p.
    MergedFeatureBuilder curr(*p);
    curr.SetType(type);

    // Iterate through key points while merging.
    size_t ind = 0;
    while (ind < curr.GetKeyPointsCount())  // GetKeyPointsCount() can be different on each iteration
    {
      std::pair<m2::PointD, bool> const pt = curr.GetKeyPoint(ind++);
      auto it = m_map.find(GetKey(pt.first));

      MergedFeatureBuilder * pp = 0;
      if (it != m_map.end())
      {
        // Find best feature to continue.
        double bestPr = -1.0;
        for (size_t i = 0; i < it->second.size(); ++i)
        {
          MergedFeatureBuilder * pTest = it->second[i];
          if (pTest->HasType(type))
          {
            double const pr = pTest->GetPriority();
            // It's not necessery assert, because it's possible in source data
//            ASSERT_GREATER ( pr, 0.0, () );
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
          if ((pt.first.SquaredLength(pp->FirstPoint()) > pt.first.SquaredLength(pp->LastPoint())) == toBack)
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
      if (!m_last.HasType(type))
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
  uint32_t const type = classif().GetTypeByPath(std::vector<std::string>(arr, arr + n));
  CHECK_NOT_EQUAL(type, ftype::GetEmptyValue(), ());
  return type;
}

void FeatureTypesProcessor::CorrectType(uint32_t & t) const
{
  if (m_dontNormalize.count(t) > 0) return;

  // 1. get normalized type:
  // highway-motorway-bridge => highway-motorway
  ftype::TruncValue(t, 2);

  // 2. get mapping type:
  auto i = m_mapping.find(t);
  if (i != m_mapping.end())
    t = i->second;
}

void FeatureTypesProcessor::SetMappingTypes(char const * arr1[2], char const * arr2[2])
{
  m_mapping[GetType(arr1, 2)] = GetType(arr2, 2);
}

MergedFeatureBuilder * FeatureTypesProcessor::operator() (FeatureBuilder const & fb)
{
  MergedFeatureBuilder * p = new MergedFeatureBuilder(fb);

  p->ForEachChangeTypes(do_change_types(*this));

  // do preprocessing after types correction
  if (!feature::PreprocessForWorldMap(*p))
  {
    delete p;
    return 0;
  }

  // zero all additional params for world merged features (names, ranks, ...)
  p->ZeroParams();

  return p;
}


namespace feature
{

class RemoveSolver
{
  int m_lowScale, m_upScale;
  bool m_doNotRemoveSpecialTypes;
  bool m_doNotRemoveSponsoredTypes;

public:
  RemoveSolver(int lowScale, int upScale, bool doNotRemoveSpecialTypes, bool doNotRemoveSponsoredTypes)
    : m_lowScale(lowScale)
    , m_upScale(upScale)
    , m_doNotRemoveSpecialTypes(doNotRemoveSpecialTypes)
    , m_doNotRemoveSponsoredTypes(doNotRemoveSponsoredTypes)
  {
  }

  bool operator() (uint32_t type) const
  {
    if (m_doNotRemoveSponsoredTypes && ftypes::IsSponsoredChecker::Instance()(type))
      return false;

    std::pair<int, int> const range = feature::GetDrawableScaleRange(type);
    // We have feature types without any drawing rules.
    // This case was processed before:
    // - feature::TypeAlwaysExists;
    // - FeatureBuilder::RemoveInvalidTypes;
    // Don't delete them here.
    if (m_doNotRemoveSpecialTypes && range.first == -1)
    {
      ASSERT(range.second == -1, ());
      return false;
    }

    // Remove when |type| is invisible.
    return (range.first == -1 || (range.first > m_upScale || range.second < m_lowScale));
  }
};

bool PreprocessForWorldMap(FeatureBuilder & fb)
{
  int const upperScale = scales::GetUpperWorldScale();

  if (fb.RemoveTypesIf(RemoveSolver(0, upperScale, false /* doNotRemoveSpecialTypes */,
                                    true /* doNotRemoveSponsoredTypes */)))
  {
    return false;
  }

  fb.RemoveNameIfInvisible(0, upperScale);

  return true;
}

bool PreprocessForCountryMap(FeatureBuilder & fb)
{
  using namespace scales;

  if (fb.RemoveTypesIf(RemoveSolver(GetUpperWorldScale() + 1, GetUpperStyleScale(),
                                    true /* doNotRemoveSpecialTypes */,
                                    true /* doNotRemoveSponsoredTypes */)))
  {
    return false;
  }

  return true;
}
}
