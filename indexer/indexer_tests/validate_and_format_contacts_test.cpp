#include "testing/testing.hpp"

#include "indexer/validate_and_format_contacts.hpp"

#include <string>

UNIT_TEST(EditableMapObject_ValidateFacebookPage)
{
  TEST(osm::ValidateFacebookPage(""), ());
  TEST(osm::ValidateFacebookPage("facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("http://facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("http://www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://en-us.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("some.good.page"), ());
  TEST(osm::ValidateFacebookPage("Quaama-Volunteer-Bushfire-Brigade-526790054021506"), ());
  TEST(osm::ValidateFacebookPage(u8"Páter-Bonifác-Restaurant-Budapest-111001693867133"), ());
  TEST(osm::ValidateFacebookPage(u8"MÊGÅ--CÄFË-3141592653589793"), ());
  TEST(osm::ValidateFacebookPage(u8"ресторан"), ()); // Cyrillic
  TEST(osm::ValidateFacebookPage(u8"საქართველო"), ()); // Georgian
  TEST(osm::ValidateFacebookPage(u8"日本語"), ()); // Japanese
  TEST(osm::ValidateFacebookPage("@tree-house-interiors"), ());
  TEST(osm::ValidateFacebookPage("allow_underscore-1234567890"), ());
  TEST(osm::ValidateFacebookPage("alexander.net"), ());

  TEST(!osm::ValidateFacebookPage("instagram.com/openstreetmapus"), ());
  TEST(!osm::ValidateFacebookPage("https://instagram.com/openstreetmapus"), ());
  TEST(!osm::ValidateFacebookPage("osm"), ());
  TEST(!osm::ValidateFacebookPage("@spaces are not welcome here"), ());
  TEST(!osm::ValidateFacebookPage("spaces are not welcome here"), ());

  constexpr char kForbiddenFBSymbols[] = " !@^*()~[]{}#$%&;,:+\"'/\\";
  for(size_t i=0; i<std::size(kForbiddenFBSymbols)-1; i++)
  {
    auto test_str = std::string("special-symbol-") + kForbiddenFBSymbols[i] + "-forbidden";
    TEST(!osm::ValidateFacebookPage(test_str), (test_str));
  }

  // Symbols "£€¥" are not allowed, but to check such cases it requires unicode magic. Not supported currently.
  // TODO: find all restricted *Unicode* symbols from https://www.facebook.com/pages/create page
  //       and them to the test
  //TEST(!osm::ValidateFacebookPage(u8"you-shall-not-pass-£€¥"), ());
}

UNIT_TEST(EditableMapObject_ValidateInstagramPage)
{
  TEST(osm::ValidateInstagramPage(""), ());
  TEST(osm::ValidateInstagramPage("instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("www.instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("http://instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("https://instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("http://www.instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("https://www.instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("https://en-us.instagram.com/openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("openstreetmapus"), ());
  TEST(osm::ValidateInstagramPage("open.street.map.us"), ());
  TEST(osm::ValidateInstagramPage("open_street_map_us"), ());
  TEST(osm::ValidateInstagramPage("@open_street_map_us"), ());
  TEST(osm::ValidateInstagramPage("_osm_"), ());

  TEST(!osm::ValidateInstagramPage("facebook.com/osm_us"), ());
  TEST(!osm::ValidateInstagramPage("https://facebook.com/osm_us"), ());
  TEST(!osm::ValidateInstagramPage(".osm"), ());
  TEST(!osm::ValidateInstagramPage("osm."), ());
  TEST(!osm::ValidateInstagramPage(".dots_not_allowed."), ());
  TEST(!osm::ValidateInstagramPage("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), ());
}

UNIT_TEST(EditableMapObject_ValidateTwitterPage)
{
  TEST(osm::ValidateTwitterPage(""), ());
  TEST(osm::ValidateTwitterPage("twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("www.twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("http://twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("https://twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("http://www.twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("https://www.twitter.com/osm_tech"), ());
  TEST(osm::ValidateTwitterPage("osm_tech"), ());
  TEST(osm::ValidateTwitterPage("_osm_tech_"), ());
  TEST(osm::ValidateTwitterPage("@_osm_tech_"), ());
  TEST(osm::ValidateTwitterPage("1"), ());

  TEST(!osm::ValidateTwitterPage("instagram.com/osm_tech"), ());
  TEST(!osm::ValidateTwitterPage("https://instagram.com/osm_tech"), ());
  TEST(!osm::ValidateTwitterPage("dots.not.allowed"), ());
  TEST(!osm::ValidateTwitterPage("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), ());
}

UNIT_TEST(EditableMapObject_ValidateVkPage)
{
  TEST(osm::ValidateVkPage(""), ());
  TEST(osm::ValidateVkPage("vk.com/id404"), ());
  TEST(osm::ValidateVkPage("vkontakte.ru/id404"), ());
  TEST(osm::ValidateVkPage("www.vk.com/id404"), ());
  TEST(osm::ValidateVkPage("http://vk.com/id404"), ());
  TEST(osm::ValidateVkPage("https://vk.com/id404"), ());
  TEST(osm::ValidateVkPage("https://vkontakte.ru/id404"), ());
  TEST(osm::ValidateVkPage("http://www.vk.com/id404"), ());
  TEST(osm::ValidateVkPage("https://www.vk.com/id404"), ());
  TEST(osm::ValidateVkPage("id432160160"), ());
  TEST(osm::ValidateVkPage("hello_world"), ());
  TEST(osm::ValidateVkPage("osm63rus"), ());
  TEST(osm::ValidateVkPage("22ab.cdef"), ());
  TEST(osm::ValidateVkPage("@hello_world"), ());
  TEST(osm::ValidateVkPage("@osm63rus"), ());
  TEST(osm::ValidateVkPage("@22ab.cdef"), ());

  TEST(!osm::ValidateVkPage("333too_many_numbers"), ());
  TEST(!osm::ValidateVkPage("vk"), ());
  TEST(!osm::ValidateVkPage("@five"), ());
  TEST(!osm::ValidateVkPage("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), ());
  TEST(!osm::ValidateVkPage("_invalid_underscores_"), ());
  TEST(!osm::ValidateVkPage("invalid-dashes"), ());
  //TEST(!osm::ValidateVkPage("to.ma.ny.do.ts"), ()); //TODO: it's hard to test such cases. Skip for now
  //TEST(!osm::ValidateVkPage("dots.__.dots"), ()); //TODO: it's hard to test such cases. Skip for now
  TEST(!osm::ValidateVkPage("instagram.com/hello_world"), ());
  TEST(!osm::ValidateVkPage("https://instagram.com/hello_world"), ());
}

UNIT_TEST(EditableMapObject_ValidateLinePage)
{
  TEST(osm::ValidateLinePage(""), ());
  TEST(osm::ValidateLinePage("http://line.me/ti/p/mzog4fnz24"), ());
  TEST(osm::ValidateLinePage("https://line.me/ti/p/xnv0g02rws"), ());
  TEST(osm::ValidateLinePage("https://line.me/ti/p/@dgxs9r6wad"), ());
  TEST(osm::ValidateLinePage("https://line.me/ti/p/%40vne5uwke17"), ());
  TEST(osm::ValidateLinePage("http://line.me/R/ti/p/bfsg1a8x9u"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/ti/p/gdltt7s380"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/ti/p/@sdb2pb3lsg"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/ti/p/%40b30h5mdj11"), ());
  TEST(osm::ValidateLinePage("http://line.me/R/home/public/main?id=hmczqsbav5"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/home/public/main?id=wa1gvx91jb"), ());
  TEST(osm::ValidateLinePage("http://line.me/R/home/public/profile?id=5qll5dyqqu"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/home/public/profile?id=r90ck7n1rq"), ());
  TEST(osm::ValidateLinePage("https://line.me/R/home/public/profile?id=r90ck7n1rq"), ());
  TEST(osm::ValidateLinePage("https://page.line.me/fom5198h"), ());
  TEST(osm::ValidateLinePage("https://page.line.me/qn58n8g?web=mobile"), ());
  TEST(osm::ValidateLinePage("https://abc.line.me/en/some/page?id=xaladqv"), ());
  TEST(osm::ValidateLinePage("@abcd"), ());
  TEST(osm::ValidateLinePage("0000"), ());
  TEST(osm::ValidateLinePage(".dots.are.allowed."), ());
  TEST(osm::ValidateLinePage("@.dots.are.allowed."), ());
  TEST(osm::ValidateLinePage("-hyphen-test-"), ());
  TEST(osm::ValidateLinePage("@-hyphen-test-"), ());
  TEST(osm::ValidateLinePage("under_score"), ());
  TEST(osm::ValidateLinePage("@under_score"), ());

  TEST(!osm::ValidateLinePage("no"), ());
  TEST(!osm::ValidateLinePage("yes"), ());
  TEST(!osm::ValidateLinePage("No-upper-case"), ());
  TEST(!osm::ValidateLinePage("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"), ());
  TEST(!osm::ValidateLinePage("https://line.com/ti/p/invalid-domain"), ());
}
