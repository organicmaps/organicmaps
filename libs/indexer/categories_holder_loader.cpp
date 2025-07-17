#include "categories_holder.hpp"

#include "platform/platform.hpp"

#include "defines.hpp"

CategoriesHolder const & GetDefaultCategories()
{
  static CategoriesHolder const instance(GetPlatform().GetReader(SEARCH_CATEGORIES_FILE_NAME));
  return instance;
}

CategoriesHolder const & GetDefaultCuisineCategories()
{
  static CategoriesHolder const instance(GetPlatform().GetReader(SEARCH_CUISINE_CATEGORIES_FILE_NAME));
  return instance;
}
