#pragma once

#include "search/feature_offset_match.hpp"
#include "search/query_params.hpp"

#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"
#include "base/dfa_helpers.hpp"
#include "base/levenshtein_dfa.hpp"

#include "std/unique_ptr.hpp"

class MwmValue;

namespace coding
{
class CompressedBitVector;
}

namespace search
{
class MwmContext;
class TokenSlice;

// Following functions retrieve from the search index corresponding to
// |value| all features matching to |request|.
unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
    MwmContext const & context, my::Cancellable const & cancellable,
    SearchTrieRequest<strings::LevenshteinDFA> const & request);

unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
    MwmContext const & context, my::Cancellable const & cancellable,
    SearchTrieRequest<strings::PrefixDFAModifier<strings::LevenshteinDFA>> const & request);

// Retrieves from the search index corresponding to |value| all
// postcodes matching to |slice|.
unique_ptr<coding::CompressedBitVector> RetrievePostcodeFeatures(
    MwmContext const & context, my::Cancellable const & cancellable, TokenSlice const & slice);

// Retrieves from the geometry index corresponding to |value| all features belonging to |rect|.
unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeatures(
    MwmContext const & context, my::Cancellable const & cancellable, m2::RectD const & rect,
    int scale);
}  // namespace search
