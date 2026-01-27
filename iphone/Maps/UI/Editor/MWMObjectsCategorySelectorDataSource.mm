#import "MWMObjectsCategorySelectorDataSource.h"

#include "LocaleTranslator.h"

#include <CoreApi/Framework.h>

#include "platform/localization.hpp"

namespace
{
using Category = std::pair<std::string, osm::NewFeatureCategories::TypeName>;
using Categories = std::vector<Category>;

std::string locale()
{
  return locale_translator::bcp47ToTwineLanguage(NSLocale.currentLocale.localeIdentifier);
}
}  // namespace

@interface MWMObjectsCategorySelectorDataSource ()
{
  osm::NewFeatureCategories m_categories;
  Categories m_categoriesList;
  Categories m_recentCategoriesList;
}

@end

@implementation MWMObjectsCategorySelectorDataSource

- (instancetype)init
{
  self = [super init];
  if (self)
    [self load];

  return self;
}

- (void)initializeList:(osm::NewFeatureCategories::TypeNames const &)types
{
  m_categoriesList.clear();
  for (auto const & type : types)
    m_categoriesList.emplace_back(platform::GetLocalizedTypeName(type), type);

  std::sort(m_categoriesList.begin(), m_categoriesList.end());
}

- (void)initializeRecentCategoriesList:(osm::NewFeatureCategories::TypeNames const &)types
{
  m_recentCategoriesList.clear();
  for (auto const & type : types)
    m_recentCategoriesList.emplace_back(platform::GetLocalizedTypeName(type), type);
}

- (void)load
{
  m_categories = GetFramework().GetEditorCategories();
  m_categories.AddLanguage(locale());
  m_categories.AddLanguage("en");

  auto const & types = m_categories.GetAllCreatableTypeNames();
  m_categoriesList.reserve(types.size());
  m_recentCategoriesList.reserve(types.size());

  [self initializeList:types];
  [self initializeRecentCategoriesList:m_categories.GetRecentCategories()];
}

- (void)search:(NSString *)query
{
  if (query.length == 0)
    [self initializeList:m_categories.GetAllCreatableTypeNames()];
  else
    [self initializeList:m_categories.Search([query UTF8String])];
}

- (void)addToRecentCategories:(NSString *)query
{
  m_categories.AddToRecentCategories([query UTF8String]);
  [self initializeRecentCategoriesList:m_categories.GetRecentCategories()];
}

- (NSString *)getTranslation:(NSInteger)row
{
  return @(m_categoriesList[row].first.c_str());
}

- (NSString *)getRecentCategoriesTranslation:(NSInteger)row
{
  return @(m_recentCategoriesList[row].first.c_str());
}

- (NSString *)getType:(NSInteger)row
{
  return @(m_categoriesList[row].second.c_str());
}

- (NSString *)getRecentCategoriesType:(NSInteger)row
{
  return @(m_recentCategoriesList[row].second.c_str());
}

- (NSInteger)size
{
  return m_categoriesList.size();
}

- (NSInteger)recentCategoriesListSize
{
  return m_recentCategoriesList.size();
}

@end
