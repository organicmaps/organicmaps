#pragma once

#include "storage/index.hpp"

#include "partners_api/taxi_base.hpp"

#include <memory>
#include <mutex>
#include <vector>

namespace taxi
{
using SuccessfullCallback =
    std::function<void(ProvidersContainer const & products, uint64_t const requestId)>;

using ErrorCallback = std::function<void(ErrorsContainer const & errors, uint64_t const requestId)>;

class ResultMaker
{
public:
  void Reset(uint64_t requestId, size_t requestsCount, SuccessfullCallback const & successCallback,
             ErrorCallback const & errorCallback);
  void DecrementRequestCount(uint64_t requestId);

  void ProcessProducts(uint64_t requestId, Provider::Type type,
                       std::vector<Product> const & products);
  void ProcessError(uint64_t requestId, Provider::Type type, ErrorCode code);

  void MakeResult(uint64_t requestId) const;

private:
  void DecrementRequestCount();

  mutable std::mutex m_mutex;
  uint64_t m_requestId = 0;
  SuccessfullCallback m_successCallback;
  ErrorCallback m_errorCallback;

  int8_t m_requestsCount = 0;
  ErrorsContainer m_errors;
  ProvidersContainer m_providers;
};

class Engine final
{
public:
  Engine();

  /// Requests list of available products. Returns request identificator immediately.
  uint64_t GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                storage::TCountriesVec const & countryIds,
                                SuccessfullCallback const & successFn,
                                ErrorCallback const & errorFn);

  /// Returns link which allows you to launch some taxi app.
  RideRequestLinks GetRideRequestLinks(Provider::Type type, std::string const & productId,
                                       ms::LatLon const & from, ms::LatLon const & to) const;

private:
  bool IsCountryDisabled(Provider::Type type, storage::TCountriesVec const & countryIds) const;
  bool IsCountryEnabled(Provider::Type type, storage::TCountriesVec const & countryIds) const;

  using ApiPtr = std::unique_ptr<ApiBase>;

  struct ApiContainerItem
  {
    ApiContainerItem(Provider::Type type, ApiPtr && api) : m_type(type), m_api(std::move(api)) {}

    Provider::Type m_type;
    ApiPtr m_api;
  };

  std::vector<ApiContainerItem> m_apis;

  struct SupportedCountriesItem
  {
    Provider::Type m_type;
    storage::TCountriesVec m_countries;
  };

  std::vector<SupportedCountriesItem> m_enabledCountries;
  std::vector<SupportedCountriesItem> m_disabledCountries;

  uint64_t m_requestId = 0;
  std::shared_ptr<ResultMaker> m_maker = std::make_shared<ResultMaker>();
};
}  // namespace taxi
