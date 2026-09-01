#pragma once

#include <string>
#include <string_view>

namespace kml
{
// Heuristics for rendering HTML bookmark/track descriptions with network-gated HTTPS images.
// This is not a sanitizer: callers must enforce the restricted rendering policy with CSP and
// native network controls, including a network-denied document when m_requiresNetwork is false.
//
// Base candidates, in priority order: HTML <video poster>, HTML <img srcset|src>, SVG <image href|xlink:href>.
// HTML <image> is an <img> alias. Equal priorities use element order; within an <img>, srcset precedes
// src. Within an SVG <image>, href overrides xlink:href, even when empty; src is ignored.
// <source srcset> and CSS url() in style attributes/<style> text are network hints only: they can
// refer to inert sources or blocked fonts/stylesheets, so they cannot select a base origin.
//
// Only https: and protocol-relative values are considered. Other HTML references and URL schemes
// are ignored. HTML entities are not decoded, except quotes around CSS URL arguments; CSS escapes
// and browser layout/resource selection are not modeled. Namespace boundaries and HTML integration
// points distinguish HTML raw text from self-closing foreign elements; this is not full HTML tree
// construction/error recovery. False positives can prompt unnecessarily or select an ineffective
// base; false negatives can prevent images from loading.
struct HtmlDescriptionScan
{
  // Strictly parsed "https://host[:port]" of the best base candidate, empty when none parses.
  std::string m_baseUrl;
  // A supported attribute/CSS URL starts with https: (case-insensitive) or //, parseable or not.
  bool m_requiresNetwork = false;

  bool operator==(HtmlDescriptionScan const &) const = default;
};

std::string DebugPrint(HtmlDescriptionScan const & scan);

// Linear scan of an untrusted document/fragment without a DOM. Auxiliary space depends only on
// nested foreign namespace boundaries and HTML integration points, not ordinary element nesting.
HtmlDescriptionScan ScanHtmlDescription(std::string_view html);
}  // namespace kml
