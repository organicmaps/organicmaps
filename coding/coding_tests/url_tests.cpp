#include "testing/testing.hpp"

#include "coding/url.hpp"

#include <queue>
#include <string>
#include <utility>

using namespace std;

namespace
{
double const kEps = 1e-10;

class TestUrl
{
public:
  explicit TestUrl(string const & url) : m_url(url) {}

  TestUrl & Scheme(string const & scheme) { m_scheme = scheme; return *this; }
  TestUrl & Path(string const & path) { m_path = path; return *this; }
  TestUrl & KV(string const & key, string const & value)
  {
    m_keyValuePairs.push(make_pair(key, value));
    return *this;
  }

  ~TestUrl()
  {
    url::Url url(m_url);
    TEST_EQUAL(url.GetScheme(), m_scheme, ());
    TEST_EQUAL(url.GetPath(), m_path, ());
    TEST(!m_scheme.empty() || !url.IsValid(), ("Scheme is empty if and only if url is invalid!"));
    url.ForEachParam(bind(&TestUrl::AddTestValue, this, placeholders::_1));
  }

private:
  void AddTestValue(url::Param const & param)
  {
    TEST(!m_keyValuePairs.empty(), ("Failed for url = ", m_url, "Passed KV = ", param));
    TEST_EQUAL(m_keyValuePairs.front().first, param.m_name, ());
    TEST_EQUAL(m_keyValuePairs.front().second, param.m_value, ());
    m_keyValuePairs.pop();
  }

  string m_url;
  string m_scheme;
  string m_path;
  queue<pair<string, string>> m_keyValuePairs;
};
}  // namespace

namespace url_encode_testdata
{
char const * orig1 = "http://google.com/main_index.php";
char const * enc1 = "http%3A%2F%2Fgoogle.com%2Fmain_index.php";
char const * orig2 = "Some File Name.ext";
char const * enc2 = "Some%20File%20Name.ext";
char const * orig3 = "Wow,  two spaces?!";
char const * enc3 = "Wow%2C%20%20two%20spaces%3F%21";
char const * orig4 = "#$%^&@~[]{}()|*+`\"\'";
char const * enc4 = "%23%24%25%5E%26%40~%5B%5D%7B%7D%28%29%7C%2A%2B%60%22%27";
}  // namespace url_encode_testdata

namespace url
{
UNIT_TEST(Url_Join)
{
  TEST_EQUAL("", Join("", ""), ());
  TEST_EQUAL("omim/", Join("", "omim/"), ());
  TEST_EQUAL("omim/", Join("omim/", ""), ());
  TEST_EQUAL("omim/strings", Join("omim", "strings"), ());
  TEST_EQUAL("omim/strings", Join("omim/", "strings"), ());
  TEST_EQUAL("../../omim/strings", Join("..", "..", "omim", "strings"), ());
  TEST_EQUAL("../../omim/strings", Join("../", "..", "omim/", "strings"), ());
  TEST_EQUAL("omim/strings", Join("omim/", "/strings"), ());
  TEST_EQUAL("../../omim/strings", Join("../", "/../", "/omim/", "/strings"), ());
  TEST_EQUAL("../omim/strings", Join("../", "", "/omim/", "/strings"), ());
}

UNIT_TEST(UrlEncode)
{
  using namespace url_encode_testdata;

  TEST_EQUAL(UrlEncode(""), "", ());
  TEST_EQUAL(UrlEncode(" "), "%20", ());
  TEST_EQUAL(UrlEncode("%% "), "%25%25%20", ());
  TEST_EQUAL(UrlEncode("20"), "20", ());
  TEST_EQUAL(UrlEncode("Guinea-Bissau"), "Guinea-Bissau", ());
  TEST_EQUAL(UrlEncode(orig1), enc1, ());
  TEST_EQUAL(UrlEncode(orig2), enc2, ());
  TEST_EQUAL(UrlEncode(orig3), enc3, ());
  TEST_EQUAL(UrlEncode(orig4), enc4, ());
}

UNIT_TEST(UrlDecode)
{
  using namespace url_encode_testdata;

  TEST_EQUAL(UrlDecode(""), "", ());
  TEST_EQUAL(UrlDecode("%20"), " ", ());
  TEST_EQUAL(UrlDecode("%25%25%20"), "%% ", ());
  TEST_EQUAL(UrlDecode("20"), "20", ());
  TEST_EQUAL(UrlDecode("Guinea-Bissau"), "Guinea-Bissau", ());
  TEST_EQUAL(UrlDecode(enc1), orig1, ());
  TEST_EQUAL(UrlDecode(enc2), orig2, ());
  TEST_EQUAL(UrlDecode(enc3), orig3, ());
  TEST_EQUAL(UrlDecode(enc4), orig4, ());
}

UNIT_TEST(ProcessURL_Smoke)
{
  {
    GeoURLInfo info("geo:53.666,27.666");
    TEST(info.IsValid(), ());
    TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
    TEST_ALMOST_EQUAL_ABS(info.m_lon, 27.666, kEps, ());
  }

  {
    GeoURLInfo info("geo://point/?lon=27.666&lat=53.666&zoom=10");
    TEST(info.IsValid(), ());
    TEST_ALMOST_EQUAL_ABS(info.m_lat, 53.666, kEps, ());
    TEST_ALMOST_EQUAL_ABS(info.m_lon, 27.666, kEps, ());
    TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());
  }

  {
    GeoURLInfo info("geo:53.666");
    TEST(!info.IsValid(), ());
  }

  {
    GeoURLInfo info("mapswithme:123.33,32.22/showmethemagic");
    TEST(!info.IsValid(), ());
  }

  {
    GeoURLInfo info("mapswithme:32.22, 123.33/showmethemagic");
    TEST(info.IsValid(), ());
    TEST_ALMOST_EQUAL_ABS(info.m_lat, 32.22, kEps, ());
    TEST_ALMOST_EQUAL_ABS(info.m_lon, 123.33, kEps, ());
  }

  {
    GeoURLInfo info("model: iphone 7,1");
    TEST(!info.IsValid(), ());
  }
}

UNIT_TEST(ProcessURL_Instagram)
{
  GeoURLInfo info("geo:0,0?z=14&q=54.683486138,25.289361259 (Forto%20dvaras)");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 54.683486138, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 25.289361259, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 14.0, kEps, ());
}

UNIT_TEST(ProcessURL_GoogleMaps)
{
  GeoURLInfo info("https://maps.google.com/maps?z=16&q=Mezza9%401.3067198,103.83282");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 1.3067198, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.83282, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());

  info = GeoURLInfo("https://maps.google.com/maps?z=16&q=House+of+Seafood+%40+180%401.356706,103.87591");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 1.356706, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 103.87591, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 16.0, kEps, ());
}

UNIT_TEST(ProcessURL_CaseInsensitive)
{
  GeoURLInfo info("geo:52.23405,21.01547?Z=10");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 10.0, kEps, ());
}

UNIT_TEST(ProcessURL_BadZoom)
{
  GeoURLInfo info("geo:52.23405,21.01547?Z=19");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 17.0, kEps, ());

  info = GeoURLInfo("geo:52.23405,21.01547?Z=nineteen");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 17.0, kEps, ());

  info = GeoURLInfo("geo:52.23405,21.01547?Z=-1");
  TEST(info.IsValid(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_lat, 52.23405, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_lon, 21.01547, kEps, ());
  TEST_ALMOST_EQUAL_ABS(info.m_zoom, 0.0, kEps, ());
}

UNIT_TEST(UrlValidScheme)
{
  Url url("mapswithme://map?ll=10.3,12.3223&n=Hello%20World");
  TEST_EQUAL(url.GetScheme(), "mapswithme", ());
}

UNIT_TEST(UrlInvalidSchemeNoColon)
{
  TEST_EQUAL(Url("mapswithme:").GetScheme(), "mapswithme", ());
}

UNIT_TEST(UrlTestValidScheme2)
{
  TestUrl("mapswithme://map?ll=10.3,12.3223&n=Hello%20World")
      .Scheme("mapswithme")
      .Path("map")
      .KV("ll", "10.3,12.3223")
      .KV("n", "Hello World");
}

UNIT_TEST(UrlComprehensive)
{
  TestUrl("");
  TestUrl("scheme:").Scheme("scheme");
  TestUrl("scheme:/").Scheme("scheme");
  TestUrl("scheme://").Scheme("scheme");
  TestUrl("sometext");
  TestUrl(":noscheme");
  TestUrl("://noscheme?");
  TestUrl("mwm://?").Scheme("mwm");
  TestUrl("http://path/to/something").Scheme("http").Path("path/to/something");
  TestUrl("http://path?").Scheme("http").Path("path");
  TestUrl("maps://path?&&key=&").Scheme("maps").Path("path").KV("key", "");
  TestUrl("mapswithme://map?ll=1.2,3.4&z=15").Scheme("mapswithme").Path("map")
      .KV("ll", "1.2,3.4").KV("z", "15");
  TestUrl("nopathnovalues://?key1&key2=val2").Scheme("nopathnovalues").Path("")
      .KV("key1", "").KV("key2", "val2");
  TestUrl("s://?key1&key2").Scheme("s").Path("").KV("key1", "").KV("key2", "");
  TestUrl("g://p?key1=val1&key2=").Scheme("g").Path("p").KV("key1", "val1").KV("key2", "");
  TestUrl("g://p?=val1&key2=").Scheme("g").Path("p").KV("", "val1").KV("key2", "");
  TestUrl("g://?k&key2").Scheme("g").KV("k", "").KV("key2", "");
  TestUrl("m:?%26Amp%26%3D%26Amp%26&name=%31%20%30").Scheme("m")
      .KV("&Amp&=&Amp&", "").KV("name", "1 0");
  TestUrl("s://?key1=value1&key1=value2&key1=value3&key2&key2&key3=value1&key3&key3=value2")
      .Scheme("s")
      .KV("key1", "value1").KV("key1", "value2").KV("key1", "value3")
      .KV("key2", "").KV("key2", "")
      .KV("key3", "value1").KV("key3", "").KV("key3", "value2");
}
}  // namespace url
