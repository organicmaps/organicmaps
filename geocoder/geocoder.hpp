#pragma once

#include "geocoder/beam.hpp"
#include "geocoder/hierarchy.hpp"
#include "geocoder/result.hpp"
#include "geocoder/types.hpp"

#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace geocoder
{
// This class performs geocoding by using the data that we are currently unable
// to distribute to mobile devices. Therefore, the class is intended to be used
// on the server side.
// On the other hand, the design is largely experimental and when the dust
// settles we may reuse some parts of it in the offline mobile application.
// In this case, a partial merge with search/ and in particular with
// search/geocoder.hpp is possible.
//
// Geocoder receives a search query and returns the osm ids of the features
// that match it. Currently, the only data source for the geocoder is
// the hierarchy of features, that is, for every feature that can be found
// the geocoder expects to have the total information about this feature
// in the region subdivision graph (e.g., country, city, street that contain a
// certain house). This hierarchy is to be obtained elsewhere.
//
// Note that search index, locality index, scale index, and, generally, mwm
// features are currently not used at all.
class Geocoder
{
public:
  // A Layer contains all entries matched by a subquery of consecutive tokens.
  struct Layer
  {
    Type m_type = Type::Count;
    std::vector<Hierarchy::Entry const *> m_entries;
  };

  // This class is very similar to the one we use in search/.
  // See search/geocoder_context.hpp.
  class Context
  {
  public:
    struct BeamKey
    {
      BeamKey(base::GeoObjectId osmId, Type type, std::vector<Type> && allTypes, bool allTokensUsed)
        : m_osmId(osmId)
        , m_type(type)
        , m_allTypes(std::move(allTypes))
        , m_allTokensUsed(allTokensUsed)
      {
        base::SortUnique(m_allTypes);
      }

      base::GeoObjectId m_osmId;
      Type m_type;
      std::vector<Type> m_allTypes;
      bool m_allTokensUsed;
    };

    Context(std::string const & query);

    void Clear();

    std::vector<Type> & GetTokenTypes();
    size_t GetNumTokens() const;
    size_t GetNumUsedTokens() const;

    Type GetTokenType(size_t id) const;

    std::string const & GetToken(size_t id) const;

    void MarkToken(size_t id, Type type);

    // Returns true if |token| is marked as used.
    bool IsTokenUsed(size_t id) const;

    // Returns true iff all tokens are used.
    bool AllTokensUsed() const;

    void AddResult(base::GeoObjectId const & osmId, double certainty, Type type,
                   std::vector<Type> && allTypes, bool allTokensUsed);

    void FillResults(std::vector<Result> & results) const;

    std::vector<Layer> & GetLayers();

    std::vector<Layer> const & GetLayers() const;

    void SetHouseNumberBit() { m_surelyGotHouseNumber = true; }

  private:
    Tokens m_tokens;
    std::vector<Type> m_tokenTypes;

    size_t m_numUsedTokens = 0;

    // Sticky bit that records a heuristic check whether
    // the current query contains a house number.
    // The rationale is that we must only emit buildings in this case
    // and implement a fallback to a more powerful geocoder if we
    // could not find a building.
    bool m_surelyGotHouseNumber = false;

    // The highest value of certainty for a fixed amount of
    // the most relevant retrieved osm ids.
    Beam<BeamKey, double> m_beam;

    std::vector<Layer> m_layers;
  };

  explicit Geocoder(std::string const & pathToJsonHierarchy);

  void ProcessQuery(std::string const & query, std::vector<Result> & results) const;

  Hierarchy const & GetHierarchy() const;

private:
  void Go(Context & ctx, Type type) const;

  void FillBuildingsLayer(Context & ctx, Tokens const & subquery, Layer & curLayer) const;

  void FillRegularLayer(Context const & ctx, Type type, Tokens const & subquery,
                        Layer & curLayer) const;

  Hierarchy m_hierarchy;
};
}  // namespace geocoder
