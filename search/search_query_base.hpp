#pragma once

#include "search/suggest.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/cancellable.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

class Index;
class CategoriesHolder;

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
class Results;

class SearchQueryBase : public my::Cancellable
{
public:
  SearchQueryBase(Index & index, CategoriesHolder const & categories,
                  vector<Suggest> const & suggests, storage::CountryInfoGetter const & infoGetter);

  virtual void Init(bool viewportSearch) = 0;

  virtual void SetInputLocale(string const & locale) = 0;
  virtual void SetPreferredLocale(string const & locale) = 0;
  virtual void SetQuery(string const & query) = 0;
  virtual void SetRankPivot(m2::PointD const & pivot) = 0;
  virtual void SetSearchInWorld(bool searchInWorld) = 0;
  virtual void SetSupportOldFormat(bool supportOldFormat) = 0;
  virtual void SetViewport(m2::RectD const & viewport, bool forceUpdate) = 0;

  virtual void Search(Results & results, size_t maxResults) = 0;
  virtual void SearchViewportPoints(Results & results) = 0;
  virtual void SearchCoordinates(Results & results) = 0;

  virtual void ClearCaches() = 0;

protected:
  Index & m_index;
  CategoriesHolder const & m_categories;
  vector<Suggest> const & m_suggests;
  storage::CountryInfoGetter const & m_infoGetter;
};
}  // namespace search
