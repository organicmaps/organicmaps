#import <CoreSpotlight/CoreSpotlight.h>
#import <Crashlytics/Crashlytics.h>
#import <MobileCoreServices/MobileCoreServices.h>
#import "AppInfo.h"
#import "MWMCommon.h"
#import "MWMSearch+CoreSpotlight.h"
#import "MWMSettings.h"

#include "Framework.h"

#include "search/displayed_categories.hpp"

@implementation MWMSearch (CoreSpotlight)

+ (void)addCategoriesToSpotlight
{
  if (isIOSVersionLessThan(9) || ![CSSearchableIndex isIndexingAvailable])
    return;

  NSString * localeLanguageId = [[AppInfo sharedInfo] languageId];
  if ([localeLanguageId isEqualToString:[MWMSettings spotlightLocaleLanguageId]])
    return;

  auto const & categories = GetFramework().GetDisplayedCategories();
  auto const & categoriesKeys = categories.GetKeys();
  NSMutableArray<CSSearchableItem *> * items = [@[] mutableCopy];

  for (auto const & categoryKey : categoriesKeys)
  {
    CSSearchableItemAttributeSet * attrSet = [[CSSearchableItemAttributeSet alloc]
        initWithItemContentType:static_cast<NSString *>(kUTTypeItem)];

    NSString * categoryName = nil;
    NSMutableDictionary<NSString *, NSString *> * localizedStrings = [@{} mutableCopy];

    categories.ForEachSynonym(categoryKey, [&localizedStrings, &localeLanguageId, &categoryName](
                                               string const & name, string const & locale) {
      NSString * nsName = @(name.c_str());
      NSString * nsLocale = @(locale.c_str());
      if ([localeLanguageId isEqualToString:nsLocale])
        categoryName = nsName;
      localizedStrings[nsLocale] = nsName;
    });
    attrSet.alternateNames = localizedStrings.allValues;
    attrSet.keywords = localizedStrings.allValues;
    attrSet.title = categoryName;
    attrSet.displayName = [[CSLocalizedString alloc] initWithLocalizedStrings:localizedStrings];

    NSString * categoryKeyString = @(categoryKey.c_str());
    NSString * imageName = [NSString stringWithFormat:@"ic_%@_spotlight", categoryKeyString];
    attrSet.thumbnailData = UIImagePNGRepresentation([UIImage imageNamed:imageName]);

    CSSearchableItem * item =
        [[CSSearchableItem alloc] initWithUniqueIdentifier:categoryKeyString
                                          domainIdentifier:@"maps.me.categories"
                                              attributeSet:attrSet];
    [items addObject:item];
  }

  [[CSSearchableIndex defaultSearchableIndex]
      indexSearchableItems:items
         completionHandler:^(NSError * _Nullable error) {
           if (error)
           {
             [[Crashlytics sharedInstance] recordError:error];
             LOG(LERROR,
                 ("addCategoriesToSpotlight failed: ", error.localizedDescription.UTF8String));
           }
           else
           {
             LOG(LINFO, ("addCategoriesToSpotlight succeded"));
             [MWMSettings setSpotlightLocaleLanguageId:localeLanguageId];
           }
         }];
}

@end
