#include "testing/testing.hpp"

#include "coding/url.hpp"

#include "base/math.hpp"
#include "base/timer.hpp"

#include <queue>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace url_tests
{
using namespace std;
using namespace url;

class TestUrl
{
public:
  explicit TestUrl(string && url) : m_url(std::move(url)) {}

  TestUrl & Scheme(string && scheme)
  {
    m_scheme = std::move(scheme);
    return *this;
  }
  TestUrl & Host(string && host)
  {
    m_host = std::move(host);
    return *this;
  }
  TestUrl & Path(string && path)
  {
    m_path = std::move(path);
    return *this;
  }
  TestUrl & KV(string && key, string && value)
  {
    m_keyValuePairs.emplace(std::move(key), std::move(value));
    return *this;
  }

  ~TestUrl()
  {
    Url url(m_url);
    TEST_EQUAL(url.GetScheme(), m_scheme, ());
    TEST_EQUAL(url.GetHost(), m_host, ());
    TEST_EQUAL(url.GetPath(), m_path, ());

    TEST(!m_scheme.empty() || !url.IsValid(), ("Scheme is empty if and only if url is invalid!"));

    url.ForEachParam([this](string const & name, string const & value)
    {
      TEST(!m_keyValuePairs.empty(), ("Failed for url = ", m_url));
      TEST_EQUAL(m_keyValuePairs.front().first, name, ());
      TEST_EQUAL(m_keyValuePairs.front().second, value, ());
      m_keyValuePairs.pop();
    });
  }

private:
  string m_url, m_scheme, m_host, m_path;
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
  TEST_EQUAL(UrlDecode("123+Main+St,+Seattle,+WA+98101"), "123 Main St, Seattle, WA 98101", ());
}

UNIT_TEST(Url_Invalid)
{
  TEST(!Url("").IsValid(), ());
  TEST(!Url(":/").IsValid(), ());
  TEST(!Url("//").IsValid(), ());
}

UNIT_TEST(Url_Valid)
{
  TestUrl("mapswithme://map?ll=10.3,12.3223&n=Hello%20World")
      .Scheme("mapswithme")
      .Host("map")
      .KV("ll", "10.3,12.3223")
      .KV("n", "Hello World");

  TestUrl("om:M&M//path?q=q&w=w").Scheme("om").Host("M&M").Path("path").KV("q", "q").KV("w", "w");

  TestUrl("http://www.sandwichparlour.com.au/").Scheme("http").Host("www.sandwichparlour.com.au").Path("");

  TestUrl("om:/&test").Scheme("om").Host("&test").Path("");
}

UNIT_TEST(Url_Fragment)
{
  TestUrl("https://www.openstreetmap.org/way/179409926#map=19/46.34998/48.03213&layers=N")
      .Scheme("https")
      .Host("www.openstreetmap.org")
      .Path("way/179409926")
      .KV("map", "19/46.34998/48.03213")
      .KV("layers", "N");

  TestUrl("https://www.openstreetmap.org/search?query=Falafel%20Sahyoun#map=16/33.89041/35.50664")
      .Scheme("https")
      .Host("www.openstreetmap.org")
      .Path("search")
      .KV("query", "Falafel Sahyoun")
      .KV("map", "16/33.89041/35.50664");
}

UNIT_TEST(UrlScheme_Comprehensive)
{
  TestUrl("");
  TestUrl("scheme:").Scheme("scheme").Host("").Path("");
  TestUrl("scheme:/").Scheme("scheme").Host("").Path("");
  TestUrl("scheme://").Scheme("scheme").Host("").Path("");
  TestUrl("sometext");
  TestUrl(":noscheme");
  TestUrl("://noscheme?");
  TestUrl("mwm://?").Scheme("mwm").Host("").Path("");
  TestUrl("http://host/path/to/something").Scheme("http").Host("host").Path("path/to/something");
  TestUrl("http://host?").Scheme("http").Host("host").Path("");
  TestUrl("maps://host?&&key=&").Scheme("maps").Host("host").KV("key", "");
  TestUrl("mapswithme://map?ll=1.2,3.4&z=15")
      .Scheme("mapswithme")
      .Host("map")
      .Path("")
      .KV("ll", "1.2,3.4")
      .KV("z", "15");
  TestUrl("nopathnovalues://?key1&key2=val2")
      .Scheme("nopathnovalues")
      .Host("")
      .Path("")
      .KV("key1", "")
      .KV("key2", "val2");
  TestUrl("s://?key1&key2").Scheme("s").Host("").Path("").KV("key1", "").KV("key2", "");
  TestUrl("g://h/p?key1=val1&key2=").Scheme("g").Host("h").Path("p").KV("key1", "val1").KV("key2", "");
  TestUrl("g://h?=val1&key2=").Scheme("g").Host("h").Path("").KV("", "val1").KV("key2", "");
  TestUrl("g://?k&key2").Scheme("g").Host("").Path("").KV("k", "").KV("key2", "");
  TestUrl("m:?%26Amp%26%3D%26Amp%26&name=%31%20%30")
      .Scheme("m")
      .Host("")
      .Path("")
      .KV("&Amp&=&Amp&", "")
      .KV("name", "1 0");
  TestUrl("s://?key1=value1&key1=value2&key1=value3&key2&key2&key3=value1&key3&key3=value2")
      .Scheme("s")
      .Host("")
      .Path("")
      .KV("key1", "value1")
      .KV("key1", "value2")
      .KV("key1", "value3")
      .KV("key2", "")
      .KV("key2", "")
      .KV("key3", "value1")
      .KV("key3", "")
      .KV("key3", "value2");
}

UNIT_TEST(UrlApi_Smoke)
{
  url::Url url("https://2gis.ru/moscow/firm/4504127908589159?m=37.618632%2C55.760069%2F15.232");
  TEST_EQUAL(url.GetScheme(), "https", ());
  TEST_EQUAL(url.GetHost(), "2gis.ru", ());
  TEST_EQUAL(url.GetPath(), "moscow/firm/4504127908589159", ());
  TEST_EQUAL(url.GetHostAndPath(), "2gis.ru/moscow/firm/4504127908589159", ());

  TEST(url.GetLastParam(), ());
  TEST(url.GetParamValue("m"), ());
}

struct OriginTestCase
{
  char const * m_url;
  char const * m_origin;  // Empty means "no origin".
};

// Returns an empty string when there is no origin, to keep the tables below readable.
string ParsedOrigin(string_view url, bool allowProtocolRelative = false)
{
  return ParseHttpOrigin(url, allowProtocolRelative).value_or(string());
}

void TestOrigins(vector<OriginTestCase> const & cases, bool allowProtocolRelative = false)
{
  for (auto const & c : cases)
    TEST_EQUAL(ParsedOrigin(c.m_url, allowProtocolRelative), string(c.m_origin), (c.m_url));
}

UNIT_TEST(HttpOrigin_Valid)
{
  TestOrigins({
      {"https://example.com", "https://example.com"},
      {"http://example.com", "http://example.com"},
      {"https://example.com/", "https://example.com"},
      {"https://example.com/path/to/x", "https://example.com"},
      {"https://example.com?q=1", "https://example.com"},
      {"https://example.com#frag", "https://example.com"},
      {"https://example.com/path?q=1#frag", "https://example.com"},
      // Scheme and host are case insensitive and get lowercased.
      {"HTTPS://EXAMPLE.COM/X", "https://example.com"},
      {"HtTpS://ExAmPlE.cOm", "https://example.com"},
      {"HTTP://Example.COM", "http://example.com"},
      // Ports.
      {"https://example.com:8443/x", "https://example.com:8443"},
      {"https://example.com:1", "https://example.com:1"},
      {"https://example.com:65535", "https://example.com:65535"},
      {"https://example.com:443", "https://example.com"},     // Default for https.
      {"http://example.com:80", "http://example.com"},        // Default for http.
      {"http://example.com:443", "http://example.com:443"},   // Not a default for http.
      {"https://example.com:80", "https://example.com:80"},   // Not a default for https.
      {"https://example.com:0443/x", "https://example.com"},  // Leading zeros are accepted.
      {"http://example.com:0080", "http://example.com"},
      {"https://example.com:08443", "https://example.com:8443"},
      // Hosts.
      {"https://localhost/x", "https://localhost"},
      {"https://a.b.c.d.example.co.uk", "https://a.b.c.d.example.co.uk"},
      {"https://my-cdn.example.com", "https://my-cdn.example.com"},
      {"https://xn--80ak6aa92e.com/x", "https://xn--80ak6aa92e.com"},  // Punycode is plain ASCII.
      {"https://123.example.com/x", "https://123.example.com"},
      // Only canonical dotted-decimal IPv4 is accepted.
      {"https://0.0.0.0", "https://0.0.0.0"},
      {"https://192.168.0.1:8080/x", "https://192.168.0.1:8080"},
      {"https://255.255.255.255", "https://255.255.255.255"},
      // The authority ends at the first delimiter, so the host here is evil.com, like in a browser.
      {"https://evil.com/@example.com", "https://evil.com"},
      {"https://evil.com?@example.com", "https://evil.com"},
      {"https://evil.com#@example.com", "https://evil.com"},
  });
}

UNIT_TEST(HttpOrigin_Rejected)
{
  TestOrigins({
      {"", ""},
      {"example.com", ""},
      {"//example.com", ""},  // Protocol relative is not allowed here.
      // Only http(s) with exactly "://".
      {"ftp://example.com", ""},
      {"file:///x", ""},
      {"javascript:https://example.com", ""},
      {"data:text/html,https://example.com", ""},
      {"about:blank", ""},
      {"xhttps://example.com", ""},  // The scheme starts at the beginning of the url.
      {"shttp://example.com", ""},
      {" https://example.com", ""},
      {"https//example.com", ""},
      {"https ://example.com", ""},
      // Browsers strip tabs and newlines before parsing, we fail closed instead.
      {"htt\tps://example.com", ""},
      {"https\n://example.com", ""},
      // Truncated urls must not be read past their end.
      {"h", ""},
      {"http", ""},
      {"https", ""},
      {"http:", ""},
      {"https:/", ""},
      {"/", ""},
      {"//", ""},
      // Browsers read a host out of all of these, we fail closed instead of guessing.
      {"https:example.com", ""},
      {"https:/example.com", ""},
      {"HTTPS:/example.com", ""},
      {"https:///x", ""},
      {"https://///example.com", ""},
      {"https://", ""},
      {"http://", ""},
      {"https://:443", ""},
      {"https://:443/x", ""},
      {"https://?q", ""},
      {"https:\\\\example.com", ""},
      // Credentials.
      {"https://user:pw@example.com", ""},
      {"https://user@example.com", ""},
      {"https://user@example.com@evil.com", ""},
      {"https://example.com@evil.com/x", ""},
      // Backslashes, which some browsers treat as slashes.
      {"https://example.com\\path", ""},
      {"https://example.com\\@evil.com", ""},
      // Percent escapes.
      {"https://ex%61mple.com", ""},
      {"https://example.com%2f@evil.com", ""},
      // Whitespace, control and non-ASCII bytes.
      {"https://exa mple.com", ""},
      {"https:// example.com", ""},
      {"https://example.com ", ""},
      {"https://exa\tmple.com", ""},
      {"https://exa\nmple.com", ""},
      {"https://exa\rmple.com", ""},
      {"https://example.com\x01", ""},
      {"https://example.com\x7f", ""},
      {"https://\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.\xd1\x80\xd1\x84", ""},  // "пример.рф".
      // Ports.
      {"https://example.com:", ""},
      {"https://example.com:/x", ""},
      {"https://example.com:abc", ""},
      {"https://example.com:80x", ""},
      {"https://example.com:8443:80", ""},
      {"https://example.com:84:80", ""},  // Short enough to reach the digit loop.
      {"https://example.com: 80", ""},
      {"https://example.com:+80", ""},
      {"https://example.com:-80", ""},
      {"https://example.com:0", ""},
      {"https://example.com:00000", ""},
      {"https://example.com:65536", ""},
      {"https://example.com:99999", ""},
      {"https://example.com:123456", ""},  // More than 5 digits, even with leading zeros.
      {"https://example.com:000080", ""},
      // Hosts.
      {"https://.example.com", ""},
      {"https://example..com", ""},
      {"https://example.com.", ""},
      {"https://.", ""},
      {"https://..", ""},
      {"https://exa_mple.com", ""},  // Underscore is not a DNS char.
      {"https://exa*mple.com", ""},
      {"https://exa!mple.com", ""},
      {"https://example.com]", ""},
      {"https://exa[mple.com", ""},
      // Numeric hosts a browser would reinterpret as IPv4, and malformed IPv4.
      {"https://127.1", ""},
      {"https://0177.0.0.1", ""},
      {"https://0x7f.1", ""},
      {"https://0x7f000001", ""},
      {"https://017700000001", ""},
      {"https://2130706433", ""},
      {"https://192.168.000.001", ""},
      {"https://1.2.3.999", ""},
      {"https://1.2.3.256", ""},
      {"https://1.2.3", ""},
      {"https://1.2.3.4.5", ""},
      {"https://foo.123", ""},
      {"https://1.2.3.0x10", ""},
      {"https://09", ""},
      {"https://0x", ""},
      // IPv6 is outside the strict safe subset, valid or not.
      {"https://[::1]", ""},
      {"https://[0:0:0:0:0:0:0:1]", ""},
      {"https://[::ffff:192.0.2.1]", ""},
      {"https://[::::]", ""},
      {"https://[::1", ""},
      {"https://[fe80::1%25eth0]", ""},
  });

  // A NUL byte can not be put into the table above.
  TEST_EQUAL(ParsedOrigin(string_view("https://exa\0mple.com", 20)), "", ());
}

UNIT_TEST(HttpOrigin_ProtocolRelative)
{
  TestOrigins(
      {
          {"//example.com", "https://example.com"},
          {"//example.com/x?y#z", "https://example.com"},
          {"//example.com:8443/x", "https://example.com:8443"},
          {"//EXAMPLE.com:443", "https://example.com"},
          // Absolute urls are parsed exactly as with the flag disabled.
          {"http://example.com:80/x", "http://example.com"},
          {"https://example.com/x", "https://example.com"},
          // Invalid authorities are still rejected.
          {"//", ""},
          {"///x", ""},
          {"/example.com", ""},
          {"//user@example.com", ""},
          {"//exa mple.com", ""},
          {"//example.com:0", ""},
          {"//https://example.com", ""},
          {"\\\\example.com", ""},  // Backslashes are not slashes for us.
          {"//example.com\\evil.com", ""},
          {"// example.com", ""},
          {"//example.com:65536", ""},
          {"//127.1/x", ""},
          {"//[::1]:8443", ""},
      },
      true /* allowProtocolRelative */);

  // The same inputs without the flag.
  TestOrigins({
      {"//example.com", ""},
      {"//example.com:8443/x", ""},
      {"http://example.com:80/x", "http://example.com"},
      {"https://example.com/x", "https://example.com"},
  });
}

UNIT_TEST(HttpOrigin_HostSizeLimits)
{
  string const label63(63, 'a');
  string const host253 = label63 + '.' + label63 + '.' + label63 + '.' + string(61, 'b');
  TEST_EQUAL(host253.size(), size_t(253), ());

  TEST_EQUAL(ParsedOrigin("https://" + host253), "https://" + host253, ());
  TEST_EQUAL(ParsedOrigin("https://" + host253 + ":8443/x"), "https://" + host253 + ":8443", ());
  TEST_EQUAL(ParsedOrigin("https://" + host253 + "/x"), "https://" + host253, ());
  TEST_EQUAL(ParsedOrigin("https://b" + host253), "", ());  // 254 chars.
  TEST_EQUAL(ParsedOrigin("https://b" + host253 + "/x"), "", ());

  // The longest possible authority is host + ':' + 5 port digits, one byte more is rejected
  // without looking at the rest of the url.
  TEST_EQUAL(ParsedOrigin("https://" + host253 + ":65535"), "https://" + host253 + ":65535", ());
  TEST_EQUAL(ParsedOrigin("https://" + host253 + ":65535/x"), "https://" + host253 + ":65535", ());
  TEST_EQUAL(ParsedOrigin("https://" + host253 + "?q"), "https://" + host253, ());
  TEST_EQUAL(ParsedOrigin("https://" + host253 + "#f"), "https://" + host253, ());
  TEST_EQUAL(ParsedOrigin("https://" + string(254, 'a') + ":65535"), "", ());
  TEST_EQUAL(ParsedOrigin("https://" + string(1024, 'a')), "", ());  // No delimiter at all.
  TEST_EQUAL(ParsedOrigin("https://" + string(1024, 'a') + "/x"), "", ());

  TEST_EQUAL(ParsedOrigin("https://" + label63 + ".com"), "https://" + label63 + ".com", ());
  TEST_EQUAL(ParsedOrigin("https://a" + label63 + ".com"), "", ());  // 64 chars in a label.
}

UNIT_TEST(HttpOrigin_Normalization)
{
  auto const origin = ParseHttpOrigin("HTTPS://Example.COM:8443/path?q#f", false);
  TEST(origin.has_value(), ());
  TEST_EQUAL(*origin, "https://example.com:8443", ());

  auto const defaultPort = ParseHttpOrigin("https://example.com:443/x", false);
  TEST(defaultPort.has_value(), ());
  TEST_EQUAL(*defaultPort, "https://example.com", ());

  // A normalized origin round trips unchanged.
  for (auto const * url : {"https://example.com/x", "http://example.com:80", "https://example.com:80",
                           "https://example.com:8443?q", "https://192.168.0.1/x"})
  {
    auto const parsed = ParseHttpOrigin(url, false);
    TEST(parsed.has_value(), (url));
    TEST_EQUAL(ParseHttpOrigin(*parsed, false), parsed, (url));
  }
}

UNIT_TEST(HttpOrigin_QueryIsNotScanned)
{
  // Every byte here would make the parser fail if it ever looked past the authority.
  string const poison = "@%\\ \t\x7f\xd0\xbf";
  size_t const kTailSize = 10 * 1024 * 1024;

  string url = "https://example.com/path?";
  url.reserve(url.size() + kTailSize + poison.size());
  while (url.size() < kTailSize)
    url += poison;

  TEST_EQUAL(ParsedOrigin(url), "https://example.com", ());
  TEST_EQUAL(ParsedOrigin("https://example.com:8443#" + string(kTailSize, '\\')), "https://example.com:8443", ());
  TEST_EQUAL(ParsedOrigin("https://example.com?" + string(kTailSize, ' ')), "https://example.com", ());

  // Without any delimiter the authority is simply too long, which is also decided without
  // scanning the tail.
  string const noDelimiter = "https://example.com" + string(kTailSize, 'a');
  TEST_EQUAL(ParsedOrigin(noDelimiter), "", ());

  // Sanity check that the work does not depend on the tail size: a parser scanning these 10 MB
  // would need far more than a second for 100 calls in a debug build.
  size_t const kIterations = 100;
  base::Timer timer;
  for (size_t i = 0; i < kIterations; ++i)
  {
    TEST(ParseHttpOrigin(url, false).has_value(), ());
    TEST(!ParseHttpOrigin(noDelimiter, false), ());
  }
  double const elapsed = timer.ElapsedSeconds();
  TEST_LESS(elapsed, 1.0, ("Parsing", kIterations, "urls with a 10 MB tail took", elapsed, "seconds"));
}

// The lenient url::Url parser is intentionally left as is, ParseHttpOrigin() is a separate,
// strict entry point.
UNIT_TEST(HttpOrigin_UrlClassIsUnchanged)
{
  Url const withCredentials("https://user:pw@example.com/x");
  TEST_EQUAL(withCredentials.GetScheme(), "https", ());
  TEST_EQUAL(withCredentials.GetHost(), "user:pw@example.com", ());
  TEST_EQUAL(withCredentials.GetPath(), "x", ());
  TEST(!ParseHttpOrigin("https://user:pw@example.com/x", false), ());

  Url const nonAscii = Url::FromString("\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.\xd1\x80\xd1\x84/x");
  TEST_EQUAL(nonAscii.GetScheme(), "https", ());
  TEST_EQUAL(nonAscii.GetHost(), "\xd0\xbf\xd1\x80\xd0\xb8\xd0\xbc\xd0\xb5\xd1\x80.\xd1\x80\xd1\x84", ());
  TEST(!ParseHttpOrigin("https://" + nonAscii.GetHostAndPath(), false), ());
}

}  // namespace url_tests
