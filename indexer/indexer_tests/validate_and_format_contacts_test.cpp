#include "testing/testing.hpp"

#include "indexer/validate_and_format_contacts.hpp"

#include <string>

UNIT_TEST(EditableMapObject_ValidateAndFormat_facebook)
{
  TEST_EQUAL(osm::ValidateAndFormat_facebook(""), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("www.facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("www.facebook.fr/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("http://facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("http://www.facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://www.facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://de-de.facebook.de/Open_Street_Map"), "Open_Street_Map", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://en-us.facebook.com/OpenStreetMap"), "OpenStreetMap", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://de-de.facebook.com/profile.php?id=100085707580841"), "100085707580841", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("http://facebook.com/profile.php?share=app&id=100086487430889#m"), "100086487430889", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("some.good.page"), "some.good.page", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("@tree-house-interiors"), "tree-house-interiors", ());

  TEST_EQUAL(osm::ValidateAndFormat_facebook("instagram.com/openstreetmapus"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("https://instagram.com/openstreetmapus"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("osm"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("@spaces are not welcome here"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_facebook("spaces are not welcome here"), "", ());
}

UNIT_TEST(EditableMapObject_ValidateAndFormat_instagram)
{
  TEST_EQUAL(osm::ValidateAndFormat_instagram(""), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("www.instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("http://instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("https://instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("http://www.instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("https://www.instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("https://en-us.instagram.com/openstreetmapus"), "openstreetmapus", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("@open_street_map_us"), "open_street_map_us", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("https://www.instagram.com/explore/locations/358536820/trivium-sport-en-dance/"), "explore/locations/358536820/trivium-sport-en-dance", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("https://www.instagram.com/p/BvkgKZNDbqN/?ghid=UwPchX7B"), "p/BvkgKZNDbqN", ());

  TEST_EQUAL(osm::ValidateAndFormat_instagram("facebook.com/osm_us"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram(".dots_not_allowed."), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_instagram("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), "", ());
}

UNIT_TEST(EditableMapObject_ValidateAndFormat_twitter)
{
  TEST_EQUAL(osm::ValidateAndFormat_twitter("twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("www.twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("http://twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("https://twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("http://www.twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("https://www.twitter.com/osm_tech"), "osm_tech", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("@_osm_tech_"), "_osm_tech_", ());

  TEST_EQUAL(osm::ValidateAndFormat_twitter("instagram.com/osm_tech"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("dots.not.allowed"), "", ());
  TEST_EQUAL(osm::ValidateAndFormat_twitter("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), "", ());
}

UNIT_TEST(EditableMapObject_ValidateAndFormat_vk)
{
  TEST_EQUAL(osm::ValidateAndFormat_vk("vk.com/id404"), "id404", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("vkontakte.ru/id4321"), "id4321", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("www.vkontakte.ru/id43210"), "id43210", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("www.vk.com/id404"), "id404", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("http://vk.com/ozon"), "ozon", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("https://vk.com/sklad169"), "sklad169", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("https://vkontakte.ru/id4321"), "id4321", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("https://www.vkontakte.ru/id4321"), "id4321", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("http://www.vk.com/ugona.net.expert"), "ugona.net.expert", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("https://www.vk.com/7cvetov18"), "7cvetov18", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("https://www.vk.com/7cvetov18/"), "7cvetov18", ());
  TEST_EQUAL(osm::ValidateAndFormat_vk("@22ab.cdef"), "22ab.cdef", ());

  TEST_EQUAL(osm::ValidateAndFormat_vk("instagram.com/hello_world"), "", ());
}

UNIT_TEST(EditableMapObject_ValidateAndFormat_contactLine)
{
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("http://line.me/ti/p/mzog4fnz24"), "mzog4fnz24", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/ti/p/xnv0g02rws"), "xnv0g02rws", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/ti/p/@dgxs9r6wad"), "dgxs9r6wad", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/ti/p/%40vne5uwke17"), "vne5uwke17", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("http://line.me/R/ti/p/bfsg1a8x9u"), "bfsg1a8x9u", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("line.me/R/ti/p/bfsg1a8x9u"), "bfsg1a8x9u", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/ti/p/gdltt7s380"), "gdltt7s380", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/ti/p/@sdb2pb3lsg"), "sdb2pb3lsg", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/ti/p/%40b30h5mdj11"), "b30h5mdj11", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("http://line.me/R/home/public/main?id=hmczqsbav5"), "hmczqsbav5", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/home/public/main?id=wa1gvx91jb"), "wa1gvx91jb", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("http://line.me/R/home/public/profile?id=5qll5dyqqu"), "5qll5dyqqu", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/home/public/profile?id=r90ck7n1rq"), "r90ck7n1rq", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.me/R/home/public/profile?id=r90ck7n1rq"), "r90ck7n1rq", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://page.line.me/fom5198h"), "fom5198h", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://page.line.me/qn58n8g?web=mobile"), "qn58n8g", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://page.line.me/?accountId=673watcr"), "673watcr", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("page.line.me/?accountId=673watcr"), "673watcr", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://liff.line.me/1645278921-kWRPP32q/?accountId=673watcr"), "liff.line.me/1645278921-kWRPP32q/?accountId=673watcr", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://abc.line.me/en/some/page?id=xaladqv"), "abc.line.me/en/some/page?id=xaladqv", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("@abcd"), "abcd", ());
  TEST_EQUAL(osm::ValidateAndFormat_contactLine("@-hyphen-test-"), "-hyphen-test-", ());

  TEST_EQUAL(osm::ValidateAndFormat_contactLine("https://line.com/ti/p/invalid-domain"), "", ());
}

UNIT_TEST(EditableMapObject_ValidateFacebookPage)
{
  TEST(osm::ValidateFacebookPage(""), ());
  TEST(osm::ValidateFacebookPage("facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("www.facebook.fr/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("http://facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("http://www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://www.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://de-de.facebook.de/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://en-us.facebook.com/OpenStreetMap"), ());
  TEST(osm::ValidateFacebookPage("https://www.facebook.com/profile.php?id=100085707580841"), ());
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

UNIT_TEST(EditableMapObject_socialContactToURL) {
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_INSTAGRAM, "some_page_name"), "https://instagram.com/some_page_name", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_INSTAGRAM, "p/BvkgKZNDbqN"), "https://instagram.com/p/BvkgKZNDbqN", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_FACEBOOK, "100086487430889"), "https://facebook.com/100086487430889", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_FACEBOOK, "nova.poshta.official"), "https://facebook.com/nova.poshta.official", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_FACEBOOK, "pg/ESQ-336537783591903/about"), "https://facebook.com/pg/ESQ-336537783591903/about", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_TWITTER, "carmelopizza"), "https://twitter.com/carmelopizza", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_TWITTER, "demhamburguesa/status/688001869269078016"), "https://twitter.com/demhamburguesa/status/688001869269078016", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_VK, "beerhousebar"), "https://vk.com/beerhousebar", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_VK, "wall-41524_29351"), "https://vk.com/wall-41524_29351", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_LINE, "a26235875"), "https://line.me/R/ti/p/@a26235875", ());
  TEST_EQUAL(osm::socialContactToURL(osm::MapObject::MetadataID::FMD_CONTACT_LINE, "liff.line.me/1645278921-kWRPP32q/?accountId=673watcr"), "https://liff.line.me/1645278921-kWRPP32q/?accountId=673watcr", ());
}