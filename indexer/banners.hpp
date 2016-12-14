#pragma once

#include "std/ctime.hpp"
#include "std/iostream.hpp"
#include "std/string.hpp"
#include "std/unordered_map.hpp"

class FeatureType;

namespace banner
{
class Banner
{
public:
  Banner() = default;
  explicit Banner(string const & id);

  bool IsEmpty() const { return m_id.empty(); }
  string GetMessageBase() const { return m_messageBase; }
  string GetIconName() const { return m_iconName; }
  string GetDefaultUrl() const { return m_defaultUrl; }
  string GetId() const { return m_id; }
  bool IsActive() const;

  /// Replaces inline variables in the URL, uses the default banner URL if url is not specified.
  string GetFormattedUrl(string const & url = {}) const;

  /// Usually called from BannerSet.
  void SetProperty(string const & name, string const & value);

private:
  string m_id;
  string m_messageBase;
  string m_iconName;
  string m_defaultUrl;
  time_t m_activeAfter;
  time_t m_activeBefore;
  unordered_map<string, string> m_properties;

  string GetProperty(string const & name) const;
};

class BannerSet
{
public:
  void LoadBanners();
  void ReadBanners(istream & s);

  bool HasBannerForType(uint32_t type) const;
  Banner const & GetBannerForType(uint32_t type) const;

  bool HasBannerForFeature(FeatureType const & ft) const;
  Banner const & GetBannerForFeature(FeatureType const & ft) const;

private:
  unordered_map<uint32_t, Banner> m_banners;

  void Add(Banner const & banner, string const & type);
};
}
