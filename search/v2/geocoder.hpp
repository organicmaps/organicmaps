#pragma once

#include "search/search_query_params.hpp"
#include "search/v2/features_layer.hpp"
#include "search/v2/search_model.hpp"

#include "indexer/mwm_set.hpp"
#include "indexer/index.hpp"

#include "coding/compressed_bit_vector.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/cancellable.hpp"
#include "base/string_utils.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

class MwmValue;

namespace coding
{
class CompressedBitVector;
}

namespace search
{
class RankTable;

namespace v2
{
class FeaturesLayerPathFinder;
class SearchModel;

class Geocoder : public my::Cancellable
{
public:
  Geocoder(Index & index);

  ~Geocoder() override;

  void SetSearchQueryParams(SearchQueryParams const & params);

  void Go(vector<FeatureID> & results);

private:
  void PrepareParams(size_t from, size_t to);

  void DoGeocoding(size_t curToken);

  coding::CompressedBitVector * RetrieveAddressFeatures(size_t curToken, size_t endToken);

  bool IsLayerSequenceSane() const;

  bool LooksLikeHouseNumber(size_t curToken, size_t endToken) const;

  void IntersectLayers();

  Index & m_index;

  // Initial search query params.
  SearchQueryParams m_params;

  // Total number of tokens (including last prefix token, if
  // non-empty).
  size_t m_numTokens;

  // This field is used to map features to a limited number of search
  // classes.
  SearchModel const & m_model;

  // Following fields are set up by Search() method and can be
  // modified and used only from Search() or it's callees.

  // Value of a current mwm.
  MwmValue * m_value;

  // Id of a current mwm.
  MwmSet::MwmId m_mwmId;

  // Cache of bit-vectors.
  unordered_map<uint64_t, unique_ptr<coding::CompressedBitVector>> m_cache;

  // Features loader.
  unique_ptr<Index::FeaturesLoaderGuard> m_loader;

  // Path finder for interpretations.
  unique_ptr<FeaturesLayerPathFinder> m_finder;

  // Search query params prepared for retrieval.
  SearchQueryParams m_retrievalParams;

  // Stack of layers filled during geocoding.
  vector<FeaturesLayer> m_layers;

  vector<FeatureID> * m_results;
};
}  // namespace v2
}  // namespace search
