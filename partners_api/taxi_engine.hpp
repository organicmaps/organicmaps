#pragma once

#include "storage/index.hpp"

#include "partners_api/taxi_base.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace taxi
{
using SuccessfulCallback =
    std::function<void(ProvidersContainer const & products, uint64_t const requestId)>;

using ErrorCallback = std::function<void(ErrorsContainer const & errors, uint64_t const requestId)>;

/// This class is used to collect replies from all taxi apis and to call callback when all replies
/// are collected. The methods are called in callbacks on different threads, so synchronization is
/// required.
class ResultMaker
{
public:
  void Reset(uint64_t requestId, size_t requestsCount, SuccessfulCallback const & successCallback,
             ErrorCallback const & errorCallback);
  /// Reduces number of requests outstanding.
  void DecrementRequestCount(uint64_t requestId);
  /// Processes successful callback from taxi api.
  void ProcessProducts(uint64_t requestId, Provider::Type type,
                       std::vector<Product> const & products);
  /// Processes error callback from taxi api.
  void ProcessError(uint64_t requestId, Provider::Type type, ErrorCode code);
  /// Checks number of requests outstanding and calls callback when number is equal to zero.
  void MakeResult(uint64_t requestId) const;

private:
  void DecrementRequestCount();

  mutable std::mutex m_mutex;
  uint64_t m_requestId = 0;
  SuccessfulCallback m_successCallback;
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
                                SuccessfulCallback const & successFn,
                                ErrorCallback const & errorFn);

  /// Returns link which allows you to launch some taxi app.
  RideRequestLinks GetRideRequestLinks(Provider::Type type, std::string const & productId,
                                       ms::LatLon const & from, ms::LatLon const & to) const;

private:
  bool IsAllCountriesDisabled(Provider::Type type, storage::TCountriesVec const & countryIds) const;
  bool IsAnyCountryEnabled(Provider::Type type, storage::TCountriesVec const & countryIds) const;

  using ApiPtr = std::unique_ptr<ApiBase>;

  struct ApiContainerItem
  {
    ApiContainerItem(Provider::Type type, ApiPtr && api) : m_type(type), m_api(std::move(api)) {}

    Provider::Type m_type;
    ApiPtr m_api;
  };

  struct SupportedCountriesItem
  {
    Provider::Type m_type;
    storage::TCountriesVec m_countries;
  };

  std::vector<ApiContainerItem> m_apis;

  std::vector<SupportedCountriesItem> m_enabledCountries;
  std::vector<SupportedCountriesItem> m_disabledCountries;

  // Id for currently processed request.
  uint64_t m_requestId = 0;
  // Use single instance of maker for all requests, for this reason,
  // all outdated requests will be ignored.
  std::shared_ptr<ResultMaker> m_maker = std::make_shared<ResultMaker>();
};
}  // namespace taxi
