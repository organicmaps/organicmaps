#include "testing/testing.hpp"

#include "partners_api/taxi_engine.hpp"
#include "partners_api/uber_api.hpp"
#include "partners_api/yandex_api.hpp"

#include "geometry/latlon.hpp"

#include "base/scope_guard.hpp"

namespace
{
std::vector<taxi::Product> GetUberSynchronous(ms::LatLon const & from, ms::LatLon const & to)
{
  std::string times;
  std::string prices;

  TEST(taxi::uber::RawApi::GetEstimatedTime(from, times), ());
  TEST(taxi::uber::RawApi::GetEstimatedPrice(from, to, prices), ());

  size_t reqId = 0;
  taxi::uber::ProductMaker maker;
  std::vector<taxi::Product> uberProducts;

  maker.Reset(reqId);
  maker.SetTimes(reqId, times);
  maker.SetPrices(reqId, prices);
  maker.MakeProducts(
      reqId, [&uberProducts](vector<taxi::Product> const & products) { uberProducts = products; },
      [](taxi::ErrorCode const code) { TEST(false, (code)); });

  return uberProducts;
}

std::vector<taxi::Product> GetYandexSynchronous(ms::LatLon const & from, ms::LatLon const & to)
{
  std::string yandexAnswer;
  std::vector<taxi::Product> yandexProducts;

  TEST(taxi::yandex::RawApi::GetTaxiInfo(from, to, yandexAnswer), ());

  taxi::yandex::MakeFromJson(yandexAnswer, yandexProducts);

  return yandexProducts;
}

taxi::ProvidersContainer GetProvidersSynchronous(ms::LatLon const & from, ms::LatLon const & to)
{
  taxi::ProvidersContainer providers;

  providers.emplace_back(taxi::Provider::Type::Uber, GetUberSynchronous(from, to));
  providers.emplace_back(taxi::Provider::Type::Yandex, GetYandexSynchronous(from, to));

  return providers;
}

void CompareProviders(taxi::ProvidersContainer const & providersContainer,
                      taxi::ProvidersContainer const & synchronousProviders)
{
  TEST_EQUAL(synchronousProviders.size(), providersContainer.size(), ());

  for (auto const & sp : synchronousProviders)
  {
    auto const it = std::find_if(
        providersContainer.cbegin(), providersContainer.cend(), [&sp](taxi::Provider const & p) {
          if (p.GetType() != sp.GetType())
            return false;

          auto const & spp = sp.GetProducts();
          auto const & pp = p.GetProducts();

          TEST_EQUAL(spp.size(), pp.size(), ());

          for (auto const & sprod : spp)
          {
            auto const prodIt =
                std::find_if(pp.cbegin(), pp.cend(), [&sprod](taxi::Product const & prod) {
                  return sprod.m_productId == prod.m_productId && sprod.m_name == prod.m_name &&
                         sprod.m_price == prod.m_price;
                });

            if (prodIt == pp.cend())
              return false;
          }

          return true;
        });

    TEST(it != providersContainer.cend(), ());
  }
}

UNIT_TEST(TaxiEngine_ResultMaker)
{
  taxi::ResultMaker maker;
  uint64_t reqId = 1;
  taxi::ProvidersContainer providers;
  taxi::ErrorsContainer errors;

  auto const successCallback = [&reqId, &providers](taxi::ProvidersContainer const & products,
                                                    uint64_t const requestId) {
    TEST_EQUAL(reqId, requestId, ());
    providers = products;
  };

  auto const successNotPossibleCallback =
      [&reqId, &providers](taxi::ProvidersContainer const & products, uint64_t const requestId) {
        TEST(false, ("successNotPossibleCallback", requestId));
      };

  auto const errorCallback = [&reqId, &errors](taxi::ErrorsContainer const e,
                                               uint64_t const requestId) {
    TEST_EQUAL(reqId, requestId, ());
    errors = e;
  };

  auto const errorNotPossibleCallback = [&reqId](taxi::ErrorsContainer const errors,
                                                 uint64_t const requestId) {
    TEST(false, ("errorNotPossibleCallback", requestId, errors));
  };

  // Try to image what products1 and products2 are lists of products for different taxi providers.
  // Only product id is important for us, all other fields are empty.
  std::vector<taxi::Product> products1 =
  {
    {"1", "", "", "", ""},
    {"2", "", "", "", ""},
    {"3", "", "", "", ""},
  };

  std::vector<taxi::Product> products2 =
  {
    {"4", "", "", "", ""},
    {"5", "", "", "", ""},
    {"6", "", "", "", ""},
  };

  maker.Reset(reqId, 2, successCallback, errorNotPossibleCallback);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Uber, products1);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);

  TEST(providers.empty(), ());
  TEST(errors.empty(), ());

  maker.Reset(reqId, 3, successCallback, errorNotPossibleCallback);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Uber, products1);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);
  maker.DecrementRequestCount(reqId);
  maker.MakeResult(reqId);

  TEST_EQUAL(providers.size(), 2, ());
  TEST_EQUAL(providers[0].GetType(), taxi::Provider::Type::Uber, ());
  TEST_EQUAL(providers[1].GetType(), taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(providers[0][0].m_productId, "1", ());
  TEST_EQUAL(providers[0][1].m_productId, "2", ());
  TEST_EQUAL(providers[0][2].m_productId, "3", ());
  TEST_EQUAL(providers[1][0].m_productId, "4", ());
  TEST_EQUAL(providers[1][1].m_productId, "5", ());
  TEST_EQUAL(providers[1][2].m_productId, "6", ());

  maker.Reset(reqId, 2, successCallback, errorNotPossibleCallback);
  maker.ProcessError(reqId, taxi::Provider::Type::Uber, taxi::ErrorCode::NoProducts);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);
  maker.MakeResult(reqId);

  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0].GetType(), taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(providers[0][0].m_productId, "4", ());
  TEST_EQUAL(providers[0][1].m_productId, "5", ());
  TEST_EQUAL(providers[0][2].m_productId, "6", ());

  maker.Reset(reqId, 2, successNotPossibleCallback, errorCallback);
  maker.ProcessError(reqId, taxi::Provider::Type::Uber, taxi::ErrorCode::NoProducts);
  maker.ProcessError(reqId, taxi::Provider::Type::Yandex, taxi::ErrorCode::RemoteError);
  maker.MakeResult(reqId);

  TEST_EQUAL(errors.size(), 2, ());
  TEST_EQUAL(errors[0].m_type, taxi::Provider::Type::Uber, ());
  TEST_EQUAL(errors[0].m_code, taxi::ErrorCode::NoProducts, ());
  TEST_EQUAL(errors[1].m_type, taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(errors[1].m_code, taxi::ErrorCode::RemoteError, ());
}

UNIT_TEST(TaxiEngine_Smoke)
{
  // Used to synchronize access into GetAvailableProducts callback method.
  std::mutex resultsMutex;
  size_t reqId = 1;
  taxi::ProvidersContainer providersContainer;
  ms::LatLon const from(38.897724, -77.036531);
  ms::LatLon const to(38.862416, -76.883316);

  taxi::uber::SetUberUrlForTesting("http://localhost:34568/partners");
  taxi::yandex::SetYandexUrlForTesting("http://localhost:34568/partners");

  MY_SCOPE_GUARD(cleanupUber, []() { taxi::uber::SetUberUrlForTesting(""); });
  MY_SCOPE_GUARD(cleanupYandex, []() { taxi::yandex::SetYandexUrlForTesting(""); });

  auto const errorCallback = [](taxi::ErrorsContainer const & errors, uint64_t const requestId) {
    TEST(false, ());
  };

  auto const errorPossibleCallback = [](taxi::ErrorsContainer const & errors,
                                        uint64_t const requestId) {
    for (auto const & error : errors)
      TEST(error.m_code == taxi::ErrorCode::NoProducts, ());
  };

  auto const standardCallback = [&reqId, &providersContainer, &resultsMutex](
                                    taxi::ProvidersContainer const & providers,
                                    uint64_t const requestId) {
    std::lock_guard<std::mutex> lock(resultsMutex);

    if (reqId == requestId)
      providersContainer = providers;
  };

  auto const lastCallback = [&standardCallback](taxi::ProvidersContainer const & products,
                                                uint64_t const requestId) {
    standardCallback(products, requestId);
    testing::StopEventLoop();
  };

  taxi::ProvidersContainer const synchronousProviders = GetProvidersSynchronous(from, to);

  {
    taxi::Engine engine;

    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(
          ms::LatLon(55.753960, 37.624513), ms::LatLon(55.765866, 37.661270),
          {"Brazil", "Russian Federation"}, standardCallback, errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(
          ms::LatLon(59.922445, 30.367201), ms::LatLon(59.943675, 30.361123),
          {"Brazil", "Russian Federation"}, standardCallback, errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(
          ms::LatLon(52.509621, 13.450067), ms::LatLon(52.510811, 13.409490),
          {"Brazil", "Russian Federation"}, standardCallback, errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(from, to, {"Brazil", "Russian Federation"}, lastCallback,
                                          errorCallback);
    }
  }

  testing::RunEventLoop();

  CompareProviders(providersContainer, synchronousProviders);
}
}  // namespace
