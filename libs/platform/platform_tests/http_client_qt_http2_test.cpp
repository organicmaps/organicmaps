#include "testing/testing.hpp"

#include "platform/http_client_qt.hpp"

namespace http_client_qt_http2_test
{
using platform::IsOsmHost;

UNIT_TEST(HttpClientQt_IsOsmHost_ProdAndDevHosts)
{
  // OSM OAuth handshake (www), editing API, and the dev server — all force HTTP/1.1 on Qt <= 6.4.
  TEST(IsOsmHost("www.openstreetmap.org"), ());
  TEST(IsOsmHost("api.openstreetmap.org"), ());
  TEST(IsOsmHost("master.apis.dev.openstreetmap.org"), ());
  // Bare apex.
  TEST(IsOsmHost("openstreetmap.org"), ());
}

UNIT_TEST(HttpClientQt_IsOsmHost_CaseInsensitive)
{
  // QUrl::host() lowercases hosts, but the predicate is robust to mixed case regardless.
  TEST(IsOsmHost("WWW.OpenStreetMap.ORG"), ());
  TEST(IsOsmHost("OPENSTREETMAP.ORG"), ());
}

UNIT_TEST(HttpClientQt_IsOsmHost_RejectsLookAlikes)
{
  // The suffix must sit on a real subdomain boundary — these are NOT OSM hosts and must
  // keep HTTP/2. A naive endsWith("openstreetmap.org") would wrongly accept the first two.
  TEST(!IsOsmHost("notopenstreetmap.org"), ());
  TEST(!IsOsmHost("evil-openstreetmap.org"), ());
  TEST(!IsOsmHost("openstreetmap.org.evil.com"), ());
  TEST(!IsOsmHost("openstreetmap.com"), ());
  TEST(!IsOsmHost("example.com"), ());
  TEST(!IsOsmHost(""), ());
}
}  // namespace http_client_qt_http2_test
