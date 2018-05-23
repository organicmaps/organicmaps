#pragma once

#include "partners_api/booking_availability_params.hpp"
#include "partners_api/booking_params_base.hpp"

#include "platform/safe_callback.hpp"

#include "indexer/feature_decl.hpp"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace search
{
class Results;
}

namespace booking
{
namespace filter
{
using Results = platform::SafeCallback<void(std::shared_ptr<ParamsBase> const & params,
                                            std::vector<FeatureID> const & sortedFeatures)>;
using ResultsUnsafe = std::function<void(search::Results const & results)>;

template <typename R>
struct ParamsImpl
{
  ParamsImpl() = default;
  ParamsImpl(std::shared_ptr<ParamsBase> params, R const & cb)
    : m_params(move(params))
    , m_callback(cb)
  {
  }

  bool IsEmpty() const { return !m_params || m_params->IsEmpty(); }

  std::shared_ptr<ParamsBase> m_params;
  R m_callback;
};

using Params = ParamsImpl<Results>;
using ParamsInternal = ParamsImpl<ResultsUnsafe>;
}  // namespace filter
}  // namespace booking
