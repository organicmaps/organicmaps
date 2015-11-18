#pragma once

#include "geometry/rect2d.hpp"

#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/string.hpp"

struct FeatureID;
class FeatureType;
class Index;

namespace osm
{

class Editor final
{
  Editor();

public:
  using TInvalidateFn = function<void()>;

  static Editor & Instance();

  void SetInvalidateFn(TInvalidateFn && fn) { m_invalidateFn = move(fn); }

  void Load(string const & fullFilePath);
  // TODO(AlexZ): Synchronize Save call/make it on a separate thread.
  void Save(string const & fullFilePath) const;

  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       function<void(FeatureID const &)> const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);
  void ForEachFeatureInMwmRectAndScale(MwmSet::MwmId const & id,
                                       function<void(FeatureType &)> const & f,
                                       m2::RectD const & rect,
                                       uint32_t scale);

  /// True if feature was deleted by user.
  bool IsFeatureDeleted(FeatureID const & fid) const;

  /// Marks feature as "deleted" from MwM file.
  void DeleteFeature(FeatureType const & feature);

  /// True if feature was edited by user.
  bool IsFeatureEdited(FeatureID const & fid) const;
  /// @returns false if feature wasn't edited.
  /// @param outFeature is valid only if true was returned.
  bool GetEditedFeature(FeatureID const & fid, FeatureType & outFeature) const;

  /// Original feature with same FeatureID as newFeature is replaced by newFeature.
  void EditFeature(FeatureType & editedFeature);

  vector<feature::Metadata::EType> EditableMetadataForType(uint32_t type) const;

private:
  // TODO(AlexZ): Synchronize multithread access to these structures.
  set<FeatureID> m_deletedFeatures;
  map<FeatureID, FeatureType> m_editedFeatures;
  map<FeatureID, FeatureType> m_createdFeatures;

  /// Invalidate map viewport after edits.
  TInvalidateFn m_invalidateFn;
};  // class Editor

}  // namespace osm
