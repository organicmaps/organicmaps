#include "testing/testing.hpp"
#include "coding/uri.hpp"
#include "base/macros.hpp"
#include "std/bind.hpp"
#include "std/queue.hpp"
#include "std/utility.hpp"

using url_scheme::Uri;

namespace
{

class TestUri
{
public:
  TestUri(string const & uri) { m_uri = uri; }
  TestUri & Scheme(string const &scheme) { m_scheme = scheme; return *this; }
  TestUri & Path(string const & path) { m_path = path; return *this; }
  TestUri & KV(string const & key, string const & value)
  {
    m_keyValuePairs.push(make_pair(key, value));
    return *this;
  }

  ~TestUri()
  {
    Uri uri(m_uri);
    TEST_EQUAL(uri.GetScheme(), m_scheme, ());
    TEST_EQUAL(uri.GetPath(), m_path, ());
    TEST(!m_scheme.empty() || !uri.IsValid(), ("Scheme is empty if and only if uri is invalid!"));
    uri.ForEachKeyValue(bind(&TestUri::AddTestValue, this, _1, _2));
  }

private:

  bool AddTestValue(string const & key, string const & value)
  {
    TEST(!m_keyValuePairs.empty(), ("Failed for uri = ", m_uri, "Passed KV = ", key, value));
    TEST_EQUAL(m_keyValuePairs.front().first,  key, ());
    TEST_EQUAL(m_keyValuePairs.front().second, value, ());
    m_keyValuePairs.pop();
    return true;
  }

  string m_uri, m_scheme, m_path;
  queue<pair<string, string> > m_keyValuePairs;
};

}  // unnamed namespace

UNIT_TEST(UriValidScheme)
{
  char const uriS[] = "mapswithme://map?ll=10.3,12.3223&n=Hello%20World";
  Uri uri(uriS, ARRAY_SIZE(uriS) - 1);
  TEST_EQUAL(uri.GetScheme(), "mapswithme", ());
}

UNIT_TEST(UriInvalidSchemeNoColon)
{
  TEST_EQUAL(Uri("mapswithme:").GetScheme(), "mapswithme", ());
}

UNIT_TEST(UriTestValidScheme2)
{
  TestUri("mapswithme://map?ll=10.3,12.3223&n=Hello%20World")
      .Scheme("mapswithme")
      .Path("map")
      .KV("ll", "10.3,12.3223")
      .KV("n", "Hello World");
}

UNIT_TEST(UriComprehensive)
{
  TestUri("");

  TestUri("scheme:").Scheme("scheme");

  TestUri("scheme:/").Scheme("scheme");

  TestUri("scheme://").Scheme("scheme");

  TestUri("sometext");

  TestUri(":noscheme");

  TestUri("://noscheme?");

  TestUri("mwm://?").Scheme("mwm");

  TestUri("http://path/to/something").Scheme("http").Path("path/to/something");

  TestUri("http://path?").Scheme("http").Path("path");

  TestUri("maps://path?&&key=&").Scheme("maps").Path("path").KV("key", "");

  TestUri("mapswithme://map?ll=1.2,3.4&z=15").Scheme("mapswithme").Path("map")
      .KV("ll", "1.2,3.4").KV("z", "15");

  TestUri("nopathnovalues://?key1&key2=val2").Scheme("nopathnovalues").Path("")
      .KV("key1", "").KV("key2", "val2");

  TestUri("s://?key1&key2").Scheme("s").Path("").KV("key1", "").KV("key2", "");

  TestUri("g://p?key1=val1&key2=").Scheme("g").Path("p").KV("key1", "val1").KV("key2", "");

  TestUri("g://p?=val1&key2=").Scheme("g").Path("p").KV("", "val1").KV("key2", "");

  TestUri("g://?k&key2").Scheme("g").KV("k", "").KV("key2", "");

  TestUri("m:?%26Amp%26%3D%26Amp%26&name=%31%20%30").Scheme("m")
      .KV("&Amp&=&Amp&", "").KV("name", "1 0");

  TestUri("s://?key1=value1&key1=value2&key1=value3&key2&key2&key3=value1&key3&key3=value2")
      .Scheme("s")
      .KV("key1", "value1").KV("key1", "value2").KV("key1", "value3")
      .KV("key2", "").KV("key2", "")
      .KV("key3", "value1").KV("key3", "").KV("key3", "value2");
}
