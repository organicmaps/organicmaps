#pragma once

#include "geocoder/hierarchy.hpp"
#include "geocoder/result.hpp"
#include "geocoder/types.hpp"

#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"

#include <string>
#include <unordered_map>
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
    Context(std::string const & query);

    void Clear();

    std::vector<Type> & GetTokenTypes();
    size_t GetNumTokens() const;
    size_t GetNumUsedTokens() const;

    strings::UniString const & GetToken(size_t id) const;

    void MarkToken(size_t id, Type type);

    // Returns true if |token| is marked as used.
    bool IsTokenUsed(size_t id) const;

    // Returns true iff all tokens are used.
    bool AllTokensUsed() const;

    void AddResult(base::GeoObjectId const & osmId, double certainty);

    void FillResults(std::vector<Result> & results) const;

    std::vector<Layer> & GetLayers();

    std::vector<Layer> const & GetLayers() const;

  private:
    // todo(@m) std::string?
    std::vector<strings::UniString> m_tokens;
    std::vector<Type> m_tokenTypes;

    size_t m_numUsedTokens = 0;

    // The highest value of certainty for each retrieved osm id.
    std::unordered_map<base::GeoObjectId, double> m_results;

    std::vector<Layer> m_layers;
  };

  explicit Geocoder(std::string pathToJsonHierarchy);

  void ProcessQuery(std::string const & query, std::vector<Result> & results) const;

  Hierarchy const & GetHierarchy() const;

private:
  void Go(Context & ctx, Type type) const;

  void EmitResult() const;

  Hierarchy m_hierarchy;
};
}  // namespace geocoder
