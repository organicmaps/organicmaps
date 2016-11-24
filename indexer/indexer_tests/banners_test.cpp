#include "testing/testing.hpp"

#include "indexer/banners.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "std/iostream.hpp"

using namespace banner;

UNIT_TEST(Banners_Load)
{
  char const kBanners[] =
      "# comment\n"
      "[abc]\n"
      "icon = test.png\n"
      "start=2016-07-14\n"
      "type= shop-clothes \n"
      "\n"
      "[error]\n"
      "[re_123]\n"
      "type=shop-shoes\n"
      "url=http://{aux}.com\n"
      "     \t   aux=\t\ttest   \n"
      "[future]\n"
      "type=shop-wine\n"
      "start=2028-01-01\n"
      "end=2028-12-31\n"
      "[final]\n"
      "type=shop-pet\n"
      "start=2016-07-13\n"
      "end=2016-07-14\n"
      "\t";

  classificator::Load();
  Classificator & c = classif();

  BannerSet bs;
  istringstream is(kBanners);
  bs.ReadBanners(is);

  TEST(bs.HasBannerForType(c.GetTypeByPath({"shop", "clothes"})), ());
  Banner const & bannerAbc = bs.GetBannerForType(c.GetTypeByPath({"shop", "clothes"}));
  TEST(!bannerAbc.IsEmpty(), ());
  TEST_EQUAL(bannerAbc.GetIconName(), "test.png", ());
  TEST_EQUAL(bannerAbc.GetMessageBase(), "banner_abc", ());
  TEST_EQUAL(bannerAbc.GetDefaultUrl(), "", ());
  TEST_EQUAL(bannerAbc.GetFormattedUrl("http://example.com"), "http://example.com", ());
  TEST_EQUAL(bannerAbc.GetFormattedUrl(), "", ());
  TEST(bannerAbc.IsActive(), ());

  TEST(bs.HasBannerForType(c.GetTypeByPath({"shop", "shoes"})), ());
  Banner const & bannerRe = bs.GetBannerForType(c.GetTypeByPath({"shop", "shoes"}));
  TEST(!bannerRe.IsEmpty(), ());
  TEST(bannerRe.IsActive(), ());
  TEST_EQUAL(bannerRe.GetIconName(), "banner_re_123.png", ());
  TEST_EQUAL(bannerRe.GetFormattedUrl(), "http://test.com", ());
  TEST_EQUAL(bannerRe.GetFormattedUrl("http://ex.ru/{aux}?var={v}"), "http://ex.ru/test?var={v}", ());

  TEST(bs.HasBannerForType(c.GetTypeByPath({"shop", "wine"})), ());
  Banner const & bannerFuture = bs.GetBannerForType(c.GetTypeByPath({"shop", "wine"}));
  TEST(!bannerFuture.IsEmpty(), ());
  TEST(!bannerFuture.IsActive(), ());

  TEST(bs.HasBannerForType(c.GetTypeByPath({"shop", "pet"})), ());
  Banner const & bannerFinal = bs.GetBannerForType(c.GetTypeByPath({"shop", "pet"}));
  TEST(!bannerFinal.IsEmpty(), ());
  TEST(!bannerFinal.IsActive(), ());
  TEST_EQUAL(bannerFinal.GetFormattedUrl("http://{aux}.ru"), "http://{aux}.ru", ());
}
