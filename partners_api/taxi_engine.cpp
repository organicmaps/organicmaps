#include "partners_api/taxi_engine.hpp"
#include "partners_api/uber_api.hpp"
#include "partners_api/yandex_api.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <initializer_list>
#include <iterator>

namespace
{
template <typename Iter>
Iter FindByProviderType(taxi::Provider::Type type, Iter first, Iter last)
{
  using IterValueType = typename std::iterator_traits<Iter>::value_type;
  return std::find_if(first, last,
                      [type](IterValueType const & item) { return item.m_type == type; });
}
}  // namespace

namespace taxi
{
// ResultMaker ------------------------------------------------------------------------------------
void ResultMaker::Reset(uint64_t requestId, size_t requestsCount,
                        SuccessfulCallback const & successCallback,
                        ErrorCallback const & errorCallback)
{
  ASSERT(successCallback, ());
  ASSERT(errorCallback, ());

  std::lock_guard<std::mutex> lock(m_mutex);

  m_requestId = requestId;
  m_requestsCount = static_cast<int8_t>(requestsCount);
  m_successCallback = successCallback;
  m_errorCallback = errorCallback;
  m_providers.clear();
  m_errors.clear();
}

void ResultMaker::DecrementRequestCount(uint64_t requestId)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_requestId != requestId)
    return;

  DecrementRequestCount();
}

void ResultMaker::ProcessProducts(uint64_t requestId, Provider::Type type,
                                  std::vector<Product> const & products)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_requestId != requestId)
    return;

  m_providers.emplace_back(type, products);

  DecrementRequestCount();
}

void ResultMaker::ProcessError(uint64_t requestId, Provider::Type type, ErrorCode code)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_requestId != requestId)
    return;

  m_errors.emplace_back(type, code);

  DecrementRequestCount();
}

void ResultMaker::MakeResult(uint64_t requestId) const
{
  SuccessfulCallback successCallback;
  ErrorCallback errorCallback;
  ProvidersContainer providers;
  ErrorsContainer errors;

  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_requestId != requestId || m_requestsCount != 0)
      return;

    successCallback = m_successCallback;
    errorCallback = m_errorCallback;
    providers = m_providers;
    errors = m_errors;
  }

  if (providers.empty())
    return errorCallback(errors, requestId);

  return successCallback(providers, requestId);
}

void ResultMaker::DecrementRequestCount()
{
  CHECK_GREATER(m_requestsCount, 0, ());
  --m_requestsCount;
}

// Engine -----------------------------------------------------------------------------------------
Engine::Engine()
{
  m_enabledCountries = {{Provider::Type::Yandex, {"Russian Federation"}}};
  m_disabledCountries = {{Provider::Type::Uber, {"Russian Federation"}}};

  m_apis.emplace_back(Provider::Type::Yandex, my::make_unique<yandex::Api>());
  m_apis.emplace_back(Provider::Type::Uber, my::make_unique<uber::Api>());
}

/// Requests list of available products. Returns request identificator immediately.
uint64_t Engine::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                      storage::TCountriesVec const & countryIds,
                                      SuccessfulCallback const & successFn,
                                      ErrorCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());

  auto const reqId = ++m_requestId;
  auto const maker = m_maker;

  maker->Reset(reqId, m_apis.size(), successFn, errorFn);
  for (auto const & api : m_apis)
  {
    auto type = api.m_type;

    if (IsAllCountriesDisabled(type, countryIds) || !IsAnyCountryEnabled(type, countryIds))
    {
      maker->DecrementRequestCount(reqId);
      maker->MakeResult(reqId);
      continue;
    }

    auto const productCallback = [type, maker, reqId](std::vector<Product> const & products)
    {
      maker->ProcessProducts(reqId, type, products);
      maker->MakeResult(reqId);
    };

    auto const errorCallback = [type, maker, reqId](ErrorCode const code)
    {
      maker->ProcessError(reqId, type, code);
      maker->MakeResult(reqId);
    };

    api.m_api->GetAvailableProducts(from, to, productCallback, errorCallback);
  }

  return reqId;
}

/// Returns link which allows you to launch some taxi app.
RideRequestLinks Engine::GetRideRequestLinks(Provider::Type type, std::string const & productId,
                                             ms::LatLon const & from, ms::LatLon const & to) const
{
  auto const it = FindByProviderType(type, m_apis.cbegin(), m_apis.cend());

  CHECK(it != m_apis.cend(), ());

  return it->m_api->GetRideRequestLinks(productId, from, to);
}

bool Engine::IsAllCountriesDisabled(Provider::Type type,
                                    storage::TCountriesVec const & countryIds) const
{
  auto const it =
      FindByProviderType(type, m_disabledCountries.cbegin(), m_disabledCountries.cend());

  if (it == m_disabledCountries.end())
    return false;

  auto const & disabledCountries = it->m_countries;
  bool isCountryDisabled = true;
  for (auto const & countryId : countryIds)
  {
    auto const countryIt =
        std::find(disabledCountries.cbegin(), disabledCountries.cend(), countryId);

    isCountryDisabled = isCountryDisabled && countryIt != disabledCountries.cend();
  }

  return isCountryDisabled;
}

bool Engine::IsAnyCountryEnabled(Provider::Type type,
                                 storage::TCountriesVec const & countryIds) const
{
  auto const it = FindByProviderType(type, m_enabledCountries.cbegin(), m_enabledCountries.cend());

  if (it == m_enabledCountries.end())
    return true;

  auto const & enabledCountries = it->m_countries;
  for (auto const & countryId : countryIds)
  {
    auto const countryIt = std::find(enabledCountries.cbegin(), enabledCountries.cend(), countryId);

    if (countryIt != enabledCountries.cend())
      return true;
  }

  return false;
}
}  // namespace taxi
