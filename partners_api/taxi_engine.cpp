#include "partners_api/taxi_engine.hpp"

#include "partners_api/citymobil_api.hpp"
#include "partners_api/freenow_api.hpp"
#include "partners_api/maxim_api.hpp"
#include "partners_api/rutaxi_api.hpp"
#include "partners_api/taxi_places_loader.hpp"
#include "partners_api/uber_api.hpp"
#include "partners_api/yandex_api.hpp"
#include "partners_api/yango_api.hpp"

#include "geometry/latlon.hpp"
#include "geometry/mercator.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

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
                        SuccessCallback const & successCallback,
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
  SuccessCallback successCallback;
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

  if (!providers.empty() || (providers.empty() && errors.empty()))
    return successCallback(providers, requestId);

  return errorCallback(errors, requestId);
}

void ResultMaker::DecrementRequestCount()
{
  CHECK_GREATER(m_requestsCount, 0, ());
  --m_requestsCount;
}

// Engine -----------------------------------------------------------------------------------------
Engine::Engine(std::vector<ProviderUrl> urls /* = {} */)
{
  AddApi<yandex::Api>(urls, Provider::Type::Yandex);
  AddApi<freenow::Api>(urls, Provider::Type::Freenow);
  AddApi<yango::Api>(urls, Provider::Type::Yango);
  AddApi<citymobil::Api>(urls, Provider::Type::Citymobil);
}

void Engine::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);

  auto it = std::find_if(m_apis.begin(), m_apis.end(), [](ApiItem const & item)
  {
    return item.m_type == Provider::Type::Rutaxi;
  });

  if (it != m_apis.end())
  {
    auto rutaxiPtr = dynamic_cast<rutaxi::Api *>(it->m_api.get());
    CHECK(rutaxiPtr != nullptr, ());
    rutaxiPtr->SetDelegate(m_delegate.get());
  }
}

/// Requests list of available products. Returns request identificator immediately.
uint64_t Engine::GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                      SuccessCallback const & successFn,
                                      ErrorCallback const & errorFn)
{
  ASSERT(successFn, ());
  ASSERT(errorFn, ());
  ASSERT(m_delegate, ());

  auto const reqId = ++m_requestId;
  auto const maker = m_maker;
  auto const pointFrom = mercator::FromLatLon(from);

  maker->Reset(reqId, m_apis.size(), successFn, errorFn);
  for (auto const & api : m_apis)
  {
    auto type = api.m_type;

    if (!IsAvailableAtPos(type, pointFrom))
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

std::vector<Provider::Type> Engine::GetProvidersAtPos(ms::LatLon const & pos) const
{
  std::vector<Provider::Type> result;
  auto const point = mercator::FromLatLon(pos);

  for (auto const & api : m_apis)
  {
    if (IsAvailableAtPos(api.m_type, point))
      result.push_back(api.m_type);
  }

  return result;
}

std::vector<Provider::Type> Engine::GetSupportedProviders() const
{
  std::vector<Provider::Type> result;
  result.reserve(m_apis.size());
  for (auto const & api : m_apis)
  {
    result.emplace_back(api.m_type);
  }

  return result;
}

bool Engine::IsAvailableAtPos(Provider::Type type, m2::PointD const & point) const
{
  if (IsDisabledAtPos(type, point))
    return false;

  return IsEnabledAtPos(type, point);
}

bool Engine::IsDisabledAtPos(Provider::Type type, m2::PointD const & point) const
{
  auto const it = FindByProviderType(type, m_apis.cbegin(), m_apis.cend());

  CHECK(it != m_apis.cend(), ());

  if (it->IsMwmDisabled(m_delegate->GetMwmId(point)))
    return true;

  auto const countryIds = m_delegate->GetCountryIds(point);
  auto const city = m_delegate->GetCityName(point);

  return it->AreAllCountriesDisabled(countryIds, city);
}

bool Engine::IsEnabledAtPos(Provider::Type type, m2::PointD const & point) const
{
  auto const it = FindByProviderType(type, m_apis.cbegin(), m_apis.cend());

  CHECK(it != m_apis.cend(), ());

  if (it->IsMwmEnabled(m_delegate->GetMwmId(point)))
    return true;

  auto const countryIds = m_delegate->GetCountryIds(point);
  auto const city = m_delegate->GetCityName(point);

  return it->IsAnyCountryEnabled(countryIds, city);
}

template <typename ApiType>
void Engine::AddApi(std::vector<ProviderUrl> const & urls, Provider::Type type)
{
  auto const it = std::find_if(urls.cbegin(), urls.cend(), [type](ProviderUrl const & item)
  {
    return item.m_type == type;
  });

  if (it != urls.cend())
    m_apis.emplace_back(type, std::make_unique<ApiType>(it->m_url));
  else
    m_apis.emplace_back(type, std::make_unique<ApiType>());
}
}  // namespace taxi
