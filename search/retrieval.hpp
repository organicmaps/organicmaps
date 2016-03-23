#pragma once

#include "search/search_query_params.hpp"

#include "indexer/mwm_set.hpp"

#include "base/cancellable.hpp"

#include "std/unique_ptr.hpp"


class MwmValue;

namespace coding
{
class CompressedBitVector;
}

namespace search
{
namespace v2
{
class MwmContext;

// Retrieves from the search index corresponding to |value| all
// features matching to |params|.
unique_ptr<coding::CompressedBitVector> RetrieveAddressFeatures(
      MwmSet::MwmId const & id, MwmValue & value, my::Cancellable const & cancellable,
      SearchQueryParams const & params);

// Retrieves from the geometry index corresponding to |value| all features belonging to |rect|.
unique_ptr<coding::CompressedBitVector> RetrieveGeometryFeatures(
      MwmContext const & context, my::Cancellable const & cancellable,
      m2::RectD const & rect, int scale);
} // namespace v2
} // namespace search
