#import <CoreApi/AppInfo.h>
#import <CoreApi/Framework.h>
#import <CoreSpotlight/CoreSpotlight.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "MWMSearch+CoreSpotlight.h"
#import "SwiftBridge.h"

#include "platform/preferred_languages.hpp"
#include "search/displayed_categories.hpp"

namespace
{
NSString * const kSpotlightCategoriesDomain = @"omaps.app.categories";

NSArray<NSString *> * GetCategoryLocaleCandidates(NSString * languageId)
{
  NSMutableOrderedSet<NSString *> * locales = [NSMutableOrderedSet orderedSet];
  std::string const locale = languageId.UTF8String;
  [locales addObject:languageId];
  [locales addObject:@(languages::GetTwine(locale).c_str())];
  return locales.array;
}

NSString * GetCategoryName(std::string const & categoryKey, NSString * locale)
{
  NSString * categoryName = nil;
  GetFramework().GetDisplayedCategories().ForEachSynonym(
      categoryKey, [&categoryName, locale](std::string const & name, std::string const & categoryLocale)
  {
    if (!categoryName && [locale isEqualToString:@(categoryLocale.c_str())])
      categoryName = @(name.c_str());
  });
  return categoryName;
}

}  // namespace

@implementation MWMSearch (CoreSpotlight)

+ (NSArray<CSSearchableItem *> *)spotlightItemsForLanguageIds:(NSArray<NSString *> *)languageIds
{
  auto const & categories = GetFramework().GetDisplayedCategories();
  auto const & categoriesKeys = categories.GetKeys();
  NSMutableArray<CSSearchableItem *> * items = [@[] mutableCopy];

  for (auto const & categoryKey : categoriesKeys)
  {
    NSMutableDictionary<NSString *, NSString *> * localizedCategoryNames = [NSMutableDictionary dictionary];

    categories.ForEachSynonym(categoryKey,
                              [&localizedCategoryNames](std::string const & name, std::string const & locale)
    {
      NSString * nsName = @(name.c_str());
      NSString * nsLocale = @(locale.c_str());
      if (!localizedCategoryNames[nsLocale])
        localizedCategoryNames[nsLocale] = nsName;
    });

    NSMutableOrderedSet<NSString *> * categoryNames = [NSMutableOrderedSet orderedSet];
    NSMutableDictionary<NSString *, NSString *> * localesByCategoryName = [NSMutableDictionary dictionary];
    for (NSString * languageId in languageIds)
    {
      for (NSString * locale in GetCategoryLocaleCandidates(languageId))
      {
        NSString * categoryName = localizedCategoryNames[locale];
        if (categoryName.length == 0)
          continue;
        [categoryNames addObject:categoryName];
        NSString * currentLocale = localesByCategoryName[categoryName];
        if (!currentLocale || [locale compare:currentLocale] == NSOrderedAscending)
          localesByCategoryName[categoryName] = locale;
        break;
      }
    }
    if (categoryNames.count == 0)
    {
      NSString * fallbackName = localizedCategoryNames[@"en"];
      CHECK(fallbackName.length > 0, (categoryKey));
      [categoryNames addObject:fallbackName];
      localesByCategoryName[fallbackName] = @"en";
    }

    NSString * categoryKeyString = @(categoryKey.c_str());
    NSString * imageName = [NSString stringWithFormat:@"ic_%@_spotlight", categoryKeyString];
    UIImage * image = [UIImage imageNamed:imageName];
    ASSERT(image, (categoryKey));
    NSData * thumbnailData = UIImagePNGRepresentation(image);
    for (NSString * categoryName in categoryNames)
    {
      CSSearchableItemAttributeSet * attributes = [[CSSearchableItemAttributeSet alloc] initWithContentType:UTTypeItem];
      attributes.title = categoryName;
      attributes.displayName = categoryName;
      attributes.thumbnailData = thumbnailData;

      NSString * identifier =
          [NSString stringWithFormat:@"%@:%@", categoryKeyString, localesByCategoryName[categoryName]];
      CSSearchableItem * item = [[CSSearchableItem alloc] initWithUniqueIdentifier:identifier
                                                                  domainIdentifier:kSpotlightCategoriesDomain
                                                                      attributeSet:attributes];
      // Category shortcuts should not expire on their own; index refreshes replace them.
      item.expirationDate = NSDate.distantFuture;
      [items addObject:item];
    }
  }
  return items;
}

+ (NSString *)categoryKeyForSpotlightIdentifier:(NSString *)identifier
{
  // Localized entries append their locale; legacy entries contain only the category key.
  NSString * categoryKey = [identifier componentsSeparatedByString:@":"].firstObject;
  for (auto const & key : GetFramework().GetDisplayedCategories().GetKeys())
    if ([categoryKey isEqualToString:@(key.c_str())])
      return categoryKey;
  return nil;
}

+ (SearchQuery *)searchQueryForSpotlightIdentifier:(NSString *)identifier
{
  NSString * categoryKey = [self categoryKeyForSpotlightIdentifier:identifier];
  if (!categoryKey)
    return nil;

  NSArray<NSString *> * components = [identifier componentsSeparatedByString:@":"];
  NSString * locale = components.count > 1 ? components[1] : nil;
  NSString * categoryName = nil;
  if (locale && search::DisplayedCategories::IsLanguageSupported(locale.UTF8String))
    categoryName = GetCategoryName(categoryKey.UTF8String, locale);

  if (!categoryName)
  {
    NSString * languageId = AppInfo.sharedInfo.languageId ?: @"en";
    for (NSString * candidate in GetCategoryLocaleCandidates(languageId))
    {
      categoryName = GetCategoryName(categoryKey.UTF8String, candidate);
      if (categoryName)
      {
        locale = candidate;
        break;
      }
    }
  }

  if (!categoryName)
  {
    locale = @"en";
    categoryName = GetCategoryName(categoryKey.UTF8String, locale);
  }
  CHECK(categoryName.length > 0, (categoryKey.UTF8String));

  return [[SearchQuery alloc] init:[categoryName stringByAppendingString:@" "]
                            locale:locale
                            source:SearchTextSourceCategory];
}

+ (void)addCategoriesToSpotlight
{
  ASSERT(NSThread.isMainThread, ());
  if (![CSSearchableIndex isIndexingAvailable])
    return;

  NSArray<NSString *> * languageIds = NSLocale.preferredLanguages;
  if (languageIds.count == 0)
    languageIds = @[@"en"];
  [self updateSpotlightIndex:[CSSearchableIndex defaultSearchableIndex] languageIds:languageIds];
}

+ (void)updateSpotlightIndex:(CSSearchableIndex *)index languageIds:(NSArray<NSString *> *)languageIds
{
  ASSERT(NSThread.isMainThread, ());
  // All accesses, including completion handlers, are confined to the main queue.
  static BOOL indexingInProgress = NO;
  static NSMapTable<CSSearchableIndex *, NSArray<NSString *> *> * indexedLanguagesByIndex =
      [NSMapTable weakToStrongObjectsMapTable];
  if (indexingInProgress)
    return;
  if ([languageIds isEqualToArray:[indexedLanguagesByIndex objectForKey:index]])
    return;
  indexingInProgress = YES;

  NSArray<CSSearchableItem *> * items = [self spotlightItemsForLanguageIds:languageIds];
  [index deleteSearchableItemsWithDomainIdentifiers:@[kSpotlightCategoriesDomain]
                                  completionHandler:^(NSError * _Nullable deletionError) {
                                    dispatch_async(dispatch_get_main_queue(), ^{
                                      if (deletionError)
                                      {
                                        LOG(LWARNING, ("Deleting Spotlight categories failed:",
                                                       deletionError.localizedDescription.UTF8String));
                                        indexingInProgress = NO;
                                        return;
                                      }

                                      [index indexSearchableItems:items
                                                completionHandler:^(NSError * _Nullable indexingError) {
                                                  dispatch_async(dispatch_get_main_queue(), ^{
                                                    if (indexingError)
                                                    {
                                                      LOG(LWARNING, ("Adding Spotlight categories failed:",
                                                                     indexingError.localizedDescription.UTF8String));
                                                    }
                                                    else
                                                    {
                                                      LOG(LINFO, ("Adding Spotlight category search items succeeded:",
                                                                  items.count));
                                                      [indexedLanguagesByIndex setObject:[languageIds copy]
                                                                                  forKey:index];
                                                    }
                                                    indexingInProgress = NO;
                                                  });
                                                }];
                                    });
                                  }];
}

@end
