#pragma once

#include "partners_api/utm.hpp"

#include "metrics/eye.hpp"

#include "platform/http_client.hpp"
#include "platform/safe_callback.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <string>
#include <vector>

#include "private.h"

namespace promo
{
struct Place
{
  bool IsEmpty() const { return m_name.empty() || m_description.empty(); }

  std::string m_name;
  std::string m_description;
};
struct Author
{
  std::string m_id;
  std::string m_name;
};
struct LuxCategory
{
  std::string m_name;
  std::string m_color;
};

struct CityGallery
{
  struct Item
  {
    std::string m_name;
    std::string m_url;
    std::string m_imageUrl;
    std::string m_access;
    std::string m_tier;
    std::string m_tourCategory;
    Place m_place;
    Author m_author;
    LuxCategory m_luxCategory;
  };

  bool IsEmpty() const
  {
    return m_items.empty() || (m_items.size() == 1 && m_items.back().m_place.IsEmpty());
  }

  std::string m_moreUrl;
  std::string m_category;
  std::vector<Item> m_items;
};

struct AfterBooking
{
  AfterBooking() = default;

  bool IsEmpty() const
  {
    return m_id.empty() || m_promoUrl.empty() || m_pictureUrl.empty();
  }

  std::string m_id;
  std::string m_promoUrl;
  std::string m_pictureUrl;
};

using CityGalleryCallback = platform::SafeCallback<void(CityGallery const & gallery)>;
using OnError = platform::SafeCallback<void()>;
using Tags = std::vector<std::string>;

class Api : public eye::Subscriber
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual std::string GetCityId(m2::PointD const & point) = 0;
    virtual platform::HttpClient::Headers GetHeaders() = 0;
  };

  Api(std::string const & baseUrl = BOOKMARKS_CATALOG_FRONT_URL,
      std::string const & basePicturesUrl = PICTURES_URL);

  void SetDelegate(std::unique_ptr<Delegate> delegate);
  AfterBooking GetAfterBooking(std::string const & lang) const;
  std::string GetLinkForDownloader(std::string const & id) const;
  std::string GetCityUrl(m2::PointD const & point) const;
  void GetCityGallery(m2::PointD const & point, std::string const & lang, UTM utm,
                      CityGalleryCallback const & onSuccess, OnError const & onError) const;
  void GetPoiGallery(m2::PointD const & point, std::string const & lang, Tags const & tags,
                     bool useCoordinates, UTM utm, CityGalleryCallback const & onSuccess,
                     OnError const & onError) const;

  // eye::Subscriber overrides:
  void OnTransitionToBooking(m2::PointD const & hotelPos) override;

private:
  std::unique_ptr<Delegate> m_delegate;

  std::string const m_baseUrl;
  std::string const m_basePicturesUrl;
};
}  // namespace promo
