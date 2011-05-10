#pragma once
#include "feature_emitter_iface.hpp"

#include "../indexer/feature.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"


class MergedFeatureBuilder1 : public FeatureBuilder1
{
public:
  MergedFeatureBuilder1() {}
  MergedFeatureBuilder1(FeatureBuilder1 const & fb);

  void AppendFeature(MergedFeatureBuilder1 const & fb);

  bool EqualGeometry(MergedFeatureBuilder1 const & fb) const;

  inline bool NotEmpty() const { return !m_Geometry.empty(); }

  inline m2::PointD FirstPoint() const { return m_Geometry.front(); }
  inline m2::PointD LastPoint() const { return m_Geometry.back(); }

  inline void SetType(uint32_t type) { m_Params.SetType(type); }
  inline bool PopAnyType(uint32_t & type) { return m_Params.PopAnyType(type); }
  inline bool PopExactType(uint32_t type) { return m_Params.PopExactType(type); }
};


class FeatureMergeProcessor
{
  typedef int64_t key_t;
  inline key_t get_key(m2::PointD const & p);

  MergedFeatureBuilder1 m_last;

  typedef vector<MergedFeatureBuilder1 *> vector_t;
  typedef map<key_t, vector_t> map_t;
  map_t m_map;

  void Remove(key_t key, MergedFeatureBuilder1 const * p);
  void Remove(MergedFeatureBuilder1 const * p);

  uint32_t m_coordBits;

public:
  FeatureMergeProcessor(uint32_t coordBits);

  void operator() (FeatureBuilder1 const & fb);

  void DoMerge(FeatureEmitterIFace & emitter);
};
