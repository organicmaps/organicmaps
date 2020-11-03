#include "testing/testing.hpp"

#include "partners_api/ads/arsenal_ads.hpp"

#include "partners_api/partners_api_tests/download_on_map_container_delegate.hpp"

UNIT_TEST(ArsenalMedic)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::ArsenalMedic arsenalMedic(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow Oblast_East", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow Oblast_East", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow Oblast_East", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalMedic.GetBanner("Russia_Moscow", point, "ru");
    TEST(!banner.empty(), ());
  }
}

UNIT_TEST(ArsenalFlat)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::ArsenalFlat arsenalFlat(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalFlat.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalFlat.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalFlat.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalFlat.GetBanner("US_Pennsylvania_Central", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("United States of America");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalFlat.GetBanner("US_Pennsylvania_Central", point, "ru");
    TEST(!banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("United States of America");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalFlat.GetBanner("US_Pennsylvania_Central", point, "ru");
    TEST(!banner.empty(), ());
  }
}

UNIT_TEST(ArsenalInsuranceCrimea)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::ArsenalInsuranceCrimea arsenalInsuranceCrimea(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalInsuranceCrimea.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceCrimea.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceCrimea.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceCrimea.GetBanner("Crimea", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalInsuranceCrimea.GetBanner("Crimea", point, "ru");
    TEST(!banner.empty(), ());
  }
}

UNIT_TEST(ArsenalInsuranceRussia)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::ArsenalInsuranceRussia ArsenalInsuranceRussia(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Krasnodar Krai_Adygeya", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Krasnodar Krai_Adygeya", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = ArsenalInsuranceRussia.GetBanner("Russia_Krasnodar Krai_Adygeya", point, "ru");
    TEST(!banner.empty(), ());
  }
}

UNIT_TEST(ArsenalInsuranceWorld)
{
  DownloadOnMapContainerDelegateForTesting delegate;
  ads::ArsenalInsuranceWorld arsenalInsuranceWorld(delegate);
  m2::PointD point;

  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalInsuranceWorld.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceWorld.GetBanner("Russia_Moscow", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceWorld.GetBanner("Russia_Moscow", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Cote dIvoire");
    delegate.SetCountryId("Cote dIvoire");
    auto const banner = arsenalInsuranceWorld.GetBanner("Russia_Krasnodar Krai_Adygeya", point, "en");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Russian Federation");
    delegate.SetCountryId("Russian Federation");
    auto const banner = arsenalInsuranceWorld.GetBanner("Slovenia", point, "ru");
    TEST(banner.empty(), ());
  }
  {
    delegate.SetTopmostParent("Slovenia");
    delegate.SetCountryId("Slovenia");
    auto const banner = arsenalInsuranceWorld.GetBanner("Slovenia", point, "ru");
    TEST(!banner.empty(), ());
  }
}
