#pragma once

#include "indexer/feature_meta.hpp"

#include "geometry/latlon.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// Builds the text shared when a user shares a place, the current position or a bookmark.
// The logic lives in the core so Android, iOS and the desktop app all produce the same output
// (see Framework::GetShareData). The place page subject line stays platform-side because it needs
// a localized format placeholder that can't be shared through the core StringsBundle.
namespace share
{
inline constexpr size_t kSharedMetadataFieldsCount = 6;

// Localized, placeholder-free labels the body builder needs, filled from the core StringsBundle.
struct Strings
{
  std::string m_myPosition;         // "I am here on Organic Maps"
  std::string m_openInOmOrBrowser;  // "Open in Organic Maps or in a browser"
  std::string m_openInMapsApp;      // "Open in another maps app"
  std::string m_getApp;             // "Get Organic Maps"
};

// A resolved place to share: name already filtered, address already reverse-geocoded if needed.
struct Place
{
  std::string m_name;       // empty for an unknown place or the current position
  std::string m_typeLabel;  // place page subtitle, e.g. "Restaurant • Italian"
  std::string m_address;
  ms::LatLon m_ll;
  int m_zoom = 0;
  bool m_isMyPosition = false;
  // Allow-listed metadata in display order: (type, readable value). Wiki values are already URLs.
  std::vector<std::pair<feature::Metadata::EType, std::string>> m_fields;
};

struct Result
{
  std::string m_url;   // clear omaps.app link (also used as the "copy link" value)
  std::string m_text;  // plain body for messengers
  std::string m_html;  // rich body for email
  // Place name, else address, else empty. Platforms build the email subject from it (they own the
  // localized "%1 on Organic Maps" format placeholder, which can't be shared through the bundle).
  std::string m_subjectBasis;
};

// Pure: builds the shareable link, the plain body and the HTML body. No I/O.
Result Build(Place const & place, Strings const & strings);

// Metadata fields included in the shared body, in display order. Exposed so that Framework
// collects exactly these from a place_page::Info.
std::array<feature::Metadata::EType, kSharedMetadataFieldsCount> const & SharedMetadataFields();
}  // namespace share
