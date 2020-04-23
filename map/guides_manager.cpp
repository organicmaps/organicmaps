#include "map/guides_manager.hpp"

GuidesManager::GuidesState GuidesManager::GetState() const
{
  return m_state;
}

void GuidesManager::SetStateListener(GuidesStateChangedFn const & onStateChangedFn)
{
  m_onStateChangedFn = onStateChangedFn;
}

void GuidesManager::UpdateViewport(ScreenBase const & screen)
{
  // TODO: Implement.
}

void GuidesManager::Invalidate()
{
  // TODO: Implement.
}

void GuidesManager::SetEnabled(bool enabled)
{
  ChangeState(enabled ? GuidesState::Enabled : GuidesState::Disabled);
}

bool GuidesManager::IsEnabled() const
{
  return m_state != GuidesState::Disabled;
}

void GuidesManager::ChangeState(GuidesState newState)
{
  if (m_state == newState)
    return;
  m_state = newState;
  if (m_onStateChangedFn != nullptr)
    m_onStateChangedFn(newState);
}

GuidesManager::GuidesGallery GuidesManager::GetGallery() const
{
  // Dummy gallery for debug only.
  GuidesGallery gallery;
  {
    GuidesGallery::Item item;
    item.m_guideId = "048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Moscow by The Village";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 32;
    item.m_cityParams.m_trackIsAvailable = false;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Riga City Tour";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = true;
    item.m_cityParams.m_bookmarksCount = 31;
    item.m_cityParams.m_trackIsAvailable = true;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_imageUrl = "https://img.oastatic.com/img2/1966324/834x417s/t.jpg";
    item.m_title = "Klassik trifft Romantik";
    item.m_subTitle = "Hiking / Trekking";
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_downloaded = false;
    item.m_outdoorsParams.m_ascent = 400;
    item.m_outdoorsParams.m_distance = 24100;
    item.m_outdoorsParams.m_duration = 749246;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Moscow by The Village";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 32;
    item.m_cityParams.m_trackIsAvailable = false;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Riga City Tour";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 31;
    item.m_cityParams.m_trackIsAvailable = true;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_imageUrl = "https://img.oastatic.com/img2/1966324/834x417s/t.jpg";
    item.m_title = "Klassik trifft Romantik";
    item.m_subTitle = "Hiking / Trekking";
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_downloaded = true;
    item.m_outdoorsParams.m_ascent = 400;
    item.m_outdoorsParams.m_distance = 24100;
    item.m_outdoorsParams.m_duration = 749246;

    gallery.m_items.push_back(item);
  }
  {
    GuidesGallery::Item item;
    item.m_guideId = "048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/048f4c49-ee80-463f-8513-e57ade2303ee";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Moscow by The Village";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 32;
    item.m_cityParams.m_trackIsAvailable = false;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/e2d448eb-7fa4-4fab-93e7-ef0fea91cfff";
    item.m_imageUrl = "https://storage.maps.me/bookmarks_catalogue/"
                      "002dc2ae-7b5c-4d3c-88bc-7c7ba109d0e8.jpg?t=1584470956.009026";
    item.m_title = "Riga City Tour";
    item.m_subTitle = "awesome city guide";
    item.m_type = GuidesGallery::Item::Type::City;
    item.m_downloaded = false;
    item.m_cityParams.m_bookmarksCount = 31;
    item.m_cityParams.m_trackIsAvailable = true;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_imageUrl = "https://img.oastatic.com/img2/1966324/834x417s/t.jpg";
    item.m_title = "Klassik trifft Romantik";
    item.m_subTitle = "Hiking / Trekking";
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_downloaded = false;
    item.m_outdoorsParams.m_ascent = 400;
    item.m_outdoorsParams.m_distance = 24100;
    item.m_outdoorsParams.m_duration = 749246;

    gallery.m_items.push_back(item);
  }

  {
    GuidesGallery::Item item;
    item.m_guideId = "d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_url = "https://routes.maps.me/en/v3/mobilefront/route/d26a6662-20a3-432c-a357-c9cb3cce6d57";
    item.m_imageUrl = "https://img.oastatic.com/img2/1966324/834x417s/t.jpg";
    item.m_title = "Klassik trifft Romantik";
    item.m_subTitle = "Hiking / Trekking";
    item.m_type = GuidesGallery::Item::Type::Outdoor;
    item.m_downloaded = false;
    item.m_outdoorsParams.m_ascent = 400;
    item.m_outdoorsParams.m_distance = 24100;
    item.m_outdoorsParams.m_duration = 749246;

    gallery.m_items.push_back(item);
  }

  return gallery;
}

std::string GuidesManager::GetActiveGuide() const
{
  // Dummy active guide for debug only.
  return "048f4c49-ee80-463f-8513-e57ade2303ee";
}

void GuidesManager::SetActiveGuide(std::string const & guideId)
{
  // TODO: Implement.
}

void GuidesManager::SetGalleryListener(GuidesGalleryChangedFn const & onGalleryChangedFn)
{
  m_onGalleryChangedFn = onGalleryChangedFn;
}

void GuidesManager::SetApiDelegate(std::unique_ptr<GuidesOnMapDelegate> apiDelegate)
{
  m_api.SetDelegate(std::move(apiDelegate));
}
