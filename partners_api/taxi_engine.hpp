#pragma once

#include "partners_api/taxi_base.hpp"

#include "platform/safe_callback.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace taxi
{
using SuccessCallback =
    platform::SafeCallback<void(ProvidersContainer const & products, uint64_t const requestId)>;

using ErrorCallback =
    platform::SafeCallback<void(ErrorsContainer const & errors, uint64_t const requestId)>;

class Delegate
{
public:
  virtual ~Delegate() = default;

  virtual storage::TCountriesVec GetCountryIds(m2::PointD const & point) = 0;
  virtual std::string GetCityName(m2::PointD const & point) = 0;
  virtual storage::TCountryId GetMwmId(m2::PointD const & point) = 0;
};

/// This class is used to collect replies from all taxi apis and to call callback when all replies
/// are collected. The methods are called in callbacks on different threads, so synchronization is
/// required.
class ResultMaker
{
public:
  void Reset(uint64_t requestId, size_t requestsCount, SuccessCallback const & successCallback,
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
  SuccessCallback m_successCallback;
  ErrorCallback m_errorCallback;

  // The number of the number of unfinished requests to the taxi providers.
  // The maximum possible amount of requests is equal to the number of supported taxi providers.
  int8_t m_requestsCount = 0;
  ErrorsContainer m_errors;
  ProvidersContainer m_providers;
};

struct ProviderUrl
{
  Provider::Type m_type;
  std::string m_url;
};

class Engine final
{
public:
  explicit Engine(std::vector<ProviderUrl> urls = {});

  void SetDelegate(std::unique_ptr<Delegate> delegate);

  /// Requests list of available products. Returns request identificator immediately.
  uint64_t GetAvailableProducts(ms::LatLon const & from, ms::LatLon const & to,
                                SuccessCallback const & successFn, ErrorCallback const & errorFn);

  /// Returns link which allows you to launch some taxi app.
  RideRequestLinks GetRideRequestLinks(Provider::Type type, std::string const & productId,
                                       ms::LatLon const & from, ms::LatLon const & to) const;

  std::vector<Provider::Type> GetProvidersAtPos(ms::LatLon const & pos) const;

private:
  bool IsAvailableAtPos(Provider::Type type, m2::PointD const & point) const;
  bool IsDisabledAtPos(Provider::Type type, m2::PointD const & point) const;
  bool IsEnabledAtPos(Provider::Type type, m2::PointD const & point) const;

  template <typename ApiType>
  void AddApi(std::vector<ProviderUrl> const & urls, Provider::Type type);

  std::vector<ApiItem> m_apis;

  // Id for currently processed request.
  uint64_t m_requestId = 0;
  // Use single instance of maker for all requests, for this reason,
  // all outdated requests will be ignored.
  std::shared_ptr<ResultMaker> m_maker = std::make_shared<ResultMaker>();

  std::unique_ptr<Delegate> m_delegate;
};
}  // namespace taxi
