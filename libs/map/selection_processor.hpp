#pragma once

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <functional>
#include <string>
#include <vector>

class FeatureType;
class Framework;

namespace place_page
{
class Info;
}

/// Encapsulates feature selection and place_page::Info filling logic.
/// Accesses Framework internals as a friend class.
class SelectionProcessor
{
public:
  using FeatureMatcher = std::function<bool(FeatureType & ft)>;

  /// Result of classifying features found in a search rect.
  struct TapFeatures
  {
    FeatureID m_poi;       ///< Closest point feature.
    FeatureID m_building;  ///< Best (smallest/inside) building (area).
    FeatureID m_line;      ///< Closest line feature.
    FeatureID m_area;      ///< Best (smallest/inside) non-building area feature.
    /// All line candidates within search rect, sorted by distance (closest first).
    std::vector<std::pair<double, FeatureID>> m_lineCandidates;

    /// Get best feature using GetFeatureAtPoint priority: poi > line > building > area.
    FeatureID GetBest() const;
  };

  explicit SelectionProcessor(Framework const & framework);

  /// Find and classify features in @p searchRect around @p mercator.
  /// @param matcher  optional primary filter.
  TapFeatures FindFeaturesInRect(m2::PointD const & mercator, m2::RectD const & searchRect,
                                 FeatureMatcher const & matcher = nullptr) const;

  /// Get "best for the user" feature at given point.
  /// Ignores coastlines and prefers buildings over other area features.
  FeatureID GetFeatureAtPoint(m2::PointD const & mercator, FeatureMatcher const & matcher = nullptr) const;

  void FillFeatureInfo(FeatureID const & fid, place_page::Info & info) const;
  void FillInfoFromFeatureType(FeatureType & ft, place_page::Info & info) const;

  /// @param customTitle, if not empty, overrides any other calculated name.
  void FillPointInfo(place_page::Info & info, m2::PointD const & mercator, std::string const & customTitle = {},
                     FeatureMatcher const & matcher = nullptr) const;

  void FillNotMatchedPlaceInfo(place_page::Info & info, m2::PointD const & mercator,
                               std::string const & customTitle = {}) const;

  void SetPlacePageLocation(place_page::Info & info) const;

  bool CanEditMapForPosition(m2::PointD const & position) const;

private:
  void FillDescriptions(FeatureType & ft, place_page::Info & info) const;

  Framework const & m_fw;
};
