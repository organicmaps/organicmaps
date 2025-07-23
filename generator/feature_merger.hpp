#pragma once

#include "generator/feature_builder.hpp"
#include "generator/feature_emitter_iface.hpp"

#include <map>
#include <set>
#include <utility>
#include <vector>

/// Feature builder class that used while feature type processing and merging.
class MergedFeatureBuilder : public feature::FeatureBuilder
{
  bool m_isRound;

  PointSeq m_roundBounds[2];

public:
  MergedFeatureBuilder() : m_isRound(false) {}
  MergedFeatureBuilder(feature::FeatureBuilder const & fb);

  void SetRound();
  bool IsRound() const { return m_isRound; }

  void ZeroParams() { m_params.MakeZero(); }

  void AppendFeature(MergedFeatureBuilder const & fb, bool fromBegin, bool toBack);

  bool EqualGeometry(MergedFeatureBuilder const & fb) const;

  inline bool NotEmpty() const { return !GetGeometry().empty(); }

  inline m2::PointD FirstPoint() const { return GetOuterGeometry().front(); }
  inline m2::PointD LastPoint() const { return GetOuterGeometry().back(); }

  inline bool PopAnyType(uint32_t & type) { return m_params.PopAnyType(type); }

  template <class ToDo>
  void ForEachChangeTypes(ToDo toDo)
  {
    for_each(m_params.m_types.begin(), m_params.m_types.end(), toDo);
    m_params.FinishAddingTypes();
  }

  template <class ToDo>
  void ForEachMiddlePoints(ToDo toDo) const
  {
    PointSeq const & poly = GetOuterGeometry();
    for (size_t i = 1; i < poly.size() - 1; ++i)
      toDo(poly[i]);
  }

  std::pair<m2::PointD, bool> GetKeyPoint(size_t i) const;
  size_t GetKeyPointsCount() const;

  // Used to determine which connected line to merge.
  double GetSquaredLength() const;
};

/// Feature merger.
class FeatureMergeProcessor
{
  using Key = int64_t;
  Key GetKey(m2::PointD const & p);

  MergedFeatureBuilder m_last;

  using MergedFeatureBuilders = std::vector<MergedFeatureBuilder *>;
  using KeyToMergedFeatureBuilders = std::map<Key, MergedFeatureBuilders>;
  KeyToMergedFeatureBuilders m_map;

  void Insert(m2::PointD const & pt, MergedFeatureBuilder * p);

  void Remove(Key key, MergedFeatureBuilder const * p);
  inline void Remove1(m2::PointD const & pt, MergedFeatureBuilder const * p) { Remove(GetKey(pt), p); }
  void Remove(MergedFeatureBuilder const * p);

  uint8_t m_coordBits;

public:
  FeatureMergeProcessor(uint32_t coordBits);

  void operator()(feature::FeatureBuilder const & fb);
  void operator()(MergedFeatureBuilder * p);

  void DoMerge(FeatureEmitterIFace & emitter);
};

/// Feature types corrector.
class FeatureTypesProcessor
{
  std::set<uint32_t> m_dontNormalize;
  std::map<uint32_t, uint32_t> m_mapping;

  static uint32_t GetType(char const * arr[], size_t n);

  void CorrectType(uint32_t & t) const;

  class do_change_types
  {
    FeatureTypesProcessor const & m_pr;

  public:
    do_change_types(FeatureTypesProcessor const & pr) : m_pr(pr) {}
    void operator()(uint32_t & t) { m_pr.CorrectType(t); }
  };

public:
  /// For example: highway-motorway_link-* => highway-motorway.
  void SetMappingTypes(char const * arr1[2], char const * arr2[2]);

  /// Leave original types, for example: boundary-administrative-2.
  template <size_t N>
  void SetDontNormalizeType(char const * (&arr)[N])
  {
    m_dontNormalize.insert(GetType(arr, N));
  }

  MergedFeatureBuilder * operator()(feature::FeatureBuilder const & fb);
};

namespace feature
{
/// @return false If fb became invalid (no any suitable types).
//@{
bool PreprocessForWorldMap(FeatureBuilder & fb);
bool PreprocessForCountryMap(FeatureBuilder & fb);
//@}
}  // namespace feature
