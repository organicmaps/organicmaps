#pragma once

#include "feature_merger.hpp"

#include "../defines.hpp"

#include "../base/logging.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../indexer/point_to_int64.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/iostream.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/unordered_map.hpp"

template <class FeatureOutT>
class WorldMapGenerator
{
  /// if NULL, separate world data file is not generated
  scoped_ptr<FeatureOutT> m_worldBucket;
  /// features visible before or at this scale level will go to World map
  int m_maxWorldScale;
  bool m_mergeCoastlines;

  size_t m_mergedCounter;
  size_t m_areasCounter;

  uint32_t m_coordBits;

  typedef unordered_map<uint64_t, FeatureBuilder1Merger> FeaturesContainerT;
  typedef map<uint32_t, FeaturesContainerT> TypesContainerT;
  TypesContainerT m_features;

private:
  bool EmitAreaFeature(FeatureBuilder1Merger & fbm)
  {
    if (fbm.FirstPoint() == fbm.LastPoint())
    {
      if (fbm.SetAreaSafe())
      {
        (*m_worldBucket)(fbm);
        ++m_areasCounter;
      }
      return true;
    }
    else return false;
  }

  /// scans all features and tries to merge them with each other
  /// @return true if one feature was merged
  bool ReMergeFeatures(FeaturesContainerT & features)
  {
    for (FeaturesContainerT::iterator base = features.begin(); base != features.end(); ++base)
    {
      FeaturesContainerT::iterator found =
          features.find(PointToInt64(base->second.LastPoint(), m_coordBits));
      if (found != features.end())
      {
        CHECK(found != base, ());
        base->second.AppendFeature(found->second);
        features.erase(found);
        ++m_mergedCounter;

        if (EmitAreaFeature(base->second))
          features.erase(base);
        return true;
      }
    }
    return false;
  }

  void TryToMerge(FeatureBuilder1Merger & fbm)
  {
    FeaturesContainerT & container = m_features[fbm.KeyType()];
    FeaturesContainerT::iterator found = container.find(PointToInt64(fbm.LastPoint(), m_coordBits));
    if (found != container.end())
    {
      fbm.AppendFeature(found->second);
      container.erase(found);
      ++m_mergedCounter;
    }

    if (!EmitAreaFeature(fbm))
    {
      pair<FeaturesContainerT::iterator, bool> result =
          container.insert(make_pair(PointToInt64(fbm.FirstPoint(), m_coordBits), fbm));
      // if we found feature with the same starting point, emit it directly
      if (!result.second)
      {
        LOG(LWARNING, ("Found features with common first point, points counts are:",
                       result.first->second.GetPointsCount(), fbm.GetPointsCount()));
        (*m_worldBucket)(fbm);
      }
    }
  }

  //struct FeatureTypePrinter
  //{
  //  void operator()(uint32_t type) const
  //  {
  //    cout << classif().GetFullObjectName(type) << ".";
  //  }
  //};

  vector<uint32_t> m_MergeTypes;

public:
  WorldMapGenerator(int maxWorldScale, bool mergeCoastlines,
                    typename FeatureOutT::InitDataType featureOutInitData)
  : m_maxWorldScale(maxWorldScale), m_mergeCoastlines(mergeCoastlines),
    m_mergedCounter(0), m_areasCounter(0), m_coordBits(30)
  {
    if (maxWorldScale >= 0)
      m_worldBucket.reset(new FeatureOutT(WORLD_FILE_NAME, featureOutInitData));

    // fill vector with types that need to be merged
    static size_t const MAX_TYPES_IN_PATH = 3;
    char const * arrMerge[][MAX_TYPES_IN_PATH] = {
      {"natural", "coastline", ""},
      {"boundary", "administrative", "2"},

      {"highway", "motorway", ""},
      {"highway", "motorway_link", ""},
      {"highway", "motorway", "oneway"},
      {"highway", "motorway_link", "oneway"},

      {"highway", "primary", ""},
      {"highway", "primary_link", ""},

      {"highway", "trunk", ""},
      {"highway", "trunk_link", ""},

      {"natural", "water", ""}
    };

    for (size_t i = 0; i < ARRAY_SIZE(arrMerge); ++i)
    {
      vector<string> path;
      for (size_t j = 0; j < MAX_TYPES_IN_PATH; ++j)
      {
        string const strType(arrMerge[i][j]);
        if (!strType.empty())
          path.push_back(strType);
      }
      m_MergeTypes.push_back(classif().GetTypeByPath(path));

      ASSERT_NOT_EQUAL ( m_MergeTypes.back(), ftype::GetEmptyValue(), () );
    }

    sort(m_MergeTypes.begin(), m_MergeTypes.end());
  }

  ~WorldMapGenerator()
  {
    if (m_mergeCoastlines)
    {
      LOG(LINFO, ("Final merging of coastlines started"));
    }

    // try to merge all merged features with each other
    for (TypesContainerT::iterator it = m_features.begin(); it != m_features.end(); ++it)
    {
      LOG(LINFO, (it->second.size()));
      while (ReMergeFeatures(it->second))
      {}
      // emit all merged features
      for (FeaturesContainerT::iterator itF = it->second.begin(); itF != it->second.end(); ++itF)
        (*m_worldBucket)(itF->second);
    }

    if (m_mergeCoastlines)
    {
      LOG(LINFO, ("Final merging of coastlines ended"));
      LOG(LINFO, ("Merged features:", m_mergedCounter, "new areas created:", m_areasCounter));
    }
  }

  void operator()(FeatureBuilder1 const & fb)
  {
    if (m_worldBucket)
    {
      FeatureBase fBase = fb.GetFeatureBase();
      int const minScale = feature::MinDrawableScaleForFeature(fBase);
      CHECK_GREATER(minScale, -1, ("Non-drawable feature found!?"));

      if (m_maxWorldScale >= minScale)
      {
        if (m_mergeCoastlines && fBase.GetFeatureType() == FeatureBase::FEATURE_TYPE_LINE)
        {
          for (size_t i = 0; i < m_MergeTypes.size(); ++i)
          {
            if (fb.IsTypeExist(m_MergeTypes[i]))
            {
              FeatureBuilder1Merger fbm(fb);
              fbm.SetType(m_MergeTypes[i]);
              TryToMerge(fbm);
            }
          }
        }

        FeatureBuilder1 fbm(fb);
        if (fbm.AssignType_SetDifference(m_MergeTypes))
            (*m_worldBucket)(fbm);
      }
    }
  }
};
