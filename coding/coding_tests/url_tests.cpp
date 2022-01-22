#include "testing/testing.hpp"

#include "coding/url.hpp"

#include "base/math.hpp"

#include <queue>
#include <string>
#include <utility>

namespace url_tests
{
using namespace std;
using namespace url;

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
    Url url(m_url);
    TEST_EQUAL(url.GetScheme(), m_scheme, ());
    TEST_EQUAL(url.GetPath(), m_path, ());
    TEST(!m_scheme.empty() || !url.IsValid(), ("Scheme is empty if and only if url is invalid!"));
    url.ForEachParam(bind(&TestUrl::AddTestValue, this, placeholders::_1));
  }

private:
  void AddTestValue(Param const & param)
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

char const * orig1 = "http://google.com/main_index.php";
char const * enc1 = "http%3A%2F%2Fgoogle.com%2Fmain_index.php";
char const * orig2 = "Some File Name.ext";
char const * enc2 = "Some%20File%20Name.ext";
char const * orig3 = "Wow,  two spaces?!";
char const * enc3 = "Wow%2C%20%20two%20spaces%3F%21";
char const * orig4 = "#$%^&@~[]{}()|*+`\"\'";
char const * enc4 = "%23%24%25%5E%26%40~%5B%5D%7B%7D%28%29%7C%2A%2B%60%22%27";

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

UNIT_TEST(Url_Encode)
{
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

UNIT_TEST(Url_Decode)
{
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

UNIT_TEST(UrlScheme_Valid)
{
  Url url("mapswithme://map?ll=10.3,12.3223&n=Hello%20World");
  TEST_EQUAL(url.GetScheme(), "mapswithme", ());
}

UNIT_TEST(UrlScheme_NoColon)
{
  TEST_EQUAL(Url("mapswithme:").GetScheme(), "mapswithme", ());
}

UNIT_TEST(UrlScheme_Valid2)
{
  TestUrl("mapswithme://map?ll=10.3,12.3223&n=Hello%20World")
      .Scheme("mapswithme")
      .Path("map")
      .KV("ll", "10.3,12.3223")
      .KV("n", "Hello World");
}

UNIT_TEST(UrlScheme_Comprehensive)
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

UNIT_TEST(UrlApi_Smoke)
{
  url::Url url("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15.232");
  TEST_EQUAL(url.GetScheme(), "https", ());
  TEST_EQUAL(url.GetPath(), "2gis.ru/moscow/firm/4504127908589159", ());
  TEST_EQUAL(url.GetWebDomain(), "2gis.ru", ());
  TEST_EQUAL(url.GetWebPath(), "moscow/firm/4504127908589159", ());

  TEST(url.GetLastParam(), ());
  TEST(url.GetParamValue("m"), ());
}

}  // namespace url_tests
