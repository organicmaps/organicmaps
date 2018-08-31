#include "testing/testing.hpp"

#include "partners_api/maxim_api.hpp"
#include "partners_api/taxi_engine.hpp"
#include "partners_api/uber_api.hpp"
#include "partners_api/yandex_api.hpp"

#include "platform/platform_tests_support/async_gui_thread.hpp"

#include "platform/gui_thread.hpp"

#include "geometry/latlon.hpp"

#include "base/scope_guard.hpp"
#include "base/worker_thread.hpp"

#include <memory>

using namespace platform::tests_support;

namespace
{
class BelarusMinskDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override { return {"Belarus"}; }

  std::string GetCityName(m2::PointD const & point) override { return "Minsk"; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override { return "Belarus_Minsk Region"; }
};

class UkraineOdessaDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override { return {"Ukraine"}; }

  std::string GetCityName(m2::PointD const & point) override { return "Odessa"; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override
  {
    return "Ukraine_Odessa Oblast";
  }
};

class UkraineMariupolDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override { return {"Ukraine"}; }

  std::string GetCityName(m2::PointD const & point) override { return "Mariupol"; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override
  {
    return "Ukraine_Donetsk Oblast";
  }
};

class BulgariaSofiaDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override { return {"Bulgaria"}; }

  std::string GetCityName(m2::PointD const & point) override { return "Sofia"; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override { return {}; }
};

class UsaDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override
  {
    return {"United States of America"};
  }

  std::string GetCityName(m2::PointD const & point) override { return {}; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override { return {}; }
};

class RussiaKonetsDelegate : public taxi::Delegate
{
public:
  storage::TCountriesVec GetCountryIds(m2::PointD const & point) override
  {
    return {"Russian Federation"};
  }

  std::string GetCityName(m2::PointD const & point) override { return "Konets"; }
  storage::TCountryId GetMwmId(m2::PointD const & point) override { return {}; }
};

std::vector<taxi::Product> GetUberSynchronous(ms::LatLon const & from, ms::LatLon const & to,
                                              std::string const & url)
{
  std::string times;
  std::string prices;

  TEST(taxi::uber::RawApi::GetEstimatedTime(from, times, url), ());
  TEST(taxi::uber::RawApi::GetEstimatedPrice(from, to, prices, url), ());

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

std::vector<taxi::Product> GetYandexSynchronous(ms::LatLon const & from, ms::LatLon const & to,
                                                std::string const & url)
{
  std::string yandexAnswer;
  std::vector<taxi::Product> yandexProducts;

  TEST(taxi::yandex::RawApi::GetTaxiInfo(from, to, yandexAnswer, url), ());

  taxi::yandex::MakeFromJson(yandexAnswer, yandexProducts);

  return yandexProducts;
}

std::vector<taxi::Product> GetMaximSynchronous(ms::LatLon const & from, ms::LatLon const & to,
                                               std::string const & url)
{
  std::string maximAnswer;
  std::vector<taxi::Product> maximProducts;

  TEST(taxi::maxim::RawApi::GetTaxiInfo(from, to, maximAnswer, url), ());

  taxi::maxim::MakeFromJson(maximAnswer, maximProducts);

  return maximProducts;
}

taxi::ProvidersContainer GetProvidersSynchronous(taxi::Engine const & engine,
                                                 ms::LatLon const & from, ms::LatLon const & to,
                                                 std::string const & url)
{
  taxi::ProvidersContainer providers;

  for (auto const provider : engine.GetProvidersAtPos(from))
  {
    switch (provider)
    {
    case taxi::Provider::Type::Uber:
      providers.emplace_back(taxi::Provider::Type::Uber, GetUberSynchronous(from, to, url));
      break;
    case taxi::Provider::Type::Yandex:
      providers.emplace_back(taxi::Provider::Type::Yandex, GetYandexSynchronous(from, to, url));
      break;
    case taxi::Provider::Type::Maxim:
      providers.emplace_back(taxi::Provider::Type::Maxim, GetMaximSynchronous(from, to, url));
      break;
    case taxi::Provider::Type::Count:
      LOG(LERROR, ());
      break;
    }
  }

  return providers;
}

bool CompareProviders(taxi::ProvidersContainer const & lhs, taxi::ProvidersContainer const & rhs)
{
  TEST_EQUAL(rhs.size(), lhs.size(), ());

  auto const Match = [](taxi::Provider const & lhs, taxi::Provider const & rhs) -> bool {
    if (lhs.GetType() != rhs.GetType())
      return false;

    auto const & lps = lhs.GetProducts();
    auto const & rps = rhs.GetProducts();

    TEST_EQUAL(lps.size(), rps.size(), ());

    for (auto const & lp : lps)
    {
      auto const it = std::find_if(rps.cbegin(), rps.cend(), [&lp](taxi::Product const & rp) {
        return lp.m_productId == rp.m_productId && lp.m_name == rp.m_name &&
               lp.m_price == rp.m_price;
      });

      if (it == rps.cend())
        return false;
    }

    return true;
  };

  auto const m = lhs.size();

  vector<bool> used(m);
  // TODO (@y) Change it to algorithm, based on bipartite graphs.
  for (auto const & rItem : rhs)
  {
    bool isMatched = false;
    for (size_t i = 0; i < m; ++i)
    {
      if (used[i])
        continue;

      if (Match(rItem, lhs[i]))
      {
        used[i] = true;
        isMatched = true;
        break;
      }
    }

    if (!isMatched)
      return false;
  }
  return true;
}

UNIT_CLASS_TEST(AsyncGuiThread, TaxiEngine_ResultMaker)
{
  taxi::ResultMaker maker;
  uint64_t reqId = 1;
  taxi::ProvidersContainer providers;
  taxi::ErrorsContainer errors;

  auto const successCallback = [&reqId, &providers](taxi::ProvidersContainer const & products,
                                                    uint64_t const requestId) {
    TEST_EQUAL(reqId, requestId, ());
    providers = products;
    testing::Notify();
  };

  auto const successNotPossibleCallback = [](taxi::ProvidersContainer const & products,
                                             uint64_t const requestId) {
    TEST(false, ("successNotPossibleCallback", requestId));
  };

  auto const errorCallback = [&reqId, &errors](taxi::ErrorsContainer const e,
                                               uint64_t const requestId) {
    TEST_EQUAL(reqId, requestId, ());
    errors = e;
    testing::Notify();
  };

  auto const errorNotPossibleCallback = [](taxi::ErrorsContainer const errors,
                                           uint64_t const requestId) {
    TEST(false, ("errorNotPossibleCallback", requestId, errors));
  };

  // Try to image what products1, products2 and products3 are lists of products for different taxi
  // providers. Only product id is important for us, all other fields are empty.
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

  std::vector<taxi::Product> products3 =
  {
    {"7", "", "", "", ""},
  };

  maker.Reset(reqId, 3, successCallback, errorNotPossibleCallback);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Uber, products1);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Maxim, products3);

  TEST(providers.empty(), ());
  TEST(errors.empty(), ());

  maker.Reset(reqId, 4, successCallback, errorNotPossibleCallback);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Uber, products1);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Maxim, products3);
  maker.DecrementRequestCount(reqId);
  maker.MakeResult(reqId);

  testing::Wait();

  TEST_EQUAL(providers.size(), 3, ());
  TEST_EQUAL(providers[0].GetType(), taxi::Provider::Type::Uber, ());
  TEST_EQUAL(providers[1].GetType(), taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(providers[2].GetType(), taxi::Provider::Type::Maxim, ());
  TEST_EQUAL(providers[0][0].m_productId, "1", ());
  TEST_EQUAL(providers[0][1].m_productId, "2", ());
  TEST_EQUAL(providers[0][2].m_productId, "3", ());
  TEST_EQUAL(providers[1][0].m_productId, "4", ());
  TEST_EQUAL(providers[1][1].m_productId, "5", ());
  TEST_EQUAL(providers[1][2].m_productId, "6", ());
  TEST_EQUAL(providers[2][0].m_productId, "7", ());

  maker.Reset(reqId, 3, successCallback, errorNotPossibleCallback);
  maker.ProcessError(reqId, taxi::Provider::Type::Uber, taxi::ErrorCode::NoProducts);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Yandex, products2);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Maxim, products3);
  maker.MakeResult(reqId);

  testing::Wait();

  TEST_EQUAL(providers.size(), 2, ());
  TEST_EQUAL(providers[0].GetType(), taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(providers[1].GetType(), taxi::Provider::Type::Maxim, ());
  TEST_EQUAL(providers[0][0].m_productId, "4", ());
  TEST_EQUAL(providers[0][1].m_productId, "5", ());
  TEST_EQUAL(providers[0][2].m_productId, "6", ());
  TEST_EQUAL(providers[1][0].m_productId, "7", ());

  maker.Reset(reqId, 3, successCallback, errorNotPossibleCallback);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Uber, products1);
  maker.ProcessError(reqId, taxi::Provider::Type::Yandex, taxi::ErrorCode::NoProducts);
  maker.ProcessProducts(reqId, taxi::Provider::Type::Maxim, products3);
  maker.MakeResult(reqId);

  testing::Wait();

  TEST_EQUAL(providers.size(), 2, ());
  TEST_EQUAL(providers[0].GetType(), taxi::Provider::Type::Uber, ());
  TEST_EQUAL(providers[1].GetType(), taxi::Provider::Type::Maxim, ());
  TEST_EQUAL(providers[0][0].m_productId, "1", ());
  TEST_EQUAL(providers[0][1].m_productId, "2", ());
  TEST_EQUAL(providers[0][2].m_productId, "3", ());
  TEST_EQUAL(providers[1][0].m_productId, "7", ());

  maker.Reset(reqId, 3, successNotPossibleCallback, errorCallback);
  maker.ProcessError(reqId, taxi::Provider::Type::Uber, taxi::ErrorCode::NoProducts);
  maker.ProcessError(reqId, taxi::Provider::Type::Yandex, taxi::ErrorCode::RemoteError);
  maker.ProcessError(reqId, taxi::Provider::Type::Maxim, taxi::ErrorCode::NoProducts);
  maker.MakeResult(reqId);

  testing::Wait();

  TEST_EQUAL(errors.size(), 3, ());
  TEST_EQUAL(errors[0].m_type, taxi::Provider::Type::Uber, ());
  TEST_EQUAL(errors[0].m_code, taxi::ErrorCode::NoProducts, ());
  TEST_EQUAL(errors[1].m_type, taxi::Provider::Type::Yandex, ());
  TEST_EQUAL(errors[1].m_code, taxi::ErrorCode::RemoteError, ());
  TEST_EQUAL(errors[2].m_type, taxi::Provider::Type::Maxim, ());
  TEST_EQUAL(errors[2].m_code, taxi::ErrorCode::NoProducts, ());

  maker.Reset(reqId, 3, successCallback, errorNotPossibleCallback);
  maker.DecrementRequestCount(reqId);
  maker.DecrementRequestCount(reqId);
  maker.DecrementRequestCount(reqId);
  maker.MakeResult(reqId);

  testing::Wait();
}

UNIT_CLASS_TEST(AsyncGuiThread, TaxiEngine_Smoke)
{
  // Used to synchronize access into GetAvailableProducts callback method.
  std::mutex resultsMutex;
  size_t reqId = 1;
  taxi::ProvidersContainer providersContainer;
  ms::LatLon const from(38.897724, -77.036531);
  ms::LatLon const to(38.862416, -76.883316);
  std::string const kTesturl = "http://localhost:34568/partners";

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

  taxi::Engine engine({{taxi::Provider::Type::Uber, kTesturl},
                       {taxi::Provider::Type::Yandex, kTesturl},
                       {taxi::Provider::Type::Maxim, kTesturl}});

  engine.SetDelegate(std::make_unique<BelarusMinskDelegate>());

  taxi::ProvidersContainer const synchronousProviders =
      GetProvidersSynchronous(engine, from, to, kTesturl);

  TEST(!synchronousProviders.empty(), ());

  {
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(ms::LatLon(55.753960, 37.624513),
                                          ms::LatLon(55.765866, 37.661270), standardCallback,
                                          errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(ms::LatLon(59.922445, 30.367201),
                                          ms::LatLon(59.943675, 30.361123), standardCallback,
                                          errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(ms::LatLon(52.509621, 13.450067),
                                          ms::LatLon(52.510811, 13.409490), standardCallback,
                                          errorPossibleCallback);
    }
    {
      lock_guard<mutex> lock(resultsMutex);
      reqId = engine.GetAvailableProducts(from, to, lastCallback, errorCallback);
    }
  }

  testing::RunEventLoop();

  TEST(!providersContainer.empty(), ());
  TEST(CompareProviders(providersContainer, synchronousProviders), ());
}

UNIT_TEST(TaxiEngine_GetProvidersAtPos)
{
  taxi::Engine engine;
  // Lat lon is no needed for this test.
  ms::LatLon latlon(0.0, 0.0);
  std::vector<taxi::Provider::Type> providers;

  engine.SetDelegate(std::make_unique<BelarusMinskDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0], taxi::Provider::Type::Yandex, ());

  engine.SetDelegate(std::make_unique<UkraineOdessaDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0], taxi::Provider::Type::Yandex, ());

  engine.SetDelegate(std::make_unique<UkraineMariupolDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0], taxi::Provider::Type::Maxim, ());

  engine.SetDelegate(std::make_unique<BulgariaSofiaDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0], taxi::Provider::Type::Maxim, ());

  engine.SetDelegate(std::make_unique<UsaDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST_EQUAL(providers.size(), 1, ());
  TEST_EQUAL(providers[0], taxi::Provider::Type::Uber, ());

  engine.SetDelegate(std::make_unique<RussiaKonetsDelegate>());
  providers = engine.GetProvidersAtPos(latlon);
  TEST(providers.empty(), (providers));
}
}  // namespace
