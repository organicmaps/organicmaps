#include "testing/testing.hpp"

#include "kml/html_description.hpp"

#include <initializer_list>
#include <iterator>
#include <random>
#include <string>
#include <string_view>

namespace html_description_tests
{
using kml::HtmlDescriptionScan;
using kml::ScanHtmlDescription;

struct ScanCase
{
  std::string_view m_html;
  std::string_view m_baseUrl;
  bool m_requiresNetwork;
};

void TestScans(std::initializer_list<ScanCase> const cases)
{
  for (auto const & c : cases)
  {
    auto const scan = ScanHtmlDescription(c.m_html);
    TEST_EQUAL(scan.m_baseUrl, c.m_baseUrl, (c.m_html));
    TEST_EQUAL(scan.m_requiresNetwork, c.m_requiresNetwork, (c.m_html));
  }
}

UNIT_TEST(HtmlDescription_Empty)
{
  TEST_EQUAL(ScanHtmlDescription(""), HtmlDescriptionScan{}, ());
  TEST_EQUAL(ScanHtmlDescription("Just a plain text description, 5 < 6 > 4."), HtmlDescriptionScan{}, ());
}

UNIT_TEST(HtmlDescription_PermittedResources)
{
  TestScans({
      // Base candidates and network-only hints.
      {"<img src=\"https://a.example.com/x.png\">", "https://a.example.com", true},
      {"<img srcset=\"https://a.example.com/x.png\">", "https://a.example.com", true},
      {"<picture><source srcset=\"https://a.example.com/x.png\"></picture>", "", true},
      {"<image src=\"https://a.example.com/x.svg\">", "https://a.example.com", true},
      {"<svg><image href=\"https://a.example.com/x.svg\"/></svg>", "https://a.example.com", true},
      {"<svg><image xlink:href=\"https://a.example.com/x.svg\"/></svg>", "https://a.example.com", true},
      {"<video poster=\"https://a.example.com/x.jpg\"></video>", "https://a.example.com", true},
      {"<div style=\"background-image:url(https://a.example.com/x.png)\">t</div>", "", true},
      {"<style>.b { background: url(https://a.example.com/x.png) no-repeat; }</style>", "", true},

      // Case insensitivity of tag, attribute and scheme names; the host is lowercased.
      {"<IMG SRC=\"HTTPS://A.Example.COM/x.png\">", "https://a.example.com", true},
      {"<Video PoStEr=\"https://A.example.com/x.jpg\"></Video>", "https://a.example.com", true},
      {"<div STYLE=\"background:URL(https://A.example.com/x.png)\"></div>", "", true},
      {"<STYLE>a{background:url(https://A.example.com/x.png)}</STYLE>", "", true},
      {"<STYLE>a{color:red}</STYLE><SCRIPT>var a=1</SCRIPT><img src=\"https://a.example.com/x.png\">",
       "https://a.example.com", true},

      // Attribute syntax variations.
      {"<img src=https://a.example.com/x.png>", "https://a.example.com", true},
      {"<img src=https://a.example.com/x.png alt=hi>", "https://a.example.com", true},
      {"<img\n\tsrc\t=\n \"https://a.example.com/x.png\"  />", "https://a.example.com", true},
      {"<img src='https://a.example.com/x.png'>", "https://a.example.com", true},
      {"<img src=\"  https://a.example.com/x.png  \">", "https://a.example.com", true},
      {"<img alt=\"a > b\" src=\"https://a.example.com/x.png\">", "https://a.example.com", true},
      {"<img src=\"https://a.example.com/x.png\"", "https://a.example.com", true},

      // Protocol relative urls are https under the restricted policy.
      {"<img src=\"//a.example.com/x.png\">", "https://a.example.com", true},
      {"<div style=\"background:url(//a.example.com/x.png)\"></div>", "", true},

      // Subdomains and non default ports.
      {"<img src=\"https://cdn.images.a.example.com/x.png\">", "https://cdn.images.a.example.com", true},
      {"<img src=\"https://a.example.com:8443/x.png\">", "https://a.example.com:8443", true},
      {"<img src=\"https://a.example.com:443/x.png\">", "https://a.example.com", true},

      // Srcset entries and descriptors.
      {"<img srcset=\"https://a.example.com/1.png 1x, https://b.example.com/2.png 2x\">", "https://a.example.com",
       true},
      {"<img srcset=\"local.png 1x, https://b.example.com/2.png 640w\">", "https://b.example.com", true},
      {"<img srcset=\"local.png, https://b.example.com/2.png\">", "https://b.example.com", true},
      {"<img srcset=\"\n  https://a.example.com/1.png   1.5x ,\n  https://b.example.com/2.png 2x\n\">",
       "https://a.example.com", true},
      {"<source srcset=\"local.png 1x\"><source srcset=\"//b.example.com/2.png 2x\">", "", true},
      {"<img srcset=\"https://a.example.com/x?w=1,2 2x, https://b.example.com/y 1x\">", "https://a.example.com", true},
      {"<img srcset=\",https://a.example.com/1.png,\">", "https://a.example.com", true},

      // A fragment and a complete document are scanned the same way.
      {"<p>Text</p><img src=\"https://a.example.com/x.png\"><p>More</p>", "https://a.example.com", true},
      {"<!DOCTYPE html>\n<html lang=\"en\"><head><title>T</title>"
       "<style>body{background:url(https://css.example.com/b.png)}</style></head>"
       "<body><p>Hi</p><img src=\"https://a.example.com/x.png\"></body></html>",
       "https://a.example.com", true},
  });
}

UNIT_TEST(HtmlDescription_Priorities)
{
  TestScans({
      // A poster beats an image beats an SVG image; CSS cannot select a base.
      {"<div style=\"background:url(https://css.example.com/b.png)\"></div>"
       "<svg><image href=\"https://svg.example.com/i.svg\"/></svg>"
       "<img src=\"https://img.example.com/x.png\">"
       "<video poster=\"https://poster.example.com/p.jpg\"></video>",
       "https://poster.example.com", true},
      {"<img src=\"https://img.example.com/x.png\"><video poster=\"https://poster.example.com/p.jpg\"></video>",
       "https://poster.example.com", true},
      {"<svg><image href=\"https://svg.example.com/i.svg\"/></svg><img src=\"https://img.example.com/x.png\">",
       "https://img.example.com", true},
      {"<div style=\"background:url(https://css.example.com/b.png)\"></div>"
       "<svg><image href=\"https://svg.example.com/i.svg\"/></svg>",
       "https://svg.example.com", true},

      // Equal priorities are resolved by the document order.
      {"<img src=\"https://first.example.com/a.png\"><img src=\"https://second.example.com/b.png\">",
       "https://first.example.com", true},
      {"<image src='https://first.example/x'><img src='https://second.example/y'>", "https://first.example", true},
      {"<img src='https://a.example.com/x' srcset='https://b.example.com/y 2x'>", "https://b.example.com", true},
      {"<img srcset='https://b.example.com/y 2x' src='https://a.example.com/x'>", "https://b.example.com", true},
      {"<img src='https://first.example.com/x'><img srcset='https://second.example.com/y 2x'>",
       "https://first.example.com", true},
      {"<picture><source srcset=\"https://s.example.com/x 2x\"><img src=\"https://i.example.com/y\"></picture>",
       "https://i.example.com", true},
      {"<video poster=\"https://p1.example.com/x\"></video><video poster=\"https://p2.example.com/y\"></video>",
       "https://p1.example.com", true},

      // The best origin is the best *parseable* one, an unparseable url only requires the network.
      {"<img src=\"https://exa mple.com/x\"><img src=\"https://b.example.com/y\">", "https://b.example.com", true},
      {"<video poster=\"https://p@evil.example.com/x\"></video><img src=\"https://b.example.com/y\">",
       "https://b.example.com", true},
  });
}

UNIT_TEST(HtmlDescription_NetworkOnlyHints)
{
  TestScans({
      {"<source srcset='https://source.example/x'><img src='https://image.example/y'>", "https://image.example", true},
      {"<style>@font-face{font-family:X;src:url(https://fonts.example/font.woff2)}"
       "body{background:url(https://images.example/photo.png)}</style>",
       "", true},
      {"<style>@import url(https://styles.example/main.css)</style><img src='https://images.example/x'>",
       "https://images.example", true},
  });
}

UNIT_TEST(HtmlDescription_DuplicateAttributes)
{
  TestScans({
      {"<img src=local.png SRC=https://ignored.example/x><img src=https://images.example/y>", "https://images.example",
       true},
      {"<img src='' src=https://ignored.example/x>", "", false},
      {"<img srcset='' SRCSET='https://ignored.example/x 2x' src=https://images.example/y>", "https://images.example",
       true},
      {"<video poster='' POSTER=https://ignored.example/x></video>", "", false},
      {"<svg><image href='' HREF=https://ignored.example/x></svg>", "", false},
      {"<div style='' STYLE='background:url(https://ignored.example/x)'></div>", "", false},
      {"<img src=https://images.example/y src=https://ignored.example/x>", "https://images.example", true},
  });
}

UNIT_TEST(HtmlDescription_SvgReferences)
{
  TestScans({
      {"<svg><image xlink:href='https://unused.example/x' href='https://actual.example/y'/></svg>",
       "https://actual.example", true},
      {"<svg><image href='https://actual.example/y' xlink:href='https://unused.example/x'/></svg>",
       "https://actual.example", true},
      {"<svg><image xlink:href='https://unused.example/x' href=''/></svg>", "", false},
      {"<svg><image href='' xlink:href='https://unused.example/x'/></svg>", "", false},
      {"<svg><image href='' HREF='https://unused.example/x' xlink:href='https://unused.example/y'/></svg>", "", false},
      {"<svg><image href='local.png' xlink:href='https://unused.example/x'/></svg>", "", false},
      {"<svg><image src='https://unused.example/x' href='https://actual.example/y'/></svg>", "https://actual.example",
       true},
      {"<svg><image src='https://unused.example/x'/></svg>", "", false},
      {"<image src='https://actual.example/x' href='https://unused.example/y'>", "https://actual.example", true},
      {"<image href='https://unused.example/y' src='https://actual.example/x'>", "https://actual.example", true},
      {"<image href='https://unused.example/y'>", "", false},
  });
}

UNIT_TEST(HtmlDescription_TextContexts)
{
  TestScans({
      {"<textarea><script></textarea><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<title><style></title><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<textarea><video poster='https://unused.example/y'></textarea><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<xmp><video poster='https://unused.example/y'></xmp><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<iframe><video poster='https://unused.example/y'></iframe><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<plaintext><img src='https://unused.example/x'>", "", false},

      // A trailing slash closes foreign elements, but cannot close HTML raw-text elements.
      {"<svg><script/><image href='https://actual.example/x'/></svg>", "https://actual.example", true},
      {"<svg><style/></svg><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<math><style/></math><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<script/><img src='https://unused.example/x'>", "", false},
      {"<style/><img src='https://unused.example/x'>", "", false},

      // SVG integration points contain HTML, including HTML script and title semantics.
      {"<svg><title><video poster='https://actual.example/x'></title></svg>", "https://actual.example", true},
      {"<svg><title><script/></title></svg><img src='https://unused.example/x'>", "", false},
      {"<svg><foreignObject><title><script></title><img src='https://actual.example/x'></foreignObject></svg>",
       "https://actual.example", true},
      {"<svg><foreignObject><svg><style/></svg><img src='https://actual.example/x'></foreignObject></svg>",
       "https://actual.example", true},
      {"<svg><g><div><script/><img src='https://unused.example/x'>", "", false},
      {"<svg><font color=''><script/><img src='https://unused.example/x'>", "", false},
      {"<svg><font><script/></font></svg><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<svg><g><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<math><mtext><title><style></title><img src='https://actual.example/x'></mtext></math>",
       "https://actual.example", true},
      {"<math><annotation-xml encoding='text/html'><title><style></title>"
       "<img src='https://actual.example/x'></annotation-xml></math>",
       "https://actual.example", true},
      {"<math><annotation-xml encoding=' text/html '><style/></annotation-xml></math>"
       "<img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<math><annotation-xml encoding='' ENCODING='text/html'><style/></annotation-xml></math>"
       "<img src='https://actual.example/x'>",
       "https://actual.example", true},
  });
}

UNIT_TEST(HtmlDescription_ScriptEscapes)
{
  TestScans({
      {"<script><!--<script></script><video poster='https://unused.example/y'>--></script>"
       "<img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<script><!--<SCRIPT ></sCrIpT/><video poster='https://unused.example/y'></script>"
       "<img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<script><!--<script>--></script><img src='https://actual.example/x'>", "https://actual.example", true},
      {"<script><!--<scripture></script><img src='https://actual.example/x'>", "https://actual.example", true},
  });
}

UNIT_TEST(HtmlDescription_InertMarkup)
{
  TestScans({
      {"<template><video poster='https://unused.example/y'></template><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<template><template><img src='https://unused.example/y'></template>"
       "<style>body{background:url(https://unused.example/z)}</style></template>"
       "<img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<template><math><annotation-xml encoding='text/html'><title><script></title>"
       "</annotation-xml></math></template><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"<template><img src='https://unused.example/x'></template>", "", false},
      {"</div data-x=\"junk > <video poster='https://unused.example/y'>\"><img src='https://actual.example/x'>",
       "https://actual.example", true},
      {"</div style='background:url(https://unused.example/y)'><img src='https://actual.example/x'>",
       "https://actual.example", true},
  });
}

UNIT_TEST(HtmlDescription_CssUrls)
{
  TestScans({
      // Quoting, including the entity encoded quotes of an escaped style attribute.
      {"<div style=\"background:url('https://a.example.com/x.png')\"></div>", "", true},
      {"<div style='background:url(\"https://a.example.com/x.png\")'></div>", "", true},
      {"<div style=\"background:url( &quot;https://a.example.com/x.png&quot; )\"></div>", "", true},
      {"<div style=\"background:url(&#39;https://a.example.com/x.png&#39;)\"></div>", "", true},
      {"<div style=\"background:url(&apos;https://a.example.com/x.png&apos;)\"></div>", "", true},
      {"<div style=\"background:url(&#34;https://a.example.com/x.png&#34;)\"></div>", "", true},
      {"<div style=\"background:url(&#x27;https://a.example.com/x.png&#x27;)\"></div>", "", true},
      {"<div style=\"background:url(&#X22;https://a.example.com/x.png&#X22;)\"></div>", "", true},
      {"<style>a{background:url(  \"https://a.example.com/x.png\"  )}</style>", "", true},

      // A quoted value keeps its spaces, an unquoted one ends at the first space.
      {"<style>a{background:url(\" https://a.example.com/x y.png \")}</style>", "", true},
      {"<style>a{background:url(https://a.example.com/x?a=1&b=2)}</style>", "", true},

      // Values that are not closed by the ')' of the function make a browser drop the whole
      // declaration, so they are not resources at all.
      {"<div style=\"background:url(https://exa mple.com/x)\"></div>", "", false},
      {"<div style=\"background:url(https://a.example.com/x junk)\"></div>", "", false},
      {"<div style=\"background:url(https://a.example.com/x')\"></div>", "", false},

      // A value that the css text ends on is still a resource, exactly like in a browser.
      {"<style>a{background:url(https://a.example.com/x</style>", "", true},
      {"<style>a{background:url(\"https://a.example.com/x</style>", "", true},

      {"<div style=\"background:image-set(url(https://a.example.com/x) 1x)\"></div>", "", true},

      // Comments and ordinary strings neither fetch nor hide subsequent URL functions.
      {"<style>/* url(https://a.example.com/x.png) */</style>", "", false},
      {"<style>body::before{content:\"url('https://a.example.com/x')\"}</style>", "", false},
      {"<style>/* url(\" */ body{background:url(https://a.example.com/x)}</style>", "", true},
      {"<style>body::before{content:\"url('\"}body{background:url(https://a.example.com/x)}</style>", "", true},
      {R"(<style>body::before{content:"a\" url('"}body{background:url(https://a.example.com/x)}</style>)", "", true},
      {"<style>/* https://a.example.com */body{background:url(https://b.example.com/x)}</style>", "", true},

      // A function name is a whole identifier and touches its opening parenthesis.
      {"<div style=\"background:url (https://a.example.com/x)\"></div>", "", false},
      {"<div style=\"background:no-url(https://a.example.com/x)\"></div>", "", false},
      {"<style>a{background:xurl(https://a.example.com/x)}</style>", "", false},
  });
}

UNIT_TEST(HtmlDescription_IgnoredResources)
{
  TestScans({
      // Everything the restricted policy blocks anyway.
      {"<a href=\"https://a.example.com/x\">link</a>", "", false},
      {"<form action=\"https://a.example.com/x\"><input formaction=\"https://b.example.com/y\"></form>", "", false},
      {"<script src=\"https://a.example.com/x.js\"></script>", "", false},
      {"<iframe src=\"https://a.example.com/x\"></iframe><frame src=\"https://b.example.com/y\">", "", false},
      {"<object data=\"https://a.example.com/x\"></object><embed src=\"https://b.example.com/y\">", "", false},
      {"<link rel=\"stylesheet\" href=\"https://a.example.com/x.css\">", "", false},
      {"<link rel=\"preload\" as=\"font\" href=\"https://a.example.com/x.woff2\" crossorigin>", "", false},
      {"<style>@font-face{font-family:X;src:local(X)}</style><base href=\"https://a.example.com/\">", "", false},
      {"<meta http-equiv=\"refresh\" content=\"0;url=https://a.example.com/x\">", "", false},
      {"<video src=\"https://a.example.com/x.mp4\"><source src=\"https://b.example.com/y.mp4\"></video>", "", false},
      {"<audio src=\"https://a.example.com/x.mp3\"></audio>", "", false},

      // Only media is ignored in a <video>, its poster is still an image.
      {"<video src=\"https://v.example.com/m.mp4\" poster=\"https://p.example.com/x.jpg\"></video>",
       "https://p.example.com", true},

      // Only explicit HTTPS/protocol-relative references contribute hints.
      {"<img src=\"http://a.example.com/x.png\">", "", false},
      {"<img src=\"HTTP://a.example.com/x.png\">", "", false},
      {"<img src=\"x.png\"><img src=\"/abs/x.png\"><img src=\"../up/x.png\">", "", false},
      {"<img src=\"data:image/png;base64,iVBORw0KGgo=\">", "", false},
      {"<img src=\"blob:https://a.example.com/1234\">", "", false},
      {"<img src=\"file:///etc/passwd\">", "", false},
      {"<img src=\"javascript:alert(1)\">", "", false},
      {"<div style=\"background:url(http://a.example.com/x.png)\"></div>", "", false},
      {"<div style=\"background:url(data:image/gif;base64,R0lGOD)\"></div>", "", false},
  });
}

UNIT_TEST(HtmlDescription_Tokenizer)
{
  TestScans({
      // Markup that only looks like a resource.
      {"<!-- <img src=\"https://a.example.com/x.png\"> -->", "", false},
      {"<!-- unterminated <img src=\"https://a.example.com/x.png\">", "", false},
      {"<!--!><img src=https://ignored.example/x>", "", false},
      {"<!---!><img src=https://ignored.example/x>", "", false},
      {"<!--!><img src=https://ignored.example/x> --><img src=https://images.example/y>", "https://images.example",
       true},
      {"<script>var s = '<img src=\"https://a.example.com/x.png\">';</script>", "", false},
      {"<script><style>a{background:url(https://a.example.com/x)}</style></script>", "", false},
      {"<script type=\"text/template\"><img src=\"https://a.example.com/x.png\"></script>", "", false},
      {"<!DOCTYPE html PUBLIC \"<img src=https://a.example.com/x.png>\">", "", false},
      {"<?xml-stylesheet href=\"https://a.example.com/x.css\"?>", "", false},
      {"</img src=\"https://a.example.com/x.png\">", "", false},

      // A quoted attribute value swallows the markup inside it: only the real src is a candidate.
      {"<img alt='<img src=\"https://evil.example.com/x\">' src=\"https://good.example.com/y\">",
       "https://good.example.com", true},

      // Short comments and both comment terminators release subsequent markup.
      {"<!--><img src=\"https://a.example.com/x\">", "https://a.example.com", true},
      {"<!---><img src=\"https://a.example.com/x\">", "https://a.example.com", true},
      {"<!----><img src=\"https://a.example.com/x\">", "https://a.example.com", true},
      {"<!-- comment --!><img src=\"https://a.example.com/x\">", "https://a.example.com", true},
      {"<script>var s = \"</script>\";</script><img src=\"https://a.example.com/x\">", "https://a.example.com", true},

      // A raw-text end tag must have the complete matching name and a delimiter.
      {"<script>const s='</scripture><video poster=https://ignored.example/x>';</script>"
       "<img src=https://images.example/y>",
       "https://images.example", true},
      {"<style>/* </stylesheet> */body{background:url(https://a.example.com/x)}</style>", "", true},
      {"<script></script\v><video poster=https://ignored.example/x></script><img src=https://images.example/y>",
       "https://images.example", true},
      {"<img\vsrc=https://ignored.example/x>", "", false},
  });
}

UNIT_TEST(HtmlDescription_MalformedUrls)
{
  // A browser may fetch a url that the strict origin parser rejects, hence requiresNetwork without a base.
  TestScans({
      {"<img src=\"https://exa mple.com/x\">", "", true},
      {"<img src=\"https://user:pw@evil.example.com/x\">", "", true},
      {"<img src=\"https://пример.рф/x\">", "", true},
      {"<img src=\"https://example.com:0/x\">", "", true},
      {"<img src=\"https://example.com:65536/x\">", "", true},
      {"<img src=\"https://example.com:port/x\">", "", true},
      {"<img src=\"https://exa%6dple.com/x\">", "", true},
      {"<img src=\"https:/example.com/x\">", "", true},
      {"<img src=\"https://\">", "", true},
      {"<img src=\"https:\">", "", true},
      {"<img src=\"//\">", "", true},

      // Entities are not decoded, so an obfuscated authority never produces a base origin.
      {"<img src=\"https://&#101;xample.com/x\">", "", true},
      {"<img src=\"&#104;ttps://example.com/x\">", "", false},
      {"<div style=\"background:url(https://&#101;xample.com/x)\"></div>", "", true},
  });
}

UNIT_TEST(HtmlDescription_RealWorldDescriptions)
{
  TestScans({
      // Google My Maps: a hosted photo, a line break and a link back to the map.
      {"<div dir=\"ltr\"><img src=\"https://lh3.googleusercontent.com/p/AF1QipN-abc=w408-h306-k-no\" "
       "height=\"200\" width=\"267\"><br>Nice place, see "
       "<a href=\"https://www.google.com/maps/place/?q=place_id:ChIJ123\">the map</a>.<br></div>",
       "https://lh3.googleusercontent.com", true},

      // An Amazon product snippet: a <picture> with a srcset and its fallback image.
      {"<picture><source media=\"(max-width:400px)\" "
       "srcset=\"https://m.media-amazon.com/images/I/71abc._AC_SL200_.jpg 1x, "
       "https://m.media-amazon.com/images/I/71abc._AC_SL400_.jpg 2x\">"
       "<img src=\"https://m.media-amazon.com/images/I/71abc._AC_SL1000_.jpg\" alt=\"Product\"></picture>",
       "https://m.media-amazon.com", true},

      // A kml CDATA section is unwrapped by the xml parser, but even when its delimiters survive
      // they are a bogus comment that ends at the first '>', exactly like in a browser.
      {"<![CDATA[<b>Hotel</b><br><img src=\"https://cdn.example.org/hotel.jpg\"/>]]>", "https://cdn.example.org", true},
      {"<![CDATA[<img src=\"https://cdn.example.org/hotel.jpg\">]]>", "", false},

      // A complete document: the css background of the head loses to the image of the body.
      {"<!DOCTYPE html><html><head><style>body{background:url(https://cdn.example.org/bg.png)}</style></head>"
       "<body><p>Hi</p><img src=\"https://img.example.net/photo.jpg\"></body></html>",
       "https://img.example.net", true},

      // Nothing that the restricted policy would ever load.
      {"<p>See</p><img src=\"http://cdn.example.org/photo.jpg\">", "", false},
      {"<img src=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAAAAAA6fptVAAAACklEQVR4nGMAAQAABQAB\">", "",
       false},
      {"<script>var t = \"<img src='https://evil.example/x'>\"; document.write(t);</script>", "", false},

      // Upper case markup and an svg image reference.
      {"<IMG SRC=HTTPS://A.EXAMPLE/X>", "https://a.example", true},
      {"<svg width=\"10\" height=\"10\"><image xlink:href=\"https://cdn.example/x.svg\"/></svg>", "https://cdn.example",
       true},
  });
}

UNIT_TEST(HtmlDescription_KnownFalseNegatives)
{
  // Unsupported syntax requires the caller's network-denied document when no hint is found.
  TestScans({
      {"<input type=\"image\" src=\"https://a.example.com/x.png\">", "", false},
      {"<body background=\"https://a.example.com/x.png\">", "", false},
      {"<div style='background:image-set(\"https://a.example.com/x.png\" 1x)'></div>", "", false},
  });
}

std::string Repeat(std::string_view chunk, size_t count)
{
  std::string res;
  res.reserve(chunk.size() * count);
  for (size_t i = 0; i < count; ++i)
    res += chunk;
  return res;
}

UNIT_TEST(HtmlDescription_Adversarial)
{
  size_t constexpr kMb = 1024 * 1024;

  // A megabyte of tag openings, nothing to find and no rescanning.
  TEST_EQUAL(ScanHtmlDescription(std::string(kMb, '<')), HtmlDescriptionScan{}, ());

  // An unterminated quote swallows the rest of the input, an unterminated comment too.
  TEST_EQUAL(ScanHtmlDescription("<img alt=\"" + std::string(kMb, 'a')), HtmlDescriptionScan{}, ());
  TEST_EQUAL(ScanHtmlDescription("<img alt=\"" + std::string(kMb, 'a') + "> <img src='https://a.example.com/x'>"),
             HtmlDescriptionScan{}, ());
  TEST_EQUAL(ScanHtmlDescription("<!-- " + std::string(kMb, 'a') + "<img src=\"https://a.example.com/x\">"),
             HtmlDescriptionScan{}, ());
  // Repeated alternate comment endings must not repeatedly search the remaining document.
  TEST_EQUAL(ScanHtmlDescription(Repeat("<!-- --!>", 200000) + "<img src=https://a.example.com/x>"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());
  // The origin is still found when the closing quote is missing.
  TEST_EQUAL(ScanHtmlDescription("<img src=\"https://a.example.com/x" + std::string(kMb, 'a')),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());

  // 200k attributes on a single tag, with the only interesting one at the end.
  TEST_EQUAL(ScanHtmlDescription("<img " + Repeat("data-x=1 ", 200000) + "src=\"https://a.example.com/x\">"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());

  // A 5 MB srcset with the only https candidate at the very end.
  TEST_EQUAL(ScanHtmlDescription("<img srcset=\"" + Repeat("local.png 1x, ", 375000) + "https://a.example.com/x 2x\">"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());

  // A megabyte of tag openings inside a raw text element: its end tag is searched for only once.
  TEST_EQUAL(ScanHtmlDescription("<style>" + std::string(kMb, '<') + "</style><img src=\"https://a.example.com/x\">"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());
  TEST_EQUAL(ScanHtmlDescription("<script>" + std::string(kMb, '<') + "</script><img src=\"https://a.example.com/x\">"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());

  // A megabyte of css function openings, nothing to find and no rescanning.
  TEST_EQUAL(ScanHtmlDescription("<style>" + Repeat("url(", kMb / 4)), HtmlDescriptionScan{}, ());

  // 100k nested elements, no recursion is involved.
  TEST_EQUAL(
      ScanHtmlDescription(Repeat("<div>", 100000) + "<img src=\"https://a.example.com/x\">" + Repeat("</div>", 100000)),
      (HtmlDescriptionScan{"https://a.example.com", true}), ());

  // Foreign boundaries use a compact stack; unrelated end tags must not search that stack.
  TEST_EQUAL(ScanHtmlDescription(Repeat("<svg>", 20000) + Repeat("</g>", 20000) + Repeat("</svg>", 20000) +
                                 "<img src='https://a.example.com/x'>"),
             (HtmlDescriptionScan{"https://a.example.com", true}), ());
}

UNIT_TEST(HtmlDescription_RandomAttributeValues)
{
  // Include markup-like text and raw bytes inside a quoted attribute.
  std::string_view const kAlphabet = "<>/\\\"'&#;=- \t\n\ripgsrcvtylueoah:.0123456789";

  std::minstd_rand rnd(2026);  // A fixed seed keeps the test reproducible.
  std::string html;
  for (size_t i = 0; i < 2000; ++i)
  {
    std::string const baseUrl = "https://seed" + std::to_string(i) + ".example";
    char const quote = i % 2 == 0 ? '\'' : '"';
    html = "<img alt=";
    html += quote;
    size_t const size = rnd() % 4096;
    bool const rawBytes = (rnd() % 4) == 0;
    for (size_t j = 0; j < size; ++j)
    {
      char const c = rawBytes ? static_cast<char>(rnd() & 0xFF) : kAlphabet[rnd() % kAlphabet.size()];
      if (c == quote)
        html += quote == '\'' ? "&#39;" : "&quot;";
      else
        html += c;
    }
    html += quote;
    html += " src='" + baseUrl + "/x'>";

    // Attribute contents cannot hide the real src or introduce resource attributes of their own.
    TEST_EQUAL(ScanHtmlDescription(html), (HtmlDescriptionScan{baseUrl, true}), (i));
  }
}

UNIT_TEST(HtmlDescription_LargeMixedInput)
{
  std::string_view constexpr kChunk =
      "<img src='https://a.example.com/x.png'>"
      "<<<<<<<<<!-- comment -->"
      "<img alt='quoted <img src=https://unused.example/x>'>"
      "<div style='background:url(https://b.example.com/y.png)'>text</div>"
      "<img srcset='https://c.example.com/z.png 2x, d.png 1x'>"
      "<script>if (a < b) { }</script></div></p>";
  // Complete chunks leave the final poster outside quotes/comments. Its priority proves that the
  // whole mixed input was scanned, even after an earlier image had already selected an origin.
  TEST_EQUAL(ScanHtmlDescription(Repeat(kChunk, 8 * 1024 * 1024 / kChunk.size() + 1) +
                                 "<video poster='https://late.example/poster.png'></video>"),
             (HtmlDescriptionScan{"https://late.example", true}), ());
}
}  // namespace html_description_tests
